# This file is part of !RiscPkg.
# Copyright © 2003 Graham Shaw.            
# Distribution and use are subject to the GNU General Public License,
# a copy of which may be found in the file !RiscPkg.Copyright.

include ../Rules.make

OBJ = auto_dir.o \
 filesystem.o \
 version.o \
 dependency.o \
 control.o \
 binary_control.o \
 status.o \
 table.o \
 status_table.o \
 binary_control_table.o \
 source_table.o \
 zipfile.o \
 download.o \
 thread.o

all: $(OBJ)

include $(OBJ:.o=.d)
