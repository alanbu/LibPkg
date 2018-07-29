// This file is part of LibPkg.
// Copyright � 2003-2005 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.
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
