// This file is part of LibPkg.
// Copyright � 2003-2005 Graham Shaw.
// Copyright � 2015 Alan Buckley.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.
//
#ifndef LIBPKG_TRIGGER
#define LIBPKG_TRIGGER

#include <string>
#include <vector>

namespace pkg
{
	class pkgbase;
	class trigger_run;
	class log;

	/**
	 * class to represent a trigger action from a script
	 */
	class trigger
	{
	public:
		enum action_type {pre_remove, pre_install, post_install, post_remove, abort_pre_remove, abort_pre_install};
		enum state_type {state_none, state_running, state_success, state_error};
	private:
		pkgbase &_pb;
		std::string _pkgname;
		action_type _action;
		std::string _old_version;
		std::string _new_version;
		trigger_run *_runner;
		state_type _state;
		std::string _message;
		std::vector<std::string> _paths;
		log *_log;

	public:
		trigger(pkgbase &pb, const std::string &pkgname, trigger::action_type action, const std::string &old_version, const std::string &new_version, trigger_run *runner);

		void log_to(log *log) {_log = log;}
		void run();

		/** Check if the triggers has finished running */
		bool finished() const { return (_state == state_success) || (_state == state_error); }

		const std::string &pkgname() const {return _pkgname;}
		action_type action() const {return _action;}
		state_type state() const {return _state;}
		const std::string &message() const {return _message;}

		void trigger_start_failed(const std::string &reason);
		void trigger_log(const std::string &text);
		void trigger_finished();
	private:
		std::string trigger_path();
		void delete_env_vars();
	};

    /**
	 * Interface to execute a trigger
	 *
	 * Must be provided by the front end to run the trigger scripts
	 */
	class trigger_run
	{
	public:
		virtual ~trigger_run() {}
		/**
		 * Run the given trigger during install/remove/upgrade
		 *
		 * For a wimp program the front end should use a TaskWindow
		 * with 128K memory or equivalent.
		 *
		 * Should callback on the trigger
		 *   trigger_start_failed if the file could not be run/started
		 *   trigger_log to log any output from the trigged
		 *   trigger_finished when the file has finished executing
		 *
		 * @param file_name name of file to run
		 * @param trigger trigger to inform of state of run
		 */
		virtual void run(const std::string &file_name, pkg::trigger *trigger) = 0;
	};
}

#endif
