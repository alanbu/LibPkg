// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include <fstream>

#include "libpkg/source_table.h"

namespace pkg {

source_table::source_table(const string& pathname):
	_pathname(pathname)
{
	update();
}

source_table::~source_table()
{}

void source_table::update()
{
	_data.clear();
	ifstream in(_pathname.c_str());
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
	notify();
}

}; /* namespace pkg */
