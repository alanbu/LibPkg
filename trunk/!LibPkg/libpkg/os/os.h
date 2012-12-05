// This file is part of the RISC OS Toolkit (RTK).
// Copyright © 2003 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !RTK.Copyright.
//
// Cut down version to use with LibPkg created by Alan Buckley
// to remove the RTK dependency.

#ifndef _LPPKG_OS_OS

namespace libpkg {
namespace os {

struct file_info
{
	unsigned int loadaddr;
	unsigned int execaddr;
	unsigned int length;
	unsigned int attr;
	unsigned int objtype;
	unsigned int filetype;
	char name[];
};

/** Read byte from CMOS RAM.
 * @param address the address to read
 * @param _value a buffer for the returned value
 */
void OS_Byte161(int address,int* _value);

/** Write catalogue information.
 * @param name the object name
 * @param loadaddr the required load address
 * @param execaddr the required execution address
 * @param attr the required file attributes
 */
void OS_File1(const char* name,unsigned int loadaddr,unsigned int execaddr,
	unsigned int attr);

/** Write file attributes.
 * @param name the object name
 * @param attr the required file attributes
 */
void OS_File4(const char* name,unsigned int attr);

/** Delete object.
 * @param name the object name
 * @param _objtype a buffer for the returned object type
 * @param _loadaddr a buffer for the returned load address
 * @param _execaddr a buffer for the returned execution address
 * @param _length a buffer for the returned object length
 * @param _attr a buffer for the returned object attributes
 */
void OS_File6(const char* name,unsigned int* _objtype,unsigned int* _loadaddr,
	unsigned int* _execaddr,unsigned int* _length,unsigned int* _attr);

/** Create directory.
 * @param name the object name
 * @param entries the initial number of entries, or 0 for default
 */
void OS_File8(const char* name,unsigned int entries);

/** Read catalogue information.
 * @param name the object name
 * @param _objtype a buffer for the returned object type
 * @param _loadaddr a buffer for the returned load address
 * @param _execaddr a buffer for the returned execution address
 * @param _length a buffer for the returned object length
 * @param _attr a buffer for the returned object attributes
 */
void OS_File17(const char* name,unsigned int* _objtype,unsigned int* _loadaddr,
	unsigned int* _execaddr,unsigned int* _length,unsigned int* _attr);

/** Write filetype.
 * @param name the object name
 * @param filetype the required filetype
 */
void OS_File18(const char* name,unsigned int filetype);

/** Read EOF status.
 * @param handle the file handle
 * @param _eof a buffer for the returned EOF status (true=EOF)
 */
void OS_Args5(int handle,bool* _eof);

/** Open file.
 * @param code the reason code
 * @param name the object name
 * @param path the path (if any)
 * @param _handle a buffer for the returned file handle
 */
void OS_Find(int code,const char* name,const char* path,int* _handle);

/** Close file.
 * @param handle the file handle
 */
void OS_Find0(int handle);

/** Write bytes to file.
 * @param handle the file handle
 * @param buffer the data to be written
 * @param count the number of bytes to be written
 * @param _fp a buffer for the returned file pointer
 */
void OS_GBPB2(int handle,const void* buffer,unsigned int count,
	unsigned int* _fp);

/** Read bytes from file.
 * @param handle the file handle
 * @param buffer a buffer for the data to be read
 * @param count the number of bytes to be read
 * @param _excess a buffer for the returned number of bytes not transferred
 * @param _fp a buffer for the returned file pointer
 */
void OS_GBPB4(int handle,void* buffer,unsigned int count,
	unsigned int* _excess,unsigned int* _fp);

/** Read catalogue information from directory.
 * @param name the directory pathname
 * @param buffer a buffer for the catalogue entries to be read
 * @param count the number of catalogue entries to be read
 * @param offset the offset at which to begin
 * @param length the length of the buffer
 * @param pattern the pattern to match
 * @param _count a buffer for the returned number of directory entries read
 * @param _offset a buffer for the returned offset, or -1 if finished
 */
void OS_GBPB12(const char* name,void* buffer,unsigned int count,
	int offset,unsigned int length,const char* pattern,
	unsigned int* _count,int* _offset);

/** Set system variable.
 * @param varname the variable name
 * @param value the required value
 * @param length the length of the required value
 * @param context the context pointer, or 0 if none
 * @param vartype the variable type
 * @param _context a buffer for the returned context pointer
 * @param _vartype a buffer for the returned variable type
 */
void OS_SetVarVal(const char* varname,const char* value,unsigned int length,
	unsigned int context,unsigned int vartype,unsigned int* _context,
	unsigned int* _vartype);

/** Rename object.
 * @param src_name the source name
 * @param dst_name the destination name
 */
void OS_FSControl25(const char* src_name,const char* dst_name);

/** Copy objects.
 * @param src_name the source name
 * @param dst_name the destination name
 * @param mask a bitmap describing the required action
 * @param start_time the optional inclusive start time
 * @param end_time the optional inclusive end time
 * @param extra_info an optional pointer to extra information
 */
void OS_FSControl26(const char* src_name,const char* dst_name,
	unsigned int mask,unsigned long long start_time,
	unsigned long long end_time,void* extra_info);

/** Canonicalise pathname.
 * @param name the object name
 * @param buffer a buffer for the result
 * @param pathvar the name of a system variable containing the path
 * @param path the path to use if path_variable is null or non-existant
 * @param size the size of the buffer
 * @param _size the space remaining after the result (but the terminator)
 *  has been placed in the buffer
 */
void OS_FSControl37(const char* name,char* buffer,const char* pathvar,
	const char* path,unsigned int size,unsigned int* _size);

/** Read mode variable.
 * @param index the variable number
 * @param _value a buffer for the returned value
 */
void OS_ReadModeVariable(int index,int* _value);

/** Read monotonic time.
 * @param _time a buffer for the returned time (in centiseconds)
 */
void OS_ReadMonotonicTime(unsigned int* _time);

} /* namespace os */
} /* namespace libpkg */

#endif
