// This file is part of LibPkg.
// Copyright � 2003 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef LIBPKG_BINARY_CONTROL
#define LIBPKG_BINARY_CONTROL

#include "libpkg/control.h"

namespace pkg {

class pkg_env;

/** A class to represent the content of a RiscPkg binary control file.
 * Behaviour is that of a map<string,string>, except that:
 * - key comparison is case-insensitive;
 * - standard key values for a binary control file are recognised,
 *   and given a priority which overrides the normal sort order.
 *
 * The sort order is subject to change without notice.  This should not
 * break anything, because nothing should depend on the sort order.
 */
class binary_control:
	public control
{
public:
	/** Construct binary control file. */
	binary_control();

	/** Destroy binary control file. */
	virtual ~binary_control();

	std::string environment_id() const;
	const pkg_env *package_env() const;

	/** Get package install priority or use environment default install priority if 0 or unset
	 * @return the install priority used to choose the most appropriate package for an environment.
	 */
	int install_priority() const;

protected:
	virtual int priority(const string& value) const;
	mutable pkg_env *_environment;
	mutable int _install_priority;
};

}; /* namespace pkg */

#endif
