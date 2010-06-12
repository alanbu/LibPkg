// This file is part of LibPkg.
// Copyright © 2003-2010 Graham Shaw.
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
	rollback();
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
		if ((f==_data.end())||(f->second.size()==0))
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

path_table::const_iterator path_table::find(const string& src_pathname)
{
	return _data.find(src_pathname);
}

void path_table::alter(const string& src_pathname,const string& dst_pathname)
{
	_data[src_pathname]=dst_pathname;
}

void path_table::erase(const string& src_pathname)
{
	_data.erase(src_pathname);
	notify();
}

void path_table::clear()
{
	_data.clear();
	notify();
}

void path_table::commit()
{
	// Take no action unless a pathname has been specified.
	if (_pathname.size())
	{
		// Set pathnames.
		string dst_pathname=_pathname;
		string tmp_pathname=_pathname+string("++");
		string bak_pathname=_pathname+string("--");

		// Write new paths file.
		std::ofstream out(tmp_pathname.c_str());
		for (std::map<key_type,mapped_type>::const_iterator i=_data.begin();
			i!=_data.end();++i)
		{
			out << i->first << " = " << i->second << std::endl;
		}
		out.close();
		if (!out) throw commit_error();

		try
		{
			// Backup existing paths file if it exists.
			if (object_type(dst_pathname)!=0)
			{
				force_move(dst_pathname,bak_pathname,true);
			}
		
			// Move new status file to destination.
			force_move(tmp_pathname,dst_pathname,false);
		
			// Delete backup.
			force_delete(bak_pathname);
		}
		catch (...)
		{
			throw commit_error();
		}
	}
}

void path_table::rollback()
{
	// Take no action unless a pathname has been specified.
	if (_pathname.size())
	{
		_data.clear();
		bool done=read(_pathname);
		if (!done) read(_pathname+string("--"));
	}
}

bool path_table::read(const string& pathname)
{
	std::ifstream in(pathname.c_str());
	bool done=in;
	in.peek();
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
		std::vector<string> fields;
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
			if (fields.size()>3)
				throw parse_error("end of line expected");

			// Insert path into table.
			string src_path=fields[0];
			string dst_path=(fields.size()>=3)?fields[2]:"";
			_data[src_path]=dst_path;
		}

		in.peek();
	}
	notify();
	return done;
}

path_table::parse_error::parse_error(const string& message):
	runtime_error(message)
{}

path_table::invalid_source_path::invalid_source_path():
	runtime_error("invalid source path")
{}

path_table::commit_error::commit_error():
	runtime_error("failed to commit path table")
{}

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
