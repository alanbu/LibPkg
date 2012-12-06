# This file is part of !RiscPkg.
# Copyright © 2003-2012 Graham Shaw.
# Distribution and use are subject to the GNU General Public License,
# a copy of which may be found in the file !RiscPkg.Copyright.

CPPFLAGS = -Izlib:zlib -Ilibcurl: -Ilibpkg_build:
CXXFLAGS = -mthrowback -munixlib -mpoke-function-name \
 -Wall -W -Wno-unused -Wno-uninitialized -O2

include fixdeps:Rules/make

OBJ = exception.o \
      os.o

.PHONY: all clean

all: timestamp

timestamp: $(OBJ)
	touch timestamp

clean:
	rm -f $(OBJ) $(OBJ:.o=.d)

include $(OBJ:.o=.d)
