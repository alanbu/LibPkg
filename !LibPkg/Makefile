# This file is part of !LibPkg.
# Copyright © 2003-2004 Graham Shaw.
# Distribution and use are subject to the GNU Lesser General Public License,
# a copy of which may be found in the file !LibPkg.Copyright.

AR = libfile

DOXYFILES = \
 Doxyfile \
 footer.html \
 mainpage.h \
 namespace.h

.PHONY: bin doc all always

bin: libpkg.a

doc: html/index.html

all: bin doc

libpkg.a: libpkg/timestamp
	mkdir -p a
	$(AR) -c a.libpkg libpkg.o.*

libpkg/timestamp: always
	make -C libpkg

html/index.html: libpkg/timestamp $(DOXYFILES)
	doxygen
