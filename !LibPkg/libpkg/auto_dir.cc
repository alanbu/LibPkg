// This file is part of LibPkg.
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

#include <algorithm>

#include "libpkg/os/os.h"

#include "libpkg/auto_dir.h"

namespace pkg {

namespace {

/** Get length of prefix common to pathnames.
 * The common prefix is composed of complete path components that occur
 * in both pathnames.  Thus, the common prefix for the pathnames "a.b1.c"
 * and "a.b2.c" is "a" (as opposed to "a.b").
 * @param a the first pathname
 * @param b the second pathname
 * @return the length of the common prefix.
 */
unsigned int common(const string& a,const string& b)
{
	unsigned int i=0;
	unsigned int e=std::min(a.length(),b.length());
	while ((i<e)&&(a[i]==b[i])) ++i;
	while ((i>0)&&(a[i-1]!='.')) --i;
	if (i>0) --i;
	return i;
}

}; /* anonymous namespace */

auto_dir::auto_dir()
{}

auto_dir::~auto_dir()
{}

void auto_dir::operator()(const string& pathname)
{
	unsigned int i=_pathname.length();
	unsigned int j=pathname.length();
	unsigned int k=common(pathname,_pathname);
	unsigned int type=0;

	// Remove component from pathname.
	// If common prefix not reached then deleted directory and repeat.
	while (i>k)
	{
		--i;
		while ((i>k)&&(_pathname[i]!='.')) --i;
		if (i>k)
		{
			// There is no need to check whether the directory
			// is empty, because an attempt to delete a non-empty
			// directory should fail.
			string dirname(_pathname,0,i);
			try
			{
				// only try and delete the directory if it is a directory,
				// not an image file
				pkg::os::OS_File17(dirname.c_str(),&type,0,0,0,0);
				if (type==2)
					pkg::os::OS_File6(dirname.c_str(),0,0,0,0,0);
 			}
			catch (...) {}
		}
	}

	// Add component to pathname.
	// If requested pathname not reached then create directory and repeat.
	while (i<j)
	{
		++i;
		while ((i<j)&&(pathname[i]!='.')) ++i;
		if (i<j)
		{
			string dirname(pathname,0,i);
			try
			{
				pkg::os::OS_File8(dirname.c_str(),0);
			}
			catch (...) {}
		}
	}

	_pathname=pathname;
}

}; /* namespace pkg */
