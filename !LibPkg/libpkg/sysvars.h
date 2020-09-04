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

#ifndef LIBPKG_SYSVARS
#define LIBPKG_SYSVARS

namespace pkg {

class pkgbase;

/** Update system variable definitions.
 * System variable definitions are found and merged into a single obey
 * file which is executed at boot time.
 * @param pb the package database
 */
void update_sysvars(pkgbase& pb);

}; /* namespace pkg */

#endif
