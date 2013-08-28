// This file is part of the RISC OS Toolkit (RTK).
// Copyright © 2003-2005 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !RTK.Copyright.
// Moved to LibPkg by Alan Buckley to remove the RTK dependency.
// Modified from RTK version to add extra exception information
// by Theo Markettos

#ifndef _LIBPKG_OS_SWI
#define _LIBPKG_OS_SWI

#include "kernel.h"

#include "exception.h"

namespace pkg {
namespace os {

/** Call a RISC OS software interrupt.
 * @param number the software interrupt number
 * @param regs the register state (for input and output)
 */
inline void call_swi(unsigned int number,_kernel_swi_regs* regs)
{
	const int X=0x20000;
	unsigned int r0=regs->r[0];
 	_kernel_oserror* err=_kernel_swi(X+number,regs,regs);
	if (err) throw exception(err,number,r0);
}

} /* namespace os */
} /* namespace pkg */

#endif
