// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef _LIBPKG_FILESYSTEM
#define _LIBPKG_FILESYSTEM

#include <string>

namespace pkg {

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

}; /* namespace pkg */

#endif
