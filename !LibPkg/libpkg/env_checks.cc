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
#include <libpkg/env_checker.h>
#include <libpkg/env_checks.h>
#include <libpkg/os/call_swi.h>
#include <libpkg/os/osswi.h>
#include <libpkg/module_info.h>

#include "env_checks.h"


// To add a new check, create a class derived from env_check and add
// a new instance to the env_checker::initialise call at the end of this file.

namespace pkg {


/**
 * Check platform features
 */
unsigned int platform_features()
{
	_kernel_swi_regs regs;
	try
	{
		regs.r[0] = 0;
		os::call_swi(swi::OS_PlatformFeatures, &regs);
	} catch(...)
	{
		// OS_PlatformFeatures not available so assume all bits are zero as
		// suggested in the documentation
		regs.r[0] = 0;
	}
	return static_cast<unsigned int>(regs.r[0]);
}

/**
 * Construct a default environment check
 * @param name short name
 * @param desc short description
 * @param id single letter optionally followed by a number. This must be unique for all checks.
 * @param type The type of the check
 * @param install_priority the priority weighting for this check
 */
env_check::env_check(const std::string &name, const std::string &desc, const std::string &id, env_check_type type, int install_priority)
  : _name(name),
	_id(id),
	_type(type),
	_description(desc),
	_detected(false),
	_available(false),
	_install_priority(install_priority)
{
}

/**
 * Class for packages that should run anywhere
 */
class all_check : public env_check
{
public:
	all_check() : env_check("any","Any", "a", System, 2)
	{
		_detected = true;
		_available = true;
	}
};

/**
 * Class for packages that need to run 32 bit code
 */
class arm_check : public env_check
{
public:
	arm_check() : env_check("arm", "26/32 bit neutral code", "b", System, 8)
	{
		// All current platforms should be able to run 16/32 bit neutral code though
		// they may need the 32 bit CLib to do it.
		if (platform_features() & (1<<6)) _detected = true;
		else if ((platform_features() & (1<<7)) == 0) _detected = true;

		_available = _detected;
	}
};

/**
 * Class for packages that need to run on a 26 bit version of RISC OS
 */
class arm26_check : public env_check
{
public:
	arm26_check() : env_check("arm26", "26 bit code","b2", System, 4)
	{
		if ((platform_features() & (1<<7)) == 0) _detected = true;
		_available = _detected;
	}
};

/**
 * Class for packages that need to run on a 32 bit version of RISC OS
 */
class arm32_check : public env_check
{
public:
	arm32_check() : env_check("arm32", "32 bit code","b3", System, 5)
	{
		if (platform_features() & (1<<6)) _detected = true;
		_available = _detected;
	}
};

/**
 * Class for packages that need Vector Floating Point (VFP2) support
 */
class vfp_check : public env_check
{
public:
	vfp_check() : env_check("vfp", "Vector Floating point", "v", System, 32)
	{
		// Notes from Jeffrey Lee
		// VFPSupport_Features 0 is the main SWI you’ll want to use to detect VFP support.
		// Assuming the baseline VFPv2 support is all you care about, I think you just need
		// to check that each nibble of the MVFR0 register is non-zero, apart from the
		// nibble at bits 24-27 FPShVec, which can be zero.
		// On a RPI 3 it also seems bits 12-15 FTrap are also zero

		_kernel_swi_regs regs;
		try
		{
			regs.r[0] = 0;
			os::call_swi(swi::VFPSupport_Features, &regs);
			unsigned int mvfr0 = static_cast<unsigned int>(regs.r[1]);
			_detected = ((mvfr0 & 0xF0000000) != 0);
			unsigned int check =  0x00F00000;
			while (_detected && check > 0)
			{
				if (check != 0x0000F000)
				{
				   _detected = ((mvfr0 & check) != 0);
				}
				check >>= 4;
			}
		} catch(...)
		{
			_detected = false;
		}
		_available = _detected;
	}
};

/**
 * Class for packages that need Vector Floating Point (VFP3) support
 */
class vfpv3_check : public env_check
{
public:
	vfpv3_check() : env_check("vfpv3", "Vector Floating point V3", "v3", System, 34)
	{
		_kernel_swi_regs regs;
		try
		{
			regs.r[0] = 0;
			os::call_swi(swi::VFPSupport_Features, &regs);
			unsigned int mvfr0 = static_cast<unsigned int>(regs.r[1]);
			_detected = ((mvfr0 & 0xF00) == 0x200 || (mvfr0 & 0xF0) == 0x20);
		} catch(...)
		{
			_detected = false;
		}
		_available = _detected;
	}
};

/**
 * Class for packages that use the SWP instruction that was discontinued in ARMv8?
 */
class swp_check : public env_check
{
public:
	swp_check() : env_check("swp", "ARM SWP/SWPB instruction available","s", System, 16)
	{
		if ((platform_features() & (1<<11)) == 0) _detected = true;
		_available = _detected;
	}
};

/**
 * Check if module is loaded - declaration in header as used in enc_checker.cpp
 */
module_check::module_check(const std::string &title) : env_check(title, title,
		env_checker::instance()->get_module_id(title), Module, 100)
{
	module_info mi;
	_detected = mi.lookup(title);
	_available = _detected;
	if (_detected) _description = mi.help_string();
}


/**
 * Initialise all the known environment checks
 * @param module_map_path file path to map of short module ids to module titles
 */
void env_checker::initialise(const std::string &module_map_path)
{
	_module_map_path = module_map_path;
	read_module_map();
	// Ensure that the ids are all different for the checks
	// The id must consist of a single lower case letter, optionally followed by a number
	// All ids starting with a "u" (unset and unknown) and "m" (modules are reserved)
	add_check(new all_check());
	add_check(new arm_check());
	add_check(new arm26_check());
	add_check(new arm32_check());
	add_check(new swp_check());
	add_check(new vfp_check());
	add_check(new vfpv3_check());
	// Unset check is created in constructor
	// Module checks are added as they are found
	// Unknown checks are added as they are found
}


} /* namespace lippkg */
