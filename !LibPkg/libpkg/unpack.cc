// This file is part of LibPkg.
//
// Copyright 2003-2020 Graham Shaw
// Copyright 2013-2020 Alan Buckley
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

#include <stdexcept>

#include "libpkg/filesystem.h"
#include "libpkg/version.h"
#include "libpkg/binary_control.h"
#include "libpkg/status.h"
#include "libpkg/pkgbase.h"
#include "libpkg/zipfile.h"
#include "libpkg/standards_version.h"
#include "libpkg/unpack.h"
#include "libpkg/log.h"
#include "libpkg/module_info.h"
#include "libpkg/triggers.h"
#include "libpkg/trigger.h"

/* Uncomment the following line to shove loads of extra information
 * to stdout to help debugging */
//#define TRACE_TO_STD_OUT
#ifdef TRACE_TO_STD_OUT
#include <iostream>
#endif


namespace {

using std::string;

/** The source pathname of the package control file. */
const string ctrl_src_pathname("RiscPkg.Control");

/** The source pathnames of the triggers */
const string pre_remove_src_pathname("RiscPkg.Triggers.PreRemove");
const string pre_install_src_pathname("RiscPkg.Triggers.PreInstall");
const string post_remove_src_pathname("RiscPkg.Triggers.PostRemove");
const string post_install_src_pathname("RiscPkg.Triggers.PostInstall");

/** The destination filename of the manifest file. */
const string mf_dst_filename("Files");

/** The temporary filename of the manifest file. */
const string mf_tmp_filename("Files++");

/** The backup filename of the manifest file. */
const string mf_bak_filename("Files--");

/** The temporary directory name.
 * Files are unpacked into a directory of this name before being moved
 * to their final destinations.
 */
const string tmp_dirname("~RiscPkg++");

/** The backup directory name.
 * Files are backed-up into a directory of this name prior to removal.
 */
const string bak_dirname("~RiscPkg--");

/** Convert zip file pathname to source pathname.
 * @param zip_pathname the zip file pathname
 * @return the source pathname
 */
string zip_to_src(const string& zip_pathname)
{
	string src_pathname(zip_pathname);
	for (unsigned int i=0;i!=src_pathname.length();++i)
	{
		switch (src_pathname[i])
		{
		case '/':
			src_pathname[i]='.';
			break;
		case '.':
			src_pathname[i]='/';
			break;
		}
	}
	return src_pathname;
}

/** Convert source pathname to zip file pathname.
 * @param src_pathname the source pathname
 * @return the zip file pathname
 */
string src_to_zip(const string& src_pathname)
{
	string zip_pathname(src_pathname);
	for (unsigned int i=0;i!=zip_pathname.length();++i)
	{
		switch (zip_pathname[i])
		{
		case '/':
			zip_pathname[i]='.';
			break;
		case '.':
			zip_pathname[i]='/';
			break;
		}
	}
	return zip_pathname;
}

/** Convert destination pathname to temporary pathname.
 * @param dst_pathname the destination pathname
 * @return the temporary pathname
 */
string dst_to_tmp(const string& dst_pathname)
{
	unsigned int ds=dst_pathname.rfind('.');
	string prefix(dst_pathname,0,ds);
	string suffix(dst_pathname,ds,string::npos);
	return string(prefix+string(".")+tmp_dirname+suffix);
}

/** Convert destination pathname to backup pathname.
 * @param dst_pathname the destination pathname
 * @return the backup pathname
 */
string dst_to_bak(const string& dst_pathname)
{
	unsigned int ds=dst_pathname.rfind('.');
	string prefix(dst_pathname,0,ds);
	string suffix(dst_pathname,ds,string::npos);
	return string(prefix+string(".")+bak_dirname+suffix);
}

}; /* anonymous namespace */

namespace pkg {

/** An exception class for reporting that one or more packages cannot
 * be processed. */
class unpack::cannot_process:
	public std::runtime_error
{
public:
	/** Construct cannot-process error. */
	cannot_process();
};

/** An exception class for reporting that one or more files conflict
 * with those already on the system. */
class unpack::file_conflict:
	public std::runtime_error
{
public:
	/** Construct file-conflict error. */
	file_conflict();
};

/** An exception class for reporting that a file information record could
 * not be found. */
class unpack::file_info_not_found:
	public std::runtime_error
{
public:
	/** Construct file-info-not-found error. */
	file_info_not_found();
};

unpack::unpack(pkgbase& pb,const std::set<string>& packages):
	_pb(pb),
	_state(state_pre_unpack),
	_zf(0),
	_files_done(0),
	_files_total(npos),
	_bytes_done(0),
	_bytes_total(npos),
	_files_total_unpack(0),
	_files_total_remove(0),
	_bytes_total_unpack(0),
	_triggers(0),
	_trigger_run(0),
	_trigger(0),
	_log(0),
	_state_text_changed(true),
	_state_text("Preparing file lists")
{
	// For each package to be processed, determine whether it should
	// be unpacked and/or removed.
	for (std::set<string>::const_iterator i=packages.begin();
		i!=packages.end();++i)
	{
		string pkgname=*i;
		const status& curstat=_pb.curstat()[pkgname];
		const status& selstat=_pb.selstat()[pkgname];
		if (unpack_req(curstat,selstat))
			_packages_to_unpack.insert(pkgname);
		if (remove_req(curstat,selstat))
			_packages_to_remove.insert(pkgname);
	}
}

unpack::~unpack()
{
	delete _triggers;
}

void unpack::use_trigger_run(trigger_run *tr)
{
	_trigger_run = tr;
}

void unpack::log_to(pkg::log *use_log)
{
   _log = use_log;
}

triggers *unpack::detach_triggers()
{
	triggers *t = _triggers;
	_triggers = 0;
	return t;
}

void unpack::poll()
{
	try
	{
		_poll();
	}
	catch (std::exception& ex)
	{
		_message=ex.what();
		if (!_exception_item.empty())
		{
			_message += ". " + _exception_item;
		}
		_pb.curstat().rollback();
		delete _zf;
		_zf=0;
		if (_log) _log->message(LOG_ERROR_UNPACK_EXCEPTION, _message);
		switch (_state)
		{
		case state_pre_unpack:
		case state_pre_remove:
			if (!_existing_module_packages.empty()) unwind_existing_modules();
			state(state_fail);
			break;
		case state_remove_files_replaced_by_dirs:
			state(state_unwind_remove_files_replaced_by_dirs);
			break;

		case state_unpack:
			state(state_unwind_unpack);
			break;
		case state_replace:
		case state_remove:
			state(state_unwind_replace);
			break;
		case state_run_pre_remove_triggers:
			state(state_unwind_pre_remove_triggers);
			delete _trigger;
			_trigger = 0;
			break;
		case state_run_pre_install_triggers:
			state(state_unwind_pre_install_triggers);
			delete _trigger;
			_trigger = 0;
			break;
		case state_create_empty_dirs:
		  state(state_unwind_create_empty_dirs);
 		  break;

		default:
			state(state_fail);
			break;
		}
	}
}

void unpack::_poll()
{
	switch (_state)
	{
	case state_pre_unpack:
		if (_files_to_unpack.size())
		{
			pre_unpack_check_files();
		} else if (_empty_dirs_to_create.size())
		{
			pre_unpack_check_empty_dirs();
		} else if (_parent_dirs.size())
		{
			pre_unpack_check_parent_dirs();
		} else if (_packages_to_unpack.size())
		{
			pre_unpack_select_package();
		}
		else
		{
			// Progress to next state.
			delete _zf;
			_zf=0;
			state(state_pre_remove);
		}
		break;
	case state_pre_remove:
		if (_packages_to_remove.size())
		{
			pre_remove_select_package();
		}
		else
		{
			// If there are packages that cannot be processed
			// then the installation fails.
			if (_packages_cannot_process.size())
				throw cannot_process();

			// If there are files that conflict, and which are not
			// scheduled for removal, then the installation fails.
			if (_files_that_conflict.size())
				throw file_conflict();

			// Commit package status changes.  All packages to be
			// removed or unpacked should by now have been marked
			// as half-unpacked.
			_pb.curstat().commit();

			state(state_copy_post_remove);
		}
		break;

	case state_copy_post_remove:
		if (_triggers && _triggers->post_remove_files_to_copy())
		{
			if (!_triggers->copy_post_remove_file())
			{
				state(state_unwind_copy_post_remove);
			}
		}
		else
		{
			state(state_run_pre_remove_triggers);
		}
		break;

	case state_run_pre_remove_triggers:
		if (_trigger)
		{
			switch (_trigger->state())
			{
			case trigger::state_error:
				_message = _trigger->message();
				state(state_unwind_pre_remove_triggers);
				delete _trigger;
				_trigger = 0;
				break;
			case trigger::state_success:
				delete _trigger;
				_trigger = 0;
				break;
			default:
				// Trigger still running
				break;
			}
		}
		else if (_triggers && _triggers->pre_remove_triggers_to_run())
		{
			_trigger = _triggers->next_pre_remove_trigger();
			_trigger->log_to(_log);
			_trigger->run();
		} else
		{
			// Progress to next state.
			state(state_remove_files_replaced_by_dirs);
			_files_total = (_files_total_unpack + _files_total_remove) * 2;
			_bytes_total = _bytes_total_unpack;
		}
		break;

	case state_remove_files_replaced_by_dirs:
	    if (_files_to_replace_by_dirs.size())
		{
			string dst_pathname=*_files_to_replace_by_dirs.begin();
#ifdef TRACE_TO_STD_OUT
			std::cout << "Removing file that will be replaced by dir ";
#endif			
			remove_file(dst_pathname);
			_files_replaced_by_dirs.insert(dst_pathname);
			_files_to_replace_by_dirs.erase(dst_pathname);
			// File has been removed so don't remove it later
			_files_to_remove.erase(dst_pathname);
			_files_being_removed.insert(dst_pathname);
		} else
		{
			// Progress to next state.
			state(state_unpack);
		}
		break;		

	case state_unpack:
		if (_files_to_unpack.size())
		{
			// Unpack file to temporary location.
			string src_pathname=*_files_to_unpack.begin();
			bool is_dir = (src_pathname.back() == '.');
      		if (is_dir)
			{
				string dst_pathname=_pb.paths()(src_pathname.substr(0, src_pathname.size()-1),_pkgname);
				// Build list of all dirs to create for all packages
				_empty_dirs_to_check.insert(dst_pathname);
			} else
			{			
				string dst_pathname=_pb.paths()(src_pathname,_pkgname);
				// Make a note that we're trying to unpack this file,
				// so can revert it later if necessary
				unpack_file(src_pathname,dst_pathname);
				_files_being_unpacked.insert(dst_pathname);
			}
			_files_to_unpack.erase(src_pathname);
		}
		else if (_packages_pre_unpacked.size())
		{
			unpack_select_package();
		}
		else
		{
			// Progress to next state.
			_ad("");
			delete _zf;
			_zf=0;
			state(state_run_pre_install_triggers);
		}
		break;
	case state_run_pre_install_triggers:
		if (_trigger)
		{
			switch (_trigger->state())
			{
			case trigger::state_error:
				_message = _trigger->message();
				state(state_unwind_pre_install_triggers);
				delete _trigger;
				_trigger = 0;
				break;
			case trigger::state_success:
				delete _trigger;
				_trigger = 0;
				break;
			default:
				// Trigger still running
				break;
			}
		}
		else if (_triggers && _triggers->pre_install_triggers_to_run())
		{
			_trigger = _triggers->next_pre_install_trigger();
			_trigger->log_to(_log);
			_trigger->run();
		}
		else
		{
			state(state_replace);
		}
		break;
	case state_replace:
		if (_files_being_unpacked.size())
		{
			// If the unpacked file is expected to be a replacement for
			// an existing file then backup the existing file, otherwise
			// throw an exception if an existing file is found.
			// Move the unpacked file to its final destination.
			string dst_pathname=*_files_being_unpacked.begin();
			bool overwrite=_files_to_remove.find(dst_pathname)!=
				_files_to_remove.end();
			if (!overwrite && _dirs_to_remove.find(dst_pathname) != _dirs_to_remove.end())
			{
				// Need to delete directory so it can be replaced
				soft_delete(dst_pathname);
				_dirs_removed.insert(dst_pathname);				
				_dirs_to_remove.erase(dst_pathname);
			}
			replace_file(dst_pathname,overwrite);
			if (overwrite)
			{
				_files_being_removed.insert(dst_pathname);
				_files_to_remove.erase(dst_pathname);
			}

			// Progress to next file.
			_files_unpacked.insert(dst_pathname);
			_files_being_unpacked.erase(dst_pathname);
		} else if (_empty_dirs_to_check.size())
		{
			string dst_pathname=*_empty_dirs_to_check.begin();
			if (_dirs_to_remove.find(dst_pathname) != _dirs_to_remove.end())
			{
				_dirs_to_remove.erase(dst_pathname);
				// We don't need to process this directory so remove from totals
				_files_total--;
				_files_total_remove--;
			} else
			{
				_empty_dirs_to_create.insert(dst_pathname);
			}
			_empty_dirs_to_check.erase(dst_pathname);
		}
		else
		{
			// Progress to next state.
			_ad("");
			state(state_remove);
		}
		break;
	case state_remove:
		if (_files_to_remove.size())
		{
			// Remove backup file.
			string dst_pathname=*_files_to_remove.begin();
			remove_file(dst_pathname);

			// Progress to next file.
			_files_being_removed.insert(dst_pathname);
			_files_to_remove.erase(dst_pathname);
		} else if (_dirs_to_remove.size())
		{
			string dst_pathname=*_dirs_to_remove.begin();
			_ad(dst_pathname);
			soft_delete(dst_pathname);
			_files_done++;
			_dirs_removed.insert(dst_pathname);
			_dirs_to_remove.erase(dst_pathname);			
		}
		else
		{
			// Progress to next state.
			// This is the point of no return.
			_ad("");
			state(state_post_remove);
		}
		break;
	case state_post_remove:
		if (_files_being_removed.size())
		{
			// Delete backup of file being removed.
			string dst_pathname=*_files_being_removed.begin();
			remove_backup(dst_pathname);

			// Progress to next file.
			_files_removed.insert(dst_pathname);
			_files_being_removed.erase(dst_pathname);
		}
		else if (_packages_being_removed.size())
		{
			// Select package.
			_pkgname=*_packages_being_removed.begin();
			status curstat=_pb.curstat()[_pkgname];

 			// If package is not also being unpacked:
			if (_packages_being_unpacked.find(_pkgname)==
				_packages_being_unpacked.end())
			{
				// Remove manifest.
				remove_manifest(_pkgname);

				// Remove control file.
				string ctrl_dst_pathname=
					_pb.paths()(ctrl_src_pathname,_pkgname);
				remove_file(ctrl_dst_pathname);
				remove_backup(ctrl_dst_pathname);

				// Mark package as removed.
				curstat.state(status::state_removed);
				curstat.flag(status::flag_auto,false);
				_pb.curstat().insert(_pkgname,curstat);
			}

			// Progress to next package.
			_packages_removed.insert(_pkgname);
			_packages_being_removed.erase(_pkgname);

      if (_log) _log->message(LOG_INFO_UNPACK_REMOVED, _pkgname);

		}
		else
		{
			// Progress to next state.
			_ad("");
			state(state_create_empty_dirs);
		}
		break;

	case state_create_empty_dirs:
	  if (_empty_dirs_to_create.size())
		{
			std::string dst_pathname = *_empty_dirs_to_create.begin();
			_ad(dst_pathname);
			create_directory(dst_pathname);
			_files_done++;
			_dirs_created.insert(dst_pathname);
			_empty_dirs_to_create.erase(dst_pathname);
		} else
		{
			// Progress to next state.
			_ad("");
			state(state_post_unpack);
		}
		break;		

	case state_post_unpack:
		if (_packages_being_unpacked.size())
		{
			// Select package.
			_pkgname=*_packages_being_unpacked.begin();
			status curstat=_pb.curstat()[_pkgname];
			const status& prevstat=_pb.prevstat()[_pkgname];

			// Remove backup of control file.
			string ctrl_dst_pathname=
				_pb.paths()(ctrl_src_pathname,_pkgname);
			bool overwrite=prevstat.state()>=status::state_removed;
			if (overwrite) remove_backup(ctrl_dst_pathname);

			// Activate manifest.
			activate_manifest(_pkgname);

			// Mark package as unpacked.
			curstat.state(status::state_unpacked);
			_pb.curstat().insert(_pkgname,curstat);

			// Progress to next package.
			_packages_unpacked.insert(_pkgname);
			_packages_being_unpacked.erase(_pkgname);

       		if (_log) _log->message(LOG_INFO_UNPACKED_PACKAGE, _pkgname);
		}
		else
		{
			// If existing modules were used update the package database
			if (!_existing_module_packages.empty()) update_existing_modules();

			// Commit package status changes.  All packages to be
			// removed or unpacked should by now have been marked
			// as state_removed or state_unpacked respectively.
			_pb.curstat().commit();

			// Progress to next state.
			state(state_done);
		}
		break;
	case state_done:
		// Unpack operation complete: do nothing.
		break;

	case state_unwind_create_empty_dirs:
	  if (_dirs_created.size())
		{
			std::string dst_pathname = *_dirs_created.begin();
			_ad(dst_pathname);
			soft_delete(dst_pathname);
			_dirs_created.erase(dst_pathname);
			_files_done--;
		} else
		{
			_ad("");
			state(state_unwind_replace);
		}
		break;
		
	case state_unwind_replace:
		if (_files_unpacked.size())
		{
			// Delete file.  If it was a replacement for an existing file,
			// restore the original from its backup.
			string dst_pathname=*_files_unpacked.begin();
			bool overwrite=_files_being_removed.find(dst_pathname)!=
				_files_being_removed.end();
			if (!overwrite && _dirs_removed.find(dst_pathname) != _dirs_removed.end())
			{
				// Replaced an empty directory so ensure it's in list to recreate
				_dirs_removed.insert(dst_pathname);
			}

			unwind_replace_file(dst_pathname,overwrite);
			if (overwrite)
				_files_being_removed.erase(dst_pathname);
			_files_unpacked.erase(dst_pathname);
		}
		else
		{
			// Progress to next state.
			_ad("");
			state(state_unwind_remove);
		}
		break;
	case state_unwind_remove:
		if (_files_being_removed.size())
		{
			// Restore file from backup.
			string dst_pathname=*_files_being_removed.begin();
			// If file was replaced by an auto created directory it needs to be restored later
			if (_files_replaced_by_dirs.find(dst_pathname) != _files_replaced_by_dirs.end())
			{
				if (dst_pathname.back() == '.')
				{
					// Just need to recreate directories they are not backed up
					create_directory(dst_pathname.substr(0,dst_pathname.size()-1));
				} else
				{
					unwind_remove_file(dst_pathname);
				}
			}
			_files_being_removed.erase(dst_pathname);
		} else if (_dirs_removed.size())
		{
			// Recreate empty directory that's been removed
			string dst_pathname=*_dirs_removed.begin();
			_ad(dst_pathname);
			create_directory(dst_pathname);
			_dirs_removed.erase(dst_pathname);
		}
		else
		{
			// Progress to next state.
			_ad("");
			state(state_unwind_pre_install_triggers);
		}
		break;
	case state_unwind_pre_install_triggers:
		if (_trigger)
		{
			if (_trigger->finished())
			{
				delete _trigger;
				_trigger = 0;
			}
		}
		else if (_triggers && _triggers->pre_install_to_unwind())
		{
			_trigger = _triggers->next_pre_install_unwind();
			_trigger->log_to(_log);
			_trigger->run();
		}
		else
		{
			state(state_unwind_unpack);
		}
		break;
	case state_unwind_unpack:
		if (_files_being_unpacked.size())
		{
			// Delete temporary file.
			string dst_pathname=*_files_being_unpacked.begin();
			if (dst_pathname.back() == '.')
			{
       		 	soft_delete(dst_pathname.substr(0,dst_pathname.size()-1));
			} else
			{			
			   unwind_unpack_file(dst_pathname);
			}
			_files_being_unpacked.erase(dst_pathname);
		}
		else if (_packages_being_unpacked.size())
		{
			// Select package.
			_pkgname=*_packages_being_unpacked.begin();
			const status& prevstat=_pb.prevstat()[_pkgname];

			// Restore control file.
			string ctrl_dst_pathname=
				_pb.paths()(ctrl_src_pathname,_pkgname);
			bool overwrite=prevstat.state()>=status::state_removed;
			unwind_replace_file(ctrl_dst_pathname,overwrite);

			// Progress to next package.
			_packages_pre_unpacked.insert(_pkgname);
			_packages_being_unpacked.erase(_pkgname);

            if (_log) _log->message(LOG_INFO_RESTORE_CONTROL, _pkgname);
		}
		else
		{
			// Progress to next state.
			_ad("");
			state(state_unwind_remove_files_replaced_by_dirs);
		}
		break;

	case state_unwind_remove_files_replaced_by_dirs:
		if (_files_replaced_by_dirs.size())
		{
			string dst_pathname=*_files_replaced_by_dirs.begin();
			unwind_remove_file(dst_pathname);
			_files_replaced_by_dirs.erase(dst_pathname);
		} else
		{
			state(state_unwind_pre_remove_triggers);
			_trigger = 0;
		}
		break;
		
	case state_unwind_pre_remove_triggers:
		if (_trigger)
		{
			if (_trigger->finished())
			{
				delete _trigger;
				_trigger = 0;
			}
		}
		else if (_triggers && _triggers->pre_remove_to_unwind())
		{
			_trigger = _triggers->next_pre_remove_unwind();
			_trigger->log_to(_log);
			_trigger->run();
		} else
		{
			state(state_unwind_copy_post_remove);
		}
		break;

	case state_unwind_copy_post_remove:
		if (_triggers && _triggers->post_remove_files_to_remove())
		{
			_triggers->remove_post_remove_file();
		}
		else
		{
			state(state_unwind_pre_remove);
		}
		break;

	case state_unwind_pre_remove:
		if (_packages_being_removed.size())
		{
			// Select package.
			_pkgname=*_packages_being_removed.begin();
			status curstat=_pb.curstat()[_pkgname];
			const status& prevstat=_pb.prevstat()[_pkgname];

			// If package was previously unpacked or better then
			// return to previous state (but do not commit until
			// unwind-pre-remove and unwind-pre-unpack phases have
			// been completed.)
			if (prevstat.state()>=status::state_unpacked)
			{
				curstat.state(prevstat.state());
				curstat.version(prevstat.version());
				curstat.environment_id(prevstat.environment_id());
				_pb.curstat().insert(_pkgname,curstat);
				if (_log) _log->message(LOG_INFO_UNWIND_STATE, _pkgname);
			}

			// Progress to next package.
			_packages_being_removed.erase(_pkgname);
		}
		else
		{
			// Progress to next state.
			state(state_unwind_pre_unpack);
		}
		break;
	case state_unwind_pre_unpack:
		if (_packages_pre_unpacked.size())
		{
			// Select package.
			_pkgname=*_packages_pre_unpacked.begin();
			status curstat=_pb.curstat()[_pkgname];
			const status& prevstat=_pb.prevstat()[_pkgname];

			// If package was previously removed or purged then mark
			// as removed (but do not commit until unwind-pre-remove
			// and unwind-pre-unpack phases have been completed.)
			if (prevstat.state()<=status::state_removed)
			{
				curstat.state(status::state_removed);
				curstat.flag(status::flag_auto,false);
				_pb.curstat().insert(_pkgname,curstat);
				if (_log) _log->message(LOG_INFO_UNWIND_STATE_REMOVED,_pkgname);
			}

			// Progress to next package.
			_packages_pre_unpacked.erase(_pkgname);
		}
		else
		{
			// Commit package status changes.  All packages to be
			// removed or unpacked should by now have been marked
			// as half-unpacked.
			_pb.curstat().commit();

			// Delete trigger shared variables
			if (_triggers) _triggers->delete_shared_vars();
			if (_log) _log->message(LOG_INFO_UNWIND_DONE);

			// Progress to next state.
			state(state_fail);
		}
		break;
	case state_fail:
		// Unpack operation has failed: do nothing.
		break;
	}
}

void unpack::pre_unpack_check_files()
{
	// Test whether file exists, add to set of conflicts if it does.
	string src_pathname=*_files_to_unpack.begin();
	string parent_dir;
	if (src_pathname.back() == '.')
	{
		parent_dir = src_pathname.substr(0, src_pathname.size()-1);
		_empty_dirs_to_create.insert(parent_dir);
	} else
	{
		parent_dir = src_pathname;
		string dst_pathname=_pb.paths()(src_pathname,_pkgname);
		if (object_type(dst_pathname))
		{
			_files_that_conflict.insert(dst_pathname);
		}
	}

	// Make a list of parent directories to check for conflicts
	string::size_type dir_pos = parent_dir.rfind('.');
	while (dir_pos != string::npos)
	{
		parent_dir.erase(dir_pos);
		if (_parent_dirs.insert(parent_dir).second)
		{
			dir_pos = parent_dir.rfind('.');
		} else
		{
			dir_pos = string::npos;
		}					
	}
	_files_to_unpack.erase(src_pathname);			
}

void unpack::pre_unpack_check_empty_dirs()
{
	// Test if an empty directory exists, add it to conflicts
	// if it clashes with a file
	string src_pathname=*_empty_dirs_to_create.begin();
	string dst_pathname=_pb.paths()(src_pathname,_pkgname);
	int type = object_type(dst_pathname);
	if (type != 0 && type != 2)
	{
		_files_that_conflict.insert(dst_pathname);
	}
	_empty_dirs_to_create.erase(src_pathname);
}

void unpack::pre_unpack_check_parent_dirs()
{
	// Test if any of the parent directories that will be automatically
	// created clash with a file
	string src_pathname=*_parent_dirs.begin();
	string dst_pathname=_pb.paths()(src_pathname,_pkgname);
	int type = object_type(dst_pathname);
	switch(type)
	{
		case 0: // Not found
		    // No clash and subdirectories won't clash either
			{
				std::string subdir_start(src_pathname + ".");
				std::set<std::string>::const_iterator it = _parent_dirs.lower_bound(subdir_start);
				while (it != _parent_dirs.end())
				{
					if (subdir_start.compare(0, subdir_start.size(), *it) == 0)
					{
#ifdef TRACE_TO_STD_OUT
						std::cout << "removing subirectory from parent check" << *it << std::endl;
#endif						
						it = _parent_dirs.erase(it);
					} else
					{
						break; // No more to delete
					}					
				}
			}
			break;
		case 2: // directory
		    // Already a directory nothing to do
			break;
		default: // 1 file or 3 image file
#ifdef TRACE_TO_STD_OUT
		    std::cout << "Found file that conflicts with directory " << dst_pathname << std::endl;
#endif			
			_files_that_conflict.insert(dst_pathname);
			_files_to_replace_by_dirs.insert(dst_pathname);
			_files_total_remove++;
			break;
	}
	_parent_dirs.erase(src_pathname);
}

void unpack::pre_unpack_select_package()
{
	// Select package.
	_pkgname=*_packages_to_unpack.begin();
	status curstat=_pb.curstat()[_pkgname];
	const status& selstat=_pb.selstat()[_pkgname];
	const status& prevstat=_pb.prevstat()[_pkgname];

	if (_log) _log->message(LOG_INFO_PREUNPACK, _pkgname);

	// Mark package as half-unpacked (but do not commit until
	// pre-remove and pre-unpack phases have been completed).
	curstat.state(status::state_half_unpacked);
	curstat.version(selstat.version());
	curstat.environment_id(selstat.environment_id());
	curstat.flag(status::flag_auto,selstat.flag(status::flag_auto));
	_pb.curstat().insert(_pkgname,curstat);

	// Check whether standards-version can be processed.
	binary_control_table::key_type key(_pkgname,selstat.version(),selstat.environment_id());
	const control& ctrl=_pb.control()[key];
	if (!can_process(ctrl.standards_version()))
		_packages_cannot_process.insert(_pkgname);

	// Open zip file.
	string pathname=_pb.cache_pathname(_pkgname,selstat.version(),selstat.environment_id());
	delete _zf;
	_exception_item = pathname;
	_zf=new zipfile(pathname);

	// Build manifest from zip file (excluding package control file).
	std::set<string> mf;
	build_manifest(mf,*_zf,&_bytes_total_unpack);
	mf.erase(ctrl_src_pathname);

	// Check if it's a module already on the system in which case
	// we don't need to reinstall it
	if (!already_installed(ctrl, mf))
	{
		// Copy manifest to list of files to be unpacked.
		_files_to_unpack=mf;
		_files_total_unpack+=mf.size()+1;
		bool overwrite_removed=prevstat.state()==status::state_removed;
		if (overwrite_removed) _files_total_remove+=1;

		if (mf.count(pre_install_src_pathname) == 1)
		{
			add_pre_install_trigger(_pkgname, (mf.count(post_remove_src_pathname) == 1));
		}
		if (mf.count(post_install_src_pathname) == 1) add_post_install_trigger(_pkgname);

		// Progress to next package.
		_packages_pre_unpacked.insert(_pkgname);
	}
	_exception_item.clear();
	_packages_to_unpack.erase(_pkgname);
}

void unpack::pre_remove_select_package()
{
	// Select package.
	_pkgname=*_packages_to_remove.begin();
	status curstat=_pb.curstat()[_pkgname];
	if (_log) _log->message(LOG_INFO_PREREMOVE, _pkgname);

	// Mark package as half-unpacked (but do not commit until
	// pre-remove and pre-unpack phases have been completed).
	curstat.state(status::state_half_unpacked);
	_pb.curstat().insert(_pkgname,curstat);

	// Check whether package format is supported.
	binary_control_table::key_type key(_pkgname,curstat.version(),curstat.environment_id());
	const control& ctrl=_pb.control()[key];
	if (!can_process(ctrl.standards_version()))
		_packages_cannot_process.insert(_pkgname);

	// Read manifest from package info directory.
	std::set<string> mf;
	read_manifest(mf,_pkgname);

	// Add manifest to list of files to remove, provided that
	// the files in question do currently exist.
	// Remove manifest from list of files that conflict.
	for (std::set<string>::const_iterator i=mf.begin();i!=mf.end();++i)
	{
		string src_pathname=*i;
		bool is_dir = (src_pathname.back()=='.');
		if (is_dir) src_pathname.erase(src_pathname.size()-1);
		string dst_pathname=_pb.paths()(src_pathname,_pkgname);
		_files_that_conflict.erase(dst_pathname);
		if (object_type(dst_pathname))
		{
			if (is_dir)
			{
				_dirs_to_remove.insert(dst_pathname);
			} else
			{
				_files_to_remove.insert(dst_pathname);
			}
			_files_total_remove+=1;
		}
	}
	_files_total_remove+=1;

	if (mf.count(pre_remove_src_pathname) == 1)	add_pre_remove_trigger(_pkgname);
	if (mf.count(post_install_src_pathname) != 0) set_post_install_unwind(_pkgname);
	if (mf.count(post_remove_src_pathname) == 1) add_post_remove_trigger(_pkgname,mf);

	// Progress to next package.
	_packages_being_removed.insert(_pkgname);
	_packages_to_remove.erase(_pkgname);
}

void unpack::unpack_select_package()
{
	// Select package.
	_pkgname=*_packages_pre_unpacked.begin();
	const status& selstat=_pb.selstat()[_pkgname];
	const status& prevstat=_pb.prevstat()[_pkgname];

	if (_log) _log->message(LOG_INFO_UNPACKING_PACKAGE, _pkgname);

	// Open zip file.
	string pathname=_pb.cache_pathname(_pkgname,selstat.version(),selstat.environment_id());
	delete _zf;
	_zf=new zipfile(pathname);

	// Unpack control file.
	string ctrl_dst_pathname=_pb.paths()(ctrl_src_pathname,_pkgname);
	bool overwrite=prevstat.state()>=status::state_removed;
	unpack_file(ctrl_src_pathname,ctrl_dst_pathname);
	replace_file(ctrl_dst_pathname,overwrite);

	// Build manifest from zip file (excluding package control file).
	std::set<string> mf;
	build_manifest(mf,*_zf);
	mf.erase(ctrl_src_pathname);

	// Copy manifest to list of files to be unpacked.
	_files_to_unpack=mf;
	// _empty_dirs_to_check is not reset as it accumulates all dirs
				
	// Merge with manifest in package info directory.
	read_manifest(mf,_pkgname);
	prepare_manifest(mf,_pkgname);
	activate_manifest(_pkgname);

	// Prepeate final manifest (to be activated later).
	prepare_manifest(_files_to_unpack,_pkgname);

	// Progress to next package.
	_packages_being_unpacked.insert(_pkgname);
	_packages_pre_unpacked.erase(_pkgname);

	if (_log) _log->message(LOG_INFO_UNPACK_FILES, _pkgname);
}

void unpack::state(state_type new_state)
{
	_state = new_state;
	// Only log operations that are going to do anything
	LogCode code = LOG_ERROR_UNINITIALISED;

	switch (_state)
	{
	case state_pre_unpack:
	case state_pre_remove:
		// Not logging as individual packages are logged anyway
		break;
	case state_copy_post_remove:
		if (_triggers && _triggers->post_remove_files_to_copy())
		{
			code = LOG_INFO_COPY_POST_REMOVE;
			state_text("Saving post-remove triggers");
		}
		break;
	case state_run_pre_remove_triggers:
		if (_triggers && _triggers->pre_remove_triggers_to_run())
		{
			code = LOG_INFO_PRE_REMOVE_TRIGGERS;
			state_text("Running pre-remove triggers");
		}
		break;
	case state_remove_files_replaced_by_dirs:
	    code = LOG_INFO_REMOVE_FILES_REPLACED_BY_DIRS;
		state_text("Removing files that will be replaced by directories");
		break;
	    
	case state_unpack:
		// Not logging as unpack is logged for each package
		state_text("Unpacking files");
		break;
	case state_run_pre_install_triggers:
		if (_triggers && _triggers->pre_install_triggers_to_run())
		{
			code = LOG_INFO_PRE_INSTALL_TRIGGERS;
			state_text("Running pre-install triggers");
		}
		break;
	case state_replace:
		if (!_files_being_unpacked.empty())
		{
			code = LOG_INFO_UNPACK_REPLACE;
			state_text("Replacing files");
		}
		break;
	case state_remove:
		if (!_files_to_remove.empty())
		{
			code = LOG_INFO_UNPACK_REMOVE;
			state_text("Removing files");
		}
		break;
	case state_post_remove:
	case state_post_unpack:
		state_text("Removing backups");
		// Not logging as post remove/unpack is logged for each package
		break;
	case state_create_empty_dirs:
	  if (!_empty_dirs_to_create.empty())
		{
			code = LOG_INFO_CREATE_EMPTY_DIRS;
			state_text("Creating empty directories");
		}
		break;

	case state_done:
		code = LOG_INFO_UNPACK_DONE;
		state_text("Finished");
		break;
	case state_unwind_create_empty_dirs:
	  if (!_dirs_created.empty())
		{
			code = LOG_INFO_UNWIND_EMPTY_DIRS;
			state_text("Unwinding after error");
		}
		break;
	case state_unwind_remove:
		if (!_files_being_removed.empty())
		{
			code =  LOG_INFO_UNWIND_REMOVED;
			state_text("Unwinding after error");
		}
		break;
	case state_unwind_replace:
		if (!_files_unpacked.empty())
		{
			code = LOG_INFO_UNWIND_REPLACED_FILES;
			state_text("Unwinding after error");
		}
		break;
	case state_unwind_pre_install_triggers:
		if (_triggers && _triggers->pre_install_to_unwind())
		{
			code = LOG_INFO_UNWIND_PRE_INSTALL_TRIGGERS;
			state_text("Unwinding calling post-remove triggers");
		}
		break;
	case state_unwind_unpack:
		if (!_files_being_unpacked.empty())
		{
			code = LOG_INFO_UNWIND_UNPACK_FILES;
			state_text("Unwinding after error");
		}
		break;
	case state_unwind_remove_files_replaced_by_dirs:
	    code = LOG_INFO_UNWIND_REMOVE_FILES_REPLACED_BY_DIRS;
		state_text("Restoring files that were replaced by directories");
		break;

	case state_unwind_pre_remove_triggers:
		if (_triggers && _triggers->pre_remove_to_unwind())
		{
			code = LOG_INFO_UNWIND_PRE_REMOVE_TRIGGERS;
			state_text("Unwinding calling post-install triggers");
		}
		break;
	case state_unwind_copy_post_remove:
		if (_triggers && _triggers->post_remove_files_to_remove()) code = LOG_INFO_REMOVE_POST_REMOVE_TRIGGERS;
		state_text("Unwinding after error");
		break;
	case state_unwind_pre_remove:
	case state_unwind_pre_unpack:
		// Not logging as unwind pre_remove/pre_unpack is logged for each package
		state_text("Unwinding after error");
		break;
	case state_fail:
		// Not logged as actual error is logged.
		state_text("Failed");
		break;
	}

	if (_log && code != LOG_ERROR_UNINITIALISED)
	{
		_log->message(code);
	}
}

void unpack::state_text(const std::string &text)
{
	if (text != _state_text)
	{
		_state_text = text;
		_state_text_changed = true;
#ifdef TRACE_TO_STD_OUT
		std::cout << "state text changed to " << text << std::endl;
#endif		
	}
}

void unpack::read_manifest(std::set<string>& mf,const string& pkgname)
{
	string prefix=_pb.info_pathname(pkgname);
	string dst_pathname=prefix+string(".")+mf_dst_filename;
	string bak_pathname=prefix+string(".")+mf_bak_filename;

	std::ifstream dst_in(dst_pathname.c_str());
	dst_in.peek();
	while (dst_in&&!dst_in.eof())
	{
		string line;
		getline(dst_in,line);
		if (line.length()) mf.insert(line);
		dst_in.peek();
	}

	std::ifstream bak_in(bak_pathname.c_str());
	bak_in.peek();
	while (bak_in&&!bak_in.eof())
	{
		string line;
		getline(bak_in,line);
		if (line.length()) mf.insert(line);
		bak_in.peek();
	}
}

void unpack::build_manifest(std::set<string>& mf,zipfile& zf,size_type* usize)
{
	std::set<string> dir_names;
	std::set<string> dirs_with_contents;
	std::string::size_type leaf_pos;

	for (unsigned int i=0;i!=zf.size();++i)
	{
		string pathname=zf[i].pathname();
		string src_pathname = zip_to_src(zf[i].pathname());
		if (pathname.size())
		{
			if(pathname[pathname.length()-1]!='/')
			{
				mf.insert(src_pathname);
				if (usize) *usize+=zf[i].usize();
			  	leaf_pos = src_pathname.rfind('.');
				if (leaf_pos != std::string::npos) dirs_with_contents.insert(src_pathname.substr(0,leaf_pos+1));
			} else
			{
			  	leaf_pos = src_pathname.rfind('.',src_pathname.size()-2);
				if (leaf_pos != std::string::npos) dirs_with_contents.insert(src_pathname.substr(0,leaf_pos+1));
				dir_names.insert(src_pathname);
			}
		}
	}
	// Add in empty directories
	if (!dir_names.empty())
	{
		std::set<string>::iterator dir_iter;
		for (dir_iter = dir_names.begin(); dir_iter != dir_names.end(); ++dir_iter)
		{
		   	if (!dirs_with_contents.count(*dir_iter))
			{
				 mf.insert(*dir_iter);
			}
		}
	}
}

void unpack::prepare_manifest(std::set<string>& mf,const string& pkgname)
{
	string prefix=_pb.info_pathname(pkgname);
	string tmp_pathname=prefix+string(".")+mf_tmp_filename;
	_ad(tmp_pathname);
	std::ofstream out(tmp_pathname.c_str());
	for (std::set<string>::const_iterator i=mf.begin();i!=mf.end();++i)
		out << *i << std::endl;
}

void unpack::activate_manifest(const string& pkgname)
{
	string prefix=_pb.info_pathname(pkgname);
	string dst_pathname=prefix+string(".")+mf_dst_filename;
	string tmp_pathname=prefix+string(".")+mf_tmp_filename;
	string bak_pathname=prefix+string(".")+mf_bak_filename;
	_ad(dst_pathname);

	// Backup existing manifest file if it exists.
	if (object_type(dst_pathname)!=0)
	{
		force_move(dst_pathname,bak_pathname,true);
	}

	// Move new manifest file to destination.
	force_move(tmp_pathname,dst_pathname,false);

	// Delete backup.
	force_delete(bak_pathname);
}

void unpack::remove_manifest(const string& pkgname)
{
	string prefix=_pb.info_pathname(pkgname);
	string dst_pathname=prefix+string(".")+mf_dst_filename;
	string tmp_pathname=prefix+string(".")+mf_tmp_filename;
	string bak_pathname=prefix+string(".")+mf_bak_filename;

	_ad(dst_pathname);
	force_delete(dst_pathname);
	force_delete(tmp_pathname);
	force_delete(bak_pathname);
}

void unpack::add_pre_install_trigger(const string &pkgname, bool has_unwind)
{
	if (_trigger_run)
	{
		std::string old_version, new_version;
		get_trigger_versions(pkgname, old_version, new_version);

		if (!_triggers) _triggers = new triggers(_pb, _trigger_run, _log);
		_triggers->add_pre_install(pkgname, old_version, new_version, has_unwind);
	} else if (_warning)
		_warning(LOG_WARNING_NO_TRIGGER_RUN, "pre-install", pkgname);
}

void unpack::add_post_install_trigger(const string &pkgname)
{
	if (_trigger_run)
	{
		std::string old_version, new_version;
		get_trigger_versions(pkgname, old_version, new_version);
		if (!_triggers) _triggers = new triggers(_pb, _trigger_run, _log);
		_triggers->add_post_install(pkgname, old_version, new_version);
	} else if (_warning)
		_warning(LOG_WARNING_NO_TRIGGER_RUN, "post-install", pkgname);
}

void unpack::add_pre_remove_trigger(const string &pkgname)
{
	if (_trigger_run)
	{
		std::string old_version, new_version;
		get_trigger_versions(pkgname, old_version, new_version);
		if (!_triggers) _triggers = new triggers(_pb, _trigger_run, _log);
		_triggers->add_pre_remove(pkgname, old_version, new_version);
	} else if (_warning)
		_warning(LOG_WARNING_NO_TRIGGER_RUN, "pre-remove", pkgname);
}

void unpack::set_post_install_unwind(const string &pkgname)
{
	if (_trigger_run)
	{
		std::string old_version, new_version;
		get_trigger_versions(pkgname, old_version, new_version);
		if (!_triggers) _triggers = new triggers(_pb, _trigger_run, _log);
		_triggers->add_post_install_abort(pkgname, old_version, new_version);
	}
}

void unpack::add_post_remove_trigger(const string &pkgname, std::set<string> &mf)
{
	if (_trigger_run)
	{
		std::string old_version, new_version;
		get_trigger_versions(pkgname, old_version, new_version);
		if (!_triggers) _triggers = new triggers(_pb, _trigger_run, _log);
		_triggers->add_post_remove(pkgname, old_version, new_version);
		std::set<std::string>::iterator ft = mf.lower_bound("RiscPkg.Triggers");
		std::set<std::string>::iterator et = mf.upper_bound("RiscPkg.TriggersX");
		for (std::set<std::string>::iterator i = ft; i != et; ++i)
		{
			_triggers->add_post_remove_file(_pb.paths()(*i, _pkgname));
		}
	} else if (_warning)
		_warning(LOG_WARNING_NO_TRIGGER_RUN, "post-remove", pkgname);
}

void unpack::unpack_file(const string& src_pathname,const string& dst_pathname)
{
#ifdef TRACE_TO_STD_OUT
	std::cout << "unpack::unpack_file " << src_pathname << " to " << dst_pathname << std::endl;
#endif	
	if (dst_pathname.size())
	{
        _exception_item = "unpack file " + dst_pathname;
		string zip_pathname=src_to_zip(src_pathname);
		string tmp_pathname=dst_to_tmp(dst_pathname);

		_ad(tmp_pathname);
		_zf->extract(zip_pathname,tmp_pathname);

		const zipfile::file_info* finfo=_zf->find(zip_pathname);
		if (!finfo) throw file_info_not_found();
		if (const zipfile::riscos_info* rinfo=
			finfo->find_extra<zipfile::riscos_info>())
		{
			write_file_info(tmp_pathname,rinfo->loadaddr(),
			rinfo->execaddr(),rinfo->attr());
		}

		_bytes_done+=finfo->usize();
		_files_done+=1;
		_exception_item.clear();
	}
}

void unpack::replace_file(const string& dst_pathname,bool overwrite)
{
#ifdef TRACE_TO_STD_OUT
	std::cout << "unpack::replace_file " << dst_pathname << " overwrite="<<overwrite << std::endl;
#endif	
	if (dst_pathname.size())
	{
		string tmp_pathname=dst_to_tmp(dst_pathname);
		string bak_pathname=dst_to_bak(dst_pathname);

	    _exception_item = "replace file "+dst_pathname;
		// Force removal of backup pathname.
		// From this point on, if the backup pathname exists
		// then it is a usable backup.
		_ad(bak_pathname);
#ifdef TRACE_TO_STD_OUT
		std::cout << "unpack::replace_file: delete " << bak_pathname << std::endl;
#endif		
		force_delete(bak_pathname);

		if (overwrite)
		{
			// If overwrite enabled then attempt to make backup of
			// existing file, but do not report an error if the
			// file does not exist.
			try
			{
#ifdef TRACE_TO_STD_OUT
				std::cout << "unpack::replace_file: try move " << dst_pathname << " to "<< bak_pathname << std::endl;
#endif				
				force_move(dst_pathname,bak_pathname);
			}
			catch (...) {}
			_files_done+=1;
		}

		// Move file regardless of file attributes, but not regardless
		// of whether destination is present.
		_ad(tmp_pathname);
#ifdef TRACE_TO_STD_OUT
		std::cout << "unpack::replace_file: force move "<<tmp_pathname << " to "<< dst_pathname << std::endl;
#endif		
		force_move(tmp_pathname,dst_pathname);
		_files_done+=1;
		_exception_item.clear();
	}
}

void unpack::remove_file(const string& dst_pathname)
{
	string bak_pathname=dst_to_bak(dst_pathname);
#ifdef TRACE_TO_STD_OUT
	std::cout << "unpack::remove_file " << dst_pathname << " backup " << bak_pathname << std::endl;
#endif	

	// Force removal of backup pathname.
	// From this point on, if the backup pathname exists
	// then it is a usable backup.
	_ad(bak_pathname);
	force_delete(bak_pathname);

	// Attempt to make backup of existing file, but do not report
	// and error if the file does not exist.
	try
	{
		force_move(dst_pathname,bak_pathname);
	}
	catch (...) 
	{
		#ifdef TRACE_TO_STD_OUT
		std::cout << "unpack::remove_file: backup failed" << std::endl;
		#endif
	}
	_files_done+=1;
	#ifdef TRACE_TO_STD_OUT
	std::cout << "unpack::remove_file: done" << std::endl;
	#endif
	_exception_item.clear();
}

void unpack::remove_backup(const string& dst_pathname)
{
	if (dst_pathname.size())
	{
		string bak_pathname=dst_to_bak(dst_pathname);
		#ifdef TRACE_TO_STD_OUT
		std::cout << "unpack::remove_backup of " << dst_pathname << " backup " << bak_pathname << std::endl;
		#endif
		_ad(bak_pathname);
		try
		{
			force_delete(bak_pathname);
		}
		catch (...) {}
		_files_done+=1;
	}
}

void unpack::unwind_remove_file(const string& dst_pathname)
{
#ifdef TRACE_TO_STD_OUT
	std::cout << "unpack::unwind_remove_file "<< dst_pathname << std::endl;
#endif	
	if (dst_pathname.size())
	{
		string bak_pathname=dst_to_bak(dst_pathname);
		_files_done-=1;
		_ad(bak_pathname);
		try
		{
			force_move(bak_pathname,dst_pathname);
		}
		catch (...) {}
	}
}

void unpack::unwind_replace_file(const string& dst_pathname,bool overwrite)
{
#ifdef TRACE_TO_STD_OUT
	std::cout << "unpack::unwind_replace_file " << dst_pathname << " overwrite="<<overwrite << std::endl;
#endif
	if (dst_pathname.size())
	{
		string bak_pathname=dst_to_bak(dst_pathname);

		_bytes_done-=object_length(dst_pathname);
		_files_done-=2;

		if (overwrite)
		{
			_files_done-=1;
			_ad(bak_pathname);

			// try to unwind, but if the file isn't there we can't restore it
			// eg if Control file never got written due to an error
			if (object_type(bak_pathname) != 0)
				force_move(bak_pathname,dst_pathname,true);
		}
		else
		{
			_ad(dst_pathname);
			force_delete(dst_pathname);
		}
	}
}

void unpack::unwind_unpack_file(const string& dst_pathname)
{
#ifdef TRACE_TO_STD_OUT
	std::cout << "unpack::unwind_unpack_file " << dst_pathname << std::endl;
#endif	
	if (dst_pathname.size())
	{
		string tmp_pathname=dst_to_tmp(dst_pathname);
		_bytes_done-=object_length(tmp_pathname);
		_files_done-=1;
		_ad(tmp_pathname);
		force_delete(tmp_pathname);
	}
}

bool unpack::already_installed(const control& ctrl, const std::set<string> &mf)
{
	// Check if the package is a RISC OS module only package
	string module;
	for (std::set<string>::const_iterator lp = mf.begin(); lp != mf.end(); ++lp)
	{
		const string &check = *lp;
		string::size_type dir_pos = check.find('.');
		if (dir_pos != string::npos && check.back() != '.')
		{
			string base_dir = check.substr(0, dir_pos);
			if (base_dir[0] == '!') base_dir.erase(0,1); // remove "!" alias
			// Convert to lower case for easier comparison
			for (string::iterator i = base_dir.begin(); i != base_dir.end(); ++i)
			{
				*i = tolower(*i);
			}
			if (base_dir == "system")
			{
				 if (module.empty()) module = check;
				 else
				 {
				   // Can only have one module in a package
				 	 return false;
				 }
			} else if (base_dir != "riscpkg" && base_dir != "manuals")
			{
				// Package must only be the module, control files and optionally manuals
				return false;
		  }
		}
	}

	if (module.empty()) return false;

	string module_pathname = _pb.paths()(module,_pkgname);

	// We have a module file name at this point
	module_info mod(module_pathname);
	if (!mod.read_ok()) return false; // Not a module or not installed

	if (_log) _log->message(LOG_INFO_MODULE_CHECK, mod.title(), mod.version());

	std::string chk_version(mod.version());
	chk_version += "-1"; // Always check against package version 1

  version curr_version(chk_version), inst_version(ctrl.version());
  if (curr_version < inst_version)
  {
  	 // Need to upgrade current version
		 status curstat=_pb.curstat()[_pkgname];

		 // Proceed as normal if we have a previous version installed
		 // as it will automatically overwrite the existing module
		 if (curstat.state() == status::state_installed) return false;

		 if (_log) _log->message(LOG_INFO_MODULE_REPLACE, _pkgname);
		 // Pretend package is already installed and needs removing
		 _packages_to_remove.insert(_pkgname);
		 // Build manifest so remove processing can pick up files to remove
		 string prefix = _pb.info_pathname(_pkgname);
	   string mf_pathname = prefix + ".Files";
	   pkg::auto_dir ad;
	   ad(mf_pathname);
	   std::ofstream mfs(mf_pathname.c_str(), std::ios_base::trunc);
	   if (mfs)
	   {
    	 for (std::set<string>::const_iterator i=mf.begin();i!=mf.end();++i)
		     mfs << *i << std::endl;
	  	 mfs.close();
	   }

		 return false;
  } else
  {
  	if (_log) _log->message(LOG_INFO_MODULE_USE, _pkgname);
		_files_that_conflict.erase(module_pathname);
		_packages_to_remove.erase(_pkgname);
		_existing_module_packages.insert(_pkgname);

		// Write details to temporary files to update real files if commit succeeds
		// Make new control record with new version
		binary_control new_control;
		for (control::const_iterator f = ctrl.begin(); f != ctrl.end(); ++f)
		{
			new_control[f->first] = f->second;
	  }
	  new_control["Version"] = chk_version;
	  new_control["Description"] = ctrl.description() + "\n* Using already installed version";

	  // Update Info files
	  string prefix = _pb.info_pathname(_pkgname);
	  string ctrl_pathname = prefix + ".Control";
	  string mf_pathname = prefix + ".Files";
	  string cpy_pathname = prefix + ".Copyright";
	  string ctrl_tmp_pathname = ctrl_pathname+"++";
	  string mf_tmp_pathname = mf_pathname+"++";
	  string cpy_tmp_pathname = cpy_pathname+"++";
	  pkg::create_directory(prefix);

	  std::ofstream ncs(ctrl_tmp_pathname.c_str(), std::ios_base::trunc);
	  if (ncs)
	  {
	  		ncs << new_control;
	  		ncs.close();
	  }
	  std::ofstream mfs(mf_tmp_pathname.c_str(), std::ios_base::trunc);
	  if (mfs)
	  {
    	for (std::set<string>::const_iterator i=mf.begin();i!=mf.end();++i)
		    mfs << *i << std::endl;
	  	mfs.close();
	  }
	  std::ofstream cs(cpy_tmp_pathname.c_str(), std::ios_base::trunc);
	  if (cs)
	  {
	  	cs << "This package is using an existing version of the module found on" << std::endl;
	  	cs << "the machine." << std::endl << std::endl;
	  	string help_string = mod.help_string();
	  	string::size_type cr_pos =  help_string.find('\x0d');
	  	if (cr_pos != string::npos) help_string[cr_pos] = '\n';
	  	cs << "Module help string: " << mod.help_string() << std::endl;
	  	cs.close();
	  }

	  return true;
	}
}

void unpack::update_existing_modules()
{
 	for (std::set<string>::iterator i = _existing_module_packages.begin();
 		   i != _existing_module_packages.end(); ++i)
 	{
 	  try
 		{
 			string pkgname = *i;
 			if (_log) _log->message(LOG_INFO_MODULE_UPDATE, pkgname);
			string prefix = _pb.info_pathname(pkgname);
			string ctrl_pathname = prefix + ".Control";
			string mf_pathname = prefix + ".Files";
			string cpy_pathname = prefix + ".Copyright";
			string ctrl_tmp_pathname = ctrl_pathname+"++";
			string mf_tmp_pathname = mf_pathname+"++";
			string cpy_tmp_pathname = cpy_pathname+"++";

	  	// Copy temp files to correct place
	 	  force_move(ctrl_tmp_pathname, ctrl_pathname, true);
	  	force_move(mf_tmp_pathname, mf_pathname, true);
	  	force_move(cpy_tmp_pathname, cpy_pathname, true);

			binary_control new_control;
			std::ifstream is(ctrl_pathname.c_str());
			is >> new_control;
			is.close();

			string chk_version = new_control.version();
			if (!chk_version.empty())
			{
			  // Update main list
			  _pb.control().insert(new_control);
			  _pb.control().commit();

			 	// Mark package as installed with the module version
				status curstat=_pb.curstat()[pkgname];
				curstat.state(status::state_installed);
				curstat.version(chk_version);
				curstat.flag(status::flag_auto,false);
				_pb.curstat().insert(pkgname,curstat);
			} else
			{
				if (_log) _log->message(LOG_WARNING_MODULE_PACKAGE_UPDATE_FAILED, "version missing from new Control file");
			}
		} catch(std::exception &e)
		{
			// Just report errors to the log as it should not effect following installs
		  if (_log) _log->message(LOG_WARNING_MODULE_PACKAGE_UPDATE_FAILED, e.what());
		}
	}
	_existing_module_packages.clear();
}

void unpack::unwind_existing_modules()
{
 	for (std::set<string>::iterator i = _existing_module_packages.begin();
 		   i != _existing_module_packages.end(); ++i)
 	{
 	  try
 		{
 			string pkgname = *i;
 			if (_log) _log->message(LOG_INFO_MODULE_UNWIND, pkgname);
			string prefix = _pb.info_pathname(pkgname);
			string ctrl_pathname = prefix + ".Control";
			string mf_pathname = prefix + ".Files";
			string cpy_pathname = prefix + ".Copyright";
			string ctrl_tmp_pathname = ctrl_pathname+"++";
			string mf_tmp_pathname = mf_pathname+"++";
			string cpy_tmp_pathname = cpy_pathname+"++";

			force_delete(ctrl_tmp_pathname);
			force_delete(mf_tmp_pathname);
			force_delete(cpy_tmp_pathname);
 		} catch(...)
 		{
 			// Ignore errors
 		}
 	}
 	_existing_module_packages.clear();
}

void unpack::get_trigger_versions(const std::string &pkgname, std::string &old_version, std::string &new_version)
{
	const status& selstat = _pb.selstat()[pkgname];
	const status& prevstat = _pb.prevstat()[pkgname];

	if (selstat.state() == status::state_installed)
	{
		new_version = selstat.version();
	} else
	{
		new_version.clear();
	}
	if (prevstat.state() == status::state_installed)
	{
		old_version = prevstat.version();
	} else
	{
		old_version.clear();
	}


}

/* currently the only reason packages cannot be processed is a standards-version mismatch, so make the error descriptive */
unpack::cannot_process::cannot_process():
	runtime_error("A newer version of the package manager is required to install a package. Try finding PackMan in the package list, click the upgrade button, quit and restart it, and try again")
{}

unpack::file_conflict::file_conflict():
	runtime_error("conflict with existing file(s)")
{}

unpack::file_info_not_found::file_info_not_found():
	runtime_error("file information record not found")
{}

}; /* namespace pkg */
