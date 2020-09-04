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

#include "libpkg/version.h"
#include "libpkg/standards_version.h"

namespace pkg {

namespace {

/** The earliest standards version that cannot be processed by this library. */
const version bad_standards_version("0.7");

}; /* anonymous namespace */

bool can_process(const version& standards_version)
{
	return standards_version<bad_standards_version;
}

}; /* namespace pkg */
