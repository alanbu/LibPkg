// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef _LIBPKG_CONTROL_BINARY
#define _LIBPKG_CONTROL_BINARY

#include "libpkg/control.h"

namespace pkg {

/** A class to represent the content of a RiscPkg binary control file.
 * Behaviour is that of a map<string,string>, except that:
 * - key comparison is case-insensitive;
 * - standard key values for a binary control file are recognised,
 *   and given a priority which overrides the normal sort order.
 *
 * The sort order is subject to change without notice.  This should not
 * break anything, because nothing should depend on the sort order.
 */
class control_binary:
	public control
{
public:
	/** Construct binary control file. */
	control_binary();

	/** Destroy binary control file. */
	virtual ~control_binary();
protected:
	virtual int priority(const string& value) const;
};

}; /* namespace pkg */

#endif
