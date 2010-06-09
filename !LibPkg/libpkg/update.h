// This file is part of LibPkg.
// Copyright © 2003-2005 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef LIBPKG_UPDATE
#define LIBPKG_UPDATE

#include <string>
#include <map>
#include <set>
#include <iosfwd>

#include "libpkg/source_table.h"
#include "libpkg/binary_control_table.h"
#include "libpkg/thread.h"

namespace pkg {

using std::string;

class pkgbase;
class download;

/** A class for updating the package database.
 * Control files from remote sources take precedence over local packages
 * because they contain more information (such as the download URL).
 */
class update:
	public thread
{
public:
	/** A type for representing byte counts. */
	typedef unsigned long long size_type;

	/** A null value for use in place of a byte count. */
	static const size_type npos=static_cast<size_type>(-1);

	/** An enumeration for describing the state of the update operation. */
	enum state_type
	{
		/** The state in which the sources list is being read. */
		state_srclist,
		/** The state in which the package indexes are being downloaded. */
		state_download,
		/** The state in which remote sources are incorporated into the
		 * available list. */
		state_build_sources,
		/** The state in which local packages are incorporated into the
		 * available list. */
		state_build_local,
		/** The state in which the update operation has been successfully
		 * completed. */
		state_done,
		/** The state in which the update operation failed. */
		state_fail
	};
private:
	/** The package database. */
	pkgbase& _pb;

	/** The current state of the update operation. */
	state_type _state;

	/** The URL for the current source. */
	string _url;

	/** The download operation for the current source, or 0 if none. */
	download* _dload;

	/** The set of sources awaiting download. */
	std::set<string> _sources_to_download;

	/** The set of sources that have been downloaded but not built. */
	std::set<string> _sources_to_build;

	/** The set of packages (name and version) that have been written
	 * to the available list. */
	std::set<binary_control_table::key_type> _packages_written;

	/** An output stream for writing the available list. */
	std::ostream* _out;

	/** The number of bytes downloaded. */
	size_type _bytes_done;

	/** The total number of bytes to download. */
	size_type _bytes_total;

	/** The error message.
	 * This is meaningful when _state==state_fail. */
	string _message;

	struct progress;

	/** A map giving the current progress for each source. */
	std::map<string,progress> _progress_table;
public:
	/** Construct update operation.
	 * @param pb the package database
	 */
	update(pkgbase& pb);

	/** Destroy update operation. */
	virtual ~update();

	/** Get current state.
	 * @return the current state
	 */
	state_type state() const
		{ return _state; }

	/** Get number of bytes downloaded.
	 * @return the number of bytes downloaded
	 */
	size_type bytes_done() const
		{ return _bytes_done; }

	/** Get total number of bytes to download.
	 * @return the total number of bytes to download
	 */
	size_type bytes_total() const
		{ return _bytes_total; }

	/** Get error message.
	 * When state()==state_fail, this function returns a human-readable
	 * description of what went wrong.
	 * @return the error message
	 */
	string message() const
		{ return _message; }
protected:
	void poll();
private:
	/** Poll this thread, without exception handling.
	 * This function is equivalent to poll(), except that it does not
	 * handle exceptions.  The reason for this split is to make the code
	 * more manageable.
	 */
	void _poll();

	/** Update reported progress.
	 * This function recalculates the number of bytes downloaded and
	 * the total number of bytes to download.
	 */
	void update_progress();
};

/** A structure for monitoring the progress of one source. */
struct update::progress
{
	/** The number of bytes downloaded. */
	size_type bytes_done;
	/** The total number of bytes to download, or npos if not known. */
	size_type bytes_total;
	/** The total number of bytes when most recently downloaded, or npos
	 * if no previous total is available. */
	size_type bytes_prev;
	/** Construct progress structure.
	 * By default no bytes have been downloaded, the total to download
	 * is unknown, and there is no total from a previous download. */
	progress();
};

}; /* namespace pkg */

#endif
