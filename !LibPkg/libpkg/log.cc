// This file is part of LibPkg.
// Copyright � 2003-2005 Graham Shaw.
// Copyright � 2013 Alan Buckley
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include "log.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace pkg {

	// Text strings for errors
	const char *error_text[] =
	{
		"Uninitialised log entry used",
		"Exception in update package database from source lists: %0",
		"Failed to download source from '%0', error: %1",
		"Failed to insert package '%0' into cache, error: %1",
		"Failed to download package '%0', error: %1",
		"Failed to unpack the packages, error: %0",
		"Error during unpacking '%0'",
		"Failed to update paths for the components, error: %0",
		"Failed to rollback paths after an error, rollback error: %0",
		"Failed to copy post remove trigger '%0'"
	};
	const char *warning_text[] =
	{
		"Missing log entry: ", // Special entry to trap undefined log text messages
		"Unable to parse components for removal for package '%0', error: %1",
		"Failed to update '%0 boot options', error: %1",
		"Component '%0' has not been installed'",
		"Failed to Filer_Boot '%0', error: %1",
		"Failed to Filer_Run '%0', error: %1",
		"Failed to AddApp '%0', error: %1",
		"Failed to mark component updates as done",
		"Failed to update database to reflect existing module, error: %0",
		"Package front end does not support triggers '%0' trigger for '%1' ignored",
		"Post remove trigger failed for package '%0', error: '%1'",
		"Post install trigger failed for package '%0', error: '%1'"
	};

	const char *trace_text[] =
	{
		"Trace: %0",
		"Trace: %0 %1"
	};
	const char *info_text[] =
	{
		"Reading list of sources from disc",
		"Downloading source lists",
		"Downloading source list from '%0'",
		"Source '%0' downloaded",
		"All sources downloaded",
		"Adding packages to available list from '%0'",
		"Packages from downloaded sources added to available list",
		"Adding local packages",
		"Updating package database",
		"Package database update completed",
		"Started processing changes to packages",
		"Preprocessing package '%0' version '%1'",
		"Using cached version of package '%0'",
		"Downloading package '%0' from '%1'",
		"Package '%0' downloaded",
		"Unpacking packages",
		"Packages unpacked",
		"Package '%0' marked as installed",
		"Packages status saved",
		"Package '%0' marked as purged",
		"Updating system variables",
		"System variables updated",
		"Updating sprites",
		"Sprites updated",
		"Package changes completed",
		"Preparing '%0' for unpacking",
		"Preparing '%0' for removal",
		"Opening package '%0' and reading manifest",
		"Unpacking files for '%0'",
		"Replacing files",
		"Removing files",
		"Package '%0' files removed",
		"Package '%0' files unpacked",
		"Unpacking/removal of files completed",
		"Restoring replaced files",
		"Restoring removed files",
		"Removing unpacked new files",
		"Restoring control file for '%0'",
		"Restoring status of previously installed package '%0'",
		"Restoring status of previously removed package '%0'",
		"Unwinding from failed unpack completed",
		"Updating component paths",
		"Marking path '%0' from package '%1' for removal from boot options",
		"Updating logical path '%0' to '%1'",
		"Component paths updated",
		"Updating boot option files",
		"Boot option files updated",
		"Booting files",
		"Booting file '%0'",
		"Running files",
		"Running file '%0'",
		"Adding files to apps",
		"Adding file '%0' to apps",
		"The files for the packages have been installed correctly, but",
		"the following warnings occurred during configuration.",
		"Module version check for '%0' version '%1'",
		"Using existing module to fulfil installation of package '%0'",
		"Replacing existing module with packaged version from package '%0'",
		"Updating database for existing module in package '%0'",
		"Unwinding files for existing module in package '%0'",
		"Checking for post install triggers",
		"Running %0 trigger for package '%1'",
		"Trigger output: %0",
		"Running pre-remove triggers",
		"Running pre-install triggers",
		"Preserving post-remove triggers",
		"Unwinding pre-install triggers",
		"Unwinding pre-remove triggers",
		"Removing copied post-remove triggers",
		"Running post-remove triggers",
		"Running post-install triggers",
		"Deleting shared variable '%0'",
		"Package environment '%0' with OS dependency '%1'"
	};

	// Array putting it all together
	struct log_text_item
	{
		int num_entries;
		const char **text;
	} log_text[] =
	{
		{ sizeof(error_text) / sizeof(char *), error_text },
		{ sizeof(warning_text) / sizeof(char *), warning_text },
		{ sizeof(trace_text) / sizeof(char *), trace_text },
		{ sizeof(info_text) / sizeof(char *), info_text },
	};

	/**
	* Make a copy of a string if it's not null
	*
	* @param str string to copy
	* @returns string copy or 0 if str is 0
	*/
	inline char *copystr(const char *str) {
		if (!str) return 0;
		char *newstr = new char[strlen(str) + 1];
		std::strcpy(newstr, str);
		return newstr;
	}

	log_entry::log_entry() : _code(LOG_ERROR_UNINITIALISED), _when(0), _param1(0), _param2(0)
	{
	};

	log_entry::log_entry(const log_entry &other) : _param1(0), _param2(0)
	{
		_code = other._code;
		_when = other._when;
		_param1 = copystr(other._param1);
		_param2 = copystr(other._param2);
	}

	log_entry::~log_entry()
	{
		delete[] _param1;
		delete[] _param2;
	}

	log_entry &log_entry::operator=(const log_entry &other)
	{
		_code = other._code;
		_when = other._when;
		_param1 = copystr(other._param1);
		_param2 = copystr(other._param2);
		return *this;
	}

	log_entry::log_entry(LogCode code, const char *param1 /*= 0*/, const char *param2 /*= 0*/) : _code(code), _param1(0), _param2(0)
	{
		_code = code;
		std::time_t now;
		std::time(&now);
		struct std::tm *tm_now = std::localtime(&now);
		_when = tm_now->tm_sec + tm_now->tm_min * 60 + tm_now->tm_hour * 60 * 24;
		_param1 = copystr(param1);
		_param2 = copystr(param2);
	}

	std::string log_entry::when_text() const
	{
		int sec = _when % 60;
		int min = (_when / 60) % 60;
		int hour = _when / (60 * 24);
		std::ostringstream ss;

		ss << std::setfill('0') << std::setw(2) << hour
			<< ':' << std::setw(2) << min
			<< ':' << std::setw(2) << sec;

		return ss.str();
	}


	std::string log_entry::text() const
	{
		std::string text;
		int type_idx = type();
		int sub_code_idx = sub_code();
		if (type_idx >= int(sizeof(log_text) / sizeof(log_text_item))
			|| sub_code_idx > log_text[type_idx].num_entries)
		{
			std::ostringstream ss;
			ss << warning_text << _code;
			text = ss.str();
		}
		else
		{
			text = log_text[type_idx].text[sub_code_idx];
			// Replace arguments
			std::string::size_type pc_pos = 0;
			while ((pc_pos = text.find('%', pc_pos)) != std::string::npos)
			{
				if (pc_pos < text.size() - 1)
				{
					switch (text[pc_pos + 1])
					{
					case '0':
						if (_param1)
						{
							text.replace(pc_pos, 2, _param1);
							pc_pos += std::strlen(_param1);
						}
						break;
					case '1':
						if (_param2)
						{
							text.replace(pc_pos, 2, _param2);
							pc_pos += std::strlen(_param2);
						}
						break;
					case '%':
						pc_pos++;
						break;
					}
				}
				pc_pos++;
			}
		}

		return text;
	}

	log::log() : _bad(false)
	{
		_counts[0] = _counts[1] = 0;
	}

	void log::message(LogCode code, const char *param1 /*= 0*/, const char *param2 /*= 0*/)
	{
		try
		{
			_entries.push_back(log_entry(code, param1, param2));
			int type = code >> 15;
			if (type < 2) _counts[type]++;
		}
		catch (...)
		{
			// Never let the log throw an exception
			_bad = true;
		}
	}

	std::ostream &operator<<(std::ostream &stream, const log &olog)
	{
		stream << "Time     Code  Description" << std::endl;
		for (log::const_iterator i = olog._entries.begin(); i != olog._entries.end(); ++i)
		{
			stream << i->when_text()
				<< " " << std::hex << std::setfill('0') << std::setw(5) << i->code()
				<< " " << i->text() << std::endl;
		}
		return stream;
	}
}
