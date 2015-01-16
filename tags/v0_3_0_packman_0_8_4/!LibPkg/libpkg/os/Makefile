# This file is part of !RiscPkg.
# Copyright © 2003-2012 Graham Shaw.
# Distribution and use are subject to the GNU General Public License,
# a copy of which may be found in the file !RiscPkg.Copyright.

ifeq ($(GCCSDK_INSTALL_ENV),)
# Native compile
CPPFLAGS = -Izlib:zlib -Ilibcurl: -Ilibpkg_build:  
else
# Cross compiling with GCCSDK Autobuilder
CPPFLAGS = -I..
endif

CXXFLAGS = -mthrowback -munixlib -mpoke-function-name \
 -Wall -W -Wno-unused -Wno-uninitialized -O2

ifeq ($(GCCSDK_INSTALL_ENV),)
include fixdeps:Rules/make
else
%.dd: %.cc
	$(CC) -M $(CPPFLAGS) $< > $@

%.d: %.dd
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $< > $@
endif

OBJ = exception.o \
      os.o

.PHONY: all clean

all: timestamp

timestamp: $(OBJ)
	touch timestamp

clean:
	rm -f $(OBJ) $(OBJ:.o=.d)

include $(OBJ:.o=.d)
