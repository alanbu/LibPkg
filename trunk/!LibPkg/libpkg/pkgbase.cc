// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include "libpkg/control.h"
#include "libpkg/pkgbase.h"

namespace {

const char hexchar[]="0123456789ABCDEF";

}; /* anonymous namespace */

namespace pkg {

pkgbase::pkgbase(const string& pathname):
	_pathname(pathname),
	_curstat(pathname+string(".Status")),
	_selstat(pathname+string(".Selected")),
	_control(pathname+string(".Available")),
	_sources("Choices:RiscPkg.Sources"),
	_paths("Choices:RiscPkg.Paths"),
	_changed(false)
{}

pkgbase::~pkgbase()
{}

string pkgbase::cache_pathname(const string& pkgname,const string& version)
{
	string _version=version;
	for (string::iterator i=_version.begin();i!=_version.end();++i)
	{
		if (*i=='.') *i='/';
	}
	return _pathname+string(".Cache.")+pkgname+string("_")+_version;
}

string pkgbase::info_pathname(const string& pkgname)
{
	return _pathname+string(".Info.")+pkgname;
}

string pkgbase::list_pathname(const string& url)
{
	const string lists(".Lists.");
	string::size_type length=_pathname.length()+lists.length();
	for (string::const_iterator i=url.begin();i!=url.end();++i)
	{
		char ch=*i;
		if (isalpha(ch)||(ch=='/')||(ch=='~')||(ch=='.'))
		{
			length+=1;
		}
		else
		{
			length+=3;
		}
	}
	string filename;
	filename.reserve(length);
	filename.append(_pathname);
	filename.append(lists);
	for (string::const_iterator i=url.begin();i!=url.end();++i)
	{
		char ch=*i;
		if (ch=='.')
		{
			filename.push_back('_');
		}
		else if (isalpha(ch)||(ch=='/')||(ch=='~'))
		{
			filename.push_back(ch);
		}
		else
		{
			filename.push_back('=');
			filename.push_back(hexchar[(ch>>4)&0xf]);
			filename.push_back(hexchar[ch&0xf]);
		}
	}
	return filename;
}

string pkgbase::available_pathname()
{
	return _pathname+string(".Available");
}

string pkgbase::sysvars_pathname()
{
	return _pathname+string(".SysVars");
}

string pkgbase::setvars_pathname()
{
	return _pathname+string(".SetVars");
}

bool pkgbase::fix_dependencies(const set<string>& seed)
{
	// Initialise internal flags.
	for (status_table::const_iterator i=_selstat.begin();
		i!=_selstat.end();++i)
	{
		string pkgname=(*i).first;
		status selstat=(*i).second;
		selstat.flag(status::flag_must_remove,false);
		selstat.flag(status::flag_must_install,false);
		selstat.flag(status::flag_must_upgrade,false);
		if (seed.find((*i).first)!=seed.end())
		{
			// Ensure that the state of packages in the seed set
			// cannot be overridden (except to upgrade).
			if (selstat.state()>status::state_removed)
			{
				selstat.flag(status::flag_must_install,true);
			}
			else
			{
				selstat.flag(status::flag_must_remove,true);
			}
		}
		_selstat.insert(pkgname,selstat);
	}

	// Process packages.  Repeat until no change to flags.
	// (This loop will terminate, because the flags can only change
	// in one direction.)
	_changed=true;
	while (_changed)
	{
		_changed=false;
		for (status_table::const_iterator i=_selstat.begin();
			i!=_selstat.end();++i)
		{
			string pkgname=i->first;
			const status& selstat=i->second;

			if (selstat.flag(status::flag_must_install)&&
				!selstat.flag(status::flag_must_remove))
			{
				// This package must be installed, remain installed or
				// be upgraded if all dependencies are to be satisfied.
				bool success=false;
				if (!selstat.flag(status::flag_must_upgrade)&&
					(selstat.state()>=status::state_installed))
				{
					// If the must-upgrade flag is false and if the
					// package is currently installed then look to
					// see whether the installed version can be left
					// as it is.
					binary_control_table::key_type
						key(pkgname,selstat.version());
					success=fix_dependencies(_control[key],true);
				}
				if (!success)
				{
					// If the previous step did not find a suitable
					// candidate then try the latest version available.
					const pkg::control& ctrl=_control[pkgname];
					success=fix_dependencies(ctrl,true);
					if (success)
						ensure_installed(pkgname,ctrl.version());
				}
				if (!success&&!selstat.flag(status::flag_must_remove))
				{
					// No suitable candidate was found, therefore this
					// package must be removed.  Because it must also
					// be installed, this will cause the operation as
					// a whole to fail.
					ensure_removed(pkgname);
				}
			}
			if ((selstat.state()>status::state_removed)&&
				!selstat.flag(status::flag_must_remove))
			{
				// This package is selected for installation, and as yet
				// there are no known grounds for its removal.
				binary_control_table::key_type key(pkgname,selstat.version());
				bool success=fix_dependencies(_control[key],false,false);
				if (!success)
				{
					// If dependency list cannot be satisfied then
					// there are now grounds for removal.
					ensure_removed(pkgname);
				}
			}
		}
	}

	// Apply flags
	bool success=true;
	for (status_table::const_iterator i=_selstat.begin();
		i!=_selstat.end();++i)
	{
		const string& pkgname=(*i).first;
		status selstat=(*i).second;
		if (selstat.flag(status::flag_must_install)&&
			!selstat.flag(status::flag_must_remove))
		{
			if (selstat.flag(status::flag_must_upgrade)||
				(selstat.state()<status::state_installed))
			{
				if (selstat.state()<=status::state_removed)
					selstat.flag(status::flag_auto,true);
				selstat.state(status::state_installed);
				const pkg::control& ctrl=_control[pkgname];
				selstat.version(ctrl.version());
				_selstat.insert(pkgname,selstat);
			}
		}
		else if (selstat.flag(status::flag_must_remove)&&
			!selstat.flag(status::flag_must_install))
		{
			selstat.flag(status::flag_auto,false);
			if (selstat.state()>status::state_removed)
				selstat.state(status::state_removed);
			_selstat.insert(pkgname,selstat);
		}
		else if (selstat.flag(status::flag_must_remove)&&
			selstat.flag(status::flag_must_install))
		{
			success=false;
		}
	}
	return success;
}

void pkgbase::remove_auto()
{
	bool changed=true;
	while (changed)
	{
		changed=false;

		// Mark auto-installed packages for removal
		for (status_table::const_iterator i=_selstat.begin();
			i!=_selstat.end();++i)
		{
			string pkgname=i->first;
			status selstat=i->second;
			selstat.flag(status::flag_must_remove,false);
			selstat.flag(status::flag_must_install,false);
			selstat.flag(status::flag_must_upgrade,false);
			_selstat.insert(pkgname,selstat);
		}

		// Unmark packages that are still needed
		for (status_table::const_iterator i=_selstat.begin();
			i!=_selstat.end();++i)
		{
			string pkgname=i->first;
			const status& selstat=i->second;
			if (selstat.state()>=status::state_installed)
			{
				binary_control_table::key_type key(pkgname,selstat.version());
				fix_dependencies(_control[key],true,true);
			}
		}

		// Remove packages no longer needed
		for (map<string,status>::const_iterator i=_selstat.begin();
			i!=_selstat.end();++i)
		{
			string pkgname=i->first;
			status selstat=i->second;
			if (selstat.flag(status::flag_auto)&&
				!selstat.flag(status::flag_must_install))
			{
				selstat.state(status::state_removed);
				selstat.flag(status::flag_auto,false);
				_selstat.insert(pkgname,selstat);
				changed=true;
			}
		}
	}
}

bool pkgbase::fix_dependencies(const pkg::control& ctrl,bool allow_new)
{
	bool success=fix_dependencies(ctrl,allow_new,false);
	if (success) success=fix_dependencies(ctrl,allow_new,true);
	return success;
}

bool pkgbase::fix_dependencies(const pkg::control& ctrl,bool allow_new,
	bool apply)
{
	// Parse dependency list.
	vector<vector<dependency> > deps;
	string deplist=ctrl.depends();
	parse_dependency_list(deplist.begin(),deplist.end(),&deps);

	// Process dependency list.
	bool success=true;
	for (vector<vector<dependency> >::const_iterator i=deps.begin();
		i!=deps.end();++i)
	{
		if (const pkg::control* ctrl=resolve(*i,allow_new))
		{
			if (apply)
				ensure_installed(ctrl->pkgname(),ctrl->version());
		}
		else success=false;
	}

	// Return true if all dependencies satisfied, otherwise false.
	return success;
}

const pkg::control* pkgbase::resolve(const vector<dependency>& deps,
	bool allow_new)
{
	// First try to satisfy the dependency with an existing package.
	for (vector<dependency>::const_iterator i=deps.begin();
		i!=deps.end();++i)
	{
		const pkg::control* ctrl=resolve(*i,false);
		if (ctrl) return ctrl;
	}

	// If that fails, try to satisfy the dependency with a new package
	// (if that is allowed).
	if (allow_new)
	{
		for (vector<dependency>::const_iterator i=deps.begin();
			i!=deps.end();++i)
		{
			const pkg::control* ctrl=resolve(*i,true);
			if (ctrl) return ctrl;
		}
	}

	// If no match found then return 0.
	return 0;
}

const pkg::control* pkgbase::resolve(const dependency& dep,bool allow_new)
{
	// Select package.
	string pkgname=dep.pkgname();
	const status& selstat=_selstat[pkgname];

	// Resolution always fails if the package is flagged for removal.
	if (selstat.flag(status::flag_must_remove)) return 0;

	// Try to satisfy the dependency with an existing packae
	// (one which is already selected for installation and not
	// flagged for upgrade).
	if ((selstat.state()>=status::state_installed)&&
		!selstat.flag(status::flag_must_upgrade))
	{
		string pkgvrsn=selstat.version();
		binary_control_table::key_type key(pkgname,pkgvrsn);
		const pkg::control& ctrl=_control[key];
		if (dep.matches(ctrl.pkgname(),ctrl.version()))
			return &ctrl;
	}

	// If that fails, and if either new packages are allowed or the
	// package is already flagged for upgrade, try to satisfy the
	// dependency with the most recent available version.
	if (allow_new||selstat.flag(status::flag_must_upgrade))
	{
		const pkg::control& ctrl=_control[pkgname];
		if (dep.matches(ctrl.pkgname(),ctrl.version()))
			return &ctrl;
	}

	// If no match found then return 0.
	return 0;
}

void pkgbase::ensure_installed(const string& pkgname,const string& pkgvrsn)
{
	bool changed=false;
	status selstat=_selstat[pkgname];
	if (!selstat.flag(status::flag_must_install))
	{
		selstat.flag(status::flag_must_install,true);
		changed=true;
	}
	if (!selstat.flag(status::flag_must_upgrade))
	{
		if ((selstat.state()<status::state_installed)||
			(version(selstat.version())!=version(pkgvrsn)))
		{
			selstat.flag(status::flag_must_upgrade,true);
			changed=true;
		}
	}
	if (changed)
	{
		_selstat.insert(pkgname,selstat);
		_changed=true;
	}
}

void pkgbase::ensure_removed(const string& pkgname)
{
	bool changed=false;
	status selstat=_selstat[pkgname];
	if (!selstat.flag(status::flag_must_remove))
	{
		selstat.flag(status::flag_must_remove,true);
		changed=true;
	}
	if (changed)
	{
		_selstat.insert(pkgname,selstat);
		_changed=true;
	}
}

}; /* namespace pkg */
