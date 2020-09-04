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

#include <list>
#include <algorithm>

#include "libpkg/thread.h"

namespace pkg {

namespace {

std::list<thread*>& active()
{
	static std::list<thread*> _active;
	return _active;
}

}; /* anonymous namespace */

thread::thread()
{
	active().push_back(this);
}

thread::~thread()
{
	std::list<thread*>::iterator f=find(active().begin(),active().end(),this);
	if (f!=active().end()) active().erase(f);
}

void thread::poll()
{}

void thread::poll_all()
{
	for (std::list<thread*>::const_iterator i=active().begin();
		i!=active().end();++i)
	{
		(*i)->poll();
	}
}

}; /* namespace pkg */
