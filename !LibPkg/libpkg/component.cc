// This file is part of the LibPkg.
//
// Copyright 2003-2020 Graham Shaw
// Copyright 2013-2020 Alan Buckley
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

#include "libpkg/component.h"
#include <cctype>

namespace pkg
{

const char*ComponentFlagNames[] = 
{
	"Movable",
	"LookAt",
	"Run",
	"AddToApps"
};
	
component::component() : _flags(0) {}

component::component(std::string::const_iterator first, std::string::const_iterator last)
{
	parse(first, last);
}

component::component(const std::string& compstr)
{
	parse(compstr.begin(), compstr.end());
}

component::~component()
{
}

bool component::operator==(const component &other) const
{
	return (_name == other._name && _flags == other._flags && _path == other._path);
}

bool component::operator!=(const component &other) const
{
	return (_name != other._name || _flags != other._flags || _path != other._path);
}


component::operator std::string() const
{
	std::string out(_name);
	if (_flags)
	{
		out += " (";
		bool add_space = false;
		for (int f = (int)movable; f <= (int)add_to_apps; ++f)
		{
			if (flag((flag_type)f))
			{
				if (add_space) out += " ";
				else add_space = true;
				out += ComponentFlagNames[f];
			}
		}
		out += ")";
	}

	return out;
}

void component::flag(flag_type flag,bool value)
{
	if (value) _flags |= (1<<flag);
	else _flags &= ~(1<<flag);
}

void component::parse(std::string::const_iterator first,std::string::const_iterator last)
{
	// Initialise iterator.
	std::string::const_iterator p=first;
	std::string valid("!_+-.<>/'");
	_flags = 0;

	// Parse package name.
	std::string::const_iterator q=p;
	while ((p!=last)&&(*p!=' ')&&(*p!='('))
	{
		char ch=*p++;
		if (!(std::isalnum(ch) || (ch >= 0xA0 && ch <= 0xFF))&&(valid.find(ch) == std::string::npos))
			throw parse_error("illegal character in component name");
	}
	if (p==q) throw parse_error("component name expected");
	_name=std::string(q,p);

	// If not at end of sequence, parse flags.
	if (p!=last)
	{
		// Skip whitespace.
		while ((p!=last)&&(*p==' ')) ++p;

		// Parse '('.
		if ((p!=last)&&(*p=='(')) ++p;
		else throw parse_error("'(' or end of component expected");

		// Skip whitespace.
		while ((p!=last)&&(*p==' ')) ++p;

		// Loop through flags
		while ((p!=last)&& *p!=')')
		{
			std::string::const_iterator flag_start = p;
			while ((p!=last)&&(*p!=' ')&&(*p!=')')) ++p;
			std::string flag_name(flag_start, p);
			unsigned int old_flags = _flags;
			for(int f = (int)movable; f <= (int)add_to_apps; ++f)
			{
				if (flag_name == ComponentFlagNames[f]) flag((flag_type)f, true);
			}
			if (old_flags == _flags) throw parse_error(std::string("invalid component flag '" + flag_name + "'").c_str());

			// Skip whitespace.
			while ((p!=last)&&(*p==' ')) ++p;
		}

		if (_flags)
		{
			if ((p==last)||(*p!=')')) throw parse_error("')' missing from end of component flags");
			if (p!=last) p++;
		}
	}

	// There should now be no characters remaining.
	if (p!=last) throw parse_error((std::string("end of component expected, got '") + *p + "'").c_str());
}

component::parse_error::parse_error(const char* message):
	std::runtime_error(message)
{}

void parse_component_list(std::string::const_iterator first,
	std::string::const_iterator last,std::vector<component>*out)
{
	// Initialise iterator.
	std::string::const_iterator p=first;

	// Repeat until end of sequence.
	while (p!=last)
	{
		// Mark start of sub-list.
		std::string::const_iterator q=p;

		// Search for list delimeter (comma).
		while ((p!=last)&&(*p!=',')) ++p;

		// Mark end of sub-list.
		std::string::const_iterator r=p;

		// If delimeter found, strip whitespace from end of sub-list.
		if (p!=last)
		{
			while ((r!=q)&&(*(r-1)==' ')) --r;
		}

		// Parse sub-list.
		if (r==q) throw component::parse_error("component expected");
		component comp(q,r);
		if (out) out->push_back(comp);

		// If delimeter found, skip it plus any whitespace, but a
		// dependency must follow.
		if (p!=last)
		{
			++p;
			while ((p!=last)&&(*p==' ')) ++p;
			if (p==last) throw component::parse_error("component expected");
		}
	}
}

std::ostream& operator<<(std::ostream& out,
	const component &comp)
{
	out << ((std::string)comp) << "\t";
	out << comp.path();

	return out;
}

/** Read component record from input stream.
 * @param in the input stream
 * @param comp the component record
 * @return the input stream
 */
std::istream& operator>>(std::istream& in, component &comp)
{
	// Read line from input stream.
	std::string line;
	std::getline(in,line);
	std::string::size_type tab_pos = line.find('\t');
	if (tab_pos == std::string::npos)
	{
		comp = component(line.begin(), line.end());
	} else
	{
		std::string::iterator tab_iter = line.begin() + tab_pos;
		comp = component(line.begin(), tab_iter);
		if (++tab_iter == line.end()) comp.path("");
		else comp.path(std::string(tab_iter, line.end()));
	}

	return in;
}


}; /* namespace pkg */
