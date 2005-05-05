// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include <vector>
#include <fstream>

#include "libpkg/filesystem.h"
#include "libpkg/path_table.h"

namespace pkg {

path_table::path_table(const string& dpathname,const string& pathname):
	_dpathname(dpathname),
	_pathname(pathname)
{
	update();
}

path_table::~path_table()
{}

string path_table::operator()(const string& src_pathname,
	const string& pkgname) const
{
	// Find longest matching source prefix.
	string::size_type ds=src_pathname.size();
	const_iterator f=_data.end();
	while ((f==_data.end())&&(ds!=string::npos))
	{
		string src_prefix(src_pathname,0,ds);
		f=_data.find(src_prefix);
		if (f==_data.end())
		{
			if (ds) ds=src_pathname.rfind('.',ds-1);
			else ds=string::npos;
		}
	}

	// Extract destination prefix.
	if (f==_data.end()) throw invalid_source_path();
	string dst_prefix=f->second;

	// Extract suffix.
	string suffix(src_pathname,ds,string::npos);

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
	read(_dpathname);
	read(_pathname);
}

void path_table::read(const string& pathname)
{
	ifstream in(pathname.c_str());
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

string resolve_pathrefs(const path_table& paths,const string& in)
{
	// Specify prefix to logical path name variable.
	static const string prefix("Packages$@");

	// Construct output buffer.
	string out;
	out.reserve(in.length());

	// Process input string.
	string::size_type i=0;
	while (i!=in.length())
	{
		char ch=in[i];
		if (ch=='|')
		{
			// Handle escape character ('|').
			// Pass both it and the character that follows literally.
			out+=in[i++];
			if (i!=in.length())
			{
				out+=in[i++];
			}
		}
		else if (ch=='<')
		{
			// Handle variable start character.
			// Look ahead to determine whether it is the start of a
			// logical pathname variable.
			if (in.substr(i+1,prefix.length())==prefix)
			{
				string::size_type f=in.find('>',i+1+prefix.length());
				if (f!=string::npos)
				{
					// If a logical pathname variable has been found
					// then isolate the pathname and convet it.
					i+=1+prefix.length();
					string log_pathname=in.substr(i,f-i);
					string phy_pathname=paths(log_pathname,"");
					i=f+1;
					out+=phy_pathname;
				}
				else
				{
					out+=in[i++];
				}
			}
			else
			{
				out+=in[i++];
			}
		}
		else
		{
			out+=in[i++];
		}
	}
	return out;
}

}; /* namespace pkg */
