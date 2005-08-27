// This file is part of LibPkg.
// Copyright © 2003-2005 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include <ctype.h>

#include "libpkg/dependency.h"

namespace pkg {

dependency::dependency():
	_relation(relation_al)
{}

dependency::dependency(const string& pkgname,relation_type relation,
	const pkg::version& version):
	_pkgname(pkgname),
	_relation(relation),
	_version(version)
{}

dependency::dependency(string::const_iterator first,
	string::const_iterator last)
{
	parse(first,last);
}

dependency::dependency(const string& depstr)
{
	parse(depstr.begin(),depstr.end());
}

dependency::~dependency()
{}

dependency::operator string() const
{
	// Construct relation and version strings.
	string relstr;
	switch (_relation)
	{
	case relation_al:
		break;
	case relation_eq:
		relstr="=";
		break;
	case relation_lt:
		relstr="<<";
		break;
	case relation_ge:
		relstr=">=";
		break;
	case relation_le:
		relstr="<=";
		break;
	case relation_gt:
		relstr=">>";
		break;
	}
	string verstr=_version;

	// Calculate length.
	unsigned int length=_pkgname.length();
	if (_relation!=relation_al) length+=4+relstr.length()+verstr.length();

	// Construct dependency string.
	string out;
	out.reserve(length);
	out.append(_pkgname);
	if (_relation!=relation_al)
	{
		out.append(" (");
		out.append(relstr);
		out.append(" ");
		out.append(verstr);
		out.append(")");
	}
	return out;
}

bool dependency::matches(const string& pkgname,
	const pkg::version& pkgvrsn) const
{
	if (_pkgname!=pkgname) return false;
	switch (_relation)
	{
	case relation_al:
		break;
	case relation_eq:
		if (!(pkgvrsn==_version)) return false;
		break;
	case relation_lt:
		if (!(pkgvrsn<_version)) return false;
		break;
	case relation_ge:
		if (!(pkgvrsn>=_version)) return false;
		break;
	case relation_le:
		if (!(pkgvrsn<=_version)) return false;
		break;
	case relation_gt:
		if (!(pkgvrsn>_version)) return false;
		break;
	}
	return true;
}

void dependency::parse(string::const_iterator first,
	string::const_iterator last)
{
	// Initialise iterator.
	string::const_iterator p=first;

	// Parse package name.
	string::const_iterator q=p;
	while ((p!=last)&&(*p!=' ')&&(*p!='('))
	{
		char ch=*p++;
		if (!isalnum(ch)&&(ch!='+')&&(ch!='-')&&(ch!='.'))
			throw parse_error("illegal character in package name");
	}
	if (p==q) throw parse_error("package name expected");
	_pkgname=string(q,p);

	// If not at end of sequence, parse version specification.
	if (p!=last)
	{
		// Skip whitespace.
		while ((p!=last)&&(*p==' ')) ++p;

		// Parse '('.
		if ((p!=last)&&(*p=='(')) ++p;
		else throw parse_error("'(' or end of dependency expected");

		// Skip whitespace.
		while ((p!=last)&&(*p==' ')) ++p;

		// Parse relation.
		char relch1=0;
		char relch2=0;
		if (p!=last)
		{
			relch1=*p;
			if (p+1!=last)
			{
				relch2=*(p+1);
			}
		}
		if (relch1=='=')
		{
			p+=1;
			_relation=relation_eq;
		}
		else if ((relch1=='<')&&(relch2=='<'))
		{
			p+=2;
			_relation=relation_lt;
		}
		else if ((relch1=='>')&&(relch2=='='))
		{
			p+=2;
			_relation=relation_ge;
		}
		else if ((relch1=='<')&&(relch2=='='))
		{
			p+=2;
			_relation=relation_le;
		}
		else if ((relch1=='>')&&(relch2=='>'))
		{
			p+=2;
			_relation=relation_gt;
		}
		else
		{
			throw parse_error("relation expected");
		}

		// Skip whitespace.
		while ((p!=last)&&(*p==' ')) ++p;

		// Parse version.
		q=p;
		while ((p!=last)&&(*p!=' ')&&(*p!=')')) ++p;
		_version=pkg::version(string(q,p));

		// Skip whitespace.
		while ((p!=last)&&(*p==' ')) ++p;

		// Parse ')'.
		if ((p!=last)&&(*p==')')) ++p;
		else throw parse_error("')' expected");
	}
	else
	{
		// If no version specification then dependency matches any version.
		_relation=relation_al;
		_version=string();
	}

	// There should now be no characters remaining.
	if (p!=last) throw parse_error("end of dependency expected");
}

dependency::parse_error::parse_error(const char* message):
	std::runtime_error(message)
{}

void parse_dependency_alt_list(string::const_iterator first,
	string::const_iterator last,std::vector<dependency>* out)
{
	// Initialise iterator.
	string::const_iterator p=first;

	// Repeat until end of sequence.
	while (p!=last)
	{
		// Mark start of sub-list.
		string::const_iterator q=p;

		// Search for list delimeter (pipe).
		while ((p!=last)&&(*p!='|')) ++p;

		// Mark end of sub-list.
		string::const_iterator r=p;

		// If delimeter found, strip whitespace from end of sub-list.
		if (p!=last)
		{
			while ((r!=q)&&(*(r-1)==' ')) --r;
		}

		// Parse sub-list.
		if (r==q) throw dependency::parse_error("dependency expected");
		dependency dep(q,r);
		if (out) out->push_back(dep);

		// If delimeter found, skip it plus any whitespace, but a
		// dependency must follow.
		if (p!=last)
		{
			++p;
			while ((p!=last)&&(*p==' ')) ++p;
			if (p==last) throw dependency::parse_error("dependency expected");
		}
	}
}

void parse_dependency_list(string::const_iterator first,
	string::const_iterator last,std::vector<std::vector<dependency> >* out)
{
	// Initialise iterator.
	string::const_iterator p=first;

	// Repeat until end of sequence.
	while (p!=last)
	{
		// Mark start of sub-list.
		string::const_iterator q=p;

		// Search for list delimeter (comma).
		while ((p!=last)&&(*p!=',')) ++p;

		// Mark end of sub-list.
		string::const_iterator r=p;

		// If delimeter found, strip whitespace from end of sub-list.
		if (p!=last)
		{
			while ((r!=q)&&(*(r-1)==' ')) --r;
		}

		// Parse sub-list.
		if (r==q) throw dependency::parse_error("dependency expected");
		std::vector<dependency> out2;
		parse_dependency_alt_list(q,r,&out2);
		if (out) out->push_back(out2);

		// If delimeter found, skip it plus any whitespace, but a
		// dependency must follow.
		if (p!=last)
		{
			++p;
			while ((p!=last)&&(*p==' ')) ++p;
			if (p==last) throw dependency::parse_error("dependency expected");
		}
	}
}

}; /* namespace pkg */
