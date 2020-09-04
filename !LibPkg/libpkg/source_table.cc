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

#include <fstream>

#include "libpkg/source_table.h"

namespace pkg {

source_table::source_table(const string& dpathname,const string& pathname):
	_dpathname(dpathname),
	_pathname(pathname)
{
	update();
}

source_table::~source_table()
{}

void source_table::update()
{
	_data.clear();
	bool found=read(_pathname);
	if (!found) read(_dpathname);
	notify();
}

bool source_table::read(const string& pathname)
{
	std::ifstream in(pathname.c_str());
	bool found=in;
	while (in&&!in.eof())
	{
		// Read line from input stream.
		string line;
		getline(in,line);

		// Strip comments and trailing spaces.
		string::size_type n=line.find('#');
		if (n==string::npos) n=line.size();
		while (n&&isspace(line[n-1])) --n;
		line.resize(n);

		// Extract source type and source path.
		string::size_type i=0;
		while ((i!=line.length())&&!isspace(line[i])) ++i;
		string srctype(line,0,i);
		while ((i!=line.length())&&isspace(line[i])) ++i;
		string srcpath(line,i,string::npos);

		// Ignore line if source type not recognised.
		if (srctype==string("pkg"))
		{
			_data.push_back(srcpath);
		}

		// Check for end of file.
		in.peek();
	}
	return found;
}

}; /* namespace pkg */
