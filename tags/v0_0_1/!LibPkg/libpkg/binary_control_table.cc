// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include <fstream>

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
	map<key_type,mapped_type>::const_iterator f=_data.find(key);
	return (f!=_data.end())?f->second:default_value;
}

const binary_control_table::mapped_type&
	binary_control_table::operator[](const string& pkgname) const
{
	static mapped_type default_value;
	key_type key(pkgname,version());
	map<key_type,mapped_type>::const_iterator f=_data.lower_bound(key);
	if (f==_data.end()) return default_value;
	if (f->first.pkgname!=pkgname) return default_value;
	const mapped_type* result=&f->second;
	while ((++f!=_data.end())&&(f->first.pkgname==pkgname))
		result=&f->second;
	return *result;
}

void binary_control_table::update()
{
	_data.clear();
	ifstream in(_pathname.c_str());
	in.peek();
	while (in&&!in.eof())
	{
		binary_control ctrl;
		in >> ctrl;
		string pkgname=ctrl.pkgname();
		version pkgvrsn=ctrl.version();
		key_type key(pkgname,pkgvrsn);
		map<key_type,mapped_type>::const_iterator f=_data.find(key);
		if (f==_data.end()) _data[key]=ctrl;
		while (in.peek()=='\n') in.get();
	}
	notify();
}

binary_control_table::key_type::key_type()
{}

binary_control_table::key_type::key_type(const string& _pkgname,
	const version& _pkgvrsn):
	pkgname(_pkgname),
	pkgvrsn(_pkgvrsn)
{}

bool operator<(const binary_control_table::key_type& lhs,
	const binary_control_table::key_type& rhs)
{
	if (lhs.pkgname!=rhs.pkgname)
		return lhs.pkgname<rhs.pkgname;
	else
		return lhs.pkgvrsn<rhs.pkgvrsn;
}

}; /* namespace pkg */
