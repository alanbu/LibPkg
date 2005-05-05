// This file is part of LibPkg.
// Copyright © 2003-2004 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef _LIBPKG_PKGBASE
#define _LIBPKG_PKGBASE

#include <string>

#include "libpkg/dependency.h"
#include "libpkg/status_table.h"
#include "libpkg/binary_control_table.h"
#include "libpkg/source_table.h"
#include "libpkg/path_table.h"

namespace pkg {

/** A class for representing the collection of package database tables. */
class pkgbase
{
public:
	class cache_error;
private:
	/** The pathname of the !Packages directory. */
	string _pathname;

	/** The pathname of the default choices directory. */
	string _dpathname;

	/** The pathname of the choices directory. */
	string _cpathname;

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

	/** The changed flag.
	 * True if any of the must-remove, must-install or must-upgrade
	 * flags have changed since the start of the current round of
	 * dependency resolution.  Note that this flag is only meaningful
	 * during dependency resolution, and it only captures changes
	 * made using the functions ensure_removed() and ensure_installed().
	 */
	bool _changed;
public:
	/** Create pkgbase object.
	 * @param pathname the pathname of the !Packages directory.
	 * @param dpathname the pathname of the default choices directory
	 * @param cpathname the pathname of the choices directory
	 */
	pkgbase(const string& pathname,const string& dpathname,
		const string& cpathname);

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

	/** Get pathname for sprites directory.
	 * @return the pathname
	 */
	string sprites_pathname();

	/** Get pathname for setvars file.
	 * @return the pathname
	 */
	string setvars_pathname(); 

	/** Get pathname for sprites file.
	 * @return the pathname
	 */
	string bootsprites_pathname();

	/** Verify file in cache.
	 * This function checks first whether a suitably named file exists,
	 * then whether it has the correct length, then whether it has the
	 * correct MD5Sum.  If any of these tests fail then a cache error
	 * is thrown.
	 * @param ctrl a control record for the requested package
	 */
	void verify_cached_file(const binary_control& ctrl);

	/** Fix dependencies.
	 * If a package is in the seed set then its selection state cannot
	 * change from installed to removed or vice-versa.  If it is not in
	 * the seed set then it can be installed or removed as necessary to
	 * meet all dependencies.
	 * @param seed the seed set
	 * @return true if all dependencies were fixed, otherwise false
	 */
	bool fix_dependencies(const set<string>& seed);

	/** Remove redundant auto-installed packages.
	 */
	void remove_auto();
private:
	/** Fix dependencies for package.
	 * Flags are altered if and only if all dependencies can be satisifed.
	 * @param ctrl the package control record
	 * @param allow_new true to allow packages that are not currently
	 *  installed, otherwise false
	 * @param apply true to apply changes to must-remove, must-install
	 *  and must-upgrade flags, otherwise false
	 * @return true if all dependencies were satisfied, otherwise false
	 */
	bool fix_dependencies(const pkg::control& ctrl,bool allow_new);

	/** Fix dependencies for package.
	 * @param ctrl the package control record
	 * @param allow_new true to allow packages that are not currently
	 *  installed, otherwise false
	 * @param apply true to apply changes to must-remove, must-install
	 *  and must-upgrade flags, otherwise false
	 * @return true if all dependencies were satisfied, otherwise false
	 */
	bool fix_dependencies(const pkg::control& ctrl,bool allow_new,
		bool apply);

	/** Resolve dependency alternatives.
	 * Preference is given to packages that are already installed
	 * (and are not flagged for removal), followed by packages that
	 * are flagged for installation.
	 * If two or more equally good solutions are available, the one
	 * chosen is the one that occurs earliest in the list.
	 * @param deps the list of dependencies
	 * @param allow_new true to allow packages that are not currently
	 *  installed, otherwise false
	 * @return the control record of a package that would satisfy one
	 *  of the listed dependencies, or 0 if none found
	 */
	const pkg::control* pkgbase::resolve(const vector<dependency>& deps,
		bool allow_new=true);

	/** Resolve dependency.
	 * Preference is given to packages that are already installed
	 * (and are not flagged for removal), followed by packages that
	 * are flagged for installation.
	 * @param dep the depenency to be satisfied
	 * @param allow_new true to allow packages that are not currently
	 *  installed, otherwise false
	 * @return the control record of a package that would satisfy the
	 *  dependency, or 0 if none found
	 */
	const pkg::control* pkgbase::resolve(const dependency& dep,
		bool allow_new=true);

	/** Ensure that package will be removed.
	 * The must-remove flag is set if it is not already.
	 * @param pkgname the package name
	 * @param pkgvrsn the package version
	 */
	void ensure_removed(const string& pkgname);

	/** Ensure that package will be installed.
	 * The must-install flag, and if necessary the must-upgrade flag,
	 * are set if they are not already.
	 * @param pkgname the package name
	 * @param pkgvrsn the package version
	 */
	void ensure_installed(const string& pkgname,const string& pkgvrsn);
};

/** An exception class for reporting cache errors. */
class pkgbase::cache_error:
	public runtime_error
{
private:
	/** A message which describes the cache error. */
	string _message;
public:
	/** Construct cache error.
	 * @param message a message which describes the cache error
	 * @param ctrl the control record for the relevant package
	 */
	cache_error(const char* message,const binary_control& ctrl);

	/** Destroy cache error. */
	virtual ~cache_error();

	/** Get message.
	 * @return a message which describes the cache error.
	 */
	virtual const char* what() const;
};

}; /* namespace pkg */

#endif
