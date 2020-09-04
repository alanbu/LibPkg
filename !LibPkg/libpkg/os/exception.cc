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
