// This file is part of LibPkg.
// Copyright © 2003-2005 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include "libpkg/table.h"

namespace pkg {

table::table()
{}

table::~table()
{
	while (_watchers.size())
	{
		(*_watchers.begin())->unwatch(*this);
	}
}

void table::register_watcher(watcher& w)
{
	_watchers.insert(&w);
}

void table::deregister_watcher(watcher& w)
{
	_watchers.erase(&w);
}

void table::notify()
{
	for (std::set<watcher*>::const_iterator i=_watchers.begin();
		i!=_watchers.end();++i)
	{
		(*i)->handle_change(*this);
	}
}

table::watcher::watcher()
{}

table::watcher::~watcher()
{
	for (std::set<table*>::const_iterator i=_tables.begin();
		i!=_tables.end();++i)
	{
		(*i)->deregister_watcher(*this);
	}
}

void table::watcher::watch(table& t)
{
	_tables.insert(&t);
	t.register_watcher(*this);
}

void table::watcher::unwatch(table& t)
{
	_tables.erase(&t);
	t.deregister_watcher(*this);
}

}; /* namespace pkg */
