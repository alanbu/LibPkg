// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef _LIBPKG_PATH_TABLE
#define _LIBPKG_PATH_TABLE

#include <map>
#include <string>
#include <stdexcept>

#include "libpkg/table.h"

namespace pkg {

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
	typedef map<key_type,mapped_type>::const_iterator const_iterator;
	class parse_error;
	class invalid_source_path;
private:
	/** The pathname of the underlying paths file. */
	string _pathname;

	/** A map from source pathname to destination pathname. */
	map<key_type,mapped_type> _data;
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

	/** Re-read the underlying paths file. */
	void update();
};

/** An exception class for reporting parse errors. */
class path_table::parse_error:
	public runtime_error
{
private:
	/** A message which describes the parse error. */
	string _message;
public:
	/** Construct parse error.
	 * @param message a message which describes the parse error.
	 */
	parse_error(const string& message);

	/** Destroy parse error. */
	virtual ~parse_error();

	/** Get message.
	 * @return a message which describes the parse error.
	 */
	virtual const char* what() const;
};

/** An exception class for reporting invalid source paths. */
class path_table::invalid_source_path:
	public runtime_error
{
public:
	/** Construct invalid source path error. */
	invalid_source_path();

	/** Destroy invalid source path error. */
	virtual ~invalid_source_path();

	/** Get message.
	 * @return a message which describes the invalid source path error.
	 */
	virtual const char* what() const;
};

}; /* namespace pkg */

#endif
