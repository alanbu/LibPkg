// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include <fstream>

#include "source_table.h"

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
		string line;
		getline(in,line);
		string::size_type n=line.find('#');
		if (n==string::npos) n=line.size();
		while (n&&isspace(line[n-1])) --n;
		line.resize(n);
		if (n) _data.push_back(line);
		in.peek();
	}
	notify();
}

}; /* namespace pkg */
