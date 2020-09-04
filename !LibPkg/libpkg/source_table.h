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

#ifndef LIBPKG_SOURCE_TABLE
#define LIBPKG_SOURCE_TABLE

#include <list>
#include <string>

#include "libpkg/table.h"

namespace pkg {

using std::string;

/** A class for holding a list of source URLs.
 * The underlying sources file consists of a list of source URLs,
 * one per line.  Trailing spaces are ignored, as are blank lines.
 * Comments are introduced by a hash character.
 *
 * The order of the list is significant and is preserved.  Sources
 * higher in the list take precedence over those further down.
 */
class source_table:
	public table
{
public:
	typedef string value_type;
	typedef std::list<value_type>::const_iterator const_iterator;
private:
	/** The pathname of the default soruces file. */
	string _dpathname;

	/** The pathname of the configured sources file. */
	string _pathname;

	/** A list of sources. */
	std::list<string> _data;
public:
	/** Construct source table.
	 * @param dpathname the pathname of the default sources file
	 * @param pathname the pathname of the configured sources file
	 */
	source_table(const string& dpathname,const string& pathname);

	/** Destroy source table. */
	virtual ~source_table();

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

	/** Re-read the default and configured sources files. */
	void update();
private:
	/** Read sources file.
	 * @param pathname the pathname
	 * @return true if the pathname was found, otherwise false
	 */
	bool read(const string& pathname);
};

}; /* namespace pkg */

#endif
