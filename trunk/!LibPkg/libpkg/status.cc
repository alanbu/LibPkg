// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include <vector>
#include <map>
#include <iostream>

#include "libpkg/status.h"

namespace pkg {

static vector<string> state_to_string;
static vector<string> flag_to_string;
static map<string,status::state_type> string_to_state;
static map<string,status::flag_type> string_to_flag;

static void init_state_string(status::state_type state,const string& s)
{
	unsigned int size=state+1;
	if (size>state_to_string.size()) state_to_string.resize(size);
	state_to_string[state]=s;
	string_to_state[s]=state;
}

static void init_flag_string(status::flag_type flag,const string& s)
{
	unsigned int size=flag+1;
	if (size>flag_to_string.size()) flag_to_string.resize(size);
	flag_to_string[flag]=s;
	string_to_flag[s]=flag;
}

static void init_static_data()
{
	static bool init=false;
	if (!init)
	{
		init_state_string(status::state_not_present,"not-present");
		init_state_string(status::state_removed,"removed");
		init_state_string(status::state_half_unpacked,"half-unpacked");
		init_state_string(status::state_unpacked,"unpacked");
		init_state_string(status::state_half_configured,"half-configured");
		init_state_string(status::state_installed,"installed");
		init_flag_string(status::flag_auto,"auto");
		init_flag_string(status::flag_hold,"hold");
		init=true;
	}
}

status::status():
	_state(state_not_present),
	_flags(0),
	_iflags(0)
{}

status::status(state_type state,const string& version):
	_state(state),
	_flags(0),
	_iflags(0),
	_version(version)
{}

status::~status()
{}

void status::state(state_type state)
{
	_state=state;
}

void status::flag(flag_type flag,bool value)
{
	if (value) _flags|=(1<<flag);
	else _flags&=~(1<<flag);
}

void status::flag(internal_flag_type flag,bool value)
{
	if (value) _iflags|=(1<<flag);
	else _iflags&=~(1<<flag);
}

void status::version(const string& version)
{
	_version=version;
}

status::parse_error::parse_error(const string& message):
	_message(message)
{}

status::parse_error::~parse_error()
{}

const char* status::parse_error::what() const
{
	return _message.c_str();
}

bool operator==(const status& lhs,const status& rhs)
{
	return (lhs.state()==rhs.state())&&
		(lhs.flags()==rhs.flags())&&
		(lhs.version()==rhs.version());
}

bool operator!=(const status& lhs,const status& rhs)
{
	return (lhs.state()!=rhs.state())||
		(lhs.flags()!=rhs.flags())||
		(lhs.version()!=rhs.version());
}

ostream& operator<<(ostream& out,const pair<string,status>& pkgstat)
{
	// Initialise state_to_string and flag_to_string.
	init_static_data();

	// Write package name.
	out << pkgstat.first << '\t';

	// Write package version.
	out << pkgstat.second.version() << '\t';

	// Write installation state.
	out << state_to_string[pkgstat.second.state()] << '\t';

	// Write flags.
	bool firstflag=true;
	for (map<string,status::flag_type>::const_iterator
		i=string_to_flag.begin();i!=string_to_flag.end();++i)
	{
		if (pkgstat.second.flag((*i).second))
		{
			if (firstflag) firstflag=false;
			else out << ',';
			out << (*i).first;
		}
	}
	cout << endl;
	return out;
}

istream& operator>>(istream& in,pair<string,status>& pkgstat)
{
	// Initialise string_to_state and string_to_flag.
	init_static_data();

	// Read line from input stream.
	string line;
	getline(in,line);

	// Split line into tab-separated fields.
	vector<string> fields;
	string::const_iterator first=line.begin();
	string::const_iterator last=line.end();
	string::const_iterator p=first;
	bool done=false;
	while (!done)
	{
		string::const_iterator q=p;
		while ((p!=last)&&(*p!='\t')) ++p;
		fields.push_back(string(q,p));
		if ((p!=last)&&(*p=='\t')) ++p;
		else done=true;
	}

	// Check number of fields.
	if (fields.size()!=4)
		throw status::parse_error("incorrect number of fields");

	// Parse package name.
	if (!fields[0].length())
		throw status::parse_error("missing package name");
	pkgstat.first=fields[0];

	// Parse package version.
	pkgstat.second.version(fields[1]);

	// Parse installation state.
	map<string,status::state_type>::const_iterator f=
		string_to_state.find(fields[2]);
	if (f==string_to_state.end())
		throw status::parse_error("unrecognised installation state");
	pkgstat.second.state((*f).second);

	// Parse flags.
	string flags=fields[3];
	if (flags.length())
	{
		string::const_iterator first=flags.begin();
		string::const_iterator last=flags.end();
		string::const_iterator p=first;
		bool done=false;
		while (!done)
		{
			string::const_iterator q=p;
			while ((p!=last)&&(*p!=',')) ++p;
			map<string,status::flag_type>::const_iterator f=
				string_to_flag.find(string(q,p));
			if (f==string_to_flag.end())
				throw status::parse_error("unrecognised status flag");
			pkgstat.second.flag((*f).second,true);
			if ((p!=last)&&(*p==',')) ++p;
			else done=true;
		}
	}
	return in;
}

bool unpack_req(const status& curstat,const status& selstat)
{
	return (selstat.state()>=status::state_unpacked)&&
		((curstat.state()<status::state_unpacked)||
		(curstat.version()!=selstat.version()));
}

bool remove_req(const status& curstat,const status& selstat)
{
	return (curstat.state()>status::state_removed)&&
		((selstat.state()<=status::state_removed)||
		(curstat.version()!=selstat.version()));
}

bool config_req(const status& curstat,const status& selstat)
{
	return (selstat.state()>=status::state_installed)&&
		((curstat.state()<status::state_installed)||
		(curstat.version()!=selstat.version()));
}

bool purge_req(const status& curstat,const status& selstat)
{
	return (selstat.state()<=status::state_not_present)&&
		(curstat.state()>status::state_not_present);
}

}; /* namespace pkg */
