// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef _LIBPKG_STANDARDS_VERSION
#define _LIBPKG_STANDARDS_VERSION

namespace pkg {

class version;

/** Test whether standards version can be processed by this library.
 * @return true if standards version can be processed, otherwise false
 */
bool can_process(const version& standards_version);

}; /* namespace pkg */

#endif
