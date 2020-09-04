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
