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

#include "libpkg/binary_control.h"
#include "libpkg/env_checker.h"
#include <cstdlib>


namespace pkg {

static const std::map<string,int>& init_priorities()
{
	static std::map<string,int> priorities;
	int pr=0;
	priorities["md5sum"]=--pr;
	priorities["size"]=--pr;
	priorities["url"]=--pr;

	priorities["conflicts"]=--pr;
	priorities["suggests"]=--pr;
	priorities["recommends"]=--pr;
	priorities["depends"]=--pr;
	priorities["version"]=--pr;
	priorities["source"]=--pr;
	priorities["osdepends"] = --pr;
	priorities["environment"]=--pr;

	priorities["standards-version"]=--pr;
	priorities["maintainer"]=--pr;
	priorities["installed-size"]=--pr;
	priorities["section"]=--pr;
	priorities["priority"]=--pr;
	priorities["package"]=--pr;
	pr=0;
	priorities["description"]=++pr;
	return priorities;
}

binary_control::binary_control()
  : _environment(nullptr),
	_install_priority(0)
{}

binary_control::~binary_control()
{}

int binary_control::priority(const string& value) const
{
	static const std::map<string,int>& priorities=init_priorities();
	std::map<string,int>::const_iterator f=priorities.find(value);
	return (f!=priorities.end())?(*f).second:0;
}

std::string binary_control::environment_id() const
{
	return package_env()->id();
}

const pkg_env *binary_control::package_env() const
{
	if (!_environment)
	{
		_environment = env_checker::instance()->package_env(environment(), osdepends());
	}
	return _environment;
}

int binary_control::install_priority() const
{
	if (_install_priority == 0)
	{
		const_iterator f=find("InstallPriority");
		if (f==end())
		{
			_install_priority = 0;
		} else
		{
			_install_priority = std::atoi(f->second.c_str());
		}
		if (_install_priority == 0) _install_priority = package_env()->default_install_priority();
	}
	return _install_priority;
}

}; /* namespace pkg */
