// This file is part of LibPkg.
//
// Copyright 2003-2020 Graham Shaw
// Copyright 2015-2020 Alan Buckley
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

#include "libpkg/triggers.h"
#include "libpkg/trigger.h"
#include "libpkg/filesystem.h"
#include "libpkg/log.h"
#include "libpkg/os/osswi.h"
#include <kernel.h>
#include <cstdlib>

namespace pkg
{

/** The temporary directory for post remove triggers.
* Triggers are copied to this directory, so they can be run after
* all the files for the package have been removed.
*/
const std::string post_remove_dirname("PRTriggers");

/** Convert destination pathname to pathname for post remove triggers.
* @param dst_pathname the destination pathname
* @return the pathname to be used in post remove triggers
*/
std::string dst_to_post_remove(const std::string& dst_pathname)
{
	std::string::size_type leaf_pos = dst_pathname.rfind('.');
	std::string::size_type dir_pos = dst_pathname.rfind('.', leaf_pos-1);
	std::string prefix(dst_pathname, 0, dir_pos+1);
	std::string suffix(dst_pathname, leaf_pos, string::npos);
	return std::string(prefix + post_remove_dirname + suffix);
}

triggers::triggers(pkgbase &pb, trigger_run *tr, log *lg) :
	_pb(pb),
	_trigger_run(tr),
	_log(lg),
	_shared_vars_deleted(false)
{
}
	
triggers::~triggers()
{
	_ad("");
	delete_shared_vars();
}

void triggers::add_pre_install(const std::string &pkgname, const std::string &old_version, const std::string &new_version, bool has_unwind)
{
	if (!_packages.count(pkgname))
	{
		_packages[pkgname] = trigger_info(old_version, new_version);
	}
	if (has_unwind) _packages[pkgname].has_install_unwind = true;
	_pre_install.insert(pkgname);
}

void triggers::add_post_install(const string &pkgname, const string &old_version, const std::string &new_version)
{
	if (!_packages.count(pkgname))
	{
		_packages[pkgname] = trigger_info(old_version, new_version);
	}
	_post_install.insert(pkgname);
}

void triggers::add_pre_remove(const string &pkgname, const string &old_version, const std::string &new_version)
{
	if (!_packages.count(pkgname))
	{
		_packages[pkgname] = trigger_info(old_version, new_version);
	}
	_pre_remove.insert(pkgname);
}

void triggers::add_post_remove(const string &pkgname, const string &old_version, const std::string &new_version)
{
	if (!_packages.count(pkgname))
	{
		_packages[pkgname] = trigger_info(old_version, new_version);
	}
	_post_remove.insert(pkgname);
}

void triggers::add_post_install_abort(const string &pkgname, const std::string &old_version, const std::string &new_version)
{
	if (!_packages.count(pkgname))
	{
		_packages[pkgname] = trigger_info(old_version, new_version);
	}
	_packages[pkgname].has_remove_unwind = true;
	// Don't need to add to a list as it's only called on failure
}

void triggers::add_post_remove_file(const std::string &filename)
{
	_post_remove_files_to_copy.insert(filename);
}

bool triggers::post_remove_files_to_copy() const
{
	return !_post_remove_files_to_copy.empty();
}

bool triggers::copy_post_remove_file()
{
	std::string filename = *_post_remove_files_to_copy.begin();
	std::string dst_filename = dst_to_post_remove(filename);
	_ad(dst_filename);

	force_delete(dst_filename);
	try
	{
		copy_object(filename, dst_filename);
		_post_remove_files.insert(dst_filename);
		_post_remove_files_to_copy.erase(filename);
		return true;
	}
	catch (...)
	{
		if (_log) _log->message(LOG_ERROR_POST_REMOVE_COPY, filename);
		return false;
	}
}

bool triggers::pre_remove_triggers_to_run() const
{
	return !_pre_remove.empty();
}

trigger *triggers::next_pre_remove_trigger()
{
	std::string pkgname = *_pre_remove.begin();
	trigger_info &info = _packages[pkgname];
	trigger *t = new trigger(_pb, pkgname, trigger::pre_remove, info.old_version, info.new_version, _trigger_run);
	if (info.has_remove_unwind) _pre_remove_unwind.insert(pkgname);
	_pre_remove.erase(pkgname);
	return t;
}

bool triggers::pre_install_triggers_to_run() const
{
	return !_pre_install.empty();
}

trigger *triggers::next_pre_install_trigger()
{
	std::string pkgname = *_pre_install.begin();
	trigger_info &info = _packages[pkgname];
	trigger *t = new trigger(_pb, pkgname, trigger::pre_install, info.old_version, info.new_version, _trigger_run);
	if (info.has_install_unwind) _pre_install_unwind.insert(pkgname);
	if (info.has_remove_unwind) _pre_remove_unwind.insert(pkgname);
	_pre_install.erase(pkgname);
	return t;
}

bool triggers::post_remove_triggers_to_run() const
{
	return !_post_remove.empty();
}

trigger *triggers::next_post_remove_trigger()
{
	std::string pkgname = *_post_remove.begin();
	trigger_info &info = _packages[pkgname];
	trigger *t = new trigger(_pb, pkgname, trigger::post_remove, info.old_version, info.new_version, _trigger_run);
	_post_remove.erase(pkgname);
	return t;
}

bool triggers::post_install_triggers_to_run() const
{
	return !_post_install.empty();
}

trigger *triggers::next_post_install_trigger()
{
	std::string pkgname = *_post_install.begin();
	trigger_info &info = _packages[pkgname];
	trigger *t = new trigger(_pb, pkgname, trigger::post_install, info.old_version, info.new_version, _trigger_run);
	_post_install.erase(pkgname);
	return t;
}

bool triggers::pre_install_to_unwind() const
{
	return !_pre_install_unwind.empty();
}

trigger *triggers::next_pre_install_unwind()
{
	std::string pkgname = *_pre_install_unwind.begin();
	trigger_info &info = _packages[pkgname];
	trigger *t = new trigger(_pb, pkgname, trigger::abort_pre_install, info.old_version, info.new_version, _trigger_run);
	_pre_install_unwind.erase(pkgname);
	return t;
}

bool triggers::pre_remove_to_unwind() const
{
	return !_pre_remove_unwind.empty();
}

trigger *triggers::next_pre_remove_unwind()
{
	std::string pkgname = *_pre_remove_unwind.begin();
	trigger_info &info = _packages[pkgname];
	trigger *t = new trigger(_pb, pkgname, trigger::abort_pre_remove, info.old_version, info.new_version, _trigger_run);
	_pre_remove_unwind.erase(pkgname);
	return t;
}

bool triggers::post_remove_files_to_remove() const
{
	return !_post_remove_files.empty();
}

void triggers::remove_post_remove_file()
{
	std::string filename = *_post_remove_files.begin();
	std::string dst_filename = dst_to_post_remove(filename);
	_ad(dst_filename);
	force_delete(dst_filename);
	_post_remove_files.erase(filename);
}

void triggers::delete_shared_vars()
{
	if (_shared_vars_deleted) return;
	_shared_vars_deleted = true;

	// Delete all variables beginning PkgTrigger$S_
	std::vector<std::string> vars;
	_kernel_swi_regs regs;
	regs.r[3] = 0; // Context pointer, updated with each call
	char var_value[256];
	bool more = true;
	while(more)
	{
		regs.r[0] = (int)"PkgTrigger$S_*";
		regs.r[1] = (int)var_value;
		regs.r[2] = 256;
		regs.r[4] = 0;
		_kernel_oserror *err = _kernel_swi(pkg::swi::OS_ReadVarVal, &regs, &regs);
		if (err == 0 && regs.r[2] != 0)
		{
			vars.push_back(reinterpret_cast<const char *>(regs.r[3]));
		} else
		{
			more = false;
		}
	}
	for (std::vector<std::string>::iterator v = vars.begin(); v != vars.end(); ++v)
	{
		if (_log) _log->message(LOG_INFO_DELETE_SHARED_VAR, *v);
		unsetenv(v->c_str());
	}
}

}

