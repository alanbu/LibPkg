// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include <algorithm>
#include <sstream>

#include "libpkg/filesystem.h"
#include "libpkg/version.h"
#include "libpkg/binary_control.h"
#include "libpkg/status.h"
#include "libpkg/pkgbase.h"
#include "libpkg/download.h"
#include "libpkg/unpack.h"
#include "libpkg/commit.h"

namespace pkg {

commit::commit(pkgbase& pb,const set<string>& packages):
	_pb(pb),
	_state(state_pre_download),
	_packages_to_process(packages),
	_dload(0),
	_upack(0),
	_files_done(0),
	_files_total(npos),
	_bytes_done(0),
	_bytes_total(npos)
{
	// Commit selected state to disc.
	_pb.selstat().commit();

	// Make previous package state equal to current state.
	// (This is the state to which the system will try to return
	// if an error occurs during the commit operation.)
	_pb.prevstat().clear();
	_pb.prevstat().insert(_pb.curstat());
}

commit::~commit()
{}

void commit::poll()
{
	switch (_state)
	{
	case state_pre_download:
		if (_packages_to_process.size())
		{
			// Select package.
			_pkgname=*_packages_to_process.begin();
			const status& curstat=_pb.curstat()[_pkgname];
			const status& selstat=_pb.selstat()[_pkgname];

			// Get size of package from control record.
			size_type size=npos;
			binary_control_table::key_type key(_pkgname,selstat.version());
			const binary_control& ctrl=_pb.control()[key];
			control::const_iterator f=ctrl.find("Size");
			if (f!=ctrl.end())
			{
				istringstream in(f->second);
				in >> size;
			}

			// Determine whether a download is required.
			// This is true if the package is to be unpacked,
			// but is not present in the cache.
			bool download_req=unpack_req(curstat,selstat);
			if (download_req)
			{
				string pathname=_pb.cache_pathname(_pkgname,
					selstat.version());
				download_req=(!object_type(pathname))||
					(object_length(pathname)!=size);
			}

			if (download_req)
			{
				// Create an entry in the progress table.
				_progress_table[_pkgname].bytes_ctrl=size;

				// Progress to next package.
				_packages_to_download.insert(_pkgname);
				_packages_to_process.erase(_pkgname);
			}
			else
			{
				// Progress to next package.
				// Since the package does not need to be downloaded,
				// it is listed to be unpacked.
				_packages_to_unpack.insert(_pkgname);
				_packages_to_process.erase(_pkgname);
			}
		}
		else
		{
			// Progress to next state.
			_state=state_download;
		}
		break;
	case state_download:
		if (_dload)
		{
			// Update download progress.
			update_download_progress();

			// Monitor download state.
			switch (_dload->state())
			{
			case download::state_download:
				// If download in progress then do nothing.
				break;
			case download::state_done:
				// If download complete then move to next package.
				delete _dload;
				_dload=0;
				_packages_to_unpack.insert(_pkgname);
				_packages_to_download.erase(_pkgname);
				break;
			case download::state_fail:
				// If download failed then commit failed too.
				_message=_dload->message();
				delete _dload;
				_dload=0;
				_state=state_fail;
				break;
			}
		}
		else if (_packages_to_download.size())
		{
			// Select package.
			_pkgname=*_packages_to_download.begin();
			const status& curstat=_pb.curstat()[_pkgname];
			const status& selstat=_pb.selstat()[_pkgname];

			// Obtain URL and cache pathname then begin download.
			binary_control_table::key_type key(_pkgname,selstat.version());
			const binary_control& ctrl=_pb.control()[key];
			string url=ctrl.url();
			string pathname=_pb.cache_pathname(_pkgname,selstat.version());
			_dload=new download(url,pathname);
		}
		else
		{
			// Progress to next state.
			_state=state_unpack;
			_files_done=0;
			_files_total=npos;
			_bytes_done=0;
			_bytes_total=npos;
		}
		break;
	case state_unpack:
		if (_upack)
		{
			// Update progress.
			_files_done=_upack->files_done();
			_files_total=_upack->files_total();
			_bytes_done=_upack->bytes_done();
			_bytes_total=_upack->bytes_total();

			// Monitor state of unpack operation.
			switch (_upack->state())
			{
			case unpack::state_done:
				// If unpack complete then move to next package.
				delete _upack;
				_upack=0;
				_packages_to_configure.swap(_packages_to_unpack);
				_state=state_configure;
				break;
			case unpack::state_fail:
				// If unpack failed then commit failed too.
				_message=_upack->message();
				delete _upack;
				_upack=0;
				_state=state_fail;
				break;
			default:
				// If unpack operation in progress then do nothing.
				break;
			}
		}
		else
		{
			// Begin unpack operation.
			_upack=new unpack(_pb,_packages_to_unpack);
		}
		break;
	case state_configure:
		if (_packages_to_configure.size())
		{
			// Select package.
			_pkgname=*_packages_to_configure.begin();
			const status& curstat=_pb.curstat()[_pkgname];
			const status& selstat=_pb.selstat()[_pkgname];

			// If configuration required then mark package as installed.
			if (config_req(curstat,selstat)&&!unpack_req(curstat,selstat))
			{
				status st=_pb.curstat()[_pkgname];
				st.state(status::state_installed);
				_pb.curstat().insert(_pkgname,st);
			}

			// Move to next package.
			_packages_to_purge.insert(_pkgname);
			_packages_to_configure.erase(_pkgname);
		}
		else
		{
			// Commit package status changes.
			_pb.curstat().commit();

			// Progress to next state.
			_state=state_purge;
		}
		break;
	case state_purge:
		if (_packages_to_purge.size())
		{
			// Select package.
			_pkgname=*_packages_to_purge.begin();
			const status& curstat=_pb.curstat()[_pkgname];
			const status& selstat=_pb.selstat()[_pkgname];

			// If purge required then mark package as not present.
			if (purge_req(curstat,selstat)&&!remove_req(curstat,selstat))
			{
				status st=_pb.curstat()[_pkgname];
				st.state(status::state_not_present);
				_pb.curstat().insert(_pkgname,st);
			}

			// Move to next package.
			_packages_to_purge.erase(_pkgname);
		}
		else
		{
			// Commit package status changes.
			_pb.curstat().commit();

			// Progress to next state.
			_state=state_done;
		}
		break;
	case state_done:
		// Commit operation complete: do nothing.
		break;
	case state_fail:
		// Commit operation failed: do nothing.
		break;
	}
}

void commit::update_download_progress()
{
	// If download active then update progress for that package.
	if (_dload)
	{
		progress& pr=_progress_table[_pkgname];
		pr.bytes_done=_dload->bytes_done();
		pr.bytes_total=_dload->bytes_total();
	}

	// Sum progress over all packages.  Calculate number of packages (count)
	// and number for which a total total is available (known).
	_bytes_done=0;
	_bytes_total=0;
	unsigned int count=0;
	unsigned int known=0;
	for (map<string,progress>::const_iterator i=_progress_table.begin();
		i!=_progress_table.end();++i)
	{
		_bytes_done+=i->second.bytes_done;
		if (i->second.bytes_total!=npos)
		{
			// If a total has been obtained during download then use it.
			_bytes_total+=i->second.bytes_total;
			++known;
		}
		else if (i->second.bytes_ctrl!=npos)
		{
			// Otherwise, if there is a total in the control record then
			// use that instead.
			_bytes_total+=i->second.bytes_ctrl;
			++known;
		}
		++count;
	}

	// Use linear extrapolation to estimate packages for which no
	// information is available (provided there is at least one total
	// or estimated total from which to extrapolate).
	if (known) _bytes_total+=(_bytes_total*(count-known))/known;
}

commit::progress::progress():
	bytes_done(0),
	bytes_total(npos),
	bytes_ctrl(npos)
{}

}; /* namespace pkg */
