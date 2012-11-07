// This file is part of LibPkg.
// Copyright © 2003-2005 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include <stdexcept>
#include <iostream>

#include "libpkg/filesystem.h"
#include "libpkg/version.h"
#include "libpkg/binary_control.h"
#include "libpkg/status.h"
#include "libpkg/pkgbase.h"
#include "libpkg/zipfile.h"
#include "libpkg/standards_version.h"
#include "libpkg/unpack.h"

namespace {

using std::string;

/** The source pathname of the package control file. */
const string ctrl_src_pathname("RiscPkg.Control");

/** The destination filename of the manifest file. */
const string mf_dst_filename("Files");

/** The temporary filename of the manifest file. */
const string mf_tmp_filename("Files++");

/** The backup filename of the manifest file. */
const string mf_bak_filename("Files--");

/** The temporary directory name.
 * Files are unpacked into a directory of this name before being moved
 * to their final destinations.
 */
const string tmp_dirname("~RiscPkg++");

/** The backup directory name.
 * Files are backed-up into a directory of this name prior to removal.
 */
const string bak_dirname("~RiscPkg--");

/** Convert zip file pathname to source pathname.
 * @param zip_pathname the zip file pathname
 * @return the source pathname
 */
string zip_to_src(const string& zip_pathname)
{
	string src_pathname(zip_pathname);
	for (unsigned int i=0;i!=src_pathname.length();++i)
	{
		switch (src_pathname[i])
		{
		case '/':
			src_pathname[i]='.';
			break;
		case '.':
			src_pathname[i]='/';
			break;
		}
	}
	return src_pathname;
}

/** Convert source pathname to zip file pathname.
 * @param src_pathname the source pathname
 * @return the zip file pathname
 */
string src_to_zip(const string& src_pathname)
{
	string zip_pathname(src_pathname);
	for (unsigned int i=0;i!=zip_pathname.length();++i)
	{
		switch (zip_pathname[i])
		{
		case '/':
			zip_pathname[i]='.';
			break;
		case '.':
			zip_pathname[i]='/';
			break;
		}
	}
	return zip_pathname;
}

/** Convert destination pathname to temporary pathname.
 * @param dst_pathname the destination pathname
 * @return the temporary pathname
 */
string dst_to_tmp(const string& dst_pathname)
{
	unsigned int ds=dst_pathname.rfind('.');
	string prefix(dst_pathname,0,ds);
	string suffix(dst_pathname,ds,string::npos);
	return string(prefix+string(".")+tmp_dirname+suffix);
}

/** Convert destination pathname to backup pathname.
 * @param dst_pathname the destination pathname
 * @return the backup pathname
 */
string dst_to_bak(const string& dst_pathname)
{
	unsigned int ds=dst_pathname.rfind('.');
	string prefix(dst_pathname,0,ds);
	string suffix(dst_pathname,ds,string::npos);
	return string(prefix+string(".")+bak_dirname+suffix);
}

}; /* anonymous namespace */

namespace pkg {

/** An exception class for reporting that one or more packages cannot
 * be processed. */
class unpack::cannot_process:
	public std::runtime_error
{
public:
	/** Construct cannot-process error. */
	cannot_process();
};

/** An exception class for reporting that one or more files conflict
 * with those already on the system. */
class unpack::file_conflict:
	public std::runtime_error
{
public:
	/** Construct file-conflict error. */
	file_conflict();
};

/** An exception class for reporting that a file information record could
 * not be found. */
class unpack::file_info_not_found:
	public std::runtime_error
{
public:
	/** Construct file-info-not-found error. */
	file_info_not_found();
};

unpack::unpack(pkgbase& pb,const std::set<string>& packages):
	_pb(pb),
	_state(state_pre_unpack),
	_zf(0),
	_files_done(0),
	_files_total(npos),
	_bytes_done(0),
	_bytes_total(npos),
	_files_total_unpack(0),
	_files_total_remove(0),
	_bytes_total_unpack(0)
{
	// For each package to be processed, determine whether it should
	// be unpacked and/or removed.
	for (std::set<string>::const_iterator i=packages.begin();
		i!=packages.end();++i)
	{
		string pkgname=*i;
		const status& curstat=_pb.curstat()[pkgname];
		const status& selstat=_pb.selstat()[pkgname];
		if (unpack_req(curstat,selstat))
			_packages_to_unpack.insert(pkgname);
		if (remove_req(curstat,selstat))
			_packages_to_remove.insert(pkgname);
	}
}

unpack::~unpack()
{}

void unpack::poll()
{
	try
	{
		_poll();
	}
	catch (std::exception& ex)
	{
		_message=ex.what();
		_pb.curstat().rollback();
		delete _zf;
		_zf=0;
		switch (_state)
		{
		case state_pre_unpack:
		case state_pre_remove:
			_state=state_fail;
			break;
		case state_unpack:
			_state=state_unwind_unpack;
			break;
		case state_replace:
		case state_remove:
			_state=state_unwind_replace;
			break;
		default:
			_state=state_fail;
			break;
		}
	}
}

void unpack::_poll()
{
	switch (_state)
	{
	case state_pre_unpack:
		if (_files_to_unpack.size())
		{
			// Test whether file exists, add to set of conflicts if it does.
			string src_pathname=*_files_to_unpack.begin();
			string dst_pathname=_pb.paths()(src_pathname,_pkgname);
			if (object_type(dst_pathname)!=0)
				_files_that_conflict.insert(dst_pathname);
			_files_to_unpack.erase(src_pathname);
		}
		else if (_packages_to_unpack.size())
		{
			// Select package.
			_pkgname=*_packages_to_unpack.begin();
			status curstat=_pb.curstat()[_pkgname];
			const status& selstat=_pb.selstat()[_pkgname];
			const status& prevstat=_pb.prevstat()[_pkgname];

			// Mark package as half-unpacked (but do not commit until
			// pre-remove and pre-unpack phases have been completed).
			curstat.state(status::state_half_unpacked);
			curstat.version(selstat.version());
			curstat.flag(status::flag_auto,selstat.flag(status::flag_auto));
			_pb.curstat().insert(_pkgname,curstat);

			// Check whether standards-version can be processed.
			binary_control_table::key_type key(_pkgname,selstat.version());
			const control& ctrl=_pb.control()[key];
			if (!can_process(ctrl.standards_version()))
				_packages_cannot_process.insert(_pkgname);

			// Open zip file.
			string pathname=_pb.cache_pathname(_pkgname,selstat.version());
			delete _zf;
			_zf=new zipfile(pathname);

			// Build manifest from zip file (excluding package control file).
			std::set<string> mf;
			build_manifest(mf,*_zf,&_bytes_total_unpack);
			mf.erase(ctrl_src_pathname);

			// Copy manifest to list of files to be unpacked.
			_files_to_unpack=mf;
			_files_total_unpack+=mf.size()+1;
			bool overwrite_removed=prevstat.state()==status::state_removed;
			if (overwrite_removed) _files_total_remove+=1;

			// Progress to next package.
			_packages_pre_unpacked.insert(_pkgname);
			_packages_to_unpack.erase(_pkgname);
		}
		else
		{
			// Progress to next state.
			delete _zf;
			_zf=0;
			_state=state_pre_remove;
		}
		break;
	case state_pre_remove:
		if (_packages_to_remove.size())
		{
			// Select package.
			_pkgname=*_packages_to_remove.begin();
			status curstat=_pb.curstat()[_pkgname];
			const status& prevstat=_pb.prevstat()[_pkgname];

			// Mark package as half-unpacked (but do not commit until
			// pre-remove and pre-unpack phases have been completed).
			curstat.state(status::state_half_unpacked);
			_pb.curstat().insert(_pkgname,curstat);

			// Check whether package format is supported.
			binary_control_table::key_type key(_pkgname,curstat.version());
			const control& ctrl=_pb.control()[key];
			if (!can_process(ctrl.standards_version()))
				_packages_cannot_process.insert(_pkgname);

			// Read manifest from package info directory.
			std::set<string> mf;
			read_manifest(mf,_pkgname);

			// Add manifest to list of files to remove, provided that
			// the files in question do currently exist.
			// Remove manifest from list of files that conflict.
			for (std::set<string>::const_iterator i=mf.begin();i!=mf.end();++i)
			{
				string src_pathname=*i;
				string dst_pathname=_pb.paths()(src_pathname,_pkgname);
				_files_that_conflict.erase(dst_pathname);
				if (object_type(dst_pathname))
				{
					_files_to_remove.insert(dst_pathname);
					_files_total_remove+=1;
				}
			}
			_files_total_remove+=1;

			// Progress to next package.
			_packages_being_removed.insert(_pkgname);
			_packages_to_remove.erase(_pkgname);
		}
		else
		{
			// If there are packages that cannot be processed
			// then the installation fails.
			if (_packages_cannot_process.size())
				throw cannot_process();

			// If there are files that conflict, and which are not
			// scheduled for removal, then the installation fails.
			if (_files_that_conflict.size())
				throw file_conflict();

			// Commit package status changes.  All packages to be
			// removed or unpacked should by now have been marked
			// as half-unpacked.
			_pb.curstat().commit();

			// Progress to next state.
			_state=state_unpack;
			_files_total=(_files_total_unpack+_files_total_remove)*2;
			_bytes_total=_bytes_total_unpack;
		}
		break;
	case state_unpack:
		if (_files_to_unpack.size())
		{
			// Unpack file to temporary location.
			string src_pathname=*_files_to_unpack.begin();
			string dst_pathname=_pb.paths()(src_pathname,_pkgname);
			// Make a note that we're trying to unpack this file,
			// so can revert it later if necessary
			unpack_file(src_pathname,dst_pathname);
			_files_being_unpacked.insert(dst_pathname);
			_files_to_unpack.erase(src_pathname);
		}
		else if (_packages_pre_unpacked.size())
		{
			// Select package.
			_pkgname=*_packages_pre_unpacked.begin();
			const status& selstat=_pb.selstat()[_pkgname];
			const status& prevstat=_pb.prevstat()[_pkgname];

			// Open zip file.
			string pathname=_pb.cache_pathname(_pkgname,selstat.version());
			delete _zf;
			_zf=new zipfile(pathname);

			// Unpack control file.
			string ctrl_dst_pathname=_pb.paths()(ctrl_src_pathname,_pkgname);
			bool overwrite=prevstat.state()>=status::state_removed;
			unpack_file(ctrl_src_pathname,ctrl_dst_pathname);
			replace_file(ctrl_dst_pathname,overwrite);

			// Build manifest from zip file (excluding package control file).
			std::set<string> mf;
			build_manifest(mf,*_zf);
			mf.erase(ctrl_src_pathname);

			// Copy manifest to list of files to be unpacked.
			_files_to_unpack=mf;

			// Merge with manifest in package info directory.
			read_manifest(mf,_pkgname);
			prepare_manifest(mf,_pkgname);
			activate_manifest(_pkgname);

			// Prepeate final manifest (to be activated later).
			prepare_manifest(_files_to_unpack,_pkgname);

			// Progress to next package.
			_packages_being_unpacked.insert(_pkgname);
			_packages_pre_unpacked.erase(_pkgname);
		}
		else
		{
			// Progress to next state.
			_ad("");
			delete _zf;
			_zf=0;
			_state=state_replace;
		}
		break;
	case state_replace:
		if (_files_being_unpacked.size())
		{
			// If the unpacked file is expected to be a replacement for
			// an existing file then backup the existing file, otherwise
			// throw an exception if an existing file is found.
			// Move the unpacked file to its final destination.
			string dst_pathname=*_files_being_unpacked.begin();
			bool overwrite=_files_to_remove.find(dst_pathname)!=
				_files_to_remove.end();
			replace_file(dst_pathname,overwrite);
			if (overwrite)
			{
				_files_being_removed.insert(dst_pathname);
				_files_to_remove.erase(dst_pathname);
			}

			// Progress to next file.
			_files_unpacked.insert(dst_pathname);
			_files_being_unpacked.erase(dst_pathname);
		}
		else
		{
			// Progress to next state.
			_ad("");
			_state=state_remove;
		}
		break;
	case state_remove:
		if (_files_to_remove.size())
		{
			// Remove backup file.
			string dst_pathname=*_files_to_remove.begin();
			remove_file(dst_pathname);

			// Progress to next file.
			_files_being_removed.insert(dst_pathname);
			_files_to_remove.erase(dst_pathname);
		}
		else
		{
			// Progress to next state.
			// This is the point of no return.
			_ad("");
			_state=state_post_remove;
		}
		break;
	case state_post_remove:
		if (_files_being_removed.size())
		{
			// Delete backup of file being removed.
			string dst_pathname=*_files_being_removed.begin();
			remove_backup(dst_pathname);

			// Progress to next file.
			_files_removed.insert(dst_pathname);
			_files_being_removed.erase(dst_pathname);
		}
		else if (_packages_being_removed.size())
		{
			// Select package.
			_pkgname=*_packages_being_removed.begin();
			status curstat=_pb.curstat()[_pkgname];

			// If package is not also being unpacked:
			if (_packages_being_unpacked.find(_pkgname)==
				_packages_being_unpacked.end())
			{
				// Remove manifest.
				remove_manifest(_pkgname);

				// Remove control file.
				string ctrl_dst_pathname=
					_pb.paths()(ctrl_src_pathname,_pkgname);
				remove_file(ctrl_dst_pathname);
				remove_backup(ctrl_dst_pathname);

				// Mark package as removed.
				curstat.state(status::state_removed);
				curstat.flag(status::flag_auto,false);
				_pb.curstat().insert(_pkgname,curstat);
			}

			// Progress to next package.
			_packages_removed.insert(_pkgname);
			_packages_being_removed.erase(_pkgname);
		}
		else
		{
			// Progress to next state.
			_ad("");
			_state=state_post_unpack;
		}
		break;
	case state_post_unpack:
		if (_packages_being_unpacked.size())
		{
			// Select package.
			_pkgname=*_packages_being_unpacked.begin();
			status curstat=_pb.curstat()[_pkgname];
			const status& prevstat=_pb.prevstat()[_pkgname];

			// Remove backup of control file.
			string ctrl_dst_pathname=
				_pb.paths()(ctrl_src_pathname,_pkgname);
			bool overwrite=prevstat.state()>=status::state_removed;
			if (overwrite) remove_backup(ctrl_dst_pathname);

			// Activate manifest.
			activate_manifest(_pkgname);

			// Mark package as unpacked.
			curstat.state(status::state_unpacked);
			_pb.curstat().insert(_pkgname,curstat);

			// Progress to next package.
			_packages_unpacked.insert(_pkgname);
			_packages_being_unpacked.erase(_pkgname);
		}
		else
		{
			// Commit package status changes.  All packages to be
			// removed or unpacked should by now have been marked
			// as state_removed or state_unpacked respectively.
			_pb.curstat().commit();

			// Progress to next state.
			_state=state_done;
		}
		break;
	case state_done:
		// Unpack operation complete: do nothing.
		break;
	case state_unwind_replace:
		if (_files_unpacked.size())
		{
			// Delete file.  If it was a replacement for an existing file,
			// restore the original from its backup.
			string dst_pathname=*_files_unpacked.begin();
			bool overwrite=_files_being_removed.find(dst_pathname)!=
				_files_being_removed.end();
			unwind_replace_file(dst_pathname,overwrite);
			if (overwrite)
				_files_being_removed.erase(dst_pathname);
			_files_unpacked.erase(dst_pathname);
		}
		else
		{
			// Progress to next state.
			_ad("");
			_state=state_unwind_remove;
		}
		break;
	case state_unwind_remove:
		if (_files_being_removed.size())
		{
			// Restore file from backup.
			string dst_pathname=*_files_being_removed.begin();
			unwind_remove_file(dst_pathname);
			_files_being_removed.erase(dst_pathname);
		}
		else
		{
			// Progress to next state.
			_ad("");
			_state=state_unwind_unpack;
		}
		break;
	case state_unwind_unpack:
		if (_files_being_unpacked.size())
		{
			// Delete temporary file.
			string dst_pathname=*_files_being_unpacked.begin();
			unwind_unpack_file(dst_pathname);
			_files_being_unpacked.erase(dst_pathname);
		}
		else if (_packages_being_unpacked.size())
		{
			// Select package.
			_pkgname=*_packages_being_unpacked.begin();
			const status& prevstat=_pb.prevstat()[_pkgname];

			// Restore control file.
			string ctrl_dst_pathname=
				_pb.paths()(ctrl_src_pathname,_pkgname);
			bool overwrite=prevstat.state()>=status::state_removed;
			unwind_replace_file(ctrl_dst_pathname,overwrite);

			// Progress to next package.
			_packages_pre_unpacked.insert(_pkgname);
			_packages_being_unpacked.erase(_pkgname);
		}
		else
		{
			// Progress to next state.
			_ad("");
			_state=state_unwind_pre_remove;
		}
		break;
	case state_unwind_pre_remove:
		if (_packages_being_removed.size())
		{
			// Select package.
			_pkgname=*_packages_being_removed.begin();
			status curstat=_pb.curstat()[_pkgname];
			const status& prevstat=_pb.prevstat()[_pkgname];

			// If package was previously unpacked or better then mark
			// as unpacked (but do not commit until unwind-pre-remove
			// and unwind-pre-unpack phases have been completed.)
			if (prevstat.state()>=status::state_unpacked)
			{
				curstat.state(status::state_unpacked);
				curstat.version(prevstat.version());
				curstat.flag(status::flag_auto,
					prevstat.flag(status::flag_auto));
				_pb.curstat().insert(_pkgname,curstat);
			}

			// Progress to next package.
			_packages_being_removed.erase(_pkgname);
		}
		else
		{
			// Progress to next state.
			_state=state_unwind_pre_unpack;
		}
		break;
	case state_unwind_pre_unpack:
		if (_packages_pre_unpacked.size())
		{
			// Select package.
			_pkgname=*_packages_pre_unpacked.begin();
			status curstat=_pb.curstat()[_pkgname];
			const status& prevstat=_pb.prevstat()[_pkgname];

			// If package was previously removed or purged then mark
			// as removed (but do not commit until unwind-pre-remove
			// and unwind-pre-unpack phases have been completed.)
			if (prevstat.state()<=status::state_removed)
			{
				curstat.state(status::state_removed);
				curstat.flag(status::flag_auto,false);
				_pb.curstat().insert(_pkgname,curstat);
			}

			// Progress to next package.
			_packages_pre_unpacked.erase(_pkgname);
		}
		else
		{
			// Commit package status changes.  All packages to be
			// removed or unpacked should by now have been marked
			// as half-unpacked.
			_pb.curstat().commit();

			// Progress to next state.
			_state=state_fail;
		}
		break;
	case state_fail:
		// Unpack operation has failed: do nothing.
		break;
	}
}

void unpack::read_manifest(std::set<string>& mf,const string& pkgname)
{
	string prefix=_pb.info_pathname(pkgname);
	string dst_pathname=prefix+string(".")+mf_dst_filename;
	string bak_pathname=prefix+string(".")+mf_bak_filename;

	std::ifstream dst_in(dst_pathname.c_str());
	dst_in.peek();
	while (dst_in&&!dst_in.eof())
	{
		string line;
		getline(dst_in,line);
		if (line.length()) mf.insert(line);
		dst_in.peek();
	}

	std::ifstream bak_in(bak_pathname.c_str());
	bak_in.peek();
	while (bak_in&&!bak_in.eof())
	{
		string line;
		getline(bak_in,line);
		if (line.length()) mf.insert(line);
		bak_in.peek();
	}
}

void unpack::build_manifest(std::set<string>& mf,zipfile& zf,size_type* usize)
{
	for (unsigned int i=0;i!=zf.size();++i)
	{
		string pathname=zf[i].pathname();
		if (pathname.size()&&(pathname[pathname.length()-1]!='/'))
		{
			mf.insert(zip_to_src(zf[i].pathname()));
			if (usize) *usize+=zf[i].usize();
		}
	}
}

void unpack::prepare_manifest(std::set<string>& mf,const string& pkgname)
{
	string prefix=_pb.info_pathname(pkgname);
	string tmp_pathname=prefix+string(".")+mf_tmp_filename;
	_ad(tmp_pathname);
	std::ofstream out(tmp_pathname.c_str());
	for (std::set<string>::const_iterator i=mf.begin();i!=mf.end();++i)
		out << *i << std::endl;
}

void unpack::activate_manifest(const string& pkgname)
{
	string prefix=_pb.info_pathname(pkgname);
	string dst_pathname=prefix+string(".")+mf_dst_filename;
	string tmp_pathname=prefix+string(".")+mf_tmp_filename;
	string bak_pathname=prefix+string(".")+mf_bak_filename;
	_ad(dst_pathname);

	// Backup existing manifest file if it exists.
	if (object_type(dst_pathname)!=0)
	{
		force_move(dst_pathname,bak_pathname,true);
	}

	// Move new manifest file to destination.
	force_move(tmp_pathname,dst_pathname,false);

	// Delete backup.
	force_delete(bak_pathname);
}

void unpack::remove_manifest(const string& pkgname)
{
	string prefix=_pb.info_pathname(pkgname);
	string dst_pathname=prefix+string(".")+mf_dst_filename;
	string tmp_pathname=prefix+string(".")+mf_tmp_filename;
	string bak_pathname=prefix+string(".")+mf_bak_filename;

	_ad(dst_pathname);
	force_delete(dst_pathname);
	force_delete(tmp_pathname);
	force_delete(bak_pathname);
}

void unpack::unpack_file(const string& src_pathname,const string& dst_pathname)
{
	//std::cout << "unpack::unpack_file " << src_pathname << " to " << dst_pathname << std::endl;
	if (dst_pathname.size())
	{
		string zip_pathname=src_to_zip(src_pathname);
		string tmp_pathname=dst_to_tmp(dst_pathname);

		_ad(tmp_pathname);
		_zf->extract(zip_pathname,tmp_pathname);

		const zipfile::file_info* finfo=_zf->find(zip_pathname);
		if (!finfo) throw file_info_not_found();
		if (const zipfile::riscos_info* rinfo=
			finfo->find_extra<zipfile::riscos_info>())
		{
			write_file_info(tmp_pathname,rinfo->loadaddr(),
				rinfo->execaddr(),rinfo->attr());
		}

		_bytes_done+=finfo->usize();
		_files_done+=1;
	}
}

void unpack::replace_file(const string& dst_pathname,bool overwrite)
{
	//std::cout << "unpack::replace_file " << dst_pathname << " overwrite="<<overwrite << std::endl;
	if (dst_pathname.size())
	{
		string tmp_pathname=dst_to_tmp(dst_pathname);
		string bak_pathname=dst_to_bak(dst_pathname);

		// Force removal of backup pathname.
		// From this point on, if the backup pathname exists
		// then it is a usable backup.
		_ad(bak_pathname);
		//std::cout << "unpack::replace_file: delete " << bak_pathname << std::endl;
		force_delete(bak_pathname);

		if (overwrite)
		{
			// If overwrite enabled then attempt to make backup of
			// existing file, but do not report an error if the
			// file does not exist.
			try
			{
				//std::cout << "unpack::replace_file: try move " << dst_pathname << " to "<< bak_pathname << std::endl;
				force_move(dst_pathname,bak_pathname);
			}
			catch (...) {}
			_files_done+=1;
		}

		// Move file regardless of file attributes, but not regardless
		// of whether destination is present.
		_ad(tmp_pathname);
		//std::cout << "unpack::replace_file: force move "<<tmp_pathname << " to "<< dst_pathname << std::endl;
		force_move(tmp_pathname,dst_pathname);
		_files_done+=1;
	}
}

void unpack::remove_file(const string& dst_pathname)
{
	//std::cout << "unpack::remove_file " << dst_pathname << std::endl;
	if (dst_pathname.size())
	{
		string bak_pathname=dst_to_bak(dst_pathname);

		// Force removal of backup pathname.
		// From this point on, if the backup pathname exists
		// then it is a usable backup.
		_ad(bak_pathname);
		force_delete(bak_pathname);

		// Attempt to make backup of existing file, but do not report
		// and error if the file does not exist.
		try
		{
			force_move(dst_pathname,bak_pathname);
		}
		catch (...) {}
		//catch (...) {std::cout << "unpack::remove_file: backup failed" << std::endl;}
		_files_done+=1;
		//std::cout << "unpack::remove_file: done" << std::endl;
	}
}

void unpack::remove_backup(const string& dst_pathname)
{
	//std::cout << "unpack::remove_backup " << dst_pathname << std::endl;
	if (dst_pathname.size())
	{
		string bak_pathname=dst_to_bak(dst_pathname);
		_ad(bak_pathname);
		try
		{
			force_delete(bak_pathname);
		}
		catch (...) {}
		_files_done+=1;
	}
}

void unpack::unwind_remove_file(const string& dst_pathname)
{
	//std::cout << "unpack::unwind_remove_file "<< dst_pathname << std::endl;
	if (dst_pathname.size())
	{
		string bak_pathname=dst_to_bak(dst_pathname);
		_files_done-=1;
		_ad(bak_pathname);
		try
		{
			force_move(bak_pathname,dst_pathname);
		}
		catch (...) {}
	}
}

void unpack::unwind_replace_file(const string& dst_pathname,bool overwrite)
{
	//std::cout << "unpack::unwind_replace_file " << dst_pathname << " overwrite="<<overwrite << std::endl;
	if (dst_pathname.size())
	{
		string bak_pathname=dst_to_bak(dst_pathname);

		_bytes_done-=object_length(dst_pathname);
		_files_done-=2;

		if (overwrite)
		{
			_files_done-=1;
			_ad(bak_pathname);
			
			// try to unwind, but if the file isn't there we can't restore it
			// eg if Control file never got written due to an error
			if (object_type(bak_pathname) != 0)
				force_move(bak_pathname,dst_pathname,true);
		}
		else
		{
			_ad(dst_pathname);
			force_delete(dst_pathname);
		}
	}
}

void unpack::unwind_unpack_file(const string& dst_pathname)
{
	//std::cout << "unpack::unwind_unpack_file " << dst_pathname << std::endl;
	if (dst_pathname.size())
	{
		string tmp_pathname=dst_to_tmp(dst_pathname);
		_bytes_done-=object_length(tmp_pathname);
		_files_done-=1;
		_ad(tmp_pathname);
		force_delete(tmp_pathname);
	}
}

/* currently the only reason packages cannot be processed is a standards-version mismatch, so make the error descriptive */
unpack::cannot_process::cannot_process():
	runtime_error("A newer version of the package manager is required to install a package. Try finding PackMan in the package list, click the upgrade button, quit and restart it, and try again")
{}

unpack::file_conflict::file_conflict():
	runtime_error("conflict with existing file(s)")
{}

unpack::file_info_not_found::file_info_not_found():
	runtime_error("file information record not found")
{}

}; /* namespace pkg */
