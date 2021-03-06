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

#ifndef LIBPKG_FILESYSTEM
#define LIBPKG_FILESYSTEM

#include <string>

namespace pkg {

using std::string;

/** Canonicalise pathname.
 * @param pathname the pathname to be canonicalised
 * @return the canonicalised pathname
 */
string canonicalise(const string& pathname);

/** Delete file without regard for file attributes.
 * If the file attributes prevent deletion then they are changed.
 * It is not an error if the file does not exist.
 * It is an error if the file cannot be deleted (after attempting
 * to change the file attributes).
 * @param pathname the pathname of the file to be deleted
 */
void force_delete(const string& pathname);

/** Delete file without changing file attributes
 * It is not an error if the file does not exist.
 * It is not an error if the file cannot be deleted (due to attributes
 * or other reasons).
 * @param pathname the pathname of the file to be deleted
 */
void soft_delete(const string& pathname);

/** Move file without regard for file attributes.
 * If the file attributes prevent movement then they are changed for the
 * duration of the operation then restored afterwards.
 * If the overwrite flag is set then an existng file at the destination
 * will be overwritten, also without regard for its file attributes.
 * It is an error if the source file does not exist, or cannot be moved
 * (after changing its attributes if necessary).
 * It is not an error if overwrite is requested but the destination does
 * not exist.
 * The source and destination must be on the same disc.
 * @param src_pathname the pathname of the file to be moved.
 * @param dst_pathname the pathname the file is to be moved to.
 * @param overwrite true if overwriting permitted, otherwise false.
 */
void force_move(const string& src_pathname,const string& dst_pathname,
	bool overwrite=false);

/** Recursively copy object.
 * @param src_pathname the pathname of the object to be moved.
 * @param dst_pathname the pathname the object is to be moved to.
 */
void copy_object(const string& src_pathname,const string& dst_pathname);

/** Create directory.
 * @param pathname the required pathname
 */
void create_directory(const string& pathname);

/** Write file information.
 * A filetype and timestamp may be given in place of a load address and
 * execution address by encoding them in the normal way.
 * @param pathname the pathname of the file
 * @param loadaddr the required load address
 * @param execaddr the required execution address
 * @param attr the required file attributes
 */
void write_file_info(const string& pathname,unsigned int loadaddr,
	unsigned int execaddr,unsigned int attr);

/** Write filetype.
 * @param pathname the pathname of the file
 * @param filetype the required filetype
 */
void write_filetype(const string& pathname,unsigned int filetype);

/** Get object type.
 * @param pathname the pathname
 * @return the object type
 */
unsigned int object_type(const string& pathname);

/** Get object length.
 * @param pathname the pathname
 * @return the object length
 */
unsigned int object_length(const string& pathname);

/** Get version of pathname made relative the the boot drive
 * @param pathname the pathname
 * @return boot relative pathname or original if not on the boot drive
 */
std::string boot_drive_relative(const string& pathname);

}; /* namespace pkg */

#endif
