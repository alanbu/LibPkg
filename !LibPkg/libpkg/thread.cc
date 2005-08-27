// This file is part of LibPkg.
// Copyright © 2003-2005 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

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
