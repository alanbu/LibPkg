# This file is part of !LibPkg.
# Copyright © 2003 Graham Shaw.
# Distribution and use are subject to the GNU Lesser General Public License,
# a copy of which may be found in the file !LibPkg.Copyright.

AR = libfile

bin: always
	make -C libpkg
	mkdir -p a
	$(AR) -c a.libpkg libpkg.o.*

doc: always
	doxygen

all: bin doc

always:
