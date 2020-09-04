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

#ifndef LIBPKG_TRIGGERS
#define LIBPKG_TRIGGERS

#include <string>
#include <map>
#include <set>

#include "libpkg/auto_dir.h"

namespace pkg
{
	class pkgbase;
	class trigger;	
	class trigger_run;
	class log;

	/**
	 * A class to manage the package triggers executed during the
	 * commiting of the packages. 
	 */
	class triggers
	{
		pkgbase &_pb;
		trigger_run *_trigger_run;
		log *_log;
		bool _shared_vars_deleted;

		/** A structure with information on the versions for
		 * packages with triggers */
		struct trigger_info
		{
			trigger_info() : has_install_unwind(false), has_remove_unwind(false) {}
			trigger_info(const std::string &oldver, const std::string &newver) :
				old_version(oldver), new_version(newver),
				has_install_unwind(false), has_remove_unwind(false)
			{}
		
			/** The version of a package being removed */
			std::string old_version;
			/** The version of a package being installed */
			std::string new_version;
			/** A script to unwind the pre-install trigger exists */
			bool has_install_unwind;
			/** A script to unwind the pre-remove trigger exists */
			bool has_remove_unwind;
		};
		/** A map of packages to the versions to be commited */
		std::map<std::string, trigger_info> _packages;
		/** Packages with pre install triggers */
		std::set<std::string> _pre_install;
		/** Packages with post install triggers */
		std::set<std::string> _post_install;
		/** Packages with pre remove triggers */
		std::set<std::string> _pre_remove;
		/** Packages with post remove triggers */
		std::set<std::string> _post_remove;
		/** Files to be saved for running post remove triggers */
		std::set<std::string> _post_remove_files_to_copy;
		/** Files saved for running post remove triggers */
		std::set<std::string> _post_remove_files;
		/** Packages where the pre remove trigger has been run */
		std::set<std::string> _pre_install_unwind;
		/** Packages where the pre remove trigger has been run */
		std::set<std::string> _pre_remove_unwind;
		/** Class to create directories for post remove triggers */
		auto_dir _ad;
	public:
		/**
		 * Construct the class to maintain lists and create triggers
		 * @param tr the trigger_run object that will run the triggers
		 * @param lg the log to log trigger output to. Can be 0 for no logging.
		 */
		triggers(pkgbase &pb, trigger_run *tr, log *lg);
		~triggers();

		/** Add pre install trigger to list of triggers
		* @param pkgname name of package containing the trigger
		* @param old_version version of package the package to remove or "" if install only
		* @param new_version version of the package to install or "" if remove only
		* @param has_unwind post remove script exists for unwind
		*/
		void add_pre_install(const std::string &pkgname, const std::string &old_version, const std::string &new_version, bool has_unwind);

		/** Add post install trigger to list of triggers
		* @param pkgname name of package containing the trigger
		* @param old_version version of package the package to remove or "" if install only
		* @param new_version version of the package to install or "" if remove only
		*/
		void add_post_install(const std::string &pkgname, const std::string &old_version, const std::string &new_version);

		/** Add pre remove trigger to list of triggers
		* @param pkgname name of package containing the trigger
		* @param old_version version of package the package to remove or "" if install only
		* @param new_version version of the package to install or "" if remove only
		*/
		void add_pre_remove(const std::string &pkgname, const std::string &old_version, const std::string &new_version);

		/** Add post remove trigger to list of triggers
		* @param pkgname name of package containing the trigger
		* @param old_version version of package the package to remove or "" if install only
		* @param new_version version of the package to install or "" if remove only
		*/
		void add_post_remove(const std::string &pkgname, const std::string &old_version, const std::string &new_version);

		/** Set package being removed has a post install trigger.
		* @param pkgname name of package containing the trigger
		* @param old_version version of package the package to remove or "" if install only
		* @param new_version version of the package to install or "" if remove only
		*/
		void add_post_install_abort(const string &pkgname, const std::string &old_version, const std::string &new_version);

		/** Add file to list of files that need to be copied
		* so they are available to run after the package has
		* been removed.
		* @param filename the path to the file that will be copied
		*/
		void add_post_remove_file(const std::string &filename);

		/** Check if there are any post remove triggers left to copy */
		bool post_remove_files_to_copy() const;
		/** Copy one post remove trigger file for use later */
		bool copy_post_remove_file();

		/** Check if there are any pre remove triggers to run */
		bool pre_remove_triggers_to_run() const;
		/** Remove and return the next pre remove trigger to run */
		trigger *next_pre_remove_trigger();
		/** Check if there are any pre install triggers to run */
		bool pre_install_triggers_to_run() const;
		/** Remove and return the next pre install trigger to run */
		trigger *next_pre_install_trigger();
		/** Check if there are any pre install triggers to unwind */
		bool pre_install_to_unwind() const;
		/** Return a post remove trigger to be called to unwind the
		 * action of a pre install trigger */
		trigger *next_pre_install_unwind();
		/** Check if there are any pre remove triggers to remove */
		bool pre_remove_to_unwind() const;
		/** Return a post install trigger to be called to unwind the
		 * action of a pre remove trigger */
		trigger *next_pre_remove_unwind();
		/** Check if there are any post remove trigger copies to remove */
		bool post_remove_files_to_remove() const;
		/** Remove a post remove copied field */
		void remove_post_remove_file();

		/** Check if there are any post remove triggers to run */
		bool post_remove_triggers_to_run() const;
		/** Remove and return the next post remove trigger to run */
		trigger *next_post_remove_trigger();
		/** Check if there are any post install triggers to run */
		bool post_install_triggers_to_run() const;
		/** Remove and return the next postinstall trigger to run */
		trigger *next_post_install_trigger();

		/** Delete the shared environmental variables created by
		 * the triggers.
		 * i.e. All variables starting PkgTrigger$S_
		 */
		void delete_shared_vars();
	};

}

#endif
