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


#include "module_info.h"
#include <fstream>
#include <libpkg/os/call_swi.h>
#include <libpkg/os/osswi.h>

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
		 extract_version();
		 
		 _read_ok = true;
	} catch(...)
	{
			// Fail silently if read fails
	}
	
	return _read_ok;
}		

/**
 * Look up module in loaded module list
 *
 * @param module title (Can use "." as a wild card)
 * @returns true if module exists
 */
bool module_info::lookup(const std::string &title)
{
	_read_ok = false;
	try
	{
		_kernel_swi_regs regs;
		regs.r[0] = 18;
		regs.r[1] = reinterpret_cast<int>(title.c_str());
		os::call_swi(swi::OS_Module, &regs);
		unsigned int *module_start = reinterpret_cast<unsigned int *>(regs.r[3]);
		if (!module_start[4] || !module_start[5])
		{
			// Can't access module information
			return false;
		}
		_title = ((char *)module_start) + module_start[4];
		_help_string = ((char *)module_start) + module_start[5];
		extract_version();
		_read_ok = true;
	} catch(...)
	{
		// Assume module isn't there on failure
	}
	return _read_ok;
}

/**
 * Extract version from module help string
 */
void module_info::extract_version()
{
	 const char *vp = _help_string.c_str();
	 while (*vp && (*vp != ' ' && *vp != '\t')) vp++;
	 while (*vp && (*vp < '0' || *vp > '9')) vp++;
	 _version.clear();
	 while (*vp >= '0' && *vp <= '9') _version += *vp++;
	 if (*vp == '.')
	 {
	 		_version += *vp++;
	    while (*vp >= '0' && *vp <= '9') _version += *vp++;
	 }
}

}

