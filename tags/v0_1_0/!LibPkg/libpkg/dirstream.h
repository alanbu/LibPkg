// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef _LIBPKG_DIRSTREAM
#define _LIBPKG_DIRSTREAM

#include <string>

namespace pkg {

/** A class for reading directories. */
class dirstream
{
public:
	struct object;
private:
	/** The size of the file information buffer. */
	unsigned int _buffer_size;

	/** A buffer for the file information. */
	char* _buffer;

	/** The current offset. */
	int _offset;

	/** The pathname of the directory. */
	string _pathname;

	/** The pattern to be matched. */
	string _pattern;

	/** The buffer full flag.
	 * True if the buffer is full, otherwise false.
	 */
	bool _buffer_full;
public:
	/** Construct directory stream.
	 * @param pathname the pathname of the directory
	 * @param pattern the pattern to be matched
	 */
	dirstream(const string& pathname,const string& pattern="*");

	/** Destroy directory stream. */
	~dirstream();

	/** Test state of directory stream.
	 * @return true if stream state is good, otherwise false
	 */
	operator bool();

	/** Read file information.
	 * @param obj the file information record to be read
	 * @return a reference to this
	 */
	dirstream& operator>>(object& obj);
private:
	/** Ensure that the buffer is full if that is possible. */
	void fill_buffer();
};

struct dirstream::object
{
	/** The load address. */
	unsigned long loadaddr;
	/** The execution address. */
	unsigned long execaddr;
	/** The file length. */
	unsigned long length;
	/** The file attributes. */
	unsigned long attr;
	/** The object type. */
	unsigned long objtype;
	/** The file type. */
	unsigned long filetype;
	/** The object name. */
	string name;
};

}; /* namespace pkg */

#endif
