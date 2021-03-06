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

#ifndef LIBPKG_STATUS_TABLE
#define LIBPKG_STATUS_TABLE

#include <map>
#include <string>

#include "libpkg/status.h"
#include "libpkg/table.h"

namespace pkg {

using std::string;

/** A class for mapping package name to package status. */
class status_table:
	public table
{
public:
	typedef string key_type;
	typedef status mapped_type;
	typedef std::map<key_type,mapped_type>::const_iterator const_iterator;
	class commit_error;
private:
	/** The pathname of the underlying status file,
	 * or the empty string if none. */
	string _pathname;

	/** A map from package name to package status. */
	std::map<key_type,mapped_type> _data;
public:
	/** Construct status table.
	 * @param pathname the pathname of the underlying status file
	 */
	status_table(const string& pathname=string());

	/** Destroy status table. */
	virtual ~status_table();

	/** Get status of package.
	 * The table cannot be modified using this operator.
	 * @param key the package name
	 * @return the status of the package
	 */
	const mapped_type& operator[](const key_type& key) const;

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

	/** Find const iterator for package.
	 * @param key the package name
	 * @return the const iterator, or end() if not found
	 */
	const_iterator find(const key_type& key) const;

	/** Set status of package.
	 * The underlying status file will not be modified until the change is
	 * committed, but watchers are notified immediately.
	 * @param key the package name
	 * @param value the required status
	 */
	void insert(const key_type& key,const mapped_type& value);

	/** Set status of packages.
	 * @param table the table from which the values are copied
	 */
	void insert(const status_table& table);

	/** Clear status of all packages. */
	void clear();

	/** Commit changes.
	 * Any changes since the last call to commit() or rollback() are
	 * committed to disc.
	 */
	void commit();

	/** Roll back changes.
	 * Any changes since the last call to commit() or rollback() are
	 * discarded.
	 */
	void rollback();
private:
	/** Read status file.
	 * @param pathname the pathname of the status file
	 * @return true if the file was found, otherwise false
	 */
	bool read(const string& pathname);
};

/** An exception class for reporting failure to commit table. */
class status_table::commit_error:
	public std::runtime_error
{
public:
	/** Construct commit error. */
	commit_error();
};

}; /* namespace pkg */

#endif
