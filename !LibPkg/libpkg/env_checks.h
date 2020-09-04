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

// Created on: 3 Jul 2018
// Author: Alan Buckley
//
// See env_checks.cpp for current checks provided

#ifndef LIBPKG_LIBPKG_ENV_CHECKS_H_
#define LIBPKG_LIBPKG_ENV_CHECKS_H_

#include <string>
#include "env_checker.h"

namespace pkg {


/**
 * Class for packages that need to OS to contain a specific module
 *
 * Note: Raspberry PI video can be detected by looking for the VCHIQ module.
 */
class module_check : public env_check
{
public:
	module_check(const std::string &title);
};


} /* namespace lippkg */

#endif /* LIBPKG_LIBPKG_ENV_CHECKS_H_ */
