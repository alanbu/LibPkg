// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include "rtk/os/os.h"

#include "libpkg/filesystem.h"

namespace pkg {

string canonicalise(const string& pathname)
{
	// Calculate required buffer size.
	unsigned int size=0;
	rtk::os::OS_FSControl37(pathname.c_str(),0,0,0,size,&size);
	size=1-size;

	// Canonicalise pathname.
	char buffer[size];
	rtk::os::OS_FSControl37(pathname.c_str(),buffer,0,0,size,&size);
	return string(buffer);
}

void force_delete(const string& pathname)
{
	// Do not report an error if the file does not exist.
	try
	{
		// Change file attributes to unlocked.
		rtk::os::OS_File4(pathname.c_str(),0x33);

		// Delete file.
		rtk::os::OS_File6(pathname.c_str(),0,0,0,0,0);
	}
	catch (...) {}
}

void force_move(const string& src_pathname,const string& dst_pathname,
	bool overwrite)
{
	if (overwrite)
	{
		// If overwrite flag set then attempt to delete destination,
		// but do not report an error if it does not exist.
		try
		{
			// Change file attributes to unlocked.
			rtk::os::OS_File4(dst_pathname.c_str(),0x33);

			// Delete file.
			rtk::os::OS_File6(dst_pathname.c_str(),0,0,0,0,0);
		}
		catch (...) {}
	}

	// Read source file attributes.
	unsigned int attr;
	rtk::os::OS_File17(src_pathname.c_str(),0,0,0,0,&attr);

	// Change source file attributes to unlocked (if necessary).
	if (attr&0x08) rtk::os::OS_File4(src_pathname.c_str(),attr&~0x08);

	// Move file.
	rtk::os::OS_FSControl25(src_pathname.c_str(),dst_pathname.c_str());

	// Change destination file attributes to locked (if appropriate).
	if (attr&0x08) rtk::os::OS_File4(dst_pathname.c_str(),attr);
}

void write_file_info(const string& pathname,unsigned int loadaddr,
	unsigned int execaddr,unsigned int attr)
{
	// Write load address, execution address and file attributes.
	rtk::os::OS_File1(pathname.c_str(),loadaddr,execaddr,attr);
}

unsigned int object_type(const string& pathname)
{
	// Read object type.
	unsigned int objtype;
	rtk::os::OS_File17(pathname.c_str(),&objtype,0,0,0,0);
	return objtype;
}

unsigned int object_length(const string& pathname)
{
	// Read object length.
	unsigned int length;
	rtk::os::OS_File17(pathname.c_str(),0,0,0,&length,0);
	return length;
}

}; /* namespace pkg */
