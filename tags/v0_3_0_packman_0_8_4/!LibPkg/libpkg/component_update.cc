// This file is part of LibPkg.
// Copyright © 2013 Alan Buckley
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include <fstream>

#include "libpkg/filesystem.h"
#include "libpkg/component_update.h"

namespace pkg {

component_update::component_update(const string& pathname):
	_pathname(pathname)
{
	rollback();
}

component_update::~component_update()
{}

const component& component_update::operator[](const std::string &name) const
{
	static component default_value;
	for (const_iterator f = _data.begin(); f != _data.end(); ++f)
	{
		if (f->name() == name) return *f;
	}
	return default_value;
}

component_update::const_iterator component_update::find(const std::string& name) const
{
	const_iterator f;
	for ( f = _data.begin(); f != _data.end(); ++f)
	{
		if (f->name() == name) return f;
	}
	return _data.end();
}

void component_update::insert(const component &value)
{
	for (std::vector<component>::iterator f = _data.begin(); f != _data.end(); ++f)
	{
		if (f->name() == value.name())
		{
			*f = value;
			return;
		}
	}
	_data.push_back(value);
}

void component_update::insert(const component_update& table)
{
	for (const_iterator i=table.begin();i!=table.end();++i)
	{
		insert(*i);
	}
}

void component_update::clear()
{
	_data.clear();
}

void component_update::commit()
{
	// Take no action unless a pathname has been specified.
	if (_pathname.size())
	{
		// Set pathnames.
		string dst_pathname=_pathname;
		string tmp_pathname=_pathname+string("++");
		string bak_pathname=_pathname+string("--");
	
		// Write new status file.
		std::ofstream out(tmp_pathname.c_str());
		for (const_iterator i=_data.begin();i!=_data.end();++i)
		{
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

void component_update::rollback()
{
	// Take no action unless a pathname has been specified.
	if (_pathname.size())
	{
		_data.clear();
		bool done=read(_pathname);
		if (!done) read(_pathname+string("--"));
	}
}

void component_update::done()
{
	// Take no action unless a pathname has been specified.
	if (_pathname.size())
	{
		try
		{
			// Only delete if it has been written
			if (object_type(_pathname)!=0)
			{
				// Delete backup.
				force_delete(_pathname);
			}
		}
		catch (...)
		{
			throw commit_error();
		}
	}
}

bool component_update::read(const string& pathname)
{
	std::ifstream in(pathname.c_str());
	bool done=in;
	in.peek();
	while (in&&!in.eof())
	{
		component c;
		in >> c;
		insert(c);
		in.peek();
	}
	return done;
}

component_update::commit_error::commit_error():
	runtime_error("failed to commit component update")
{}

}; /* namespace pkg */
