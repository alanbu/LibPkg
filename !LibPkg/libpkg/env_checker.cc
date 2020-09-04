// This file is part of LibPkg.
//
// Copyright 2003-2020 Graham Shaw
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Created on: 3 Jul 2018
// Author: Alan Buckley

#include <libpkg/env_checker.h>
#include <libpkg/env_checks.h>
#include <set>
#include <algorithm>
#include <cctype>
#include <string>
#include <fstream>
#include <sstream>

namespace pkg {

unsigned int env_checker::_next_module_id = 1;
const char ENV_MODULE_SEP = '\x01';

env_checker *env_checker::_instance = nullptr;


env_checker_ptr::env_checker_ptr(const std::string &module_map_path)
{
	if (!env_checker::_instance)
	{
		new env_checker(module_map_path);
	} else
	{
		env_checker::_instance->add_ref();
	}
}

env_checker_ptr::env_checker_ptr(const env_checker_ptr &other)
{
	if (env_checker::_instance) env_checker::_instance->add_ref();
}

env_checker_ptr::~env_checker_ptr()
{
	if (env_checker::_instance) env_checker::_instance->remove_ref();
}

env_checker_ptr &env_checker_ptr::operator=(const env_checker_ptr &other)
{
	// Does nothing as this already ref counted
	return *this;
}

env_checker *env_checker_ptr::operator->()
{
	return env_checker::_instance;
}


env_checker::env_checker(const std::string &module_map_path) :
		_ref_count(1),
		_unset_check(nullptr),
		_unset_env(nullptr)
{
	_instance = this;
	_unset_check = new unset_check();
	add_check(_unset_check);
	std::vector<env_check *> unset_checks = {_unset_check};
	_unset_env = new pkg_env("unset", unset_checks);
	_environments["unset"] = _unset_env;
	initialise(module_map_path);
}

env_checker::~env_checker()
{
	while (_watchers.size())
	{
		(*_watchers.begin())->unwatch(*this);
	}

	for (auto &check_pair : _checks)
	{
		delete check_pair.second;
	}
	for (auto &mod_pair : _module_checks)
	{
		delete mod_pair.second;
	}
	for (auto &env_check_pair : _environments)
	{
		delete env_check_pair.second;
	}
}

void env_checker::remove_ref()
{
	if (--_ref_count == 0)
	{
		_instance = nullptr;
		delete this;
	}
}


void env_checker::register_watcher(watcher& w)
{
	_watchers.insert(&w);
}

void env_checker::deregister_watcher(watcher& w)
{
	_watchers.erase(&w);
}

void env_checker::notify()
{
	for (std::set<watcher*>::const_iterator i=_watchers.begin();
		i!=_watchers.end();++i)
	{
		(*i)->handle_change(*this);
	}
}

env_checker::watcher::watcher()
{}

env_checker::watcher::~watcher()
{
	for (std::set<env_checker*>::const_iterator i=_env_checkers.begin();
		i!=_env_checkers.end();++i)
	{
		(*i)->deregister_watcher(*this);
	}
}

void env_checker::watcher::watch(env_checker&e)
{
	_env_checkers.insert(&e);
	e.register_watcher(*this);
}

void env_checker::watcher::unwatch(env_checker& e)
{
	_env_checkers.erase(&e);
	e.deregister_watcher(*this);
}


/**
 * Get a package environment for list of checks
 */
pkg_env *env_checker::package_env(const std::string &env_list, const std::string &os_depends)
{
	if (env_list.empty() && os_depends.empty())
	{
		// Old package with no environment information
		return _unset_env;
	}
   // First try given string verbatim in case it's already normalised
	std::string quick_lookup(env_list);
	if (!os_depends.empty()) quick_lookup += ENV_MODULE_SEP + os_depends;
	auto found = _environments.find(quick_lookup);
	if (found != _environments.end())
	{
		return found->second;
	}

	// Normalise the string and try again
	std::set<std::string> envs;
	std::set<std::string> modules;

	std::string env;
	for(const char &ch : env_list)
	{
		switch(ch)
		{
		case ' ': break;// ignore spaces
		case ',': // env delimiter
			if (!env.empty()) envs.insert(env);
			env.clear();
			break;
		default:
			env += std::tolower(ch);
			break;
		}
	}
	if (!env.empty()) envs.insert(env);
	std::string name;
	for(const std::string &env_name : envs)
	{
		if (!name.empty()) name +=", ";
		name += env_name;
	}

	if (!os_depends.empty())
	{
		std::string::const_iterator pos = os_depends.begin();
		std::string::const_iterator start_pos = pos;
		std::string::const_iterator end_pos;
		while(start_pos != os_depends.end())
		{
			while (start_pos != os_depends.end() && *start_pos == ' ') start_pos++;
			pos = start_pos;
			end_pos = start_pos;
			while (pos != os_depends.end() && *pos != ',')
			{
				if (*pos != ' ') end_pos = pos;
				pos++;
			}
			if (end_pos > start_pos)
			{
				std::string module_name(start_pos, end_pos+1);
				std::transform(module_name.begin(), module_name.end(), module_name.begin(),
				                   [](unsigned char c){ return std::tolower(c); }
				);
				modules.insert(module_name);
			}
			start_pos = pos;
			if (start_pos != os_depends.end()) start_pos++;
		}

		if (!modules.empty())
		{
			name += ENV_MODULE_SEP;
			bool add_comma= false;
			for(const std::string &mod_name : modules)
			{
				if (add_comma) name +=", ";
				else add_comma = true;
				name += mod_name;
			}
		}
	}

	found = _environments.find(name);
	if (found != _environments.end())
	{
		return found->second;
	}

	std::vector<env_check *> checks;
	for(const std::string &env_name : envs)
	{
		auto found_check = _checks.find(env_name);
		if (found_check != _checks.end())
		{
			checks.push_back(found_check->second);
		} else
		{
			// Build unique id for unknown value
			unsigned int uid = 0;
			for(char ch : env_name)
			{
				uid = uid * 100 + (unsigned int)((unsigned char)(ch) - 32);
			}
			char id[12];
			sprintf(id, "u%d", uid);
			env_check *unknown_check = new env_check(env_name, "Unknown check", id, Unknown, 1);
			add_check(unknown_check);
			checks.push_back(unknown_check);
		}
	}
	if (checks.empty()) checks.push_back(_unset_check);

	for(const std::string &mod_name : modules)
	{
		auto found_check = _module_checks.find(mod_name);
		if (found_check != _module_checks.end())
		{
			checks.push_back(found_check->second);
		} else
		{
			module_check *mcheck = new module_check(mod_name);
			_module_checks[mod_name] = mcheck;
			checks.push_back(mcheck);
		}
	}

	pkg_env *new_env = new pkg_env(name, checks);
	_environments[name] = new_env;

	return new_env;
}

/**
 * Add a new check to the list of checks
 *
 * @param check env check to add
 */
void env_checker::add_check(env_check *check)
{
	_checks[check->name()] = check;
}

/**
 * Get module id.
 *
 * ID is made unique for this machine by using a file as a database of
 * modules found.
 */
std::string env_checker::get_module_id(const std::string &title)
{
	std::string lower_title;
	for(char ch : title)
	{
		lower_title += std::tolower(ch);
	}
	auto found = _module_ids.find(lower_title);

	if (found != _module_ids.end())
	{
		return found->second;
	} else
	{
		std::ostringstream id;
		id << 'm' << _next_module_id++;
		_module_ids[lower_title] = id.str();
		write_module_map();
		return id.str();
	}
}

/**
 * Clear any enviroment overrides
 *
 * @returns true if anything changed
 */
bool env_checker::clear_environment_overrides()
{
	bool changed = false;
	for (auto &env_check_pair : _checks)
	{
		env_check *check = env_check_pair.second;
		if (check->available() != check->detected())
		{
			check->available(check->detected());
			changed = true;
		}
	}
	for (auto &mod_check_pair : _module_checks)
	{
		env_check *mod_check = mod_check_pair.second;
		if (mod_check->available() != mod_check->detected())
		{
			mod_check->available(mod_check->detected());
			changed = true;
		}
	}

	if (changed)
	{
		// reset flags on package environments
		for (auto &env_pair : _environments)
		{
			env_pair.second->reset_available();
		}
		notify();
	}

	return changed;
}

/**
 * Set the enviroment
 *
 * @param new_env set containing name environments that should be marked available
 * @param new_mods set containing name of modules that should be marked available
 * @returns true if anything changed
 */
bool env_checker::override_environment(const std::set<std::string> &new_env, const std::set<std::string> &new_mods)
{
	bool changed = false;
	for (auto &env_check_pair : _checks)
	{
		env_check *check = env_check_pair.second;
		bool make_available = (new_env.count(check->name()) != 0);
		if (check->available() != make_available)
		{
			check->available(make_available);
			changed = true;
		}
	}
	for (auto &mod_check_pair : _module_checks)
	{
		env_check *mod_check = mod_check_pair.second;
		bool make_available = (new_mods.count(mod_check->name()) != 0);
		if (mod_check->available() != make_available)
		{
			mod_check->available(make_available);
			changed = true;
		}
	}

	if (changed)
	{
		// reset flags on package environments
		for (auto &env_pair : _environments)
		{
			env_pair.second->reset_available();
		}
		notify();
	}

	return changed;
}

/**
 * Read map of module names to unique ids
 */
void env_checker::read_module_map()
{
	std::ifstream in(_module_map_path.c_str());
	if (!in) return;

	_module_ids.clear();
	_next_module_id = 0;

	unsigned int version;
	in >> version;

	std::string title;
	std::string id;
	char first_char;

	while (in && !in.eof())
	{
		in >> id >> first_char;
		in.putback(first_char);
		std::getline(in, title);
		if (!title.empty())
		{
			if (atoi(id.c_str()) >= (int)_next_module_id) _next_module_id = atoi(id.c_str()) + 1;
			_module_ids[title] = "m" + id;
		}
	}
}

/**
 * Write map of module name to internal string
 */
void env_checker::write_module_map()
{
	std::ofstream out(_module_map_path.c_str());
	unsigned int version = 1;
	out << version << "\n"; // just in case the format needs to change
	for(auto entry : _module_ids)
	{
		out << entry.second.substr(1) << "\t" << entry.first << "\n";
	}
}

/**
 * Create a new environment combination with the given checks
 */
pkg_env::pkg_env(const std::string &name, const std::vector<env_check *> &checks) : _name(name),
    _checks(checks),
	_available(true),
	_install_priority(-10000),
	_type(System)
{
   for(env_check *check : _checks)
   {
	   _available &= check->available();
	   _install_priority += check->install_priority();
	   _id = _id + check->id();
	   if (check->type() > _type) _type = check->type();
   }
}

void pkg_env::reset_available()
{
   _available = true;
   for(env_check *check : _checks)
   {
	   _available &= check->available();
   }
}

/**
 * Return the environment part of the name
 */
std::string pkg_env::env_names() const
{
	return _name.substr(0, _name.find(ENV_MODULE_SEP));
}

/**
 * Return the modules part of the name
 */
std::string pkg_env::module_names() const
{
	std::string::size_type mod_pos = _name.find(ENV_MODULE_SEP);
	return (mod_pos == std::string::npos) ? std::string() : _name.substr(mod_pos+1);
}


} /* namespace lippkg */
