// This file is part of LibPkg.
// Copyright � 2003-2005 Graham Shaw.
// Copyright � 2018 Alan Buckley.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef LIBPKG_ENV_PACKAGES_TABLE
#define LIBPKG_ENV_PACKAGES_TABLE

#include "libpkg/version.h"
#include "libpkg/binary_control_table.h"
#include "libpkg/env_checker.h"
#include <map>

namespace pkg {

/**
 * A class that filters the binary control table to create
 * a list of the packages available in the current environment.
 * It also records the latest version with the highest
 * weight to give the "best" version to install.
 */
class env_packages_table :
			public table,
			private table::watcher,
			private env_checker::watcher
{
public:
	struct best
	{
		/** The package version. */
		version pkgvrsn;
		/** The package environment id */
		string pkgenv;

		best() {}
		/** Contruct best type from package name and version.
		 * @param pkgvrsn the package version
		 * @param pkgenv the package environment id
		 */
		best(const version& _pkgvrsn, const string &_pkgenv) : pkgvrsn(_pkgvrsn), pkgenv(_pkgenv) {}
	};
	typedef std::string key_type;
	typedef best mapped_type;
	typedef std::map<key_type,mapped_type>::const_iterator const_iterator;

private:
	/** The table this provides a filtered view of */
	binary_control_table *_control;
	/** A map from package name and version to control record. */
	std::map<key_type,mapped_type> _data;

public:
	env_packages_table(binary_control_table *control);
	virtual ~env_packages_table();

	/** Get const iterator for start of table.
	 * @return the const iterator
	 */
	const_iterator begin() const
		{ return _data.begin(); }

	/** Get const iterator for end of table.
	 * @return the const iterator
	 */
	const_iterator end() const
		{ return _data.end(); }

	/**
	 * Find a package in the list
	 * @return iterator to best package or end() if not found
	 */
	const_iterator find(const std::string &pkgname) const
		{ return _data.find(pkgname);}

	/**
	 * Get the control record for the "best" package for the environment
	 */
	const binary_control &control(const std::string &pkgname) const;

	/**
	 * Get information on the "best" package for the environment
	 */
	const best &operator[](const std::string &pkgname) const;

private:
	virtual void handle_change(table& t);
	virtual void handle_change(env_checker& e);
	void rebuild();
};

} /* namespace pkg */

#endif /* LIBPKG_LIBPKG_LATEST_CONTROL_TABLE */
