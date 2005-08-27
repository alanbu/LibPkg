// This file is part of LibPkg.
// Copyright © 2003-2005 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef _LIBPKG_UNPACK
#define _LIBPKG_UNPACK

#include <string>
#include <set>

#include "libpkg/auto_dir.h"
#include "libpkg/thread.h"

namespace pkg {

using std::string;

class pkgbase;
class zipfile;

/** A class for unpacking and removing sets of packages. */
class unpack:
	public thread
{
public:
	/** A type for representing byte counts. */
	typedef unsigned long long size_type;

	/** A null value for use in place of a byte count. */
	static const size_type npos=static_cast<size_type>(-1);

	/** An enumeration for describing the state of the unpack operation. */
	enum state_type
	{
		/** The state in which the states of packages to be unpacked are
		 * changed to status::state_half_unpacked.
		 */
		state_pre_unpack,
		/** The state in which the states of packages to be removed are
		 * changed to status::state_half_unpacked.
		 */
		state_pre_remove,
		/** The state in which files are unpacked from their zip archives
		 * and moved to temporary locations. */
		state_unpack,
		/** The state in which old versions of files are backed up and
		 * replaced with new versions. */
		state_replace,
		/** The state in which old versions of files that do not have
		 * replacements are backed up then removed. */
		state_remove,
		/** The state in which backups are deleted, and the states of
		 * packages to be removed are changed to status::state_removed.
		 */
		state_post_remove,
		/** The state in which the states of packages to be unpacked
		 * are changed to status::state_unpacked. */
		state_post_unpack,
		/** The state in which all operations have been successfully
		 * completed. */
		state_done,
		/** The state in which state_remove is being backed out. */
		state_unwind_remove,
		/** The state in which state_replace is being backed out. */
		state_unwind_replace,
		/** The state in which state_unpack is being backed out. */
		state_unwind_unpack,
		/** The state in which state_pre_remove is being backed out. */
		state_unwind_pre_remove,
		/** The state in which state_pre_unpack is being backed out. */
		state_unwind_pre_unpack,
		/** The state in which an error has occurred and an attempt
		 * has been made to back out changes. */
		state_fail
	};
private:
	/** The package database. */
	pkgbase& _pb;

	/** The current state of the unpack operation. */
	state_type _state;

	/** An auto_dir object for automatically creating and deleting
	 * directories.*/
	auto_dir _ad;

	/** The current zip file access object, or 0 if there is none. */
	zipfile* _zf;

	/** The current package name, or the empty string if none. */
	string _pkgname;

	/** The number of files processed. */
	size_type _files_done;

	/** The total number of files to process. */
	size_type _files_total;

	/** The number of bytes processed. */
	size_type _bytes_done;

	/** The total number of bytes to process. */
	size_type _bytes_total;

	/** The total number of files to unpack. */
	size_type _files_total_unpack;

	/** The total number of files to remove. */
	size_type _files_total_remove;

	/** The total number of bytes to unpack. */
	size_type _bytes_total_unpack;

	/** The error message.
	 * This is meaningful when _state==state_fail. */
	string _message;

	/** The set of packages that are to be unpacked.
	 * Packages in this set have not yet changed state. */
	std::set<string> _packages_to_unpack;

	/** The set of packages that have been pre-unpacked.
	 * Packages in this set are in state_half_unpacked. */
	std::set<string> _packages_pre_unpacked;

	/** The set of packages that are being unpacked.
	 * Packages in this set are in state_half_unpacked. */
	std::set<string> _packages_being_unpacked;

	/** The set of packages that have been fully unpacked.
	 * Packages in this set are in state_unpacked. */
	std::set<string> _packages_unpacked;

	/** The set of packages that are to be removed.
	 * Packages in this set have not yet changed state. */
	std::set<string> _packages_to_remove;

	/** The set of packages that are being removed.
	 * Packages in this set are in state_half_unpacked. */
	std::set<string> _packages_being_removed;

	/** The set of packages that have been fully removed (or upgraded).
	 * Packages in this set are in state_removed or state_unpacked. */
	std::set<string> _packages_removed;

	/** The set of source pathnames (for the current package) that have
	 * not yet been unpacked. */
	std::set<string> _files_to_unpack;

	/** The set of destination pathnames (for all packages) that have
	 * been unpacked to their temporary locations. */
	std::set<string> _files_being_unpacked;

	/** The set of destination pathnames (for all packages) that have
	 * been unpacked to their final locations. */
	std::set<string> _files_unpacked;

	/** The set of destination pathnames (for all packages) that have
	 * not yet been removed. */
	std::set<string> _files_to_remove;

	/** The set of destination pathnames (for all packages) that have
	 * been backed up prior to removal, but not yet fully removed. */
	std::set<string> _files_being_removed;

	/** The set of destination pathnames (for all packages) that have
	 * been fully removed. */
	std::set<string> _files_removed;

	/** The set of packages that cannot be processed. */
	std::set<string> _packages_cannot_process;

	/** The set of destination pathnames (for all packages) that conflict
	 * with files already on the system. */
	std::set<string> _files_that_conflict;
public:
	/** Construct unpack object.
	 * @param pb the package database
	 * @param packages the set of packages to process
	 */
	unpack(pkgbase& pb,const std::set<string>& packages);

	/** Destroy unpack object. */
	virtual ~unpack();

	/** Get current state of the unpack operation.
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

	/** Get the set of packages that cannot be processed.
	 * When state()==state_fail, this function returns a list of packages
	 * that cannot be processed until the package manager has been
	 * upgraded.
	 * @return the set of packages that cannot be processed
	 */
	const std::set<string>& packages_cannot_process() const
		{ return _packages_cannot_process; }

	/** Get the set of destination pathnames that conflict with files
	 * already on the system.
	 * When state()==state_fail, this function returns a list of files
	 * that must be deleted before the given set of packages can be
	 * processed.
	 */
	const std::set<string>& files_that_conflict() const
		{ return _files_that_conflict; }
protected:
	void poll();
private:
	/** Poll this thread, without exception handling.
	 * This function is equivalent to poll(), except that it does not
	 * handle exceptions.  The reason for this split is to make the code
	 * more manageable.
	 */
	void _poll();

	/** Read manifest from info directory.
	 * The set passed into this function is not cleared before use,
	 * so any pathnames within it will be merged into the result.
	 * @param mf a set to hold the result
	 * @param pkgname the package name
	 */
	void read_manifest(std::set<string>& mf,const string& pkgname);

	/** Build manifest from package file.
	 * The set passed into this function is not cleared before use,
	 * so any pathnames within it will be merged into the result.
	 * The uncompressed size of each file is added to the byte
	 * count if one is specified.
	 * @param mf a set to hold the result
	 * @param zf a zipfile access object for the package file
	 * @param usize a byte count to which the rounded uncompressed size
	 *  of each file is added, or 0 if none
	 */
	void build_manifest(std::set<string>& mf,zipfile& zf,size_type* usize=0);

	/** Prepare manifest for activation.
	 * The manifest is written to a file called "Files++" in the info
	 * directory.  Any existing file of that name is overwritten.
	 * @param mf a set holding the manifest
	 * @param pkgname the package name
	 */
	void prepare_manifest(std::set<string>& mf,const string& pkgname);

	/** Activate manifest.
	 * In the info directory, "Files" is backed up to "Files--" then
	 * replaced by "Files++".
	 * @param pkgname the package name
	 */
	void activate_manifest(const string& pkgname);

	/** Remove manifest.
	 * In the info directory, "Files", "Files--" and "Files++" are
	 * removed.  It is not an error if any or all of them do not exist.
	 * @param pkgname the package name
	 */
	void remove_manifest(const string& pkgname);

	/** Unpack file from package.
	 * The file is unpacked into the temporary subdirectory "~RiscPkg++".
	 * @param src_pathname the pathname wrt the root of the zip file
	 * @param dst_pathname the pathname wrt the filesystem
	 * @param usize a byte count to which the uncompressed size of each
	 *  file is added, or 0 if none
	 */
	void unpack_file(const string& src_pathname,const string& dst_pathname);

	/** Replace file with copy unpacked from package, after making backup.
	 * The file is backed up to the subdirectory "~RiscPkg--", then
	 * replaced with the copy from "~RiscPkg++".
	 * @param dst_pathname the pathname wrt the filesystem
	 * @param overwrite true to silently replace existing file,
	 *  false to throw error if file exists
	 */
	void replace_file(const string& dst_pathname,bool overwrite);

	/** Remove file, after making backup.
	 * The file is backed up to the subdirectory "~RiscPkg--".
	 * It is not an error if the file does not exist.
	 * @param dst_pathname the pathname wrt the filesystem
	 */
	void remove_file(const string& dst_pathname);

	/** Remove backup of file.
	 * The backup, in the subdirectory "~RiscPkg--", is deleted.
	 * @param dst_pathname the pathname wrt the filesystem
	 */
	void remove_backup(const string& dst_pathname);

	/** Unwind removal of file.
	 * The file is restored from the backup in "~RiscPkg--".
	 * @param dst_pathname the pathname wrt the filesystem
	 */
	void unwind_remove_file(const string& dst_pathname);

	/** Unwind replacement of file.
	 * If the overwrite flag is false then the file is simply deleted,
	 * otherwise it is restored from the backup in "~RiscPkg--".
	 * Note that it is /not/ moved back to "~RiscPkg++" (so there is
	 * no need to call unwind_unpack_file() once a file has been
	 * processed by this function).
	 * @param dst_pathname the pathname wrt the filesystem
	 */
	void unwind_replace_file(const string& dst_pathname,bool overwrite);

	/** Unwind unpacking of file.
	 * The unpacked copy of the file in "~RiscPkg++" is deleted.
	 * @param dst_pathname the pathname wrt the filesystem
	 */
	void unwind_unpack_file(const string& dst_pathname);

	class cannot_process;
	class file_conflict;
	class file_info_not_found;
	class riscos_info_not_found;
};

}; /* namespace pkg */

#endif
