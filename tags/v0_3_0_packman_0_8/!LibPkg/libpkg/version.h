// This file is part of LibPkg.
// Copyright © 2003-2005 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef LIBPKG_VERSION
#define LIBPKG_VERSION

#include <exception>
#include <stdexcept>
#include <string>

namespace pkg {

using std::string;

/** A class to represent a package version.
 * Syntax and semantics are as specified in version 3.8.0 of the
 * Debian Policy Manual.
 */
class version
{
public:
	class parse_error;
private:
	/** The epoch. */
	string _epoch;

	/** The upstream version. */
	string _upstream_version;

	/** The package version. */
	string _package_version;
public:
	/** Construct version with default value.
	 * By default the epoch, upstream version and package version are
	 * all empty.
	 */
	version();

	/** Construct version from components.
	 * If any of these components contain invalid characters then a
	 * parse error is thrown.
	 * @param epoch the epoch
	 * @param upstream_version the upstream version
	 * @param package_version the package version
	 */
	version(const string& epoch,const string& upstream_version,
		const string& package_version);

	/** Construct version from sequence.
	 * If the version string contains any illegal characters then a
	 * parse error is thrown.
	 * @param first the beginning of the sequence
	 * @param last the end of the sequence
	 */
	version(string::const_iterator first,string::const_iterator last);

	/** Construct version from string.
	 * If the version string contains any illegal characters then a
	 * parse error is thrown.
	 * @param verstr the version string
	 */
	version(const string& verstr);

	/** Convert version to string.
	 * A colon (to terminate the epoch) is included if the epoch is
	 * non-empty, or if the upstream version contains one or more colons.
	 * A minus sign (to introduce the package version) is included if
	 * the package version is non-empty, or if the upstream version
	 * contains one or more minus signs.
	 * @return the version as a string
	 */
	operator string() const;

	/** Get epoch.
	 * @return the epoch
	 */
	string epoch() const
		{ return _epoch; }

	/** Get upstream version.
	 * @return the upstream version
	 */
	string upstream_version() const
		{ return _upstream_version; }

	/** Get package version.
	 * @return the package version
	 */
	string package_version() const
		{ return _package_version; }
private:
	/** Parse version.
	 * @param first the beginning of the sequence
	 * @param last the end of the sequence
	 */
	void parse(string::const_iterator first,string::const_iterator last);

	/** Validate epoch, upstream version and package version.
	 * If any of these components contain invalid characters then a
	 * parse_error is thrown.
	 */
	void validate() const;
};

/** Test whether two versions are equal.
 * @param lhs the left hand side
 * @param rhs the right hand side
 * @return true if lhs==rhs, otherwise false.
 */
bool operator==(const version& lhs,const version& rhs);

/** Test whether two versions are unequal.
 * @param lhs the left hand side
 * @param rhs the right hand side
 * @return true if lhs!=rhs, otherwise false.
 */
bool operator!=(const version& lhs,const version& rhs);

/** Test whether one version is less than another.
 * @param lhs the left hand side
 * @param rhs the right hand side
 * @return true if lhs<rhs, otherwise false.
 */
bool operator<(const version& lhs,const version& rhs);

/** Test whether one version is greater than or equal to another.
 * @param lhs the left hand side
 * @param rhs the right hand side
 * @return true if lhs>=rhs, otherwise false.
 */
bool operator>=(const version& lhs,const version& rhs);

/** Test whether one version is less than or equal to another.
 * @param lhs the left hand side
 * @param rhs the right hand side
 * @return true if lhs<=rhs, otherwise false.
 */
bool operator<=(const version& lhs,const version& rhs);

/** Test whether one version is greater than another.
 * @param lhs the left hand side
 * @param rhs the right hand side
 * @return true if lhs>rhs, otherwise false.
 */
bool operator>(const version& lhs,const version& rhs);

/** An exception class for reporting parse errors. */
class version::parse_error:
	public std::runtime_error
{
public:
	/** Construct parse error.
	 * @param message a message which describes the parse error.
	 */
	parse_error(const char* message);
};

}; /* namespace pkg */

#endif
