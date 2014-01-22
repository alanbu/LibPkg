// This file is part of LibPkg.
// Copyright © 2003-2005 Graham Shaw.
// Copyright © 2014 Alan Buckley
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.


#include "module_info.h"
#include <fstream>

namespace pkg
{
	
/**
 * Read module information from given path to module file.
 *
 * Check read_ok() method to see if read succeeded
 *
 *@param path full path to the module to read information from
 */
module_info::module_info(const std::string &path)
{
	read(path);
}


/**
 * Read module information from given path to module file.
 *
 * Sets the value returned by read_ok() method 
 *
 *@param path full path to the module to read information from
 *@returns true if read succeeded, false otherwise
 */
bool module_info::read(const std::string &path)
{
	_read_ok = false;
	std::ifstream mod;
	mod.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
	try
	{
		 mod.open(path.c_str(), std::ios_base::binary);
		 unsigned int hdr[6];
		 mod.read(reinterpret_cast<char *>(hdr), sizeof(unsigned int) * 6);
		 mod.seekg(hdr[4]); // title
		 char text[256];
		 mod.get(text, sizeof(text), 0);
		 _title = text;
		 mod.seekg(hdr[5]); // help string
		 mod.get(text, sizeof(text), 0);
		 _help_string = text;
		 char *vp = text;
		 while (*vp && (*vp != ' ' && *vp != '\t')) vp++;
		 while (*vp && (*vp < '0' || *vp > '9')) vp++;
		 _version.clear();
		 while (*vp >= '0' && *vp <= '9') _version += *vp++;
		 if (*vp == '.')
		 {
		 		_version += *vp++;
		    while (*vp >= '0' && *vp <= '9') _version += *vp++;
		 }
		 
		 _read_ok = true;
	} catch(...)
	{
			// Fail silently if read fails
	}
	
	return _read_ok;
}		

}

