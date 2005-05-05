// This file is part of LibPkg.
// Copyright © 2004 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include "libpkg/filesystem.h"
#include "libpkg/sprite_file.h"

namespace {

using namespace pkg;

typedef sprite_file::uint32 uint32;

/** Read 32-bit unsigned integer from stream.
 * Storage is little-endian.
 * @param in the input stream
 * @return the integer
 */
uint32 read_32(istream& in)
{
	uint32 value=static_cast<uint32>(in.get());
	value|=static_cast<uint32>(in.get())<<8;
	value|=static_cast<uint32>(in.get())<<16;
	value|=static_cast<uint32>(in.get())<<24;
	return value;
}

/** Write 32-bit unsigned integer to stream.
 * Storage is little-endian.
 * @param out the output stream
 * @param value the integer
 */
void write_32(ostream& out,uint32 value)
{
	out.put((value>>0)&0xff);
	out.put((value>>8)&0xff);
	out.put((value>>16)&0xff);
	out.put((value>>24)&0xff);
}

/** Read string from stream.
 * This function will always read the specified number of bytes from the
 * stream, however the string itself can be zero-terminated so may be shorter.
 * @param in the input stream
 * @param size the number of bytes to read
 * @return the string
 */
string read_string(istream& in,int size)
{
	string value(size,' ');
	for (int i=0;i!=size;++i)
		value[i]=in.get();
	string::size_type f=value.find(char(0));
	if (f!=string::npos) value.resize(f);
	return value;
}

/** A class for allocating a memory block.
 * The memory is automatically deallocated when the destructor is called.
 */
class buffer
{
private:
	/** A pointer to the memory block. */
	char* _data;
public:
	/** Construct buffer object.
	 * @param size the number of bytes required
	 */
	buffer(unsigned int size);

	/** Destroy buffer object.
	 * This deallocates the memory block.
	 */
	~buffer();

	/** Get pointer to memory block.
	 * @param a pointer to the memory block
	 */
	operator char*()
		{ return _data; }
};

buffer::buffer(unsigned int size):
	_data(new char[size])
{}

buffer::~buffer()
{
	delete[] _data;
	_data=0;
}

/** Automatically create sprite file if it does not already exist.
 * @param pathname the required pathname
 * @return the pathname
 */
const string& auto_create(const string& pathname)
{
	if (!object_type(pathname))
	{
		ofstream out(pathname.c_str());
		write_32(out,0);
		write_32(out,0x10);
		write_32(out,0x10);
		out.close();
		write_filetype(pathname,0xff9);
	}
	return pathname;
}

}; /* anonymous namespace */

namespace pkg {

bool sprite_file::cmp_nocase::operator()(const string& lhs,const string& rhs)
	const
{
	string::const_iterator i=lhs.begin();
	string::const_iterator j=rhs.begin();
	string::const_iterator e=i+min(lhs.length(),rhs.length());
	while (i!=e) if (*i++!=*j++) return *--i<*--j;
	return (lhs.length()<rhs.length());
}

sprite_file::sprite_file(const string& pathname,bool writable):
	_pathname(auto_create(pathname)),
	_sfs(pathname.c_str(),ios::in|((writable)?ios::out:0)|ios::binary)
{
	// Read sprite control block.
	_sfs.seekg(0);
	uint32 count=read_32(_sfs);
	uint32 first=read_32(_sfs)-4;
	_free=read_32(_sfs)-4;
	if (!_sfs) throw corrupt();

	// Read information for each sprite.
	_sfs.seekg(first);
	while (_directory.size()!=count)
	{
		sprite_info info(_sfs);
		if (!_sfs) throw corrupt();
		if (find(info.name())) throw already_exists(info.name());
		_directory[info.name()]=info;
	}
}

const sprite_file::sprite_info* sprite_file::find(const string& name) const
{
	const_iterator f=_directory.find(name);
	return (f!=_directory.end())?&f->second:0;
}

void sprite_file::copy(sprite_file& src,const string& name)
{
	sprite_file& dst=*this;

	// Find sprite in source file.
	const sprite_info* info=src.find(name);
	if (!info) throw not_found(name);

	// Check for existance of sprite in destination file.
	if (find(name)) throw already_exists(name);

	// Create buffer.
	unsigned int bsize=1024;
	buffer buf(bsize);

	// Move source pointer to start of sprite.
	src._sfs.seekg(info->offset());

	// Move destination pointer to start of free space.
	dst._sfs.seekp(_free);

	// Copy sprite.
	uint32 size=info->size();
	uint32 index=0;
	while (index!=size)
	{
		uint32 count=min<uint32>(bsize,size-index);
		src._sfs.read(buf,count);
		dst._sfs.write(buf,count);
		index+=count;
	}

	if (dst._sfs)
	{
		// Insert sprite information into directory.
		_directory[info->name()]=*info;

		// Update start of free space.
		_free+=info->size();

		// Update control block and flush.
		dst._sfs.seekp(0);
		write_32(dst._sfs,_directory.size());
		dst._sfs.seekp(8);
		write_32(dst._sfs,_free+4);
		dst._sfs.flush();
	}
}

sprite_file::~sprite_file()
{}

sprite_file::sprite_info::sprite_info():
	_offset(0)
{}

sprite_file::sprite_info::sprite_info(istream& in):
	_offset(in.tellg())
{
	_size=read_32(in);
	_name=read_string(in,12);
	in.seekg(_offset+_size);
}

sprite_file::sprite_info::~sprite_info()
{}

sprite_file::not_found::not_found(const string& name)
{
	_message.reserve(27+name.length());
	_message.append("\"");
	_message.append(name);
	_message.append("\" not found in sprite file");
}

sprite_file::not_found::~not_found()
{}

const char* sprite_file::not_found::what() const
{
	return _message.c_str();
}

sprite_file::already_exists::already_exists(const string& name)
{
	_message.reserve(32+name.length());
	_message.append("\"");
	_message.append(name);
	_message.append("\" already exists in sprite file");
}

sprite_file::already_exists::~already_exists()
{}

const char* sprite_file::already_exists::what() const
{
	return _message.c_str();
}

sprite_file::corrupt::corrupt()
{}

sprite_file::corrupt::~corrupt()
{}

const char* sprite_file::corrupt::what() const
{
	return "corrupt sprite file";
}

}; /* namespace pkg */
