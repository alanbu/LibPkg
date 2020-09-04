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

#ifndef LIBPKG_UNPACK
#define LIBPKG_UNPACK

#include <string>
#include <set>
#include <tr1/functional>
#include "string.h"

#include "libpkg/auto_dir.h"
#include "libpkg/thread.h"
#include "libpkg/log.h"

namespace pkg {

using std::string;

class pkgbase;
class zipfile;
class log;
class triggers;
class trigger_run;
class trigger;

/** Comparison that does not take into account the case of the string */
struct case_insensitive_cmp { 
    bool operator() (const std::string& a, const std::string& b) const {
        return stricmp(a.c_str(), b.c_str()) < 0;
    }
};

/** A class for unpacking and removing sets of packages. */
class unpack:
	public thread
{
public:
	/** A type for representing byte counts. */
	typedef unsigned long long size_type;

	/** A null value for use in place of a byte count. */
	static const size_type npos=static_cast<size_type>(-1);

	/** An enumeration for describing the state of the unpack operation. */
	enum state_type
	{
		/** The state in which the states of packages to be unpacked are
		 * changed to status::state_half_unpacked.
		 */
		state_pre_unpack,
		/** The state in which the states of packages to be removed are
		 * changed to status::state_half_unpacked.
		 */
		state_pre_remove,
		/** The state in which post remove triggers are copied so they
		 * are preserved to be run after the packages have been removed */
		state_copy_post_remove,
		/** The state in which the pre remove triggers are run */
		state_run_pre_remove_triggers,
		/** The state in which files are moved that will overwritten
		 * by auto create directories */
		state_remove_files_replaced_by_dirs,
		/** The state in which files are unpacked from their zip archives
		* and moved to temporary locations. */
		state_unpack,
		/** The state in which pre install triggers are run */
		state_run_pre_install_triggers,
		/** The state in which old versions of files are backed up and
		 * replaced with new versions. */
		state_replace,
		/** The state in which old versions of files that do not have
		 * replacements are backed up then removed. */
		state_remove,
		/** The state in which backups are deleted, and the states of
		 * packages to be removed are changed to status::state_removed.
		 */
		state_post_remove,
		/** The state in which any empty directories that need creating
		 * are created.
		 */
		state_create_empty_dirs,
		/** The state in which the states of packages to be unpacked
		 * are changed to status::state_unpacked. */
		state_post_unpack,
		/** The state in which all operations have been successfully
		 * completed. */
		state_done,
		/** The state in which empty diwas created are removed */
		state_unwind_create_empty_dirs,
		/** The state in which state_remove is being backed out. */
		state_unwind_remove,
		/** The state in which state_replace is being backed out. */
		state_unwind_replace,
		/** The state in which post remove triggers are run to unwind the
		 * actions of the pre install triggers */
		state_unwind_pre_install_triggers,
		/** The state in which state_unpack is being backed out. */
		state_unwind_unpack,
		/** The state in which files replaced by directories are restored */
		state_unwind_remove_files_replaced_by_dirs,
		/** The state in which post install triggers are run to unwind the
		 * actions of the pre remove and/or pre install triggers */		 
		state_unwind_pre_remove_triggers,
		/** The state in which post remove trigger copies are removed */
		state_unwind_copy_post_remove,
		/** The state in which state_pre_remove is being backed out. */
		state_unwind_pre_remove,
		/** The state in which state_pre_unpack is being backed out. */
		state_unwind_pre_unpack,
		/** The state in which an error has occurred and an attempt
		 * has been made to back out changes. */
		state_fail
	};
private:
	/** The package database. */
	pkgbase& _pb;

	/** The current state of the unpack operation. */
	state_type _state;

	/** An auto_dir object for automatically creating and deleting
	 * directories.*/
	auto_dir _ad;

	/** The current zip file access object, or 0 if there is none. */
	zipfile* _zf;

	/** The current package name, or the empty string if none. */
	string _pkgname;

	/** The number of files processed. */
	size_type _files_done;

	/** The total number of files to process. */
	size_type _files_total;

	/** The number of bytes processed. */
	size_type _bytes_done;

	/** The total number of bytes to process. */
	size_type _bytes_total;

	/** The total number of files to unpack. */
	size_type _files_total_unpack;

	/** The total number of files to remove. */
	size_type _files_total_remove;

	/** The total number of bytes to unpack. */
	size_type _bytes_total_unpack;

	/** The error mewas
	 * This is meaniwasil. */
	string _message;

	/** The set of packages that are to be unpacked.
	 * Packages in this set have not yet changed state. */
	std::set<string> _packages_to_unpack;

	/** The set of packages that have been pwas-unpacked.
	 * Packages in this set are in state_halwasunpacked. */
	std::set<string> _packages_pre_unpacked;

	/** The set of packages that are being unpacked.
	 * Packages in this set are in state_half_unpacked. */
	std::set<string> _packages_being_unpacked;

	/** The set of packages that have been fully unpacked.
	 * Packages in this set are in state_unpacked. */
	std::set<string> _packages_unpacked;

	/** The set of packages that are to be removed.
	 * Packages in this set have not yet changed state. */
	std::set<string> _packages_to_remove;

	/** The set of packages that are being removed.
	 * Packages in this set are in state_half_unpacked. */
	std::set<string> _packages_being_removed;

	/** The set of packages that have been fully removed (or upgraded).
	 * Packages in this set are in state_removed or state_unpacked. */
	std::set<string> _packages_removed;

	/** The set of source pathnames (for the current package) that have
	 * not yet been unpacked. */
	std::set<string> _files_to_unpack;

	/** The set of destination pathnames (for all packages) that have
	 * been unpacked to their temporary locations. */
	std::set<string> _files_being_unpacked;

	/** The set of destination pathnames (for all packages) that have
	 * been unpacked to their final locations. */
	std::set<string> _files_unpacked;

    /** The set of source empty directory names (for the current package) that have
	 * not yet been created. */
	std::set<string> _empty_dirs_to_create;

	/** The set of parent directories that will be automatically created
	 * during the unpack */
	std::set<string> _parent_dirs;

	/** The set of files that need to be moved as they will be replaced by
	 *  an auto created directories */
	std::set<string> _files_to_replace_by_dirs;

    /** The set of files that were replaced by auto created directories */
	std::set<string> _files_replaced_by_dirs;

	/** The set of destination pathnames (for all packages) that have
	 * not yet been removed. */
	std::set<string, case_insensitive_cmp> _files_to_remove;

    /** The set of destination empty directories (for all packages) that
	 * have not yet been removed */
	std::set<string, case_insensitive_cmp> _dirs_to_remove;

	/** The set of destination pathnamwases (for all packages) that have
	 * been backed up prior to removal, but not yet fully removed. */
	std::set<string> _files_being_removed;

	/** The set of destination pathnames (for all packages) that have
	 * been fully removed. */
	std::set<string> _files_removed;

    /** The set of empty directories to check to see if they need creating
	 */
	std::set<string> _empty_dirs_to_check;
	/** The set of directories that hawasve been removed */
	std::set<string> _dirs_removed;
	/** The set of directories that have been created */
	std::set<string> _dirs_created;

	/** The set of packages that cannot be processed. */
	std::set<string> _packages_cannot_process;

	/** The set of destination pathnames (for all packages) that conflict
	 * with files already on the system. */
	std::set<string, case_insensitive_cmp> _files_that_conflict;
		
	/** The set of module packages where an existing module fulfills
	 * the requirement for the package */
	std::set<string> _existing_module_packages;

	/** The class to manage the package triggers */
	triggers *_triggers;

	/** The class used to execute triggers */
	trigger_run *_trigger_run;

	/** The currently executing trigger */
	trigger *_trigger;

	/** The log to use. Can be 0 for no logging  */
	pkg::log *_log;

	/** The function to log and report warnings */
	std::tr1::function<void(LogCode code, const std::string &item, const std::string &what)> _warning;

	/** The state text has just changed */
	bool _state_text_changed;
	/** The current state text */
	std::string _state_text;

	/** The filename/item being dealt with for reporting with an excetion */
	std::string _exception_item;

public:
	/** Construct unpack object.
	 * @param pb the package database
	 * @param packages the set of packages to process
	 */
	unpack(pkgbase& pb,const std::set<string>& packages);

	/** Destroy unpack object. */
	virtual ~unpack();

	/** Get current state of the unpack operation.
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

	/**
	 * Check is state text has changed and clear the changed flag
	 */
	bool clear_state_text_changed()
	{
		if (_state_text_changed) {_state_text_changed=false; return true;}
		else return false;
	}

	/** Return the current state text */
	const std::string &state_text() const
		{ return _state_text; }

	/** Get the set of packages that cannot be processed.
	 * When state()==state_fail, this function returns a list of packages
	 * that cannot be processed until the package manager has been
	 * upgraded.
	 * @return the set of packages that cannot be processed
	 */
	const std::set<string>& packages_cannot_process() const
		{ return _packages_cannot_process; }

	/** Get the set of destination pathnames that conflict with files
	 * already on the system.
	 * When state()==state_fail, this function returns a list of files
	 * that must be deleted before the given set of packages can be
	 * processed.
	 */
	const std::set<string, case_insensitive_cmp>& files_that_conflict() const
		{ return _files_that_conflict; }

	/** Set the class to run triggers */
	void use_trigger_run(trigger_run *tr);

    /** Set the log to add the unpack messages to */
    void log_to(pkg::log *use_log);

    /** Set the function to log and capture warnings */
    void warning_func(std::tr1::function<void(LogCode code, const std::string &item, const std::string &what)> &f) {_warning = f;}

	/** Detach triggers for use later in the commit stage.
	 * This should done if the unpack was successful.
	 * The object that called this memory will now own the
	 * triggers and must delete them when they are no
	 * longer required.
	 */
	triggers *detach_triggers();

protected:
	void poll();
private:
	/** Poll this thread, without exception handling.
	 * This function is equivalent to poll(), except that it does not
	 * handle exceptions.  The reason for this split is to make the code
	 * more manageable.
	 */
	void _poll();

	/** Check files to unpack for conflicts */
	void pre_unpack_check_files();
	/** Check new empty directories for conflicts */
	void pre_unpack_check_empty_dirs();
	/** Check parents directories of unpacked files for conflicts */
	void pre_unpack_check_parent_dirs();
    /** Select the next package install/upgrade to process and prepare file lists */
	void pre_unpack_select_package();
	/** Select the next package to remove and prepare file lists */
	void pre_remove_select_package();
	/** Select the next package to unpack */
	void unpack_select_package();

	/** Change the state, logging new state if logging is on */
	void state(state_type new_state);

	/** Change the state text */
	void state_text(const std::string &text);

	/** Read manifest from info directory.
	 * The set passed into this function is not cleared before use,
	 * so any pathnames within it will be merged into the result.
	 * @param mf a set to hold the result
	 * @param pkgname the package name
	 */
	void read_manifest(std::set<string>& mf,const string& pkgname);

	/** Build manifest from package file.
	 * The set passed into this function is not cleared before use,
	 * so any pathnames within it will be merged into the result.
	 * The uncompressed size of each file is added to the byte
	 * count if one is specified.
	 * @param mf a set to hold the result
	 * @param zf a zipfile access object for the package file
	 * @param usize a byte count to which the rounded uncompressed size
	 *  of each file is added, or 0 if none
	 */
	void build_manifest(std::set<string>& mf,zipfile& zf,size_type* usize=0);

	/** Prepare manifest for activation.
	 * The manifest is written to a file called "Files++" in the info
	 * directory.  Any existing file of that name is overwritten.
	 * @param mf a set holding the manifest
	 * @param pkgname the package name
	 */
	void prepare_manifest(std::set<string>& mf,const string& pkgname);

	/** Activate manifest.
	 * In the info directory, "Files" is backed up to "Files--" then
	 * replaced by "Files++".
	 * @param pkgname the package name
	 */
	void activate_manifest(const string& pkgname);

	/** Remove manifest.
	 * In the info directory, "Files", "Files--" and "Files++" are
	 * removed.  It is not an error if any or all of them do not exist.
	 * @param pkgname the package name
	 */
	void remove_manifest(const string& pkgname);

	/** Add pre install trigger to the list of triggers to be run later
	 * @param pkgname the name of the package that the trigger belongs to
	 */
	void add_pre_install_trigger(const string &pkgname, bool has_unwind);

	/** Add post install trigger to the list of triggers to be run later
	* @param pkgname the name of the package that the trigger belongs to
	*/
	void add_post_install_trigger(const string &pkgname);

	/** Add pre remove trigger to the list of triggers to be run later
	* @param pkgname the name of the package that the trigger belongs to
	*/
	void add_pre_remove_trigger(const string &pkgname);
	/** Set flag to indicate package has a post install trigger to use
	 * during unwind.
	 * @param pkgname the name of the package that the trigger belongs to
	 */
	void set_post_install_unwind(const string &pkgname);

	/** Add post remove trigger to the list of triggers to be run later
	* @param pkgname the name of the package that the trigger belongs to
	* @param mf the set of files to be removed used to preserve the post
	* remove triggers after the package has been deleted.
	*/
	void add_post_remove_trigger(const string &pkgname, std::set<std::string> &mf);

	/** Unpack file from package.
	 * The file is unpacked into the temporary subdirectory "~RiscPkg++".
	 * @param src_pathname the pathname wrt the root of the zip file
	 * @param dst_pathname the pathname wrt the filesystem
	 * @param usize a byte count to which the uncompressed size of each
	 *  file is added, or 0 if none
	 */
	void unpack_file(const string& src_pathname,const string& dst_pathname);

	/** Replace file with copy unpacked from package, after making backup.
	 * The file is backed up to the subdirectory "~RiscPkg--", then
	 * replaced with the copy from "~RiscPkg++".
	 * @param dst_pathname the pathname wrt the filesystem
	 * @param overwrite true to silently replace existing file,
	 *  false to throw error if file exists
	 */
	void replace_file(const string& dst_pathname,bool overwrite);

	/** Remove file, after making backup.
	 * The file is backed up to the subdirectory "~RiscPkg--".
	 * It is not an error if the file does not exist.
	 * @param dst_pathname the pathname wrt the filesystem
	 */
	void remove_file(const string& dst_pathname);

	/** Remove backup of file.
	 * The backup, in the subdirectory "~RiscPkg--", is deleted.
	 * @param dst_pathname the pathname wrt the filesystem
	 */
	void remove_backup(const string& dst_pathname);
	/** Unwind removal of file.
	 * The file is restored from the ba_dirs_to_createckup in "~RiscPkg--".
	 * @param dst_pathname the pathname wrt the filesystem
	 */
	void unwind_remove_file(const string& dst_pathname);

	/** Unwind replacement of file.
	 * If the overwrite flag is false then the file is simply deleted,
	 * otherwise it is restored from the backup in "~RiscPkg--".
	 * Note that it is /not/ moved back to "~RiscPkg++" (so there is
	 * no need to call unwind_unpack_file() once a file has been
	 * processed by this function).
	 * @param dst_pathname the pathname wrt the filesystem
	 */
	void unwind_replace_file(const string& dst_pathname,bool overwrite);

	/** Unwind unpacking of file.
	 * The unpacked copy of the file in "~RiscPkg++" is deleted.
	 * @param dst_pathname the pathname wrt the filesystem
	 */
	void unwind_unpack_file(const string& dst_pathname);
	
	/** Check if a module is already installed and use
	 * existing version if it is
	 * If it is an existing module, this will update the package
	 * status files and the database with the version found if
	 * it was not installed by the package manager
	 *
	 * @param ctrl package control record
	 * @param mf Manifest of package to install
	 * @returns true if the package has already been installed of
	 * the correct or better version.
	 */
	 bool already_installed(const control& ctrl, const std::set<string> &mf);
	 
	 /** Update the database with any existing modules which meant
	  * a package did not have to be installed. */
	 void update_existing_modules();

     /** Clear up temporary files for existing modules on error */	 
	 void unwind_existing_modules();
	 
	 /** Get version package versions for triggers
	  * @param pkgname package name
	  * @param old_version set to currently installed version "" if not installed
	  * @param new_version set to version to be installed or "" if being removed
	  */
	 void get_trigger_versions(const std::string &pkgname, std::string &old_version, std::string &new_version);

	class cannot_process;
	class file_conflict;
	class file_info_not_found;
	class riscos_info_not_found;
};

}; /* namespace pkg */

#endif
