// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef _LIBPKG_DEPENDENCY
#define _LIBPKG_DEPENDENCY

#include <vector>
#include <string>

#include "libpkg/version.h"

namespace pkg {

/** A class to represent a package dependency.
 * Syntax and semantics are as specified in version 3.5.6 of the
 * Debian Policy Manual, except that:
 * - package names may contain upper case letters;
 * - the deprecated relational operators ("<" and ">") are not supported.
 */
class dependency
{
public:
	/** An enumerated type to specify a relational operator for
	 * comparing versions. */
	enum relation_type
	{
		/** A relational operator that is always true. */
		relation_al,
		/** A relational operator that is true when two versions are
		 * equal. */
		relation_eq,
		/** A relational operator that is true when the first version
		 * is earlier than the second. */
		relation_lt,
		/** A relational operator that is true when the first version
		 * is later or equal to the second. */
		relation_ge,
		/** A relational operator that is true when the first version
		 * is earlier or equal to the second. */
		relation_le,
		/** A relational operator that is true when the first version
		 * is later than the second. */
		relation_gt
	};

	class parse_error;
private:
	/** The package name. */
	string _pkgname;

	/** The relational operator to be satisfied. */
	relation_type _relation;

	/** The version to which the relational operator refers. */
	pkg::version _version;
public:
	/** Construct dependency with default value.
	 * By default the dependency has an empty package name and is
	 * satisfied by any version.
	 */
	dependency();

	/** Construct dependency from components.
	 * @param pkgname the package name
	 * @param relation the relational operator to be satisfied
	 * @param version the version to which the relational operator refers
	 */
	dependency(const string& pkgname,relation_type relation,
		const pkg::version& version);

	/** Construct dependency from sequence.
	 * @param first the beginning of the sequence
	 * @param last the end of the sequence
	 */
	dependency(string::const_iterator first,string::const_iterator last);

	/** Construct dependency from string.
	 * @param depstr the dependency string
	 */
	dependency(const string& depstr);

	/** Destroy dependency. */
	~dependency();

	/** Convert dependency to string.
	 * @return the dependency as a string
	 */ 
	operator string() const;

	/** Get package name.
	 * @return the package name
	 */
	const string& pkgname() const
		{ return _pkgname; }

	/** Get relational operator.
	 * @return the relational operator to be satisified
	 */
	relation_type relation() const
		{ return _relation; }

	/** Get version.
	 * @return the version to which the relational operator refers
	 */
	const pkg::version& version() const
		{ return _version; }

	/** Test whether dependency is satisfied by a given package/version.
	 * @param pkgname the name of the package to be tested
	 * @param pkgvrsn the version of the package to be tested
	 * @return true if the dependency is satisfied, otherwise false
	 */
	bool matches(const string& pkgname,const pkg::version& pkgvrsn) const;
private:
	/** Parse dependency.
	 * @param first the beginning of the sequence
	 * @param last tne end of the sequence
	 */
	void parse(string::const_iterator first,string::const_iterator last);
};

/** An exception class for reporting parse errors. */
class dependency::parse_error:
	public runtime_error
{
private:
	/** A message which describes the parse error. */
	const char* _message;
public:
	/** Construct parse error.
	 * @param message a message which describes the parse error.
	 */
	parse_error(const char* message);

	/** Destroy parse error. */
	virtual ~parse_error();

	/** Get message.
	 * @return a message which describes the parse error.
	 */
	virtual const char* what() const;
};

/** Parse dependency alternatives list.
 * @param first the beginning of the sequence
 * @param last the end of the sequence
 * @param out a vector to which the list is written
 */
void parse_dependency_alt_list(string::const_iterator first,
	string::const_iterator last,vector<dependency>* out);

/** Parse dependency list.
 * @param first the beginning of the sequence
 * @param last the end of the sequence
 * @param out a vector to which the list is written
 */
void parse_dependency_list(string::const_iterator first,
	string::const_iterator last,vector<vector<dependency> >* out);

}; /* namespace pkg */

#endif
