# This file is part of !RiscPkg.
# Copyright © 2003-2005 Graham Shaw.            
# Distribution and use are subject to the GNU General Public License,
# a copy of which may be found in the file !RiscPkg.Copyright.

CPPFLAGS = -Irtk: -Izlib:zlib -Ilibcurl: -Ilibpkg_build:
CXXFLAGS = -mthrowback -munixlib -mpoke-function-name \
 -Wall -W -Wno-unused -Wno-uninitialized -O2

include fixdeps:Rules/make

OBJ = md5.o \
 auto_dir.o \
 filesystem.o \
 dirstream.o \
 sprite_file.o \
 uri.o \
 version.o \
 dependency.o \
 control.o \
 binary_control.o \
 status.o \
 table.o \
 status_table.o \
 binary_control_table.o \
 source_table.o \
 path_table.o \
 pkgbase.o \
 zipfile.o \
 download.o \
 standards_version.o \
 thread.o \
 update.o \
 unpack.o \
 sysvars.o \
 sprite_pool.o \
 commit.o

.PHONY: all

all: timestamp

timestamp: $(OBJ)
	touch timestamp

include $(OBJ:.o=.d)
