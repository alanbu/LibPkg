// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef _LIBPKG_COMMIT
#define _LIBPKG_COMMIT

#include "libpkg/thread.h"

namespace pkg {

class control_binary;
class pkgbase;
class download;
class unpack;

/** A class for installing, removing and purging packages. */
class commit:
	public thread
{
public:
	/** A type for representing byte counts. */
	typedef unsigned long long size_type;

	/** A null value for use in place of a byte count. */
	static const size_type npos=static_cast<size_type>(-1);

	// An enumeration for describing the state of the commit operation. */
	enum state_type
	{
		/** The state in which packages are being considered for download. */
		state_pre_download,
		/** The state in which packages are being downloaded. */
		state_download,
		/** The state in which packages are being unpacked or removed. */
		state_unpack,
		/** The state in which packages are being configured. */
		state_configure,
		/** The state in which packages are being purged. */
		state_purge,
		/** The state in which all operations have been successfully
		 * completed. */
		state_done,
		/** The state in which an error has occurred. */
		state_fail
	};
private:
	/** The package database. */
	pkgbase& _pb;

	/** The current state of the commit operation. */
	state_type _state;

	/** Packages to be processed. */
	set<string> _packages_to_process;

	/** Packages for which a download is required. */
	set<string> _packages_to_download;

	/** Packages that have not been processed by state_unpack.
	 * Note that packages are only unpacked/removed if their current and
	 * selected status indicates that they need to be. */
	set<string> _packages_to_unpack;

	/** Packages that have not been processed by state_configure.
	 * Note that packages are only configured if their current and
	 * selected status indicates that they can be and need to be. */
	set<string> _packages_to_configure;

	/** Packages that have not been processed by state_purge.
	 * Note that packages are only purged if their current and
	 * selected status indicates that they can be and need to be. */
	set<string> _packages_to_purge;

	/** The name of the package currently being processed. */
	string _pkgname;

	/** The current download operation, or 0 if none. */
	download* _dload;

	/** The current unpack operation, or 0 if none. */
	unpack* _upack;

	/** The number of files processed. */
	size_type _files_done;

	/** The total number of files to process. */
	size_type _files_total;

	/** The number of bytes processed. */
	size_type _bytes_done;

	/** The total number of bytes to process. */
	size_type _bytes_total;

	/** The error message.
	 * This is meaningful when _state==state_fail. */
	string _message;

	struct progress;

	/** A map giving the current download progress for each package. */
	map<string,progress> _progress_table;
public:
	/** Construct commit operation.
	 * @param pb the package database
	 * @param packages the set of packages to process
	 */
	commit(pkgbase& pb,const set<string>& packages);

	/** Destroy commit operation. */
	virtual ~commit();

	/** Get current state of the commit operation.
	 * @return the current state
	 */
	state_type state() const
		{ return _state; }

	/** Get number of files processed.
	 * @return the number of files processed (in current stage)
	 */
	size_type files_done() const
		{ return _files_done; }

	/** Get total number of files to process.
	 * @return the total number of files to process (in current stage)
	 */
	size_type files_total() const
		{ return _files_total; }

	/** Get number of bytes processed.
	 * @return the number of bytes processed (in current stage)
	 */
	size_type bytes_done() const
		{ return _bytes_done; }

	/** Get total number of bytes to process.
	 * @return the total number of bytes to process (in current stage)
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
	virtual void poll();
private:
	/** Update reported progress of download.
	 * This function recalculates the number of bytes downloaded and
	 * the total number of bytes to download.
	 */
	void update_download_progress();
};

/** A structure for monitoring the download progress of one source. */
struct commit::progress
{
	/** The number of bytes downloaded. */
	size_type bytes_done;
	/** The total number of bytes to download, or npos if not known. */
	size_type bytes_total;
	/** The total number of bytes specified in the control record,
	 * or npos if no total was given. */
	size_type bytes_ctrl;
	/** Construct progress structure.
	 * By default no bytes have been downloaded, the total to download
	 * is unknown, and there is no total from the control record. */
	progress();
};

}; /* namespace pkg */

#endif
