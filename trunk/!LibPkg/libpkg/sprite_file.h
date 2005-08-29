// This file is part of LibPkg.
// Copyright © 2004-2005 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef _LIBPKG_SPRITE_FILE
#define _LIBPKG_SPRITE_FILE

#include <map>
#include <string>
#include <fstream>
#include <stdexcept>

namespace pkg {

using std::string;

/** An interface class to represent a sprite file. */
class sprite_file
{
public:
	class sprite_info;

	/** A type to represent an unsigned 32-bit integer. */
	typedef unsigned long uint32;

	class not_found;
	class already_exists;
	class corrupt;
private:
	/** A class for comparing strings without regard for case. */
	class cmp_nocase
	{
	public:
		/** Perform comparison.
		 * @param lhs the left hand side
		 * @param rhs the right hand side
		 * @return true if lhs<rhs, otherwise false
		 */
		bool operator()(const string& lhs,const string& rhs) const;
	};

	/** The map type. */
	typedef std::map<string,sprite_info,cmp_nocase> map_type;

	/** The constant iterator type. */
	typedef map_type::const_iterator const_iterator;

	/** The pathname of the underlying sprite file. */
	string _pathname;

	/** The stream used to access the underlying sprite file. */
	mutable std::fstream _sfs;

	/** A map from sprite name to sprite information record. */
	map_type _directory;

	/** The start of free space in the sprite file. */
	uint32 _free;
public:
	/** Construct sprite file object.
	 * @param pathname the pathname of the sprite file
	 * @param writable true to open read-write, false to open read-only
	 */
	sprite_file(const string& pathname,bool writable=false);
private:
	/** Copy sprite file object.
	 * This method is private (and not implemented), therefore
	 * sprite file objects cannot be copied.
	 */
	sprite_file(const sprite_file&);
public:
	/** Destroy sprite file object. */
	~sprite_file();
private:
	/** Assign sprite file object.
	 * This method is private (and not implemented), therefore
	 * sprite file objects cannot be assigned.
	 */
	sprite_file& sprite_file::operator=(const sprite_file&);
public:
	/** Get number of sprites.
	 * @return the number of sprites
	 */
	unsigned int size() const
		{ return _directory.size(); }

	/** Get const sprite information record at index.
	 * @param index the index
	 * @return a const reference to the sprite information record
	 */
	const sprite_info& operator[](unsigned int index) const;

	/** Find const sprite information record for sprite name.
	 * @param name the sprite name
	 * @return a const pointer to the sprite information record,
	 *  or 0 if not found
	 */
	const sprite_info* find(const string& name) const;

	/** Copy sprite from another sprite file.
	 * @param src the sprite file containing the source
	 * @param name the sprite name
	 */
	void copy(sprite_file& src,const string& name);
};

/** A class to represent a sprite within a sprite file. */
class sprite_file::sprite_info
{
private:
	/** The file offset of the sprite. */
	uint32 _offset;

	/** The size of the sprite. */
	uint32 _size;

	/** The sprite name. */
	string _name;
public:
	/** Construct sprite information record. */
	sprite_info();

	/** Construct sprite information record from stream.
	 * @param in the input stream
	 */
	sprite_info(std::istream& in);

	/** Destroy sprite information record. */
	~sprite_info();

	/** Get file offset.
	 * @return the file offset of the sprite
	 */
	const uint32 offset() const
		{ return _offset; }

	/** Get size.
	 * @return the size of the sprite
	 */
	const uint32 size() const
		{ return _size; }

	/** Get sprite name.
	 * @return the sprite name
	 */
	const string& name() const
		{ return _name; }
};

/** An exception class for reporting not-found errors. */
class sprite_file::not_found:
	public std::runtime_error
{
public:
	/** Construct not-found error.
	 * @param name the sprite name that was not found
	 */
	not_found(const string& name);

	/** Make error message.
	 * @param name the sprite name that was not found
	 * @return a message which describes the not-found error.
	 */
	static string make_message(const string& name);
};

/** An exception class for reporting already-exists errors. */
class sprite_file::already_exists:
	public std::runtime_error
{
public:
	/** Construct already-exists error.
	 * @param name the sprite name that already exists
	 */
	already_exists(const string& name);

	/** Make error message.
	 * @param name the sprite name that already exists
	 * @return a message which describes the not-found error.
	 */
	static string make_message(const string& name);
};

/** An exception class for reporting corrupt-sprite-file errors. */
class sprite_file::corrupt:
	public std::runtime_error
{
public:
	/** Construct corrupt-sprite-file error. */
	corrupt();
};

}; /* namespace pkg */

#endif
