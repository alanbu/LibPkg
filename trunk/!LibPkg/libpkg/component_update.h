// This file is part of LibPkg.
// Copyright © 2013 Alan Buckley.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef LIBPKG_COMPONENT_UPDATE
#define LIBPKG_COMPONENT_UPDATE

#include <vector>
#include <string>

#include "libpkg/component.h"

namespace pkg {

using std::string;

/** A class for managing the updating of package components. */
class component_update
{
public:
	typedef std::vector<component>::const_iterator const_iterator;
	class commit_error;
private:
	/** The pathname of the underlying component update file,
	 * or the empty string if none. */
	string _pathname;

	/** A vector with the components to update. */
	std::vector<component> _data;
public:
	/** Construct component update.
	 *
	 * @param pathname the pathname of the underlying component update file
	 */
	component_update(const string& pathname=string());

	/** Destroy component update. */
	virtual ~component_update();

	/** Get component details for a component name
	 * The updates cannot be modified using this operator.
	 * @param key the package name
	 * @return the status of the package
	 */
	const component& operator[](const std::string &name) const;

	/** Get const iterator for start of the updates.
	 * @return the const iterator
	 */
	const_iterator begin() const
		{ return _data.begin(); }

	/** Get const iterator for end of the updates.
	 * @return the const iterator
	 */
	const_iterator end() const
		{ return _data.end(); }

	/** Find const iterator for a component name.
	 * @param name the component name
	 * @return the const iterator, or end() if not found
	 */
	const_iterator find(const std::string& name) const;

	/** Add a component.
	 * The underlying updates file will not be modified until the change is
	 * committed.
	 * @param value the new component
	 */
	void insert(const component& value);

	/** Insert contents of another table into this one, overwriting any existing items */
	void insert(const component_update& table);


	/** Clear status of all components. */
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

	/** The updates have been completed, so remove the file */
	void done();
private:
	/** Read the update file.
	 * @param pathname the pathname of the status file
	 * @return true if the file was found, otherwise false
	 */
	bool read(const string& pathname);
};

/** An exception class for reporting failure to commit table. */
class component_update::commit_error:
	public std::runtime_error
{
public:
	/** Construct commit error. */
	commit_error();
};

}; /* namespace pkg */

#endif
