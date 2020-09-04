// This file is part of LibPkg.
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

#ifndef LIBPKG_STANDARDS_VERSION
#define LIBPKG_STANDARDS_VERSION

namespace pkg {

class version;

/** Test whether standards version can be processed by this library.
 * @return true if standards version can be processed, otherwise false
 */
bool can_process(const version& standards_version);

}; /* namespace pkg */

#endif
