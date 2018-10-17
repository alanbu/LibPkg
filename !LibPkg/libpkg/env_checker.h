// This file is part of LibPkg.
// Copyright � 2003-2005 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.
// Created on: 3 Jul 2018
// Author: Alan Buckley

#ifndef LIBPKG_LIBPKG_ENV_CHECKER_H_
#define LIBPKG_LIBPKG_ENV_CHECKER_H_

#include <string>
#include <map>
#include <vector>
#include <set>

namespace pkg {

class env_checker;

/** Type for check */
enum env_check_type
{
	System,  ///< System check
	Module,  ///< Module check
	Unknown, ///< Check unknown to this version of libpkg
	Unset    ///< No check defined on package
};


/** Base class for the environment checking classes */
class env_check
{
public:

	env_check(const std::string &name, const std::string &desc, const std::string &id, env_check_type type, int install_priority);
	virtual ~env_check() {}
	/** The name of the check, should be fairly short */
	const std::string &name() const {return _name;}
	/** A one line description for display purposes only */
	const std::string &description() const {return _description;}
	/** Short id consisting of a single letter optionally followed be a number */
	const std::string &id() const {return _id;}
	/** Automatic detection result */
	bool detected() const {return _detected;}
	/** Software override of status */
	bool available() const {return _available;}
	void available(bool override) {_available = override;}
	/** Type for this check */
	env_check_type type() const {return _type;}
	/** Priority for this check, the higher the more important */
	int install_priority() {return _install_priority;}
private:
	std::string _name;
	std::string _id;
	env_check_type _type;
protected:
	std::string _description;
	bool _detected;
	bool _available;
	int _install_priority;
};

/** Class for packages where the environment has not been set */
class unset_check : public env_check
{
public:
	unset_check() : env_check("unset","Environment not set on the package", "u", Unset, 1)
	{
		_detected = _available = true;
	}
};

/**
 * Class for environment checks not recognised by the current version
 */
class unknown_check : public env_check
{
public:
	unknown_check(const std::string &name, const std::string &id) : env_check(name,"Unknown value, upgrade the package client",id, Unknown, 1)
	{
		_detected = _available = true;
	}
};


/** A class to represent the enviroment a package is designed for.
 *
 * Pointers to this class should be retrieved from the env_checker
 * class.
 */
class pkg_env
{
public:
	/** The full name of this environment */
	const std::string &name() const {return _name;}
	/** Return true if this environment is compatible with the current machine */
	bool available() const {return _available;}

	/** Default install priority to use if the package doesn't specify one */
	int default_install_priority() const {return _install_priority;}
	/** Unique short Id string for use in maps and file caches */
	const std::string &id() const {return _id;}

	/** Main type to describe this package, chosen from the checks in the environment */
	env_check_type type() const {return _type;}

	/** Reset available flag from contained checks */
	void reset_available();

	std::string env_names() const;
	std::string module_names() const;


private:
	friend env_checker;
	pkg_env(const std::string &name, const std::vector<env_check *> &checks);
	~pkg_env() {};

	std::string _name;
	std::string _id;
	std::vector<env_check *> _checks;
	bool _available;
	int _install_priority;
	env_check_type _type;
};

/**
 * Class to help manage the single env_checker instance
 */
class env_checker_ptr
{
public:
	env_checker_ptr(const std::string &module_map_path);
	env_checker_ptr(const env_checker_ptr &other);
	~env_checker_ptr();

	env_checker_ptr &operator=(const env_checker_ptr &other);
	env_checker *operator->();
};

/** Class to check environment and convert a string environment specification into a pkg_env
 *  There is only a single instance of this. Create one or more env_checker_ptr to manager this.
 **/
class env_checker
{
public:
	class watcher;
	friend class watcher;
	friend class env_checker_ptr;

private:
	static env_checker *_instance;
	int _ref_count;
	std::set<env_checker::watcher *> _watchers;
	static unsigned int _next_module_id;
	std::map<std::string, env_check *> _checks;
	std::map<std::string, env_check *> _module_checks;
	env_check *_unset_check;
	pkg_env *_unset_env;
	std::map<std::string, pkg_env *> _environments;
	std::string _module_map_path;
	std::map<std::string, std::string> _module_ids;

public:
	static env_checker *instance() {return _instance;}
	void add_ref() {_ref_count++;}
	void remove_ref();

	pkg_env *package_env(const std::string &env_list, const std::string &os_depends);

	typedef std::map<std::string, env_check *> check_map;

	const check_map &checks() const {return _checks;}
	const check_map &module_checks() const {return _module_checks;}

	std::string get_module_id(const std::string &title);

	bool clear_environment_overrides();
	bool override_environment(const std::set<std::string> &new_env, const std::set<std::string> &new_mods);

private:
	env_checker(const std::string &module_map_path);
	virtual ~env_checker();
	void initialise(const std::string &module_map_path);
	void add_check(env_check *check);
	void read_module_map();
	void write_module_map();
	/** Register a watcher.
	 * This can only be done by the watcher in question (which is then
	 * responsible for deregistering itself before it is destroyed).
	 * @param w the watcher to be registed
	 */
	void register_watcher(watcher& w);

	/** Deregister a watcher.
	 * This can only be done by the watcher in question.
	 * @param w the watcher to be deregistered
	 */
	void deregister_watcher(watcher& w);
	/**
	 * Notify watchers of any changes
	 */
	void notify();
};

/** A mixin class to allow an object to watch the environment checker. */
class env_checker::watcher
{
private:
	/** The set of tables that are being watched. */
	std::set<env_checker*> _env_checkers;
public:
	/** Construct watcher. */
	watcher();

	/** Destroy watcher. */
	virtual ~watcher();

	/** Begin watching environment.
	 * @param e the environment to begin watching
	 */
	void watch(env_checker &e);

	/** Cease watching environment.
	 * @param e the environment to cease watching
	 */
	void unwatch(env_checker & e);

	/** Handle change to environment.
	 * @param e the environment that has changed
	 */
	virtual void handle_change(env_checker & e)=0;
};


} /* namespace lippkg */

#endif /* LIBPKG_LIBPKG_ENV_CHECKER_H_ */
