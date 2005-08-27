// This file is part of LibPkg.
// Copyright © 2003-2005 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef _LIBPKG_AUTO_DIR
#define _LIBPKG_AUTO_DIR

#include <string>

namespace pkg {

using std::string;

/** A class for automatically creating and deleting directories.
 * When an object of this class moves to a given pathname, it creates
 * any directories needed to reach that pathname.  When it leaves a
 * pathname, it deletes any directories that are no longer in use.
 */
class auto_dir
{
private:
	/** The current pathname. */
	string _pathname;
public:
	/** Construct auto-directory object.
	 * The initial pathname is the empty string.
	 */
	auto_dir();

	/** Destroy auto-directory object.
	 * Any empty directories that contain the pathname are deleted.
	 */
	~auto_dir();

	/** Move to new pathname.
	 * Any empty directories that contain the old pathname are deleted.
	 * Any directories needed to reach the new pathname are created.
	 * @param pathname the new pathname
	 */
	void operator()(const string& pathname);
};

}; /* namespace pkg */

#endif
