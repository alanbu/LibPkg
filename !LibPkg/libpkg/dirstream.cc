// This file is part of LibPkg.
//
// Copyright 2003-2020 Graham Shaw
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

#include "libpkg/os/os.h"

#include "libpkg/dirstream.h"

namespace pkg {

dirstream::dirstream(const string& pathname,const string& pattern):
	_buffer_size(0x100),
	_buffer(new char[_buffer_size]),
	_offset(0),
	_pathname(pathname),
	_pattern(pattern),
	_buffer_full(false)
{}

dirstream::~dirstream()
{
	delete[] _buffer;
}

dirstream::operator bool()
{
	fill_buffer();
	return _buffer_full;
}

dirstream& dirstream::operator>>(object& obj)
{
	fill_buffer();

	if (_buffer_full)
	{
		pkg::os::file_info& info=
			*reinterpret_cast<pkg::os::file_info*>(_buffer);
		obj.loadaddr=info.loadaddr;
		obj.execaddr=info.execaddr;
		obj.length=info.length;
		obj.attr=info.attr;
		obj.objtype=info.objtype;
		obj.filetype=info.filetype;
		obj.name=string(info.name);
		_buffer_full=false;
	}
	else
	{
		obj.loadaddr=0;
		obj.execaddr=0;
		obj.length=0;
		obj.attr=0;
		obj.objtype=0;
		obj.filetype=0;
		obj.name=string();
	}
	return *this;
}

void dirstream::fill_buffer()
{
	if (!_buffer_full)
	{
		unsigned int count=0;
		pkg::os::OS_GBPB12(_pathname.c_str(),_buffer,1,_offset,_buffer_size,
			_pattern.c_str(),&count,&_offset);
		_buffer_full=count;
	}
}

}; /* namespace pkg */
