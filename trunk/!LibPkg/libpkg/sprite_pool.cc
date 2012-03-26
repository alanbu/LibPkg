// This file is part of LibPkg.
// Copyright © 2004-2012 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include <fstream>

#include "rtk/os/os.h"

#include "libpkg/filesystem.h"
#include "libpkg/dirstream.h"
#include "libpkg/sprite_file.h"
#include "libpkg/pkgbase.h"
#include "libpkg/sprite_pool.h"

namespace {

using namespace pkg;

/** Copy sprite from pathname to sprite file using suffix list.
 * The suffix list must end with the empty string (which is both the
 * list terminator and the suffix of last resort).
 * @param dst the destination sprite file
 * @param base_pathname the source pathname with no suffix
 * @param suffix_list the suffix list
 * @param name the sprite name
 */
void copy_sprite(sprite_file& dst,const string& base_pathname,
	const char** suffix_list,const string& name)
{
	// Choose suffix.
	unsigned int index=0;
	string pathname=base_pathname+suffix_list[index];
	while (!object_type(pathname)&&*suffix_list[index])
	{
		++index;
		pathname=base_pathname+suffix_list[index];
	}

	// Copy sprite.
	sprite_file src(pathname);
	dst.copy(src,name);
}

/** Build boot-time sprite file.
 * The suffix list must begin with the suffix of the sprite file to be
 * generated (which is also the suffix of first resort) and must end
 * with the empty string (which is both the list terminator and the suffix
 * of last resort).
 * @param pb the package database
 * @param suffix_list the suffix list
 */
void build_sprite_file(pkgbase& pb,const char** suffix_list)
{
	// Set pathnames.
	string dst_pathname=pb.bootsprites_pathname()+string(suffix_list[0]);
	string tmp_pathname=dst_pathname+string("++");
	string bak_pathname=dst_pathname+string("--");

	// Ensure that temporary file does not exist.
	force_delete(tmp_pathname);

	// If sprites directory exists then process each file within it.
	bool empty=true;
	if (object_type(pb.sprites_pathname())!=0)
	{
		sprite_file dst(tmp_pathname,true);
		dirstream ds(pb.sprites_pathname());
		while (ds)
		{
			dirstream::object obj;
			ds >> obj;

			if (obj.filetype==0xfff)
			{
				// If file is a text file then use file content as pathname.
				string ref_pathname=
					pb.sprites_pathname()+string(".")+obj.name;
				std::ifstream in(ref_pathname.c_str());
				string base_pathname;
				getline(in,base_pathname);
				base_pathname=resolve_pathrefs(pb.paths(),base_pathname);
				try
				{
					copy_sprite(dst,base_pathname,suffix_list,obj.name);
				}
				catch (...)
				{
					// Ignore error if sprite failed to copy.
					// (A file containing some of the sprites
					// is better than no sprite file.)
				}
			}
		}
		empty&=!dst.size();
	}

	// Backup existing sprites file if it exists.
	if (object_type(dst_pathname)!=0)
	{
		force_move(dst_pathname,bak_pathname,true);
	}

	// If new sprites file is non-empty then move it into position,
	// otherwsie delete it.
	if (!empty) force_move(tmp_pathname,dst_pathname,false);
	else force_delete(tmp_pathname);

	// Delete backup.
	force_delete(bak_pathname);
}

}; /* anonymous namespace */

namespace pkg {

void update_sprite_pool(pkgbase& pb)
{
	const char* suffix_list[]={"11","22",""};
	build_sprite_file(pb,suffix_list+0);
	build_sprite_file(pb,suffix_list+1);
	build_sprite_file(pb,suffix_list+2);
}

}; /* namespace pkg */
