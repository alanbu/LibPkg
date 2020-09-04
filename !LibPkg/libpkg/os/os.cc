// This file is part of the LibPkg.
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
//
// Cut down version to use with LibPkg created by Alan Buckley
// to remove the RTK dependency.

#include <kernel.h>
#include "call_swi.h"
#include "os.h"
#include "osswi.h"

namespace pkg {
namespace os {

void OS_Byte161(int address,int* _value)
{
	_kernel_swi_regs regs;
	regs.r[0]=161;
	regs.r[1]=address;
	call_swi(swi::OS_Byte,&regs);
	if (_value) *_value=regs.r[2];
}

void OS_File1(const char* name,unsigned int loadaddr,unsigned int execaddr,
	unsigned int attr)
{
	_kernel_swi_regs regs;
	regs.r[0]=1;
	regs.r[1]=(int)name;
	regs.r[2]=loadaddr;
	regs.r[3]=execaddr;
	regs.r[5]=attr;
	call_swi(swi::OS_File,&regs);
}

void OS_File4(const char* name,unsigned int attr)
{
	_kernel_swi_regs regs;
	regs.r[0]=4;
	regs.r[1]=(int)name;
	regs.r[5]=attr;
	call_swi(swi::OS_File,&regs);
}

void OS_File6(const char* name,unsigned int* _objtype,unsigned int* _loadaddr,
	unsigned int* _execaddr,unsigned int* _length,unsigned int* _attr)
{
	_kernel_swi_regs regs;
	regs.r[0]=6;
	regs.r[1]=(int)name;
	call_swi(swi::OS_File,&regs);
	if (_objtype) *_objtype=regs.r[0];
	if (_loadaddr) *_loadaddr=regs.r[2];
	if (_execaddr) *_execaddr=regs.r[3];
	if (_length) *_length=regs.r[4];
	if (_attr) *_attr=regs.r[5];
}

void OS_File8(const char* name,unsigned int entries)
{
	_kernel_swi_regs regs;
	regs.r[0]=8;
	regs.r[1]=(int)name;
	regs.r[4]=entries;
	call_swi(swi::OS_File,&regs);
}

void OS_File17(const char* name,unsigned int* _objtype,unsigned int* _loadaddr,
	unsigned int* _execaddr,unsigned int* _length,unsigned int* _attr)
{
	_kernel_swi_regs regs;
	regs.r[0]=17;
	regs.r[1]=(int)name;
	call_swi(swi::OS_File,&regs);
	if (_objtype) *_objtype=regs.r[0];
	if (_loadaddr) *_loadaddr=regs.r[2];
	if (_execaddr) *_execaddr=regs.r[3];
	if (_length) *_length=regs.r[4];
	if (_attr) *_attr=regs.r[5];
}

void OS_File18(const char* name,unsigned int filetype)
{
	_kernel_swi_regs regs;
	regs.r[0]=18;
	regs.r[1]=(int)name;
	regs.r[2]=filetype;
	call_swi(swi::OS_File,&regs);
}

void OS_Args5(int handle,bool* _eof)
{
	_kernel_swi_regs regs;
	regs.r[0]=5;
	regs.r[1]=handle;
	call_swi(swi::OS_Args,&regs);
	if (_eof) *_eof=regs.r[2]!=0;
}

void OS_Find(int code,const char* name,const char* path,int* _handle)
{
	_kernel_swi_regs regs;
	regs.r[0]=code;
	regs.r[1]=(int)name;
	regs.r[2]=(int)path;
	call_swi(swi::OS_Find,&regs);
	if (_handle) *_handle=regs.r[0];
}

void OS_Find0(int handle)
{
	_kernel_swi_regs regs;
	regs.r[0]=0;
	regs.r[1]=handle;
	call_swi(swi::OS_Find,&regs);
}

void OS_GBPB2(int handle,const void* buffer,unsigned int count,
	unsigned int* _fp)
{
	_kernel_swi_regs regs;
	regs.r[0]=2;
	regs.r[1]=handle;
	regs.r[2]=(int)buffer;
	regs.r[3]=count;
	call_swi(swi::OS_GBPB,&regs);
	if (_fp) *_fp=regs.r[4];
}

void OS_GBPB4(int handle,void* buffer,unsigned int count,
	unsigned int* _excess,unsigned int* _fp)
{
	_kernel_swi_regs regs;
	regs.r[0]=4;
	regs.r[1]=handle;
	regs.r[2]=(int)buffer;
	regs.r[3]=count;
	call_swi(swi::OS_GBPB,&regs);
	if (_excess) *_excess=regs.r[3];
	if (_fp) *_fp=regs.r[4];
}

void OS_GBPB12(const char* name,void* buffer,unsigned int count,
	int offset,unsigned int length,const char* pattern,
	unsigned int* _count,int* _offset)
{
	_kernel_swi_regs regs;
	regs.r[0]=12;
	regs.r[1]=(int)name;
	regs.r[2]=(int)buffer;
	regs.r[3]=count;
	regs.r[4]=offset;
	regs.r[5]=length;
	regs.r[6]=(int)pattern;
	call_swi(swi::OS_GBPB,&regs);
	if (_count) *_count=regs.r[3];
	if (_offset) *_offset=regs.r[4];
}

void OS_SetVarVal(const char* varname,const char* value,unsigned int length,
	unsigned int context,unsigned int vartype,unsigned int* _context,
	unsigned int* _vartype)
{
	_kernel_swi_regs regs;
	regs.r[0]=(int)varname;
	regs.r[1]=(int)value;
	regs.r[2]=length;
	regs.r[3]=context;
	regs.r[4]=vartype;
	call_swi(swi::OS_SetVarVal,&regs);
	if (_context) *_context=regs.r[3];
	if (_vartype) *_vartype=regs.r[4];
}

void OS_FSControl25(const char* src_name,const char* dst_name)
{
	_kernel_swi_regs regs;
	regs.r[0]=25;
	regs.r[1]=(int)src_name;
	regs.r[2]=(int)dst_name;
	call_swi(swi::OS_FSControl,&regs);
}

void OS_FSControl26(const char* src_name,const char* dst_name,
	unsigned int mask,unsigned long long start_time,
	unsigned long long end_time,void* extra_info)
{
	_kernel_swi_regs regs;
	regs.r[0]=26;
	regs.r[1]=(int)src_name;
	regs.r[2]=(int)dst_name;
	regs.r[3]=mask;
	regs.r[4]=start_time>>0;
	regs.r[5]=start_time>>32;
	regs.r[6]=end_time>>0;
	regs.r[7]=end_time>>32;
	regs.r[8]=(int)extra_info;
	call_swi(swi::OS_FSControl,&regs);
}

void OS_FSControl37(const char* pathname,char* buffer,const char* pathvar,
	const char* path,unsigned int size,unsigned int* _size)
{
	_kernel_swi_regs regs;
	regs.r[0]=37;
	regs.r[1]=(int)pathname;
	regs.r[2]=(int)buffer;
	regs.r[3]=(int)pathvar;
	regs.r[4]=(int)path;
	regs.r[5]=size;
	call_swi(swi::OS_FSControl,&regs);
	if (_size) *_size=regs.r[5];
}

void OS_ReadModeVariable(int index,int* _value)
{
	_kernel_swi_regs regs;
	regs.r[0]=-1;
	regs.r[1]=index;
	call_swi(swi::OS_ReadModeVariable,&regs);
	if (_value) *_value=regs.r[2];
}

void OS_ReadMonotonicTime(unsigned int* _time)
{
	_kernel_swi_regs regs;
	call_swi(swi::OS_ReadMonotonicTime,&regs);
	if (_time) *_time=regs.r[0];
}

void OS_CLI(const char* command)
{
	_kernel_swi_regs regs;
	regs.r[0]=(int)command;
	call_swi(swi::OS_CLI,&regs);
}

} /* namespace os */
} /* namespace pkg */
