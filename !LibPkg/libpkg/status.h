// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef _LIBPKG_STATUS
#define _LIBPKG_STATUS

#include <string>
#include <stdexcept>
#include <iosfwd>

namespace pkg {

/** A class to represent the current or required status of a package.
 * The status consists of three components:
 * - the installation state (which indicates whether the package is
 *   installed, not-present, or at one of several points in between);
 * - a set of flags, for indicating conditions that are partly or wholly
 *   orthogonal to the installation state; and
 * - the current or required package version (if there is one).
 *
 * States are ordered, and it is meaningful to use the inequality
 * operators to compare them.  If state A is greater than state B then
 * A is closer to the fully installed state.
 *
 * Note that not-present is rarely used because this state is usually
 * indicated by the absence of an entry in the status file.
 */
class status
{
public:
	enum state_type
	{
		/** A state to indicate that a package has either been purged
		 * or was never installed. */
		state_not_present,
		/** A state to indicate that a package has been removed but
		 * its configuration files may remain. */
		state_removed,
		/** A state to indicate that a package is partially unpacked
		 * or partially removed.  This is a transient state unless
		 * a failure occurs. */
		state_half_unpacked,
		/** A state to indicate that a package has been unpacked but
		 * not configured. */
		state_unpacked,
		/** A state to indicate that a package is partially configured.
		 * This is a transient state unless a failure occurs. */
		state_half_configured,
		/** A state to indicate that a package has been successfully
		 * unpacked and configured. */
		state_installed
	};
	enum flag_type
	{
		/** A flag to indicate that a package has been installed
		 * automatically to meet a dependency (and should therefore be
		 * removed automatically if that dependency ceases to exist).
		 */
		flag_auto,
		/** A flag to indicate that a package has been placed on
		 * hold (which prevents any change of state unless explicitly
		 * requested by the user). */
		flag_hold
	};
	class parse_error;
private:
	/** The installation state. */
	state_type _state;
	/** The status flags. */
	unsigned int _flags;
	/** The package version.
	 * This field is not meaningful when the installation state
	 * is not-present. */
	string _version;
public:
	/** Construct status.
	 * By default, the package is not-present and all flags are clear.
	 */
	status();

	/** Construct status given installation state and version.
	 * By default, all flags are clear.
	 * @param state the required installation state
	 * @param version the required version
	 */
	status(state_type state,const string& version);

	/** Destroy status. */
	~status();

	/** Get installation state.
	 * @return the installation state
	 */
	state_type state() const
		{ return _state; }

	/** Get status flag.
	 * @param flag the flag to be read
	 * @return the value of the flag
	 */
	bool flag(flag_type flag) const
		{ return (_flags>>flag)&1; }

	/** Get package version.
	 * @return the package version
	 */
	string version() const
		{ return _version; }

	/** Set installation state.
	 * @param state the required installation state
	 */
	void state(state_type state);

	/** Set status flag.
	 * @param flag the flag to be altered
	 * @param value the required value
	 */
	void flag(flag_type flag,bool value);

	/** Set package version.
	 * @param version the required package version
	 */
	void version(const string& version);
};

/** An exception class for reporting parse errors. */
class status::parse_error:
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

/** Test whether two versions are equal.
 * @param lhs the left hand side
 * @param rhs the right hand side
 * @return true if lhs==rhs, otherwise false.
 */
bool operator==(const status& lhs,const status& rhs);

/** Test whether two versions are unequal.
 * @param lhs the left hand side
 * @param rhs the right hand side
 * @return true if lhs!=rhs, otherwise false.
 */
bool operator!=(const status& lhs,const status& rhs);

/** Test whether one version is less than another.
 * @param lhs the left hand side
 * @param rhs the right hand side
 * @return true if lhs<rhs, otherwise false.
 */
bool operator<(const status& lhs,const status& rhs);

/** Test whether one version is greater than or equal to another.
 * @param lhs the left hand side
 * @param rhs the right hand side
 * @return true if lhs>=rhs, otherwise false.
 */
bool operator>=(const status& lhs,const status& rhs);

/** Test whether one version is less than or equal to another.
 * @param lhs the left hand side
 * @param rhs the right hand side
 * @return true if lhs<=rhs, otherwise false.
 */
bool operator<=(const status& lhs,const status& rhs);

/** Test whether one version is greater than another.
 * @param lhs the left hand side
 * @param rhs the right hand side
 * @return true if lhs>rhs, otherwise false.
 */
bool operator>(const status& lhs,const status& rhs);

/** Write package status record to output stream.
 * @param out the output stream
 * @param pkgstat the package status record
 * @return the output stream
 */
ostream& operator<<(ostream& out,const pair<string,status>& pkgstat);

/** Read package status record from input stream.
 * @param in the input stream
 * @param pkgstat the package status record
 * @return the output stream
 */
istream& operator>>(istream& in,pair<string,status>& pkgstat);

}; /* namespace pkg */

#endif
