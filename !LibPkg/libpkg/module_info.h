// This file is part of LibPkg.
//
// Copyright 2003-2020 Graham Shaw
// Copyright 2014-2020 Alan Buckley
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

#ifndef LIBPKG_MODULE_INFO
#define LIBPKG_MODULE_INFO

#include <string>
#include <stdexcept>


namespace pkg {

/**
 * Class to read the header details from a RISC OS Module
 */
class module_info
{
	std::string _title;
	std::string _help_string;
	std::string _version;
	bool _read_ok;

public:
	/**
	 * Construct an uninitialised module_info object
	 * Call the read method to initialise it.
	 */
	module_info() {_read_ok = false;}
	module_info(const std::string &path);
	bool read(const std::string &path);
	bool lookup(const std::string &title);

	/**
	 * Check if module information has been update successfully
	 *@return true if module information was updated.
	 */
	bool read_ok() const {return _read_ok;}
	/**
	 * Get the module title
	 *@returns title of the module
	 */
	const std::string &title() {return _title;}
	/**
	 * Get module help string
	 *@return help string
	 */
	const std::string &help_string() {return _help_string;}
	/**
	 * Get the module version.
	 *
	 *@return module version as a string (e.g. 1.23)
	 */
	std::string version() const {return _version;}
private:
	void extract_version();
};

}

#endif
