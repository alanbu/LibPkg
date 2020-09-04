// This file is part of the LibPkg.
//
// Copyright 2003-2020 Graham Shaw
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
