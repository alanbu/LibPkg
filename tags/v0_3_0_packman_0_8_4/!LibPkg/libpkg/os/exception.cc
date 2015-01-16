// This file is part of the RISC OS Toolkit (RTK).
// Copyright © 2003-2005 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !RTK.Copyright.
//
// Modified from RTK exception to add extra information
// by Theo Markettos

#include <kernel.h>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "exception.h"

namespace pkg {
namespace os {

exception::exception(_kernel_oserror* err)
{
	std::memcpy(&_err, err, sizeof(_kernel_oserror));
}

exception::exception(_kernel_oserror* err, unsigned int number, unsigned int r0)
{
//	std::ostringstream ss;
//	ss << ((char *) (err->errmess)) << " (SWI " << number << ", r0=" << r0 << ")";
//	char *str=(char *)malloc(300*sizeof(char));
	_err.errnum = err->errnum;
	snprintf(_err.errmess,256,"%s (SWI %x) r0=%x",err->errmess,number,r0);
//	_err = ss;
}

const char* exception::what() const throw()
{
	return _err.errmess;
}

} /* namespace os */
} /* namespace pkg */
