// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef _LIBPKG_TABLE
#define _LIBPKG_TABLE

#include <set>

namespace pkg {

/** A base class to represent a data table.
 * The base class does not provide any access to the content of the
 * table.  It does implement the notification mechanism, whereby other
 * objects can be informed when the content has changed.
 */
class table
{
public:
	class watcher;
	friend watcher;
private:
	/** The set of currently registered watchers. */
	set<watcher*> _watchers;
public:
	/** Construct table. */
	table();

	/** Destroy table. */
	virtual ~table();
protected:
	/** Notify watchers that a change has occurred. */
	void notify();
private:
	/** Register a watcher.
	 * This can only be done by the watcher in question (which is then
	 * responsible for deregistering itself before it is destroyed).
	 * @param w the watcher to be registed
	 */
	void register_watcher(watcher& w);

	/** Deregister a watcher.
	 * This can only be done by the watcher in question.
	 * @param w the watcher to be deregistered
	 */
	void deregister_watcher(watcher& w);
};

/** A mixin class to allow an object to watch one or more tables. */
class table::watcher
{
private:
	/** The set of tables that are being watched. */
	set<table*> _tables;
public:
	/** Construct watcher. */
	watcher();

	/** Destroy watcher. */
	virtual ~watcher();

	/** Begin watching table.
	 * @param table the table to begin watching
	 */
	void watch(table& t);

	/** Cease watching table.
	 * @param table the table to cease watching
	 */
	void unwatch(table& t);

	/** Handle change to table.
	 * @param table the table that has changed
	 */
	virtual void handle_change(table& t)=0;
};

}; /* namespace pkg */

#endif
