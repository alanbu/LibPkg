// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

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
	_paths("Choices:RiscPkg.Paths")
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

}; /* namespace pkg */
