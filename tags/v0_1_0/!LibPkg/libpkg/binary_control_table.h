// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef _LIBPKG_BINARY_CONTROL_TABLE
#define _LIBPKG_BINARY_CONTROL_TABLE

#include <map>
#include <string>

#include "libpkg/version.h"
#include "libpkg/binary_control.h"
#include "libpkg/table.h"

namespace pkg {

/** A class for mapping package name and version to binary control record. */
class binary_control_table:
	public table
{
public:
	/** A class for specifying the name and version of a package. */
	class key_type
	{
	public:
		/** The package name. */
		string pkgname;
		/** The package version. */
		version pkgvrsn;
		/** Construct default key type. */
		key_type();
		/** Contruct key type from package name and version.
		 * @param _pkgname the package name
		 * @param _pkgvrsn the package version
		 */
		key_type(const string& _pkgname,const version& _pkgvrsn);
	};
	typedef binary_control mapped_type;
	typedef map<key_type,mapped_type>::const_iterator const_iterator;
private:
	/** The pathname of the underlying package index file. */
	string _pathname;

	/** A map from package name and version to control record. */
	map<key_type,mapped_type> _data;
public:
	/** Construct binary control table.
	 * @param pathname the pathname of the underlying package index file.
	 */
	binary_control_table(const string& pathname);

	/** Destroy binary control table */
	virtual ~binary_control_table();

	/** Get control record for package given package name and version.
	 * @param key the package name and version
	 * @return the control record
	 */
	const mapped_type& operator[](const key_type& key) const;

	/** Get control record for latest version of package.
	 * @param pkgname the package name
	 * @return the control record
	 */
	const mapped_type& operator[](const string& pkgname) const;

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

	/** Re-read the underlying package index file. */
	void update();
};

/** Compare two binary control table keys.
 * @param lhs the left hand side
 * @param rhs the right hand side
 * @return true if lhs<rhs, otherwise false
 */
bool operator<(const binary_control_table::key_type& lhs,
	const binary_control_table::key_type& rhs);

}; /* namespace pkg */

#endif
