// This file is part of LibPkg.
//
// Copyright 2004-2020 Graham Shaw
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

#ifndef LIBPKG_URI
#define LIBPKG_URI

#include <string>

namespace pkg {

using std::string;

/** A class to represent a uniform resource identifier.
 * This is a suitable representation for accessing the individual
 * components of a URI (the scheme, authority, path, query and fragment).
 * Where an opaque representation is sufficient, a string is likely to
 * be a more efficient method of storage.
 *
 * For simplicity, the scheme, authority, query and fragment are taken to
 * include the prefix/suffix with which they are associated.  This does
 * not match the syntax given in RFC 2396.
 */
class uri
{
private:
	/** The scheme component.
	 * This should either be blank (meaning that there is no scheme
	 * component) or end with ":".
	 */
	string _scheme;

	/** The authority component.
	 * This should either be blank (meaning that there is no authority
	 * component) or begin with "//".
	 */
	string _authority;

	/** The path component. */
	string _path;

	/** The query component.
	 * This should either be blank (meaning that there is no query
	 * component) or begin with "?".
	 */
	string _query;

	/** The fragment component.
	 * This should either be blank (meaning that there is no fragment
	 * component) or begin with "#".
	 */
	string _fragment;
public:
	/** Construct empty URI. */
	uri();

	/** Construct URI from string. */
	explicit uri(const string& s);

	/** Convert URI to string.
	 * @return the URI as a string.
	 */
	operator string();

	/** Resolve relative URI to produce absolute URI.
	 * This URI acts as the base.
	 * @param rel_uri the relative URI
	 * @return the absolute URI
	 */
	uri operator+(const uri& rel_uri);

	/** Resolve relative URI to produce absolute URI.
	 * This URI initially acts as the base.
	 * @param rel_uri the relative URI
	 * @return the absolute URI
	 */
	uri& operator+=(const uri& rel_uri);

	/** Get scheme component.
	 * This should either be blank (meaning that there is no scheme
	 * component) or end with ":".
	 * @return the scheme component
	 */
	const string& scheme() const
		{ return _scheme; }

	/** Get authority component.
	 * This should either be blank (meaning that there is no authority
	 * component) or begin with "//".
	 * @return the authority component, or 0 if undefined
	 */
	const string& authority() const
		{ return _authority; }

	/** Get path component.
	 * @return the path component
	 */
	const string& path() const
		{ return _path; }

	/** Get query component.
	 * This should either be blank (meaning that there is no query
	 * component) or begin with "?".
	 * @return the query component
	 */
	const string& query() const
		{ return _query; }

	/** Get fragment component.
	 * This should either be blank (meaning that there is no fragment
	 * component) or begin with "#".
	 * @return the fragment component
	 */
	const string& fragment() const
		{ return _fragment; }

	/** Set scheme component.
	 * This should either be blank (meaning that there is no scheme
	 * component) or end with ":".
	 * @param scheme the required scheme component
	 * @return a reference to this
	 */
	uri& scheme(const string& scheme);

	/** Set authority component.
	 * This should either be blank (meaning that there is no authority
	 * component) or begin with "//".
	 * @param authority the required authority component
	 * @return a reference to this
	 */
	uri& authority(const string& authority);

	/** Set path component.
	 * @param path the required path component
	 * @return a reference to this
	 */
	uri& path(const string& path);

	/** Set query component.
	 * This should either be blank (meaning that there is no query
	 * component) or begin with "?".
	 * @param query the required query component
	 * @return a reference to this
	 */
	uri& query(const string& query);

	/** Set fragment component.
	 * This should either be blank (meaning that there is no fragment
	 * component) or begin with "#".
	 * @param fragment the required fragment component
	 * @return a reference to this
	 */
	uri& fragment(const string& fragment);
};

}; /* namespace pkg */

#endif
