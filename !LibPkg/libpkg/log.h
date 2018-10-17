// This file is part of LibPkg.
// Copyright � 2003-2005 Graham Shaw.
// Copyright � 2013 Alan Buckley
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef LIBPKG_LOG
#define LIBPKG_LOG

#include <string>
#include <vector>
#include <ostream>

namespace pkg {

	/**
	* An enumeration of all the items that can be logged
	*/
	enum LogCode {
		LOG_ERROR_UNINITIALISED = 0,
		LOG_ERROR_UPDATE_EXCEPTION,
		LOG_ERROR_SOURCE_DOWNLOAD_FAILED,
		LOG_ERROR_CACHE_INSERT,
		LOG_ERROR_PACKAGE_DOWNLOAD_FAILED,
		LOG_ERROR_UNPACK_FAILED,
		LOG_ERROR_UNPACK_EXCEPTION,
		LOG_ERROR_PATHS_COMMIT,
		LOG_ERROR_PATHS_ROLLBACK,
		LOG_ERROR_POST_REMOVE_COPY,
		LOG_WARNING_LOG_TEXT = 0x10000,
		LOG_WARNING_REMOVE_COMPONENT,
		LOG_WARNING_BOOT_OPTIONS_FAILED,
		LOG_WARNING_COMPONENT_NOT_INSTALLED,
		LOG_WARNING_BOOTING_FAILED,
		LOG_WARNING_RUNNING_FAILED,
		LOG_WARNING_ADDING_TO_APPS_FAILED,
		LOG_WARNING_COMPONENT_UPDATE_DONE_FAILED,
		LOG_WARNING_MODULE_PACKAGE_UPDATE_FAILED,
		LOG_WARNING_NO_TRIGGER_RUN,
		LOG_WARNING_POST_REMOVE_TRIGGER_FAILED,
		LOG_WARNING_POST_INSTALL_TRIGGER_FAILED,
		LOG_TRACE = 0x20000,
		LOG_TRACE2,
		LOG_INFO_READ_SOURCES = 0x30000,
		LOG_INFO_DOWNLOADING_SOURCES,
		LOG_INFO_DOWNLOADING_SOURCE,
		LOG_INFO_DOWNLOADED_SOURCE,
		LOG_INFO_DOWNLOADED_SOURCES,
		LOG_INFO_ADDING_AVAILABLE,
		LOG_INFO_AVAILABLE_ADDED,
		LOG_INFO_ADD_LOCAL,
		LOG_INFO_UPDATING_DATABASE,
		LOG_INFO_UPDATE_DONE,
		LOG_INFO_START_COMMIT,
		LOG_INFO_PREPROCESS_PACKAGE,
		LOG_INFO_CACHE_USED,
		LOG_INFO_DOWNLOADING_PACKAGE,
		LOG_INFO_DOWNLOADED_PACKAGE,
		LOG_INFO_UNPACKING,
		LOG_INFO_UNPACKED,
		LOG_INFO_INSTALLED,
		LOG_INFO_STATE_UPDATE,
		LOG_INFO_PURGED,
		LOG_INFO_UPDATING_SYSVARS,
		LOG_INFO_SYSVARS_UPDATED,
		LOG_INFO_UPDATING_SPRITES,
		LOG_INFO_SPRITES_UPDATED,
		LOG_INFO_COMMIT_DONE,
		LOG_INFO_PREUNPACK,
		LOG_INFO_PREREMOVE,
		LOG_INFO_UNPACKING_PACKAGE,
		LOG_INFO_UNPACK_FILES,
		LOG_INFO_UNPACK_REPLACE,
		LOG_INFO_UNPACK_REMOVE,
		LOG_INFO_UNPACK_REMOVED,
		LOG_INFO_UNPACKED_PACKAGE,
		LOG_INFO_UNPACK_DONE,
		LOG_INFO_UNWIND_REPLACED_FILES,
		LOG_INFO_UNWIND_REMOVED,
		LOG_INFO_UNWIND_UNPACK_FILES,
		LOG_INFO_RESTORE_CONTROL,
		LOG_INFO_UNWIND_STATE,
		LOG_INFO_UNWIND_STATE_REMOVED,
		LOG_INFO_UNWIND_DONE,
		LOG_INFO_START_PATHS,
		LOG_INFO_REMOVE_PATH_OPTS,
		LOG_INFO_PATH_CHANGE,
		LOG_INFO_END_PATHS,
		LOG_INFO_UPDATING_BOOT_OPTIONS,
		LOG_INFO_BOOT_OPTIONS_UPDATED,
		LOG_INFO_BOOTING_FILES,
		LOG_INFO_BOOTING,
		LOG_INFO_RUNNING_FILES,
		LOG_INFO_RUNNING,
		LOG_INFO_ADDING_TO_APPS,
		LOG_INFO_ADDING,
		LOG_INFO_WARNING_INTRO1,
		LOG_INFO_WARNING_INTRO2,
		LOG_INFO_MODULE_CHECK,
		LOG_INFO_MODULE_USE,
		LOG_INFO_MODULE_REPLACE,
		LOG_INFO_MODULE_UPDATE,
		LOG_INFO_MODULE_UNWIND,
		LOG_INFO_POST_TRIGGER_CHECK,
		LOG_INFO_TRIGGER_RUN,
		LOG_INFO_TRIGGER_OUTPUT,
		LOG_INFO_PRE_REMOVE_TRIGGERS,
		LOG_INFO_PRE_INSTALL_TRIGGERS,
		LOG_INFO_COPY_POST_REMOVE,
		LOG_INFO_UNWIND_PRE_INSTALL_TRIGGERS,
		LOG_INFO_UNWIND_PRE_REMOVE_TRIGGERS,
		LOG_INFO_REMOVE_POST_REMOVE_TRIGGERS,
		LOG_INFO_POST_REMOVE_TRIGGERS,
		LOG_INFO_POST_INSTALL_TRIGGERS,
		LOG_INFO_DELETE_SHARED_VAR,
		LOG_INFO_PACKAGE_ENV
	};

	/**
	* A class to represent one log entry
	*/
	class log_entry
	{
		/** The code for the log entry */
		LogCode _code;
		/** Time of entry (seconds from midnight) */
		int _when;
		/** First parameter text or null if none */
		char *_param1;
		/** Second parameter text or null if none  */
		char *_param2;

	public:
		/**
		* Construct uninitialised log entry
		*/
		log_entry();

		/**
		* Copy constructor
		*/
		log_entry(const log_entry &other);

		/** Destroy entry freeing memory */
		~log_entry();

		/**
		* Assignment
		*/
		log_entry &operator=(const log_entry &other);

		/**
		* Construct log entry
		*
		* @param code Entry log code
		* @param param1 first parameter or 0 (the default) for none
		* @param param2 second parameter or 0 (the default) for none
		*/
		log_entry(LogCode code, const char *param1 = 0, const char *param2 = 0);

		/**
		* Log entry error code
		*
		* @returns error code
		*/
		int code() const { return _code; }

		/**
		* Log entry type
		*
		* @returns type 0-error, 1-warning, 2-trace, 3-information
		*/
		int type() const { return (_code >> 16); }

		/**
		* Log entry sub code
		*
		* @returns sub code within all codes for the type
		*/
		int sub_code() const { return (_code & 0xFFFF); }

		/**
		* Time since midnight of log entry.
		*
		* The time in the logs will wrap every day
		*/
		int when() const { return _when; }

		/**
		* Time of log entry as text
		*
		* @returns time of log entry in format HH:MM:SS
		*/
		std::string when_text() const;

		/**
		* Description of this log entry
		*
		* @returns text for this log entry
		*/
		std::string text() const;
	};

	/**
	* Class to log actions that occur in LibPkg
	*/
	class log
	{
		/** Entries that have been logged */
		std::vector<log_entry> _entries;
		/** Errors and warnings counts */
		unsigned int _counts[2];
		/** Adding an entry failed */
		bool _bad;

	public:
		/**
		* Construct an empty log
		*/
		log();

		/**
		* The log failed to add one or more items
		*
		* @returns true if add failure
		*/
		bool bad() const { return _bad; }

		/**
		* Add a new entry to the log
		*
		* @param code Entry log code
		* @param param1 first parameter or 0 (the default) for none
		* @param param2 second parameter or 0 (the default) for none
		*/
		void message(LogCode code, const char *param1 = 0, const char *param2 = 0);

		/**
		* Add a new entry to the log from a standard string
		*
		* @param code Entry log code
		* @param param first parameter
		*/
		void message(LogCode code, const std::string &param) { message(code, param.c_str()); }
		/**
		* Add a new entry to the log from two standard strings
		*
		* @param code Entry log code
		* @param param1 first parameter
		* @param param2 second parameter
		*/
		void message(LogCode code, const std::string &param1, const std::string &param2) { message(code, param1.c_str(), param2.c_str()); }

		/** Iterator type for the log entries */
		typedef std::vector<log_entry>::const_iterator const_iterator;
		/** Iterator to first log entry */
		const_iterator begin() const { return _entries.begin(); }
		/** Iterator to last log entry */
		const_iterator end() const { return _entries.end(); }
		/** Total number of entries */
		unsigned int size() const { return _entries.size(); }
		/** Number of errors */
		unsigned int errors() const { return _counts[0]; }
		/** Number of warnings */
		unsigned int warnings() const { return _counts[1]; }

		/** Return entry for a given index */
		const log_entry &operator[](int index) const { return _entries[index]; }
		/** Return entry for a given index */
		const log_entry &entry(int index) const { return _entries[index]; }

		/** Output log as text */
		friend std::ostream &operator<<(std::ostream &stream, const log &olog);
	};

	std::ostream &operator<<(std::ostream &stream, const log &olog);
};

#endif
