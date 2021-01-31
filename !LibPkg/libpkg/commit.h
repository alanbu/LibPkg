// This file is part of the LibPkg.
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

#ifndef LIBPKG_COMMIT
#define LIBPKG_COMMIT

#include "libpkg/thread.h"
#include "libpkg/log.h"
#include "libpkg/trigger.h"
#include "libpkg/download.h"

namespace pkg {

class control_binary;
class pkgbase;
class unpack;
class trigger_run;
class triggers;
class trigger;

/** A class for installing, removing and purging packages. */
class commit:
	public thread
{
public:
	/** A type for representing byte counts. */
	typedef unsigned long long size_type;

	/** A null value for use in place of a byte count. */
	static const size_type npos=static_cast<size_type>(-1);

	// An enumeration for describing the state of the commit operation. */
	enum state_type
	{
		/** The state in which paths for components are set */
		state_paths,
		/** The state in which packages are being considered for download. */
		state_pre_download,
		/** The state in which packages are being downloaded. */
		state_download,
		/** The state in which packages are being unpacked or removed. */
		state_unpack,
		/** The state in which packages are being configured. */
		state_configure,
		/** The state in which packages are being purged. */
		state_purge,
		/** The state in which the list of system variables is updated. */
		state_update_sysvars,
		/** The state in which the sprite pool is updated. */
		state_update_sprites,
		/** The state in which the RISC OS boot option files are updated */
		state_update_boot_options,
		/** The state in which files added to the boot look at and boot run files are booted */
		state_boot_files,
		/** The state in which files added to the boot run files are run */
		state_run_files,
		/** The state in which files are added to the current apps virtual directory */
		state_add_files_to_apps,
		/** The state in which post remove triggers are run */
		state_post_remove_triggers,
		/** The state in which post install triggers are run */
		state_post_install_triggers,
		/** The state in which work files and variables for triggers are cleaned up */
		state_cleanup_triggers,
		/** The state in which all operations have been successfully
		 * completed. */
		state_done,
		/** The state in which an error has occurred. */
		state_fail
	};
private:
	/** The package database. */
	pkgbase& _pb;

	/** The current state of the commit operation. */
	state_type _state;

	/** Packages to be processed. */
	std::set<string> _packages_to_process;

	/** Packages for which a download is required. */
	std::set<string> _packages_to_download;

	/** Packages that have not been processed by state_unpack.
	 * Note that packages are only unpacked/removed if their current and
	 * selected status indicates that they need to be. */
	std::set<string> _packages_to_unpack;

	/** Packages that have not been processed by state_configure.
	 * Note that packages are only configured if their current and
	 * selected status indicates that they can be and need to be. */
	std::set<string> _packages_to_configure;

	/** Packages that have not been processed by state_purge.
	 * Note that packages are only purged if their current and
	 * selected status indicates that they can be and need to be. */
	std::set<string> _packages_to_purge;

	/** Components that should be removed */
	std::set<string> _components_to_remove;

	/** Files to be booted */
	std::set<string> _files_to_boot;
	/** Files to be run */
	std::set<string> _files_to_run;
	/** Files to add to apps */
	std::set<string> _files_to_add_to_apps;

	/** The name of the package currently being processed. */
	string _pkgname;

	/** The current download operation, or 0 if none. */
	download* _dload;

	/** The current unpack operation, or 0 if none. */
	unpack* _upack;

	/** The number of files processed. */
	size_type _files_done;

	/** The total number of files to process. */
	size_type _files_total;

	/** The number of bytes processed. */
	size_type _bytes_done;

	/** The total number of bytes to process. */
	size_type _bytes_total;

	/** The error message.
	 * This is meaningful when _state==state_fail. */
	string _message;

	struct progress;

	/** A map giving the current download progress for each package. */
	std::map<string,progress> _progress_table;

	/** The set of destination pathnames that conflict with files
	 * already on the system. */
	std::set<string> _files_that_conflict;

	/** Class to run any triggers */
	trigger_run *_trigger_run;

	/** triggers to run at end of commit */
	triggers *_triggers;

	/** The current post remove/install trigger that is running */
	trigger *_trigger;

	/** Optional commit log */
	log *_log;

	/** Log created for any warnings */
	log *_warnings;

	/** Additional options for the downloads */
	pkg::download::options *_download_options;

public:
	/** Construct commit operation.
	 * @param pb the package database
	 * @param packages the set of packages to process
	 */
	commit(pkgbase& pb,const std::set<string>& packages);

	/** Destroy commit operation. */
	virtual ~commit();

	/** Get current state of the commit operation.
	 * @return the current state
	 */
	state_type state() const
		{ return _state; }

	/** Get number of files processed.
	 * @return the number of files processed (in current stage)
	 */
	size_type files_done() const
		{ return _files_done; }

	/** Get total number of files to process.
	 * @return the total number of files to process (in current stage)
	 */
	size_type files_total() const
		{ return _files_total; }

	/** Get number of bytes processed.
	 * @return the number of bytes processed (in current stage)
	 */
	size_type bytes_done() const
		{ return _bytes_done; }

	/** Get total number of bytes to process.
	 * @return the total number of bytes to process (in current stage)
	 */
	size_type bytes_total() const
		{ return _bytes_total; }

	/** Get error message.
	 * When state()==state_fail, this function returns a human-readable
	 * description of what went wrong.
	 * @return the error message
	 */
	string message() const
		{ return _message; }

	bool has_substate_text() const;
	bool clear_substate_text_changed();
	std::string substate_text() const;

	/** Get the set of destination pathnames that conflict with files
	 * already on the system.
	 * When state()==state_fail, this function returns a list of files
	 * that must be deleted before the given set of packages can be
	 * processed.
	 */
	const std::set<string>& files_that_conflict() const
		{ return _files_that_conflict; }

	/** Set the class to run triggers */
	void use_trigger_run(trigger_run *tr);

	/** Set the log to add to
	 * @param use_log log to use or 0 to stop logging
	 */
	void log_to(log *use_log);

	/** Return warnings log.
	 * @returns warnings log or 0 if there were no warnings
	 */
	log *warnings() const {return _warnings;}
    /** Detach warnings log.
	 * Detach the warnings log so it doesn't get destroyed
	 * with the commit object. It is then the responsibility
	 * of the object that detached the log to delete it.
	 * @returns warnings log or 0 if there were no warnings
	 */
	log *detach_warnings() {log *w = _warnings; _warnings = 0; return w;}

    /**
	 * Set additional options for any downloads required
	 * @param options for download
	 */
	void download_options(const download::options &options);

protected:
	virtual void poll();
private:
	/** Update reported progress of download.
	 * This function recalculates the number of bytes downloaded and
	 * the total number of bytes to download.
	 */
	void update_download_progress();

	/**
	 * Log/Report non-fatal configuration failures.
	 * @param code Log error code for logging
	 * @param item item error occurred upon
	 * @param what additional error details.
	 */
	void warning(LogCode code, const std::string &item, const std::string &what);
};

/** A structure for monitoring the download progress of one source. */
struct commit::progress
{
	/** The number of bytes downloaded. */
	size_type bytes_done;
	/** The total number of bytes to download, or npos if not known. */
	size_type bytes_total;
	/** The total number of bytes specified in the control record,
	 * or npos if no total was given. */
	size_type bytes_ctrl;
	/** Construct progress structure.
	 * By default no bytes have been downloaded, the total to download
	 * is unknown, and there is no total from the control record. */
	progress();
};

}; /* namespace pkg */

#endif
