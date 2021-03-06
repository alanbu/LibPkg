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

#include "libpkg/filesystem.h"
#include "libpkg/status.h"
#include "libpkg/status_table.h"

namespace pkg {

status_table::status_table(const string& pathname):
	_pathname(pathname)
{
	rollback();
}

status_table::~status_table()
{}

const status_table::mapped_type&
	status_table::operator[](const key_type& key) const
{
	static mapped_type default_value;
	const_iterator f=_data.find(key);
	return (f!=_data.end())?f->second:default_value;
}

status_table::const_iterator status_table::find(const key_type& key) const
{
	return _data.find(key);
}

void status_table::insert(const key_type& key,const mapped_type& value)
{
	_data[key]=value;
	notify();
}

void status_table::insert(const status_table& table)
{
	for (const_iterator i=table.begin();i!=table.end();++i)
	{
		_data[i->first]=i->second;
	}
	notify();
}

void status_table::clear()
{
	_data.clear();
	notify();
}

void status_table::commit()
{
	// Take no action unless a pathname has been specified.
	if (_pathname.size())
	{
		// Set pathnames.
		string dst_pathname=_pathname;
		string tmp_pathname=_pathname+string("++");
		string bak_pathname=_pathname+string("--");
	
		// Write new status file.
		static mapped_type default_value;
		std::ofstream out(tmp_pathname.c_str());
		for (const_iterator i=_data.begin();i!=_data.end();++i)
		{
			if (i->second!=default_value)
				out << *i << std::endl;
		}
		out.close();
		if (!out) throw commit_error();
	
		try
		{
			// Backup existing status file if it exists.
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

void status_table::rollback()
{
	// Take no action unless a pathname has been specified.
	if (_pathname.size())
	{
		_data.clear();
		bool done=read(_pathname);
		if (!done) read(_pathname+string("--"));
	}
}

bool status_table::read(const string& pathname)
{
	std::ifstream in(pathname.c_str());
	bool done=in;
	in.peek();
	while (in&&!in.eof())
	{
		std::pair<key_type,mapped_type> p;
		in >> p;
		_data[p.first]=p.second;
		in.peek();
	}
	return done;
}

status_table::commit_error::commit_error():
	runtime_error("failed to commit status table")
{}

}; /* namespace pkg */
