// This file is part of the RISC OS Toolkit (RTK).
// Copyright © 2003-2005 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !RTK.Copyright.
//
// Cut down version to use with LibPkg created by Alan Buckley
// to remove the RTK dependency.
// Modified from RTK exception to add extra information
// by Theo Markettos

#ifndef _LIBPKG_OS_EXCEPTION
#define _LIBPKG_OS_EXCEPTION

#include <exception>
#include "kernel.h"

namespace libpkg {
namespace os {

/** An exception class to represent a RISC OS error block.
 */
class exception:
	public std::exception
{
private:
	/** A pointer to the RISC OS error block. */
	_kernel_oserror _err;
public:
	/** Construct exception from RISC OS error block.
	 * The error block is not copied, and must remain in existence for
	 * the lifetime of the exception object.  It is not freed when the
	 * exception object is destroyed (in part because it is likely to
	 * be owned by the operating system rather than the application).
	 * @param err a pointer to the error block
	 */
	exception(_kernel_oserror* err);
	exception(_kernel_oserror* err, unsigned int number, unsigned int r0);

	/** Destroy exception.
	 */
	virtual ~exception() throw() {}

	/** Get message.
	 * The value returned is a pointer into the original error block used
	 * to construct the exception object.  It is supposed to remain valid
	 * for at least the lifetime of this object.
	 * @return the message from the error block
	 */
	virtual const char* what() const throw();
};

} /* namespace os */
} /* namespace rtk */

#endif
