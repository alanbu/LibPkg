// This file is part of LibPkg.
// Copyright � 2003-2005 Graham Shaw.
// Copyright � 2014 Alan Buckley
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

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
