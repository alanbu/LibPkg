// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include <vector>
#include <fstream>

#include "libpkg/filesystem.h"
#include "libpkg/path_table.h"

namespace pkg {

path_table::path_table(const string& pathname):
	_pathname(pathname)
{
	update();
}

path_table::~path_table()
{}

string path_table::operator()(const string& src_pathname,
	const string& pkgname) const
{
	// Split path into source prefix and suffix.
	unsigned int ds=src_pathname.find('.');
	if (ds==string::npos) ds=src_pathname.length();
	string src_prefix(src_pathname,0,ds);
	string suffix(src_pathname,ds,string::npos);

	// Find destination prefix.
	const_iterator f=_data.find(src_prefix);
	if (f==_data.end()) throw invalid_source_path();
	string dst_prefix=f->second;

	// Replace '@' with package name.
	string::size_type i=dst_prefix.find('@');
	while (i!=string::npos)
	{
		dst_prefix.replace(i,1,pkgname);
		i=dst_prefix.find('@');
	}

	// Canonicalise and return.
	return canonicalise(dst_prefix+suffix);
}

void path_table::update()
{
	_data.clear();
	ifstream in(_pathname.c_str());
	while (in&&!in.eof())
	{
		// Read line from input stream.
		string line;
		getline(in,line);

		// Strip comment
		string::size_type n=line.find('#');
		if (n==string::npos) n=line.size();
		line.resize(n);

		// Split line into space-separated fields.
		vector<string> fields;
		string::const_iterator first=line.begin();
		string::const_iterator last=line.end();
		string::const_iterator p=first;
		while ((p!=last)&&isspace(*p)) ++p;
		while (p!=last)
		{
			string::const_iterator q=p;
			while ((p!=last)&&!isspace(*p)) ++p;
			fields.push_back(string(q,p));
			while ((p!=last)&&isspace(*p)) ++p;
		}

		if (fields.size())
		{
			// Check syntax.
			if ((fields.size()<2)||(fields[1]!="="))
				throw parse_error("= expected");
			if (fields.size()<3)
				throw parse_error("destination path expected");
			if (fields.size()>3)
				throw parse_error("end of line expected");

			// Insert path into table.
			string src_path=fields[0];
			string dst_path=fields[2];
			_data[src_path]=dst_path;
		}

		in.peek();
	}
	notify();
}

path_table::parse_error::parse_error(const string& message):
	_message(message)
{}

path_table::parse_error::~parse_error()
{}

const char* path_table::parse_error::what() const
{
	return _message.c_str();
}

path_table::invalid_source_path::invalid_source_path()
{}

path_table::invalid_source_path::~invalid_source_path()
{}

const char* path_table::invalid_source_path::what() const
{
	return "invalid source path";
}

}; /* namespace pkg */
