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
