// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef _LIBPKG_SYSVARS
#define _LIBPKG_SYSVARS

namespace pkg {

class pkgbase;

/** Update system variable definitions.
 * System variable definitions are found and merged into a single obey
 * file which is executed at boot time.
 * @param pb the package database
 */
void update_sysvars(pkgbase& pb);

}; /* namespace pkg */

#endif
