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

#include <algorithm>
#include <sstream>
#include <fstream>

#include "libpkg/md5.h"
#include "libpkg/filesystem.h"
#include "libpkg/control.h"
#include "libpkg/pkgbase.h"
#include "libpkg/env_checker.h"

namespace {

const char hexchar[]="0123456789ABCDEF";

}; /* anonymous namespace */

namespace pkg {

pkgbase::pkgbase(const string& pathname,const string& dpathname,
	const string& cpathname):
	_pathname(pathname),
	_dpathname(dpathname),
	_cpathname(cpathname),
	_curstat(pathname+string(".Status")),
	_selstat(pathname+string(".Selected")),
	_env_checker_ptr(pathname+string(".ModuleIDs")),
	_control(pathname+string(".Available")),
	_sources(dpathname+string(".Sources"),cpathname+string(".Sources")),
	_env_packages(nullptr),
	_paths(pathname+string(".Paths")),
	_changed(false)
{
	create_directory(_pathname+string(".Cache"));
	create_directory(_pathname+string(".Lists"));

    // Update status files if necessary
    std::fstream bvf(pathname+string(".Version"));
	int db_version;
	if (!(bvf >> db_version)) db_version = 1;
	if (db_version < 2)
	{
		update_status_table(_curstat);
		update_status_table(_selstat);
		bvf.close();
		bvf.open(pathname+string(".Version"), std::ios_base::out | std::ios_base::trunc);
		bvf << 2;
	}
}

pkgbase::~pkgbase()
{
	delete _env_packages;
}

env_packages_table& pkgbase::env_packages()
{
	// Create on demand as it gives a chance for the environment override
	// to be applied before its first used.
	if (!_env_packages)
	{
		_env_packages = new env_packages_table(&_control);
	}
	return *_env_packages;
}

string pkgbase::cache_pathname(const string& pkgname,const string& version, const string& pkgenvid)
{
	string _pkgname(pkgname);
	string _version(version);
	string env_suffix;
	if (!pkgenvid.empty() && pkgenvid != "u") env_suffix = "_" + pkgenvid;
	std::replace(_pkgname.begin(),_pkgname.end(),'.','/');
	std::replace(_version.begin(),_version.end(),'.','/');
	return _pathname+string(".Cache.")+_pkgname+string("_")+_version+env_suffix;
}

string pkgbase::info_pathname(const string& pkgname)
{
	return _pathname+string(".Info.")+pkgname;
}

string pkgbase::list_pathname(const string& url)
{
	const string lists(".Lists.");
	string::size_type length=_pathname.length()+lists.length();
	for (string::const_iterator i=url.begin();i!=url.end();++i)
	{
		char ch=*i;
		if (isalpha(ch)||(ch=='/')||(ch=='~')||(ch=='.'))
		{
			length+=1;
		}
		else
		{
			length+=3;
		}
	}
	string filename;
	filename.reserve(length);
	filename.append(_pathname);
	filename.append(lists);
	for (string::const_iterator i=url.begin();i!=url.end();++i)
	{
		char ch=*i;
		if (ch=='.')
		{
			filename.push_back('_');
		}
		else if (isalpha(ch)||(ch=='/')||(ch=='~'))
		{
			filename.push_back(ch);
		}
		else
		{
			filename.push_back('=');
			filename.push_back(hexchar[(ch>>4)&0xf]);
			filename.push_back(hexchar[ch&0xf]);
		}
	}
	return filename;
}

string pkgbase::available_pathname()
{
	return _pathname+string(".Available");
}

string pkgbase::sysvars_pathname()
{
	return _pathname+string(".SysVars");
}

string pkgbase::sprites_pathname()
{
	return _pathname+string(".Sprites");
}

string pkgbase::setvars_pathname()
{
	return _pathname+string(".SetVars");
}

string pkgbase::bootsprites_pathname()
{
	return _pathname+string(".!BootSprites");
}

string pkgbase::component_update_pathname()
{
	return _pathname + string(".CompUpdate");
}

void pkgbase::verify_cached_file(const binary_control& ctrl)
{
	// Test whether file exists.
	string pathname=cache_pathname(ctrl.pkgname(),ctrl.version(),ctrl.environment_id());
	if (!object_type(pathname))
		throw cache_error("missing cache file",ctrl);

	// Test whether file can be validated.
	if ((ctrl.find("Size")==ctrl.end())&&(ctrl.find("MD5Sum")==ctrl.end()))
	{
		throw cache_error("cannot be validated",ctrl);
	}

	// Test whether file has expected size.
	{
		control::const_iterator f=ctrl.find("Size");
		if (f!=ctrl.end())
		{
			size_t size=0;
			std::istringstream in(f->second);
			in >> size;
			if (object_length(pathname)!=size)
				throw cache_error("incorrect size (do you need to 'Update lists'?)",ctrl);
		}
	}

	// Test whether file has expected MD5Sum.
	{
		control::const_iterator f=ctrl.find("MD5Sum");
		if (f!=ctrl.end())
		{
			std::ifstream in(pathname.c_str());
			md5 md5sum;
			md5sum(in);
			md5sum();
			if (string(md5sum)!=f->second)
				throw cache_error("incorrect md5sum",ctrl);
		}
	}
}

bool pkgbase::fix_dependencies(const std::set<string>& seed)
{
	// Initialise internal flags.
	for (status_table::const_iterator i=_selstat.begin();
		i!=_selstat.end();++i)
	{
		string pkgname=(*i).first;
		status selstat=(*i).second;
		selstat.flag(status::flag_must_remove,false);
		selstat.flag(status::flag_must_install,false);
		selstat.flag(status::flag_must_upgrade,false);
		if (seed.find((*i).first)!=seed.end())
		{
			// Ensure that the state of packages in the seed set
			// cannot be overridden (except to upgrade).
			if (selstat.state()>status::state_removed)
			{
				selstat.flag(status::flag_must_install,true);
			}
			else
			{
				selstat.flag(status::flag_must_remove,true);
			}
		}
		_selstat.insert(pkgname,selstat);
	}

	// Process packages.  Repeat until no change to flags.
	// (This loop will terminate, because the flags can only change
	// in one direction.)
	_changed=true;
	while (_changed)
	{
		_changed=false;
		for (status_table::const_iterator i=_selstat.begin();
			i!=_selstat.end();++i)
		{
			string pkgname=i->first;
			const status& selstat=i->second;

			if (selstat.flag(status::flag_must_install)&&
				!selstat.flag(status::flag_must_remove))
			{
				// This package must be installed, remain installed or
				// be upgraded if all dependencies are to be satisfied.
				bool success=false;
				if (!selstat.flag(status::flag_must_upgrade)&&
					(selstat.state()>=status::state_installed))
				{
					// If the must-upgrade flag is false and if the
					// package is currently installed then look to
					// see whether the installed version can be left
					// as it is.
					binary_control_table::key_type
						key(pkgname,selstat.version(),selstat.environment_id());
					success=fix_dependencies(_control[key],true);
				}
				if (!success)
				{
					// If the previous step did not find a suitable
					// candidate then try the latest version available.
					auto found_pkg = env_packages().find(pkgname);
					if (found_pkg != env_packages().end())
					{
						binary_control_table::key_type
							key(pkgname,found_pkg->second.pkgvrsn,found_pkg->second.pkgenv);
						const pkg::control& ctrl=_control[key];
						success=fix_dependencies(ctrl,true);
						if (success)
							ensure_installed(pkgname,ctrl.version(),found_pkg->second.pkgenv);
					}
				}
				if (!success&&!selstat.flag(status::flag_must_remove))
				{
					// No suitable candidate was found, therefore this
					// package must be removed.  Because it must also
					// be installed, this will cause the operation as
					// a whole to fail.
					ensure_removed(pkgname);
				}
			}
			if ((selstat.state()>status::state_removed)&&
				!selstat.flag(status::flag_must_remove))
			{
				// This package is selected for installation, and as yet
				// there are no known grounds for its removal.
				binary_control_table::key_type key(pkgname,selstat.version(),selstat.environment_id());
				bool success=fix_dependencies(_control[key],false,false);
				if (!success)
				{
					// If dependency list cannot be satisfied then
					// there are now grounds for removal.
					ensure_removed(pkgname);
				}
			}
		}
	}

	// Apply flags
	bool success=true;
	for (status_table::const_iterator i=_selstat.begin();
		i!=_selstat.end();++i)
	{
		const string& pkgname=(*i).first;
		status selstat=(*i).second;
		if (selstat.flag(status::flag_must_install)&&
			!selstat.flag(status::flag_must_remove))
		{
			if (selstat.flag(status::flag_must_upgrade)||
				(selstat.state()<status::state_installed))
			{
				if (selstat.state()<=status::state_removed)
					selstat.flag(status::flag_auto,true);
				selstat.state(status::state_installed);
				auto best = env_packages()[pkgname];
				pkg::binary_control_table::key_type key(pkgname, best.pkgvrsn, best.pkgenv );
				selstat.version(best.pkgvrsn);
				selstat.environment_id(best.pkgenv);
				_selstat.insert(pkgname,selstat);
			}
		}
		else if (selstat.flag(status::flag_must_remove)&&
			!selstat.flag(status::flag_must_install))
		{
			selstat.flag(status::flag_auto,false);
			if (selstat.state()>status::state_removed)
				selstat.state(status::state_removed);
			_selstat.insert(pkgname,selstat);
		}
		else if (selstat.flag(status::flag_must_remove)&&
			selstat.flag(status::flag_must_install))
		{
			success=false;
		}
	}
	return success;
}

void pkgbase::remove_auto()
{
	bool changed=true;
	while (changed)
	{
		changed=false;

		// Mark auto-installed packages for removal
		for (status_table::const_iterator i=_selstat.begin();
			i!=_selstat.end();++i)
		{
			string pkgname=i->first;
			status selstat=i->second;
			selstat.flag(status::flag_must_remove,false);
			selstat.flag(status::flag_must_install,false);
			selstat.flag(status::flag_must_upgrade,false);
			_selstat.insert(pkgname,selstat);
		}

		// Unmark packages that are still needed
		for (status_table::const_iterator i=_selstat.begin();
			i!=_selstat.end();++i)
		{
			string pkgname=i->first;
			const status& selstat=i->second;
			if (selstat.state()>=status::state_installed)
			{
				binary_control_table::key_type key(pkgname,selstat.version(),selstat.environment_id());
				fix_dependencies(_control[key],true,true);
			}
		}

		// Remove packages no longer needed
		for (std::map<string,status>::const_iterator i=_selstat.begin();
			i!=_selstat.end();++i)
		{
			string pkgname=i->first;
			status selstat=i->second;
			if (selstat.flag(status::flag_auto)&&
				!selstat.flag(status::flag_must_install))
			{
				selstat.state(status::state_removed);
				selstat.flag(status::flag_auto,false);
				_selstat.insert(pkgname,selstat);
				changed=true;
			}
		}
	}
}

bool pkgbase::fix_dependencies(const pkg::control& ctrl,bool allow_new)
{
	bool success=fix_dependencies(ctrl,allow_new,false);
	if (success) success=fix_dependencies(ctrl,allow_new,true);
	return success;
}

bool pkgbase::fix_dependencies(const pkg::control& ctrl,bool allow_new,
	bool apply)
{
	// Parse dependency list.
	std::vector<std::vector<dependency> > deps;
	string deplist=ctrl.depends();
	parse_dependency_list(deplist.begin(),deplist.end(),&deps);

	// Process dependency list.
	bool success=true;
	for (std::vector<std::vector<dependency> >::const_iterator i=deps.begin();
		i!=deps.end();++i)
	{
		if (const pkg::control* ctrl=resolve(*i,allow_new))
		{
			if (apply)
				ensure_installed(ctrl->pkgname(),ctrl->version(), ((pkg::binary_control *)ctrl)->environment_id());
		}
		else success=false;
	}

	// Return true if all dependencies satisfied, otherwise false.
	return success;
}

const pkg::control* pkgbase::resolve(const std::vector<dependency>& deps,
	bool allow_new)
{
	// First try to satisfy the dependency with an existing package.
	for (std::vector<dependency>::const_iterator i=deps.begin();
		i!=deps.end();++i)
	{
		const pkg::control* ctrl=resolve(*i,false);
		if (ctrl) return ctrl;
	}

	// If that fails, try to satisfy the dependency with a new package
	// (if that is allowed).
	if (allow_new)
	{
		for (std::vector<dependency>::const_iterator i=deps.begin();
			i!=deps.end();++i)
		{
			const pkg::control* ctrl=resolve(*i,true);
			if (ctrl) return ctrl;
		}
	}

	// If no match found then return 0.
	return 0;
}

const pkg::control* pkgbase::resolve(const dependency& dep,bool allow_new)
{
	// Select package.
	string pkgname=dep.pkgname();
	const status& selstat=_selstat[pkgname];

	// Resolution always fails if the package is flagged for removal.
	if (selstat.flag(status::flag_must_remove)) return 0;

	// Try to satisfy the dependency with an existing packae
	// (one which is already selected for installation and not
	// flagged for upgrade).
	if ((selstat.state()>=status::state_installed)&&
		!selstat.flag(status::flag_must_upgrade))
	{
		string pkgvrsn=selstat.version();
		string envid=selstat.environment_id();
		binary_control_table::key_type key(pkgname,pkgvrsn,envid);
		const pkg::control& ctrl=_control[key];
		if (dep.matches(ctrl.pkgname(),ctrl.version()))
			return &ctrl;
	}

	// If that fails, and if either new packages are allowed or the
	// package is already flagged for upgrade, try to satisfy the
	// dependency with the most recent available version.
	if (allow_new||selstat.flag(status::flag_must_upgrade))
	{
		auto found_pkg = env_packages().find(pkgname);
		if (found_pkg != env_packages().end())
		{
			binary_control_table::key_type key(pkgname,found_pkg->second.pkgvrsn,found_pkg->second.pkgenv);
			const pkg::control& ctrl=_control[key];
			if (dep.matches(ctrl.pkgname(),ctrl.version()))
				return &ctrl;
		}
	}

	// If no match found then return 0.
	return 0;
}

void pkgbase::ensure_installed(const string& pkgname,const string& pkgvrsn,const string &pkgenv)
{
	bool changed=false;
	status selstat=_selstat[pkgname];
	if (!selstat.flag(status::flag_must_install))
	{
		selstat.flag(status::flag_must_install,true);
		changed=true;
	}
	if (!selstat.flag(status::flag_must_upgrade))
	{
		if ((selstat.state()<status::state_installed)||
			(version(selstat.version())!=version(pkgvrsn)))
		{
			selstat.flag(status::flag_must_upgrade,true);
			changed=true;
		}
	}
	if (changed)
	{
		// Ensure we get the package for the correct environment
		selstat.environment_id(pkgenv);
		_selstat.insert(pkgname,selstat);
		_changed=true;
	}
}

void pkgbase::ensure_removed(const string& pkgname)
{
	bool changed=false;
	status selstat=_selstat[pkgname];
	if (!selstat.flag(status::flag_must_remove))
	{
		selstat.flag(status::flag_must_remove,true);
		changed=true;
	}
	if (changed)
	{
		_selstat.insert(pkgname,selstat);
		_changed=true;
	}
}


bool pkgbase::update_status_table(status_table &update_table)
{
    status_table status_updates;
	for (status_table::const_iterator i=update_table.begin();
				i!=update_table.end();++i)
	{
		string pkgname=i->first;		
		binary_control_table::key_type key(pkgname,i->second.version(),i->second.environment_id());
		if (_control[key].pkgname().empty())
		{
			// Package environment wrong, so update
			string pathname=info_pathname(pkgname)+string(".Control");
			std::ifstream in(pathname.c_str());
			binary_control ctrl;
			in >> ctrl;
			if (!ctrl.pkgname().empty()
			    && ctrl.environment_id() != i->second.environment_id()
				)
			{
				status update_status(i->second);
				update_status.environment_id(ctrl.environment_id());
				status_updates.insert(pkgname, update_status);
			}
        }
    }
	if (status_updates.begin() != status_updates.end())
	{
		update_table.insert(status_updates);
		try
		{
			update_table.commit();
			return true;
		}
		catch(...)
		{
			return false;
		}		
	}
	return false;
}


pkgbase::cache_error::cache_error(const char* message,
	const binary_control& ctrl):
	runtime_error(string(message)+string(" for package ")+ctrl.pkgname()+
		string(" (")+ctrl.version()+string(")"))
{}

}; /* namespace pkg */
