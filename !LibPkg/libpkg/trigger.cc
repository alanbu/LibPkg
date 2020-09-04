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

#include "libpkg/trigger.h"
#include "libpkg/log.h"
#include "libpkg/pkgbase.h"
#include "libpkg/component.h"


#include <cstdlib>
#include <cstdio>

namespace pkg
{

/**
 * Construct a new trigger
 *
 * @param pb package base
 * @param pkgname name of package with trigger
 * @param action when the trigger is being run
 * @param old_version old version of the package or "" if not already installed
 * @param new_version new version of the package or "" if removing
 * @param runner interface to start the trigger in the front end
 */
trigger::trigger(pkgbase &pb, const std::string &pkgname, trigger::action_type action,
		         const std::string &old_version, const std::string &new_version,
		         trigger_run *runner) :
	_pb(pb),
	_pkgname(pkgname),
	_action(action),
	_old_version(old_version),
	_new_version(new_version),
	_runner(runner),
	_state(state_none),
	_log(0)
{
	// Parse component paths
	const binary_control& ctrl=_pb.control()[_pkgname];

	if (!ctrl.components().empty())
	{
		try
		{
			std::vector<component> comps;
			std::string comp_str = ctrl.components();
			parse_component_list(comp_str.begin(), comp_str.end(), &comps);
			for (std::vector<component>::iterator i = comps.begin(); i !=comps.end(); ++i)
			{
				std::string pathname = _pb.paths()(i->name(), _pkgname);
				_paths.push_back(pathname);
			}
		} catch(...)
		{
			// Ignore failures as it just means the path won't get set
		}
	}
}

/**
 * Get the path to a trigger
 */
std::string trigger::trigger_path()
{
	std::string src_pathname("RiscPkg.");
	switch (_action)
	{
	case pre_remove:   src_pathname += "Triggers.PreRemove"; break;
	case pre_install:  src_pathname += "Triggers.~RiscPkg++.PreInstall"; break;
	case post_install: src_pathname += "Triggers.PostInstall"; break;
	case post_remove:  src_pathname += "PRTriggers.PostRemove"; break;
	case abort_pre_remove: src_pathname += "Triggers.PostInstall"; break;
	case abort_pre_install: src_pathname += "Triggers.~RiscPkg++.PostRemove"; break;
	}

	return _pb.paths()(src_pathname, _pkgname);
}

/**
 * Run the trigger
 */
void trigger::run()
{
	std::string action;
	if (_old_version.empty() || _new_version.empty())
	{
		switch(_action)
		{
		case pre_remove:
		case post_remove:
			action="remove";
			break;
		case pre_install:
		case post_install:
			action="install";
			break;
		case abort_pre_remove:
			action = "abort-remove";
			break;
		case abort_pre_install:
			action = "abort-install";
			break;
		}
	} else
	{
		switch (_action)
		{
		case pre_remove:
		case post_remove:
		case pre_install:
		case post_install:
			action = "upgrade";
			break;
		case abort_pre_remove:
		case abort_pre_install:
			action = "abort-upgrade";
			break;
		}
	}

	if (_log)
	{
		std::string full_action;
		switch(_action)
		{
		case pre_remove: full_action = "pre-remove"; break;
		case post_remove: full_action = "post-remove"; break;
		case pre_install: full_action = "pre-install"; break;
		case post_install: full_action = "post-install"; break;
		case abort_pre_remove: full_action = "abort pre-remove"; break;
		case abort_pre_install: full_action = "abort pre-install"; break;
		}
		_log->message(LOG_INFO_TRIGGER_RUN, full_action, _pkgname);
	}

	setenv("PkgTrigger$Action", action.c_str(),1);
	setenv("PkgTrigger$Abort", (_action == abort_pre_install || _action == abort_pre_remove) ? "1" : "0", 1);
	setenv("PkgTrigger$OldVersion", _old_version.c_str(),1);
	setenv("PkgTrigger$NewVersion", _new_version.c_str(),1);
	setenv("PkgTrigger$ReturnCode", "-1",1);
	setenv("PkgTrigger$ReturnText", "",1);
	char pathvar[32];
	for (unsigned int i = 0; i < _paths.size(); ++i)
	{
		std::sprintf(pathvar,"PkgTrigger$Path%d", i+1);
		setenv(pathvar, _paths[i].c_str(), 1);
	}
	std::string file_name = trigger_path();
	std::string::size_type leaf_pos = file_name.rfind('.');
	std::string trigger_dir = file_name.substr(0, leaf_pos);
	setenv("PkgTrigger$Dir", trigger_dir.c_str(), 1);
	_state = state_running;
	_runner->run(file_name, this);
}

/**
 * Callback from trigger_run if the trigger could not be started
 *
 * @param reason description of why the trigger could not be run
 */
void trigger::trigger_start_failed(const std::string &reason)
{
	_state = state_error;
	_message = reason;
	std::string msg("Failed to start trigger: ");
	msg += reason;
    trigger_log(msg);
	delete_env_vars();
}

/**
 * Log any output from the trigger.
 *
 * This is usuall any text displayed during the execution of the trigger.
 *
 * @param text output from the trigger run
 */
void trigger::trigger_log(const std::string &text)
{
	if (_log)
	{
		std::string logmsg;
		bool last_cr = false;
		// Clean up any output
		for(std::string::const_iterator i = text.begin(); i != text.end(); ++i)
		{
			if (*i == 0x0d)
			{
				last_cr = true;
				if (!logmsg.empty()) _log->message(LOG_INFO_TRIGGER_OUTPUT, logmsg);
				logmsg.clear();
			} else
			{
				if (*i == 0x0a)
				{
					if (!last_cr)
					{
						if (!logmsg.empty()) _log->message(LOG_INFO_TRIGGER_OUTPUT, logmsg);
						logmsg.clear();
					}
				} else if (*i < 32)
				{
					logmsg += ".";
				} else
				{
					logmsg += *i;
				}
				last_cr = false;
			}
		}
		if (!logmsg.empty()) _log->message(LOG_INFO_TRIGGER_OUTPUT, logmsg);
	}
}

/**
 * Callback from trigger_run when the trigger finished
 */
void trigger::trigger_finished()
{
	int ret_code = std::atoi(getenv("PkgTrigger$ReturnCode"));
	_message = getenv("PkgTrigger$ReturnText");
	switch(ret_code)
	{
	case -1: 
		_state = state_error;
		if (_message.empty()) _message = "Failed to run or return code not set";
		break;
	case 0:
		_state = state_success;
		break;
	case 1:
		_state = state_error;
		if (_message.empty()) _message = "Error during running";
		break;
	default:
		_state = state_error;
		_message = "Invalid return code from trigger";
		break;
	}
	std::string msg;
	if (_state == state_success)
	{
		msg = "Trigger succeeded";
	} else
	{
		msg = "Trigger failed: ";
		msg += _message;
	}
	trigger_log(msg);
	delete_env_vars();
}

/**
 * Delete environmental variables after the trigger finished
 */
void trigger::delete_env_vars()
{
	unsetenv("PkgTrigger$Action");
	unsetenv("PkgTrigger$Abort");
	unsetenv("PkgTrigger$OldVersion");
	unsetenv("PkgTrigger$NewVersion");
	unsetenv("PkgTrigger$Dir");
	unsetenv("PkgTrigger$ReturnCode");
	unsetenv("PkgTrigger$ReturnText");
	char pathvar[32];
	for (unsigned int i = 0; i < _paths.size(); ++i)
	{
		std::sprintf(pathvar,"PkgTrigger$Path%d", i+1);
		unsetenv(pathvar);
	}
}

}
