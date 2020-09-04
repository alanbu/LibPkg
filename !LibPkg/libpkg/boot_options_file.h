// This file is part of the LibPkg.
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


#ifndef LIBPKG_BOOT_OPTIONS_FILE
#define LIBPKG_BOOT_OPTIONS_FILE

#include <string>
#include <vector>
#include <stdexcept>


namespace pkg {

/**
 * Base class to manipulate the RISC OS boot options files in Choices
 */
class boot_options_file
{
public:
	class commit_error;

private:
	std::string _read_pathname;
	std::string _write_pathname;
	const char *_section_prefix;
	const char *_section_version;
	const char *_section_suffix;
	const char *_command;
	const char *_command2;
	char *_file_contents;
	char *_section;
	char *_end_section;
	std::vector<std::string> _apps;
	std::string _boot_drive;
	bool _modified;

public:
	boot_options_file(const char *file_name, const char *section_prefix, const char *section_version, const char *section_suffix, const char *command, const char *command2 = 0);
	virtual ~boot_options_file();

	void rollback();
	void commit();

	bool contains(const std::string &app) const;
	bool add(const std::string &app);
	bool remove(const std::string &app);
	bool replace(const std::string &was_app, const std::string &app);

	/**
	 * The path name to the location of the PreDesk file that
	 * contains the look at declarations for reading
	 */
	const std::string &read_pathname() const {return _read_pathname;}
	/**
	 * The path name to the location of the PreDesk file that
	 * any changes to the look at declarations will be written to
	 */
	const std::string &write_pathname() const {return _write_pathname;}
	
	void use_test_pathname(const std::string &pathname);
	bool has_section() const;
	bool modified() const {return _modified;}
	bool contains_raw(const std::string &app) const;

	void dump_apps() const;
	
protected:
	virtual char *find_insert_section() = 0;
	char *find_section(const char *name, const char *suffix);
	void parse_section();
	bool parse_word(char *&pos, std::string &word) const;
	char *find_section_end(char *section) const;
	char *next_line(char *line) const;
	std::string name_in_section(const std::string &app) const;
};

/** An exception class for reporting failure to commit a boot options file. */
class boot_options_file::commit_error:
	public std::runtime_error
{
public:
	/** Construct commit error. */
	commit_error();
};

/**
 * Class to configure the look at options file in the Desktop file
 */
class look_at_options : public boot_options_file
{
public:
	/** Constructor sets up base class with the correct values for the look at options */
	look_at_options() : boot_options_file("Desktop", "RISCOS BootBoot", "0.01", "Boot","Filer_Boot") {}
	virtual ~look_at_options() {}
protected:
	virtual char *find_insert_section()
	{
		char *found = find_section("Acorn BootBoot", "Boot");
		if (!found) found = find_section("RISCOS !Boot", "Auto tasks");
		return found;
	};
};

/**
 * Class to configure the run options in the Desktop file
 */
class run_options : public boot_options_file
{
public:
	/** Constructor sets up base class with the correct values for the look at options */
	run_options() : boot_options_file("Desktop", "RISCOS BootRun", "0.01", "Run", "Filer_Boot", "Filer_Run") {}
	virtual ~run_options() {}

protected:
	virtual char *find_insert_section()
	{
		char *found = find_section("Acorn BootRun", "Run");
		if (!found) found = find_section("RISCOS BootBoot", "Boot");
		if (!found) found = find_section("RISCOS !Boot", "Auto tasks");
		return found;
	}

};

/**
 * Class to configure the add to apps option section in the PreDeskop file
 */
class add_to_apps_options : public boot_options_file
{
public:
	/** Constructor sets up base class with the correct values for the look at options */
	add_to_apps_options() : boot_options_file("PreDesktop","RISCOS BootApps","0.01","ResApps","AddApp") {}

protected:
	virtual char *find_insert_section()
	{
		char *found = find_section("Acorn BootApps", "ResApps");
		if (!found) found = find_section("RISCOS !Boot", "ResApps");
		return found;
	}
};

} // end of namespace

#endif
