// This file is part of LibPkg.
// Copyright � 2003-2005 Graham Shaw.
// Copyright � 2018 Alan Buckley.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include <libpkg/env_packages_table.h>
#include "string.h"

namespace pkg {

env_packages_table::env_packages_table(binary_control_table *control) :
		_control(control)
{
	rebuild();
	table::watcher::watch(*_control);
	env_checker::watcher::watch(*env_checker::instance());
}

env_packages_table::~env_packages_table()
{
}


const binary_control &env_packages_table::control(const std::string &pkgname) const
{
	static binary_control not_found;
	auto found = find(pkgname);
	if (found == end())
	{
		return not_found;
	}
	binary_control_table::key_type key(pkgname, found->second.pkgvrsn, found->second.pkgenv);
	return (*_control)[key];
}

const env_packages_table::best &env_packages_table::operator[](const std::string &pkgname) const
{
	static best not_found;
	auto found = find(pkgname);
	if (found == end())
	{
		return not_found;
	}

	return found->second;
}


/**
 * binary control table has changed so update
 */
void env_packages_table::handle_change(table& t)
{
	rebuild();
}

/**
 * Environment has changed so build
 */
void env_packages_table::handle_change(env_checker& e)
{
	rebuild();
}

/**
 * rebuild table from binary control table
 */
void env_packages_table::rebuild()
{
	if (!_control) return;

	_data.clear();

	for (auto &bcentry : *_control)
	{
		const binary_control &bctrl = bcentry.second;
		if (bctrl.package_env()->available())
		{
			std::map<key_type,mapped_type>::iterator found = _data.find(bcentry.first.pkgname);
			version new_version(bctrl.version());
			if (found == _data.end())
			{
				best b(new_version, bctrl.environment_id());
				// New package name
				_data.insert(std::make_pair(bcentry.first.pkgname, b));
			} else
			{
				if (new_version > found->second.pkgvrsn)
				{
					// found a better version
					found->second = best(new_version, bctrl.environment_id());
				} else if (new_version == found->second.pkgvrsn)
				{
					// Versions the same check the weights
					binary_control_table::key_type key(bcentry.first.pkgname, found->second.pkgvrsn, found->second.pkgenv);
					const binary_control &pctrl = (*_control)[key];
					if (bctrl.install_priority() > pctrl.install_priority())
					{
						found->second = best(new_version, bctrl.environment_id());
					}
				}
			}
		}
	}
	notify();
}


} /* namespace pkg */
