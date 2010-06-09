// This file is part of LibPkg.
// Copyright © 2004 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef LIBPKG_SPRITE_POOL
#define LIBPKG_SPRITE_POOL

namespace pkg {

class pkgbase;

/** Update sprite pool.
 * Sprites are found and merged into a single sprite file which is loaded
 * at boot time.
 * @param pb the package database
 */
void update_sprite_pool(pkgbase& pb);

}; /* namespace pkg */

#endif
