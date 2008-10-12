// This file is part of LibPkg.
// Copyright © 2003-2004 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include "libpkg/version.h"
#include "libpkg/standards_version.h"

namespace pkg {

namespace {

/** The earliest standards version that cannot be processed by this library. */
const version bad_standards_version("0.3");

}; /* anonymous namespace */

bool can_process(const version& standards_version)
{
	return standards_version<bad_standards_version;
}

}; /* namespace pkg */
