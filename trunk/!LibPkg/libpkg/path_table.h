// This file is part of LibPkg.
// Copyright © 2003-2010 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef LIBPKG_PATH_TABLE
#define LIBPKG_PATH_TABLE

#include <map>
#include <string>
#include <stdexcept>

#include "libpkg/table.h"

namespace pkg {

using std::string;

/** A class for mapping source paths to destination paths.
 * The source path is with respect to the root of the zip file
 * (but using '.' as the path separator, not '/').
 * The destination path is with respect to the root of the filesystem.
 */
class path_table:
	public table
{
public:
	typedef string key_type;
	typedef string mapped_type;
	typedef std::map<key_type,mapped_type>::const_iterator const_iterator;
	class parse_error;
	class invalid_source_path;
	class commit_error;
private:
	/** The pathname of the underlying paths file,
	 * or the empty string if none. */
	string _pathname;

	/** A map from source pathname to destination pathname.
	 * Entries with an empty destination pathname have no effect
	 * on how pathnames are mapped.
	 */
	std::map<key_type,mapped_type> _data;
public:
	/** Construct path table.
	 * @param pathname the pathname of the underlying paths file
	 */
	path_table(const string& pathname);

	/** Destroy path table. */
	virtual ~path_table();

	/** Convert source pathname to destination pathname.
	 * @param src_pathname the source pathname
	 * @param pkgname the package name
	 * @return the destination pathname
	 */
	string operator()(const string& src_pathname,const string& pkgname) const;

	/** Get const iterator for start of table.
	 * @return the const iterator
	 */
	const_iterator begin() const
		{ return _data.begin(); }

	/** Get const iterator for end of table.
	 * @return the const iterator
	 */
	const_iterator end() const
		{ return _data.end(); }

	/** Find table entry.
	 * @param src_pathname the required source pathname
	 * @return a matching const iterator, or end() if not found
	 */
	const_iterator find(const string& src_pathname);

	/** Erase table entry.
	 * @param src_pathname the source pathname to be erased
	 */
	void erase(const string& src_pathname);

	/** Clear all paths. */
	void clear();

	/** Commit changes.
	 * Any changes since the last call to commit() or rollback() are
	 * committed to disc.
	 */
	void commit();

	/** Roll back changes.
	 * Any changes since the last call to commit() or rollback() are
	 * discarded.
	 */
	void rollback();
private:
	/** Read paths file.
	 * @param pathname the pathname
	 * @return true if the file was found, otherwise false
	 */
	bool read(const string& pathname);
};

/** An exception class for reporting parse errors. */
class path_table::parse_error:
	public std::runtime_error
{
public:
	/** Construct parse error.
	 * @param message a message which describes the parse error.
	 */
	parse_error(const string& message);
};

/** An exception class for reporting invalid source paths. */
class path_table::invalid_source_path:
	public std::runtime_error
{
public:
	/** Construct invalid source path error. */
	invalid_source_path();
};

/** An exception class for reporting failure to commit table. */
class path_table::commit_error:
	public std::runtime_error
{
public:
	/** Construct commit error. */
	commit_error();
};

/** Resolve logical path references.
 * If a string is passed through this function before it is GSTransed then
 * system variable references of the form <Packages$@x>, where x is a
 * logical pathname, are resolved to the corresponding physical pathname.
 *
 * Note that the string is not GSTransed, and system variable references
 * that are not of the above form are not expanded.
 * @param table the path table for lookups
 * @param in the string to be processed
 * @return the string with logical path references resolved
 */
string resolve_pathrefs(const path_table& table,const string& in);

}; /* namespace pkg */

#endif
