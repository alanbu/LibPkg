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

#ifndef LIBPKG_AUTO_DIR
#define LIBPKG_AUTO_DIR

#include <string>

namespace pkg {

using std::string;

/** A class for automatically creating and deleting directories.
 * When an object of this class moves to a given pathname, it creates
 * any directories needed to reach that pathname.  When it leaves a
 * pathname, it deletes any directories that are no longer in use.
 */
class auto_dir
{
private:
	/** The current pathname. */
	string _pathname;
public:
	/** Construct auto-directory object.
	 * The initial pathname is the empty string.
	 */
	auto_dir();

	/** Destroy auto-directory object.
	 * Any empty directories that contain the pathname are deleted.
	 */
	~auto_dir();

	/** Move to new pathname.
	 * Any empty directories that contain the old pathname are deleted.
	 * Any directories needed to reach the new pathname are created.
	 * @param pathname the new pathname
	 */
	void operator()(const string& pathname);
};

}; /* namespace pkg */

#endif
