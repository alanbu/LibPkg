// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include "libpkg/filesystem.h"
#include "libpkg/uri.h"
#include "libpkg/version.h"
#include "libpkg/status.h"
#include "libpkg/binary_control.h"
#include "libpkg/pkgbase.h"
#include "libpkg/download.h"
#include "libpkg/update.h"

namespace pkg {

update::update(pkgbase& pb):
	_pb(pb),
	_state(state_srclist),
	_dload(0),
	_out(0),
	_bytes_done(0),
	_bytes_total(npos)
{}

update::~update()
{}

void update::poll()
{
	try
	{
		_poll();
	}
	catch (exception& ex)
	{
		_message=ex.what();
		delete _dload;
		_dload=0;
		delete _out;
		_out=0;
		_state=state_fail;
	}
}

void update::_poll()
{
	switch (_state)
	{
	case state_srclist:
		// Re-read sources list from disc.
		_pb.sources().update();

		// For each source, create an entry in the progress table, and
		// measure the length of the most recent download if there was one.
		for (source_table::const_iterator i=_pb.sources().begin();
			i!=_pb.sources().end();++i)
		{
			_url=*i;
			_progress_table[_url];
			string pathname=_pb.list_pathname(_url);
			unsigned int objtype=object_type(pathname);
			if (objtype)
				_progress_table[_url].bytes_prev=object_length(pathname);
			_sources_to_download.insert(_url);
		}

		// Switch to state_download.
		_state=state_download;
		break;
	case state_download:
		if (_dload)
		{
			// Monitor download in progress.
			update_progress();
			switch (_dload->state())
			{
			case download::state_download:
				// If download in progress then do nothing.
				break;
			case download::state_done:
				// If download complete then move to next source.
				delete _dload;
				_dload=0;
				_sources_to_build.insert(_url);
				_sources_to_download.erase(_url);
				break;
			case download::state_fail:
				// If download failed then update failed too.
				_message=_dload->message();
				delete _dload;
				_dload=0;
				_state=state_fail;
				break;
			}
		}
		else if (_sources_to_download.size())
		{
			// If there are sources awaiting download then begin
			// downloading the first.
			_url=*_sources_to_download.begin();
			string pathname=_pb.list_pathname(_url);
			_dload=new download(_url,pathname);
		}
		else
		{
			// If there are no sources awaiting download then
			// switch to state_build_sources.
			_out=new ofstream(_pb.available_pathname().c_str());
			_state=state_build_sources;
		}
		break;
	case state_build_sources:
		if (_sources_to_build.size())
		{
			// Select next source.
			_url=*_sources_to_build.begin();
			string pathname=_pb.list_pathname(_url);
			ifstream in(pathname.c_str());

			while (in&&!in.eof())
			{
				// Read control record from source.
				binary_control ctrl;
				in >> ctrl;

				// Convert relative URL to absolute.
				if (ctrl.find("URL")!=ctrl.end())
				{
					uri base_url(_url);
					uri rel_url(ctrl["URL"]);
					uri abs_url=base_url+rel_url;
					ctrl["URL"]=abs_url;
				}

				// Extract package name and version.
				string pkgname=ctrl.pkgname();
				version pkgvrsn=ctrl.version();
				binary_control_table::key_type key(pkgname,pkgvrsn);
				if (_packages_written.find(key)==_packages_written.end())
				{
					// If not already written then write to available list.
					if (_packages_written.size()) (*_out) << endl;
					(*_out) << ctrl;
					_packages_written.insert(key);
				}

				// Absorb newlines between packages.
				while (in.peek()=='\n') in.get();
			}
			_sources_to_build.erase(_url);
		}
		else
		{
			// If there are no more sources waiting to be built
			// then switch to state_build_local.
			_state=state_build_local;
		}
		break;
	case state_build_local:
		{
			status_table& curstat=_pb.curstat();
			for (status_table::const_iterator i=curstat.begin();
				i!=curstat.end();++i)
			{
				string pkgname=i->first;
				string pathname=_pb.info_pathname(pkgname)+string(".Control");
				ifstream in(pathname.c_str());
				binary_control ctrl;
				in >> ctrl;

				// Extract package version.
				version pkgvrsn=ctrl.version();
				binary_control_table::key_type key(pkgname,pkgvrsn);
				if (_packages_written.find(key)==_packages_written.end())
				{
					// If not already written then write to available list.
					if (_packages_written.size()) (*_out) << endl;
					(*_out) << ctrl;
					_packages_written.insert(key);
				}
			}

			// Close output stream, update package database
			// from available file, and switch to state_done.
			delete _out;
			_out=0;
			_pb.control().update();
			_state=state_done;
		}
		break;
	case state_done:
		break;
	case state_fail:
		break;
	}
}

void update::update_progress()
{
	// If download active then update progress for that source.
	if (_dload)
	{
		progress& pr=_progress_table[_url];
		pr.bytes_done=_dload->bytes_done();
		pr.bytes_total=_dload->bytes_total();
	}

	// Sum progress over all sources.  Calculate number of sources (count)
	// and number for which a total or estimated total is available (known).
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
			// If total is available for this source then use it.
			_bytes_total+=i->second.bytes_total;
			++known;
		}
		else if (i->second.bytes_prev!=npos)
		{
			// Otherwise, if a total is available from a previous download
			// then use that instead.
			_bytes_total+=i->second.bytes_prev;
			++known;
		}
		++count;
	}

	// Use linear extrapolation to estimate sources for which no
	// information is available (provided there is at least one total
	// or estimated total from which to extrapolate).
	if (known) _bytes_total+=(_bytes_total*(count-known))/known;
}

update::progress::progress():
	bytes_done(0),
	bytes_total(npos),
	bytes_prev(npos)
{}

}; /* namespace pkg */
