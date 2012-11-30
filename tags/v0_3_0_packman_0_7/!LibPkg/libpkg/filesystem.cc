// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include "rtk/os/os.h"

#include "libpkg/filesystem.h"
#include <iostream>

namespace pkg {

string canonicalise(const string& pathname)
{
	// Calculate required buffer size.
	unsigned int size=0;
	//std::cout << "canonicalise: " << pathname;
	rtk::os::OS_FSControl37(pathname.c_str(),0,0,0,size,&size);
	//std::cout << std::hex << " size=" << size << std::endl;
	size=1-size;

	// Canonicalise pathname.
	char buffer[size];
	rtk::os::OS_FSControl37(pathname.c_str(),buffer,0,0,size,&size);
	//std::cout << "canonicalise: result=" << string(buffer) << std::endl;
	return string(buffer);
}

void force_delete(const string& pathname)
{
	// Do not report an error if the file does not exist.
	try
	{
		// Change file attributes to unlocked.
		//std::cout << "force_delete: " << pathname << " unlocking, ";
		rtk::os::OS_File4(pathname.c_str(),0x33);

		// Delete file.
		//std::cout << "deleting, ";
		rtk::os::OS_File6(pathname.c_str(),0,0,0,0,0);
	}
	catch (...) {}
	//catch (...) {std::cout << "failed!";}
	//std::cout << " done" << std::endl;
}

void soft_delete(const string& pathname)
{
	// try to delete a file, without forcing it unlocked
	// and don't worry if it failed
	try
	{
		// Delete file.
		//std::cout << "soft_delete: " << pathname;
		rtk::os::OS_File6(pathname.c_str(),0,0,0,0,0);
	}
	catch (...) {}
	//catch (...) {std::cout << " - failed!";}
	//std::cout << std::endl;
}

void force_move(const string& src_pathname,const string& dst_pathname,
	bool overwrite)
{

	//std::cout << "force_move: " << src_pathname << " to " << dst_pathname << 
	//	" overwrite=" << overwrite << std::endl;
	if (overwrite)
	{
		// If overwrite flag set then attempt to delete destination,
		// but do not report an error if it does not exist.
		try
		{
			// Change file attributes to unlocked.
			//std::cout << "force_move: Unlock dest" << std::endl;
			rtk::os::OS_File4(dst_pathname.c_str(),0x33);

			// Delete file.
			//std::cout << "force_move: Delete dest" << std::endl;
			rtk::os::OS_File6(dst_pathname.c_str(),0,0,0,0,0);
		}
		catch (...) {}
		//catch (...) {std::cout << "force_move: failed" << std::endl;}
	}

	// Read source file attributes.
	unsigned int attr;
	//std::cout << "force_move: attributes=";
	rtk::os::OS_File17(src_pathname.c_str(),0,0,0,0,&attr);
	//std::cout << attr << std::endl;

	// Change source file attributes to unlocked (if necessary).
	if (attr&0x08) {
		//std::cout << "force_move: unlock src" << std::endl;
		rtk::os::OS_File4(src_pathname.c_str(),attr&~0x08);
	}

	// Move file.
	//std::cout << "force_move: Rename src path " << src_pathname << " to " << dst_pathname << std::endl;
	rtk::os::OS_FSControl25(src_pathname.c_str(),dst_pathname.c_str());

	// Change destination file attributes to locked (if appropriate).
	if (attr&0x08) {
		//std::cout << "force_move: lock dest" << std::endl;
		rtk::os::OS_File4(dst_pathname.c_str(),attr);
	}
}

void copy_object(const string& src_pathname,const string& dst_pathname)
{
	//std::cout << "copy_object: " << src_pathname << " to " << dst_pathname << std::endl;
	rtk::os::OS_FSControl26(src_pathname.c_str(),dst_pathname.c_str(),
		0x0201,0,0,0);
}

void create_directory(const string& pathname)
{
	//std::cout << "create_directory: " << pathname << std::endl;
	rtk::os::OS_File8(pathname.c_str(),0);
}

void write_file_info(const string& pathname,unsigned int loadaddr,
	unsigned int execaddr,unsigned int attr)
{
	// Write load address, execution address and file attributes.
	//std::cout << std::hex << "write_file_info: " << pathname << " load=" << loadaddr << " execaddr=" << execaddr
	//	<< "attr=" << attr << std::endl;
	rtk::os::OS_File1(pathname.c_str(),loadaddr,execaddr,attr);
}

void write_filetype(const string& pathname,unsigned int filetype)
{
	//std::cout << std::hex << "write_filetype: " << pathname << " type=" <<filetype;
	rtk::os::OS_File18(pathname.c_str(),filetype);
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
