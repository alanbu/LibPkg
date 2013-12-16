// This file is part of LibPkg.
// Copyright � 2003-2005 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include "zlib.h"

#include "libpkg/zipfile.h"
#include "libpkg/filesystem.h"
#include <cstring>
#include <iostream>

#define UNZIP_BUFFER_SIZE (128*1024)

namespace pkg {

namespace {

typedef zipfile::uint16 uint16;
typedef zipfile::uint32 uint32;

/** Read 16-bit unsigned integer from stream.
 * Storage is little-endian.
 * @param in the input stream
 * @return the integer
 */
uint16 read_16(std::istream& in)
{
	uint16 value=static_cast<uint16>(in.get());
	value|=static_cast<uint16>(in.get())<<8;
	return value;
}

/** Read 32-bit unsigned integer from stream.
 * Storage is little-endian.
 * @param in the input stream
 * @return the integer
 */
uint32 read_32(std::istream& in)
{
	uint32 value=static_cast<uint32>(in.get());
	value|=static_cast<uint32>(in.get())<<8;
	value|=static_cast<uint32>(in.get())<<16;
	value|=static_cast<uint32>(in.get())<<24;
	return value;
}

/** Read string from stream.
 * This function will always read the specified number of bytes from the
 * stream, however the string itself can be zero-terminated so may be shorter.
 * @param in the input stream
 * @param size the number of bytes to read
 * @return the string
 */
string read_string(std::istream& in,int size)
{
	string value(size,' ');
	for (int i=0;i!=size;++i)
		value[i]=in.get();
	string::size_type f=value.find(char(0));
	if (f!=string::npos) value.resize(f);
	return value;
}

class buffer
{
private:
	char* _data;
public:
	buffer(unsigned int size);
	~buffer();
	operator Bytef*()
		{ return reinterpret_cast<Bytef*>(_data); }
	operator char*()
		{ return _data; }
};

buffer::buffer(unsigned int size):
	_data(new char[size])
{}

buffer::~buffer()
{
	delete[] _data;
}

}; /* anonymous namespace */

zipfile::zipfile(const string& pathname):
	_pathname(pathname),
	_zfs(pathname.c_str(),
		std::ios_base::in|std::ios_base::out|std::ios_base::binary)
{
	_zfs.peek();
	while (_zfs&&!_zfs.eof())
	{
		uint32 sig=read_32(_zfs);
		switch (sig)
		{
		case 0x04034b50:
			_directory.push_back(new file_info(_zfs));
			break;
		default:
			_zfs.clear(std::fstream::eofbit);
			break;
		}
		_zfs.peek();
	}
}

zipfile::~zipfile()
{
	for (std::vector<file_info*>::iterator i=_directory.begin();
		i!=_directory.end();++i)
	{
		delete *i;
		*i=0;
	}
}

const zipfile::file_info& zipfile::operator[](unsigned int index) const
{
	return *_directory[index];
}

const zipfile::file_info* zipfile::find(const string& pathname) const
{
	for (unsigned int i=0;i!=_directory.size();++i)
		if (_directory[i]->pathname()==pathname) return _directory[i];
	return 0;
}

unsigned int zipfile::size() const
{
	return _directory.size();
}

void zipfile::extract(const string& src_pathname,
	const string& dst_pathname) const
{
	//std::cout << "Unpacking " << src_pathname << " to " << dst_pathname << std::endl;
	// Find file information record, report error if it does not exist.
	const file_info* finfo=find(src_pathname);
	if (!finfo) throw not_found(src_pathname);

	// Reset input stream state and seek to start of compressed data.
	_zfs.clear();
	_zfs.seekg(finfo->offset());

	// Create output stream.
	std::ofstream out(dst_pathname.c_str());
	// raise exceptions if the file didn't write OK
	out.exceptions ( std::ofstream::failbit | std::ofstream::badbit );

	// Allocate buffers.
	const unsigned int csize=UNZIP_BUFFER_SIZE;
	const unsigned int usize=UNZIP_BUFFER_SIZE;
	buffer cbuffer(csize);
	buffer ubuffer(usize);

	// Create and initialise zlib stream.
	z_stream zs;
	zs.zalloc=Z_NULL;
	zs.zfree=Z_NULL;
	zs.opaque=Z_NULL;
	zs.next_in=cbuffer;
	zs.avail_in=0;
	zs.total_in=0;
	zs.next_out=ubuffer;
	zs.avail_out=usize;
	zs.total_out=0;
	int err=inflateInit2(&zs,-MAX_WBITS);
	if (err!=Z_OK) throw zlib_error(err);

	while (err==Z_OK)
	{
		// If input buffer is empty then try to fill it.
		if (!zs.avail_in)
		{
			unsigned int count=std::min<int>(csize,finfo->csize()-zs.total_in);
			_zfs.read(cbuffer,count);
			zs.next_in=cbuffer;
			zs.avail_in=_zfs.gcount();
		}

		// If output buffer is full then empty it.
		if (!zs.avail_out)
		{
			try {
				out.write(ubuffer,zs.next_out-ubuffer);
			} catch (...) {
				// we failed, so remove the file and raise an error
				// don't use force_delete here, because if the file is locked
				// we don't want to kill a pre-existing file
				out.close();
				soft_delete(dst_pathname);
				throw std::runtime_error(string("Error when writing \"")+dst_pathname+string("\" (disc full?)"));
			}
			zs.next_out=ubuffer;
			zs.avail_out=usize;
		}

		// Decompress next chunk.
		switch (finfo->method())
		{
		case 0:
			{
				unsigned int count=std::min<int>(zs.avail_in,zs.avail_out);
				memcpy(zs.next_out,zs.next_in,count);
				zs.next_out+=count;
				zs.avail_out-=count;
				zs.total_out+=count;
				zs.next_in+=count;
				zs.avail_in-=count;
				zs.total_in+=count;
			}
			break;
		case 8:
			err=inflate(&zs,Z_NO_FLUSH);
			if (err<Z_OK) throw zlib_error(err);
			break;
		default:
			throw unsupported_compression_method(finfo->method());
		}

		// If there is space in the output buffer, and nothing left to
		// place in the input buffer, then change the return code to
		// Z_STREAM_END because the end has been reached.
		if (zs.avail_out&&(finfo->csize()==zs.total_in)) err=Z_STREAM_END;
	}

	// Finalise zlib stream.
	inflateEnd(&zs);

	// Flush output buffer.
	if (zs.avail_out!=usize)
	{
		try {
			out.write(ubuffer,zs.next_out-ubuffer);
		} catch (...) {
			// we failed, so remove the file and raise an error
			// don't use force_delete here, because if the file is locked
			// we don't want to kill a pre-existing file
			out.close();
			soft_delete(dst_pathname);
			throw std::runtime_error(string("Error when writing \"")+dst_pathname+string("\" (disc full?)"));
		}
	}
}

zipfile::file_info::file_info():
	_offset(0),
	_xversion(0),
	_gpbits(0),
	_method(0),
	_modtime(0),
	_moddate(0),
	_crc32(0),
	_csize(0),
	_usize(0)
{}

zipfile::file_info::file_info(std::istream& in)
{
	read(in);
}

zipfile::file_info::~file_info()
{
	for (std::map<uint16,extra_info*>::iterator i=_extra.begin();
		i!=_extra.end();++i)
	{
		delete i->second;
		i->second=0;
	}
}

void zipfile::file_info::read(std::istream& in)
{
	_xversion=read_16(in);
	_gpbits=read_16(in);
	_method=read_16(in);
	_modtime=read_16(in);
	_moddate=read_16(in);
	_crc32=read_32(in);
	_csize=read_32(in);
	_usize=read_32(in);
	uint16 fn_length=read_16(in);
	uint16 ex_length=read_16(in);
	_pathname=read_string(in,fn_length);
	read_extra(in,ex_length);
	_offset=in.tellg();
	in.seekg(_offset+_csize);
}

void zipfile::file_info::read_extra(std::istream& in,int length)
{
	uint32 base=in.tellg();
	uint32 offset=base;
	while (offset+4<=base+length)
	{
		uint16 tag=read_16(in);
		uint16 length2=4+read_16(in);
		if (offset+length2<=base+length)
		{
			switch (tag)
			{
			case 0x4341:
				*create_extra<riscos_info>()=riscos_info(in);
				break;
			}
		}
		offset+=length2;
		in.seekg(offset);
	}
	in.seekg(base+length);
}

zipfile::extra_info::extra_info()
{}

zipfile::extra_info::~extra_info()
{}

zipfile::riscos_info::riscos_info():
	_sig(0),
	_loadaddr(0),
	_execaddr(0),
	_attr(0)
{}

zipfile::riscos_info::riscos_info(std::istream& in)
{
	read(in);
}

zipfile::riscos_info::~riscos_info()
{}

void zipfile::riscos_info::read(std::istream& in)
{
	_sig=read_32(in);
	_loadaddr=read_32(in);
	_execaddr=read_32(in);
	_attr=read_32(in);
	read_32(in);
}

zipfile::not_found::not_found(const string& pathname):
	runtime_error(string("\"")+pathname+string("\" not found in zip file"))
{}

zipfile::unsupported_compression_method::unsupported_compression_method(
	unsigned int method):
	runtime_error("unsupported compression method")
{}

zipfile::zlib_error::zlib_error(int code):
	runtime_error(make_message(code))
{}

const char* zipfile::zlib_error::make_message(int code)
{
	const char* msg;
	switch (code)
	{
	case Z_VERSION_ERROR:
		msg="version error";
		break;
	case Z_BUF_ERROR:
		msg="buffer error";
		break;
	case Z_MEM_ERROR:
		msg="memory error";
		break;
	case Z_DATA_ERROR:
		msg="data error";
		break;
	case Z_STREAM_ERROR:
		msg="stream error";
		break;
	case Z_ERRNO:
		msg="filesystem error";
		break;
	case Z_OK:
		msg="ok";
		break;
	case Z_STREAM_END:
		msg="end of stream";
		break;
	case Z_NEED_DICT:
		msg="dictionary needed";
		break;
	default:
		if (code<0) msg="unrecognised zlib error";
		else msg="unrecognised zlib return code";
		break;
	}
	return msg;
}

}; /* namespace pkg */
