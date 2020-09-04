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

#ifndef LIBPKG_COMPONENT
#define LIBPKG_COMPONENT

#include <string>
#include <iostream>
#include <vector>
#include <stdexcept>

namespace pkg
{
	
/**
 * A class to represent a component of a package
 *
 * A component is a file or folder that has extra metadata associated with it
 * to configure it at install time.
 *
 * From LibPkg 0.4 any path that can be set at install time should
 * have a component included with the Moveable flag set.
 */
class component
{
public:
	/** The options available/selected for this component
	 */
	enum flag_type
	{
		/** A flag to indicate this component can have it's install location
		 *  changed and it may be moved after install. */
		movable,
		/** A flag to indicate this component should be added to the RISC OS
		 *  boot look at file, so it is booted when the desktop starts. */
		look_at,
		/** A flag to indicate this component should be added to the RISC OS
		 *  boot run file, so it is run when the desktop starts. */
		run,
		/** A flag to indicate this component should be added to the RISC OS
		 *  application pseudo folder. */
		add_to_apps
	};

	class parse_error;
private:
	std::string _name;
	unsigned int _flags;
	std::string _path;

public:
	/**
	 * Empty constructor, unnamed component with no flags
	 */
	component();

	/**
	 * Construct a component from a sequence.
	 * 
	 * This is the name followed by space separated options in brackets
	 *
	 * @param first beginning of sequence
	 * @param last end of sequence
	 */
	component(std::string::const_iterator first, std::string::const_iterator last);

	/** Construct component from string.
	 * @param compstr the component string
	 */
	component(const std::string& compstr);

	/** Destroy component. */
	~component();

	/** Check if it is the same as another component */
	bool operator==(const component &other) const;
	/** Check if it is different from another component */
	bool operator!=(const component &other) const;

	/** Convert component to string.
	 * Does not include the path
	 * @return the component as a string
	 */ 
	operator std::string() const;

	/** Get the name of the component
	 * The name is the same as the logical path to the component.
	 * @returns the component name
	 */
	const std::string &name() const {return _name;}

	/**
	 * Get the bit field containing the component flags
	 */
	unsigned int flags() const {return _flags;}

	/** Get component flag.
	 * @param flag the flag to be read
	 * @return the value of the flag
	 */
	bool flag(flag_type flag) const
		{ return (_flags>>flag)&1; }

	/** Set component flag.
	 * @param flag the flag to be altered
	 * @param value the required value
	 */
	void flag(flag_type flag,bool value);

	/** Get the installation path
	 * @returns path or empty string for the default path.
	 */
	const std::string &path() const {return _path;}
	/**
	 * Set the installation path
	 * @param path the new installation path for the component
	 */
	void path(const std::string &path) {_path = path;}

private:
	/** Parse component.
	 * @param first the beginning of the sequence
	 * @param last tne end of the sequence
	 */
	void parse(std::string::const_iterator first,std::string::const_iterator last);

};

/** An exception class for reporting parse errors. */
class component::parse_error:
	public std::runtime_error
{
public:
	/** Construct parse error.
	 * @param message a message which describes the parse error.
	 */
	parse_error(const char* message);
};

/** Parse component list.
 * @param first the beginning of the sequence
 * @param last the end of the sequence
 * @param out a vector to which the coponents are added
 */
void parse_component_list(std::string::const_iterator first,
	std::string::const_iterator last,std::vector<component> *out);

/** Write component record to output stream.
 * @param out the output stream
 * @param comp the component record
 * @return the output stream
 */
std::ostream& operator<<(std::ostream& out,
	const component &comp);

/** Read component record from input stream.
 * @param in the input stream
 * @param comp the component record
 * @return the input stream
 */
std::istream& operator>>(std::istream& in, component &comp);


}; /* namespace pkg */

#endif
