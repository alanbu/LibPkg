// This file is part of the LibPkg.
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
#include "libpkg/binary_control_table.h"

namespace pkg {

binary_control_table::binary_control_table(const string& pathname):
	_pathname(pathname)
{
	update();
}

binary_control_table::~binary_control_table()
{}

const binary_control_table::mapped_type&
	binary_control_table::operator[](const key_type& key) const
{
	static mapped_type default_value;
	std::map<key_type,mapped_type>::const_iterator f=_data.find(key);
	return (f!=_data.end())?f->second:default_value;
}

bool binary_control_table::contains(const key_type& key) const
{
	return _data.find(key) != _data.end();
}

const binary_control_table::mapped_type&
	binary_control_table::operator[](const string& pkgname) const
{
	static mapped_type default_value;
	key_type key(pkgname,version(),string());
	std::map<key_type,mapped_type>::const_iterator f=_data.lower_bound(key);
	if (f==_data.end()) return default_value;
	if (f->first.pkgname!=pkgname) return default_value;
	const mapped_type* result=&f->second;
	while ((++f!=_data.end())&&(f->first.pkgname==pkgname))
		result=&f->second;
	return *result;
}

void binary_control_table::insert(const mapped_type& ctrl)
{
	key_type key(ctrl.pkgname(),ctrl.version(), ctrl.environment_id());
	_data[key]=ctrl;
	notify();
}

void binary_control_table::update()
{
	_data.clear();
	std::ifstream in(_pathname.c_str());
	in.peek();
	while (in&&!in.eof())
	{
		binary_control ctrl;
		in >> ctrl;
		string pkgname=ctrl.pkgname();
		version pkgvrsn=ctrl.version();
		string id = ctrl.environment_id();
		key_type key(pkgname,pkgvrsn,id);
		std::map<key_type,mapped_type>::const_iterator f=_data.find(key);
		if (f==_data.end()) _data[key]=ctrl;
		while (in.peek()=='\n') in.get();
	}
	notify();
}

void binary_control_table::commit()
{
	// Take no action unless a pathname has been specified.
	if (_pathname.size())
	{
		// Set pathnames.
		string dst_pathname=_pathname;
		string tmp_pathname=_pathname+string("++");
		string bak_pathname=_pathname+string("--");

		// Write new control file.
		std::ofstream out(tmp_pathname.c_str());
		for (const_iterator i=_data.begin();i!=_data.end();++i)
		{
			out << i->second << std::endl;
		}
		out.close();
		if (!out) throw commit_error();
	
		try
		{
			// Backup existing control file if it exists.
			if (object_type(dst_pathname)!=0)
			{
				force_move(dst_pathname,bak_pathname,true);
			}
		
			// Move new control file to destination.
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

binary_control_table::key_type::key_type()
{}

binary_control_table::key_type::key_type(const string& _pkgname,
	const version& _pkgvrsn,
	const string & _pkgenv):
	pkgname(_pkgname),
	pkgvrsn(_pkgvrsn),
	pkgenv(_pkgenv)
{}

bool operator<(const binary_control_table::key_type& lhs,
	const binary_control_table::key_type& rhs)
{
	if (lhs.pkgname!=rhs.pkgname)
		return lhs.pkgname<rhs.pkgname;
	else if (lhs.pkgvrsn!=rhs.pkgvrsn)
		return lhs.pkgvrsn<rhs.pkgvrsn;
	else
		return lhs.pkgenv<rhs.pkgenv;
}

binary_control_table::commit_error::commit_error():
	runtime_error("failed to commit status table")
{}

}; /* namespace pkg */
