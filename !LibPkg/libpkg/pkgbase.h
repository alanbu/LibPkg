// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef _LIBPKG_PKGBASE
#define _LIBPKG_PKGBASE

#include <string>

#include "libpkg/status_table.h"
#include "libpkg/binary_control_table.h"
#include "libpkg/source_table.h"
#include "libpkg/path_table.h"

namespace pkg {

/** A class for representing the collection of package database tables. */
class pkgbase
{
private:
	/** The pathname of the !Packages directory. */
	string _pathname;

	/** The current status table. */
	status_table _curstat;

	/** The selected status table. */
	status_table _selstat;

	/** The previous status table. */
	status_table _prevstat;

	/** The binary control table. */
	binary_control_table _control;

	/** The source table. */
	source_table _sources;

	/** The path table. */
	path_table _paths;
public:
	/** Create pkgbase object.
	 * @param pathname the pathname of the !Packages directory.
	 */
	pkgbase(const string& pathname);

	/** Destroy pkgbase object. */
	~pkgbase();

	/** Get current status table.
	 * @return the current status table
	 */
	status_table& curstat()
		{ return _curstat; }

	/** Get selected status table.
	 * @return the selected status table
	 */
	status_table& selstat()
		{ return _selstat; }

	/** Get previous status table.
	 * This is the state to which packages are restored when
	 * changes are unwound after an error.
	 * @return the previous status table
	 */
	status_table& prevstat()
		{ return _prevstat; }

	/** Get binary control table.
	 * @return the binary control table
	 */
	binary_control_table& control()
		{ return _control; }

	/** Get source table.
	 * @return the source table
	 */
	source_table& sources()
		{ return _sources; }

	/** Get path table.
	 * @return the path table
	 */
	path_table& paths()
		{ return _paths; }

	/** Get pathname for index file from given source.
	 * @param url the URL of the source
	 * @return the pathname
	 */
	string list_pathname(const string& url);

	/** Get pathname for available list file.
	 * @return the pathname
	 */
	string available_pathname();

	/** Get pathname for package in cache.
	 * @param pkgname the package name
	 * @param pkgvrsn the package version
	 * @return the pathname
	 */
	string cache_pathname(const string& pkgname,
		const string& pkgvrsn);

	/** Get pathname for info directory of package.
	 * @param pkgname the package name
	 * @return the pathname
	 */
	string info_pathname(const string& pkgname);

	/** Get pathname for sysvars directory.
	 * @return the pathname
	 */
	string sysvars_pathname();

	/** Get pathname for setvars file.
	 * @return the pathname
	 */
	string setvars_pathname(); 
};

}; /* namespace pkg */

#endif
