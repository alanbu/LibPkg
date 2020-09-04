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

#include "libpkg/filesystem.h"
#include "libpkg/uri.h"
#include "libpkg/version.h"
#include "libpkg/status.h"
#include "libpkg/binary_control.h"
#include "libpkg/pkgbase.h"
#include "libpkg/download.h"
#include "libpkg/update.h"
#include "libpkg/log.h"

namespace pkg {

update::update(pkgbase& pb):
	_pb(pb),
	_state(state_srclist),
	_dload(0),
	_out(0),
	_bytes_done(0),
	_bytes_total(npos),
	_log(0)
{}

update::~update()
{}

void update::poll()
{
	try
	{
		_poll();
	}
	catch (std::exception& ex)
	{
		_message=ex.what();
		delete _dload;
		_dload=0;
		delete _out;
		_out=0;
		_state=state_fail;
		if (_log) _log->message(LOG_ERROR_UPDATE_EXCEPTION, _message);
	}
}

void update::_poll()
{
	switch (_state)
	{
	case state_srclist:
		if (_log) _log->message(LOG_INFO_READ_SOURCES);
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
		if (_log) _log->message(LOG_INFO_DOWNLOADING_SOURCES);
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
				if (_log) _log->message(LOG_INFO_DOWNLOADED_SOURCE, _url);
				break;
			case download::state_fail:
				// If download failed then update failed too.
				_message=_dload->message();
				delete _dload;
				_dload=0;
				_state=state_fail;
				if (_log) _log->message(LOG_ERROR_SOURCE_DOWNLOAD_FAILED, _url, _message);
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
			if (_log) _log->message(LOG_INFO_DOWNLOADING_SOURCE, _url);
			#ifdef LOG_DOWNLOAD
			if (_log) _dload->log_to(_log);
			#endif
		}
		else
		{
			// If there are no sources awaiting download then
			// switch to state_build_sources.
			_out=new std::ofstream(_pb.available_pathname().c_str());
			_state=state_build_sources;
			if (_log) _log->message(LOG_INFO_DOWNLOADED_SOURCES);
		}
		break;
	case state_build_sources:
		if (_sources_to_build.size())
		{
			// Select next source.
			_url=*_sources_to_build.begin();
			if (_log) _log->message(LOG_INFO_ADDING_AVAILABLE, _url);
			string pathname=_pb.list_pathname(_url);
			std::ifstream in(pathname.c_str());

			// Absorb any spaces at beginning of file - some package list
			// were including extra linefeeds at the beginning
			while (in && !in.eof() && isspace(in.peek())) in.get();

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
				binary_control_table::key_type key(pkgname,pkgvrsn,ctrl.environment_id());
				if (_packages_written.find(key)==_packages_written.end())
				{
					// If not already written then write to available list.
					if (_packages_written.size()) (*_out) << std::endl;
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
		    if (_log) _log->message(LOG_INFO_AVAILABLE_ADDED);
		}
		break;
	case state_build_local:
		{
		    if (_log) _log->message(LOG_INFO_ADD_LOCAL);
			status_table& curstat=_pb.curstat();
			for (status_table::const_iterator i=curstat.begin();
				i!=curstat.end();++i)
			{
				string pkgname=i->first;
				string pathname=_pb.info_pathname(pkgname)+string(".Control");
				std::ifstream in(pathname.c_str());
				if (in)
				{
					binary_control ctrl;
					in >> ctrl;

					// Extract package version.
					version pkgvrsn=ctrl.version();
					binary_control_table::key_type key(pkgname,pkgvrsn,ctrl.environment_id());
					if (_packages_written.find(key)==_packages_written.end())
					{
						// If not already written then write to available list.
						if (_packages_written.size()) (*_out) << std::endl;
						(*_out) << ctrl;
						_packages_written.insert(key);
					}
				}
			}

		    if (_log) _log->message(LOG_INFO_UPDATING_DATABASE);

			// Close output stream, update package database
			// from available file, and switch to state_done.
			delete _out;
			_out=0;
			_pb.control().update();
			_state=state_done;
		    if (_log) _log->message(LOG_INFO_UPDATE_DONE);
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
	for (std::map<string,progress>::const_iterator i=_progress_table.begin();
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

void update::log_to(log *use_log)
{
	_log = use_log;
}

}; /* namespace pkg */
