// This file is part of LibPkg.
// Copyright � 2003-2005 Graham Shaw.
// Additions for components
// Copyright � 2013 Alan Buckley.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include <algorithm>
#include <sstream>
#include <tr1/functional>

#include "libpkg/filesystem.h"
#include "libpkg/version.h"
#include "libpkg/binary_control.h"
#include "libpkg/status.h"
#include "libpkg/pkgbase.h"
#include "libpkg/download.h"
#include "libpkg/unpack.h"
#include "libpkg/sysvars.h"
#include "libpkg/sprite_pool.h"
#include "libpkg/commit.h"
#include "libpkg/component.h"
#include "libpkg/component_update.h"
#include "libpkg/boot_options_file.h"
#include "libpkg/triggers.h"
#include "libpkg/os/os.h"

namespace pkg {

commit::commit(pkgbase& pb,const std::set<string>& packages):
	_pb(pb),
	_state(state_paths),
	_packages_to_process(packages),
	_dload(0),
	_upack(0),
	_files_done(0),
	_files_total(npos),
	_bytes_done(0),
	_bytes_total(npos),
	_trigger_run(0),
	_triggers(0),
	_trigger(0),
	_log(0),
	_warnings(0)
{
	// Commit selected state to disc.
	_pb.selstat().commit();

	// Make previous package state equal to current state.
	// (This is the state to which the system will try to return
	// if an error occurs during the commit operation.)
	_pb.prevstat().clear();
	_pb.prevstat().insert(_pb.curstat());
}

commit::~commit()
{
	delete _warnings;
	delete _triggers;
}

void commit::poll()
{
	switch (_state)
	{
	case state_paths:
		if (_log) _log->message(LOG_INFO_START_PATHS);
		if (_packages_to_process.size())
		{
			// Find paths that need to be removed from the boot option files
			for (std::set<std::string>::iterator pi = _packages_to_process.begin();
				  pi != _packages_to_process.end(); ++pi)
			{
				const std::string &pkgname = *pi;
				const status& curstat=_pb.curstat()[pkgname];
				const status& selstat=_pb.selstat()[pkgname];

				// Find control record for old version
				binary_control_table::key_type key(pkgname,curstat.version(),curstat.environment_id());
				const binary_control& ctrl=_pb.control()[key];

				if (selstat.state() <= status::state_removed)
				{
					// Add previous movable components to list to remove from
					if (!ctrl.components().empty())
					{
						try
						{
							std::vector<component> comps;
							std::string comp_str = ctrl.components();
							parse_component_list(comp_str.begin(), comp_str.end(), &comps);
							for (std::vector<component>::iterator i = comps.begin(); i !=comps.end(); ++i)
							{
								if (i->flag(component::movable))
								{
									// Add path name rather than logical name as path name may change
									std::string pathname = _pb.paths()(i->name(), pkgname);
									_components_to_remove.insert(pathname);
									if (_log) _log->message(LOG_INFO_REMOVE_PATH_OPTS, pathname, pkgname);
								}
							}
						} catch(std::exception &e)
						{
							// Failure to remove
							warning(LOG_WARNING_REMOVE_COMPONENT, _pkgname, e.what());
						}
					}
				}
			}

			// Now check to see if any paths need to be changed and update the paths file
			// if they do. The path changes will not be committed until later
			component_update update(_pb.component_update_pathname());
			bool paths_updated = true, paths_modified = false;
			path_table &paths = _pb.paths();

			// A path change may change the default for another component, so loop until
			// no more changes are required.
			while (paths_updated)
			{			
				paths_updated = false;
				for (component_update::const_iterator c = update.begin(); c != update.end(); ++c)
				{
					const component &comp = *c;
					std::string new_path = comp.path();
					if (!new_path.empty())
					{
						std::string current_path = paths(comp.name(), ""); // Only dealing with paths without package names

						if (current_path != new_path)
						{
							new_path = boot_drive_relative(new_path);
							paths.alter(comp.name(), new_path);
							paths_updated = true;
							paths_modified = true;
							if (_log) _log->message(LOG_INFO_PATH_CHANGE, comp.name(), new_path);
						}
					}
				}
			}
			if (paths_modified)
			{
				try
				{
					paths.commit();
				} catch(std::exception &pe)
				{
					if (_log) _log->message(LOG_ERROR_PATHS_COMMIT, pe.what());
					_message = std::string("Failed to update paths for components, error: ") + pe.what();
					_state = state_fail;
					try
					{
						paths.rollback();
					} catch(std::exception &re)
					{
						if (_log) _log->message(LOG_ERROR_PATHS_ROLLBACK, re.what());
					}
				}
			}
		}
		if (_log) _log->message(LOG_INFO_END_PATHS);
		// Proceed to next state
		if (_state != state_fail) _state = state_pre_download;
		break;

	case state_pre_download:
		if (_packages_to_process.size())
		{
			// Select package.
			_pkgname=*_packages_to_process.begin();
			const status& curstat=_pb.curstat()[_pkgname];
			const status& selstat=_pb.selstat()[_pkgname];

			// Find control record.
			binary_control_table::key_type key(_pkgname,selstat.version(),selstat.environment_id());
			if (_log) _log->message(LOG_INFO_PREPROCESS_PACKAGE, _pkgname, selstat.version());
			const binary_control& ctrl=_pb.control()[key];
			if (_log)
			{
				_log->message(LOG_INFO_PACKAGE_ENV, ctrl.package_env()->env_names(), ctrl.package_env()->module_names());
			}

			// Determine whether a download is required.
			// This is true if the package is to be unpacked,
			// but is not present in the cache.
			bool download_req=unpack_req(curstat,selstat);
			if (download_req)
			{
				try
				{
					_pb.verify_cached_file(ctrl);
					download_req=false;
					if (_log) _log->message(LOG_INFO_CACHE_USED, _pkgname);
				}
				catch (pkgbase::cache_error& ex)
				{
					if (_log) _log->message(LOG_INFO_NOT_USING_CACHE,_pkgname, ex.what());
				}
			}

			if (download_req)
			{
				// Create an entry in the progress table.
				size_type size=npos;
				control::const_iterator f=ctrl.find("Size");
				if (f!=ctrl.end())
				{
					std::istringstream in(f->second);
					in >> size;
				}
				_progress_table[_pkgname].bytes_ctrl=size;

				// Progress to next package.
				_packages_to_download.insert(_pkgname);
				_packages_to_process.erase(_pkgname);
			}
			else
			{
				// Progress to next package.
				// Since the package does not need to be downloaded,
				// it is listed to be unpacked.
				_packages_to_unpack.insert(_pkgname);
				_packages_to_process.erase(_pkgname);
			}
		}
		else
		{
			// Progress to next state.
			_state=state_download;
		}
		break;
	case state_download:
		if (_dload)
		{
			// Update download progress.
			update_download_progress();

			// Monitor download state.
			switch (_dload->state())
			{
			case download::state_download:
				// If download in progress then do nothing.
				break;
			case download::state_done:
				// If download complete then verify, and if correct
				// then move to next package.
				{
					delete _dload;
					_dload=0;
					if (_log) _log->message(LOG_INFO_DOWNLOADED_PACKAGE, _pkgname);
					const status& selstat=_pb.selstat()[_pkgname];
					binary_control_table::key_type key(_pkgname,
						selstat.version(),selstat.environment_id());
					const binary_control& ctrl=_pb.control()[key];
					try
					{
						_pb.verify_cached_file(ctrl);
						_packages_to_unpack.insert(_pkgname);
						_packages_to_download.erase(_pkgname);
					}
					catch (pkgbase::cache_error& ex)
					{
						_message=ex.what();
						_state=state_fail;
						if (_log) _log->message(LOG_ERROR_CACHE_INSERT, _pkgname, ex.what());
					}
				}
				break;
			case download::state_fail:
				// If download failed then commit failed too.
				_message=_dload->message();
				delete _dload;
				_dload=0;
				_state=state_fail;
				if (_log) _log->message(LOG_ERROR_PACKAGE_DOWNLOAD_FAILED, _pkgname, _message);
				break;
			}
		}
		else if (_packages_to_download.size())
		{
			// Select package.
			_pkgname=*_packages_to_download.begin();
			const status& selstat=_pb.selstat()[_pkgname];

			// Obtain URL and cache pathname then begin download.
			binary_control_table::key_type key(_pkgname,selstat.version(),selstat.environment_id());
			const binary_control& ctrl=_pb.control()[key];
			string url=ctrl.url();
			string pathname=_pb.cache_pathname(_pkgname,selstat.version(),selstat.environment_id());
			_dload=new download(url,pathname);

			if (_log) _log->message(LOG_INFO_DOWNLOADING_PACKAGE, _pkgname, url);
		}
		else
		{
			// Progress to next state.
			_state=state_unpack;
			_files_done=0;
			_files_total=npos;
			_bytes_done=0;
			_bytes_total=npos;
		}
		break;

	case state_unpack:
		if (_upack)
		{
			// Update progress.
			_files_done=_upack->files_done();
			_files_total=_upack->files_total();
			_bytes_done=_upack->bytes_done();
			_bytes_total=_upack->bytes_total();

			// Monitor state of unpack operation.
			switch (_upack->state())
			{
			case unpack::state_done:
				_triggers = _upack->detach_triggers();
				// If unpack complete then move to next package.
				delete _upack;
				_upack=0;
				_packages_to_configure.swap(_packages_to_unpack);
				_state=state_configure;
				if (_log) _log->message(LOG_INFO_UNPACKED);
				break;
			case unpack::state_fail:
				// If unpack failed then commit failed too.
				_message=_upack->message();
				_files_that_conflict.clear();
				if (!_upack->files_that_conflict().empty())
				{
					_files_that_conflict.insert(
						_upack->files_that_conflict().begin(), 
						_upack->files_that_conflict().end());
				}
				delete _upack;
				_upack=0;
				_state=state_fail;
				if (_log) _log->message(LOG_ERROR_UNPACK_FAILED, _message);
				break;
			default:
				// If unpack operation in progress then do nothing.
				break;
			}
		}
		else
		{
			// Begin unpack operation.
			if (_log) _log->message(LOG_INFO_UNPACKING);
			_upack=new unpack(_pb,_packages_to_unpack);
			_upack->use_trigger_run(_trigger_run);
			_upack->log_to(_log);
			using namespace std::tr1::placeholders;
			std::tr1::function<void(LogCode code, const std::string &item, const std::string &what)> f =
					std::tr1::bind(&commit::warning, this, _1, _2, _3);
			_upack->warning_func(f);
		}
		break;
	case state_configure:
		if (_packages_to_configure.size())
		{
			// Select package.
			_pkgname=*_packages_to_configure.begin();
			const status& curstat=_pb.curstat()[_pkgname];
			const status& selstat=_pb.selstat()[_pkgname];

			// If configuration required then mark package as installed.
			if (config_req(curstat,selstat)&&!unpack_req(curstat,selstat))
			{
				status st=_pb.curstat()[_pkgname];
				st.state(status::state_installed);
				// Ensure environment change is also detected
				st.environment_id(selstat.environment_id());
				_pb.curstat().insert(_pkgname,st);
				if (_log) _log->message(LOG_INFO_INSTALLED, _pkgname);
			}

			// Move to next package.
			_packages_to_purge.insert(_pkgname);
			_packages_to_configure.erase(_pkgname);
		}
		else
		{
			// Commit package status changes.
			_pb.curstat().commit();
			if (_log) _log->message(LOG_INFO_STATE_UPDATE);

			// Progress to next state.
			_state=state_purge;
		}
		break;
	case state_purge:
		if (_packages_to_purge.size())
		{
			// Select package.
			_pkgname=*_packages_to_purge.begin();
			const status& curstat=_pb.curstat()[_pkgname];
			const status& selstat=_pb.selstat()[_pkgname];

			// If purge required then mark package as not present.
			if (purge_req(curstat,selstat)&&!remove_req(curstat,selstat))
			{
				status st=_pb.curstat()[_pkgname];
				st.state(status::state_not_present);
				_pb.curstat().insert(_pkgname,st);
				if (_log) _log->message(LOG_INFO_PURGED, _pkgname);
			}

			// Move to next package.
			_packages_to_purge.erase(_pkgname);
		}
		else
		{
			// Commit package status changes.
			_pb.curstat().commit();
			if (_log) _log->message(LOG_INFO_STATE_UPDATE);
			// Progress to next state.
			_state=state_update_sysvars;
		}
		break;
	case state_update_sysvars:
		{
			// Update list of system variables.

			if (_log) _log->message(LOG_INFO_UPDATING_SYSVARS);
			update_sysvars(_pb);
			if (_log) _log->message(LOG_INFO_SYSVARS_UPDATED);

			// Progress to next state.
			_state=state_update_sprites;
		}
		break;
	case state_update_sprites:
		{
			// Update sprite pool.
			if (_log) _log->message(LOG_INFO_UPDATING_SPRITES);
			update_sprite_pool(_pb);
			if (_log) _log->message(LOG_INFO_SPRITES_UPDATED);

			// Progress to next state.
			_state=state_update_boot_options;
		}
		break;
	case state_update_boot_options:
		{
			// Update boot options
			if (_log) _log->message(LOG_INFO_UPDATING_BOOT_OPTIONS);

			component_update update(_pb.component_update_pathname());
			path_table &paths = _pb.paths();

			std::string option_name("LookAt");

			try
			{
				// Build list of components that need to be added
				for (component_update::const_iterator c = update.begin(); c != update.end(); ++c)
				{
					const component &comp = *c;
					std::string pathname = paths(comp.name(), ""); // Only dealing with paths without package names
					if (object_type(pathname) == 0)
					{
						warning(LOG_WARNING_COMPONENT_NOT_INSTALLED, pathname, "");
					} else
					{
						_components_to_remove.erase(pathname);
						if (comp.flag(component::look_at)) _files_to_boot.insert(pathname);
						if (comp.flag(component::run)) _files_to_run.insert(pathname);
						if (comp.flag(component::add_to_apps)) _files_to_add_to_apps.insert(pathname);
					}
				}

				if (!_files_to_boot.empty() || !_components_to_remove.empty())
				{
					look_at_options look_at;
					for (std::set<std::string>::iterator p = _components_to_remove.begin();
						   p != _components_to_remove.end(); ++p)
					{
						look_at.remove(*p);
					}
					
					for (std::set<std::string>::iterator p = _files_to_boot.begin();
						p != _files_to_boot.end(); ++p)
					{
						look_at.add(*p);
					}
					look_at.commit();
				}

				if (!_files_to_run.empty() || !_components_to_remove.empty())
				{
					option_name = "Run";

					run_options run;
					for (std::set<std::string>::iterator p = _components_to_remove.begin();
						   p != _components_to_remove.end(); ++p)
					{
						run.remove(*p);
					}
					
					for (std::set<std::string>::iterator p = _files_to_run.begin();
						p != _files_to_run.end(); ++p)
					{
						run.add(*p);
						_files_to_boot.insert(*p); // File run need to be booted first
					}
					run.commit();
				}

				if (!_files_to_add_to_apps.empty() || !_components_to_remove.empty())
				{
					option_name = "Add to Apps";

					add_to_apps_options add_to_apps;
					for (std::set<std::string>::iterator p = _components_to_remove.begin();
						   p != _components_to_remove.end(); ++p)
					{
						add_to_apps.remove(*p);
					}
					
					for (std::set<std::string>::iterator p = _files_to_add_to_apps.begin();
						p != _files_to_add_to_apps.end(); ++p)
					{
						add_to_apps.add(*p);
					}
					add_to_apps.commit();
				}

			} catch(std::exception &e)
			{
				warning(LOG_WARNING_BOOT_OPTIONS_FAILED, option_name, e.what());
			}
				
			if (_log)
			{
				_log->message(LOG_INFO_BOOT_OPTIONS_UPDATED);
				if (_files_to_boot.size()) _log->message(LOG_INFO_BOOTING_FILES);			
			}
			_state = state_boot_files;
		}
		break;

	case state_boot_files:
	    if (_files_to_boot.size())
		{
			std::string file_to_boot = *_files_to_boot.begin();
			if (_log) _log->message(LOG_INFO_BOOTING, file_to_boot);
			try
			{
				std::string command("Filer_Boot ");
				command += file_to_boot;
				pkg::os::OS_CLI(command.c_str());
			} catch(std::exception &e)
			{
				warning(LOG_WARNING_BOOTING_FAILED, file_to_boot, e.what());
			}
			_files_to_boot.erase(file_to_boot);
		} else
		{
			// Progress to next state.
			_state = state_run_files;
			if (_log && _files_to_run.size()) _log->message(LOG_INFO_RUNNING_FILES);			
		}
		break;
	case state_run_files:
		if (_files_to_run.size())
		{
			std::string file_to_run = *_files_to_run.begin();
			if (_log) _log->message(LOG_INFO_RUNNING, file_to_run);
			try
			{
				std::string command("Filer_Run ");
				command += file_to_run;
				pkg::os::OS_CLI(command.c_str());
			} catch(std::exception &e)
			{
				warning(LOG_WARNING_RUNNING_FAILED, file_to_run, e.what());
			}
			_files_to_run.erase(file_to_run);
		} else
		{
			// Progress to next state.
			_state = state_add_files_to_apps;
			if (_log && _files_to_add_to_apps.size()) _log->message(LOG_INFO_ADDING_TO_APPS);
		}
		break;
	case state_add_files_to_apps:
		if (_files_to_add_to_apps.size())
		{
			std::string file_to_add = *_files_to_add_to_apps.begin();
			if (_log) _log->message(LOG_INFO_ADDING, file_to_add);
			try
			{
				std::string command("AddApp ");
				command += file_to_add;
				pkg::os::OS_CLI(command.c_str());
			} catch(std::exception &e)
			{
				warning(LOG_WARNING_ADDING_TO_APPS_FAILED, file_to_add, e.what());
			}
			_files_to_add_to_apps.erase(file_to_add);
		} else
		{
			try
			{
				component_update update(_pb.component_update_pathname());
				update.done();
			} catch(...)
			{
				warning(LOG_WARNING_COMPONENT_UPDATE_DONE_FAILED, "","");
			}

			// Progress to next state.
			_state=state_post_remove_triggers;
			if (_log && _triggers && _triggers->post_remove_triggers_to_run())
				_log->message(LOG_INFO_POST_REMOVE_TRIGGERS);
		}
		break;

	case state_post_remove_triggers:
		if (_trigger)
		{
			switch (_trigger->state())
			{
			case trigger::state_error:
				warning(LOG_WARNING_POST_REMOVE_TRIGGER_FAILED, _trigger->pkgname(), _trigger->message());
				_message = _trigger->message();
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
		else if (_triggers && _triggers->post_remove_triggers_to_run())
		{
			_trigger = _triggers->next_post_remove_trigger();
			_trigger->log_to(_log);
			_trigger->run();
		}
		else
		{
			_state = state_post_install_triggers;
			if (_log && _triggers && _triggers->post_install_triggers_to_run())
				_log->message(LOG_INFO_POST_INSTALL_TRIGGERS);
		}
		break;

	case state_post_install_triggers:
		if (_trigger)
		{
			switch (_trigger->state())
			{
			case trigger::state_error:
				warning(LOG_WARNING_POST_INSTALL_TRIGGER_FAILED, _trigger->pkgname(), _trigger->message());
				_message = _trigger->message();
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
		else if (_triggers && _triggers->post_install_triggers_to_run())
		{
			_trigger = _triggers->next_post_install_trigger();
			_trigger->log_to(_log);
			_trigger->run();
		}
		else
		{
			_state = state_cleanup_triggers;
			if (_log && _triggers && _triggers->post_remove_files_to_remove())
				_log->message(LOG_INFO_REMOVE_POST_REMOVE_TRIGGERS);
		}
		break;

	case state_cleanup_triggers:
		if (_triggers && _triggers->post_remove_files_to_remove())
		{
			_triggers->remove_post_remove_file();
		}
		else
		{
			if (_triggers) _triggers->delete_shared_vars();
			_state = state_done;
			if (_log) _log->message(LOG_INFO_COMMIT_DONE);
		}
		break;

	case state_done:
		// Commit operation complete: do nothing.
		break;
	case state_fail:
		// Commit operation failed: do nothing.
		break;
	}
}

void commit::update_download_progress()
{
	// If download active then update progress for that package.
	if (_dload)
	{
		progress& pr=_progress_table[_pkgname];
		pr.bytes_done=_dload->bytes_done();
		pr.bytes_total=_dload->bytes_total();
	}

	// Sum progress over all packages.  Calculate number of packages (count)
	// and number for which a total total is available (known).
	_bytes_done=0;
	_bytes_total=0;
	unsigned int count=0;
	unsigned int known=0;
	for (std::map<string,progress>::const_iterator i=_progress_table.begin();
		i!=_progress_table.end();++i)
	{
		_bytes_done+=i->second.bytes_done;
		if (i->second.bytes_total!=npos)
		{
			// If a total has been obtained during download then use it.
			_bytes_total+=i->second.bytes_total;
			++known;
		}
		else if (i->second.bytes_ctrl!=npos)
		{
			// Otherwise, if there is a total in the control record then
			// use that instead.
			_bytes_total+=i->second.bytes_ctrl;
			++known;
		}
		++count;
	}

	// Use linear extrapolation to estimate packages for which no
	// information is available (provided there is at least one total
	// or estimated total from which to extrapolate).
	if (known) _bytes_total+=(_bytes_total*(count-known))/known;
}


bool commit::has_substate_text() const
{
	return (_upack != 0);
}

bool commit::clear_substate_text_changed()
{
	bool changed = false;
	if (_upack) changed = _upack->clear_state_text_changed();
	return changed;

}

std::string commit::substate_text() const
{
	if (_upack) return _upack->state_text();
	return "";
}

void commit::use_trigger_run(trigger_run *tr)
{
	_trigger_run = tr;
}

void commit::log_to(log *use_log)
{
	_log = use_log;
	if (_log) _log->message(LOG_INFO_START_COMMIT);
}

void commit::warning(LogCode code, const std::string &item, const std::string &what)
{
	if (_log) _log->message(code, item, what);
	if (_warnings == 0)
	{
		_warnings = new log();
		_warnings->message(LOG_INFO_WARNING_INTRO1);
		_warnings->message(LOG_INFO_WARNING_INTRO2);
	}
	_warnings->message(code, item, what);
}

commit::progress::progress():
	bytes_done(0),
	bytes_total(npos),
	bytes_ctrl(npos)
{}

}; /* namespace pkg */
