# This file is part of !LibPkg.
# Copyright © 2003-2004 Graham Shaw.
# Distribution and use are subject to the GNU Lesser General Public License,
# a copy of which may be found in the file !LibPkg.Copyright.

AR = ar

DOXYFILES = \
 Doxyfile \
 footer.html \
 mainpage.h \
 namespace.h

.PHONY: bin doc all always clean

bin: libpkg.a

doc: html/index.html

all: bin doc

libpkg.a: libpkg/timestamp
#	mkdir -p a
	$(AR) -ru libpkg.a $(wildcard libpkg/*.o) $(wildcard libpkg/os/*.o)
#	$(AR) -o a.libpkg

libpkg/timestamp: always
	make -C libpkg
	make -C libpkg/os

html/index.html: libpkg/timestamp $(DOXYFILES)
	doxygen

clean:
	make -C libpkg clean
	make -C libpkg/os clean

# Install for GCCSDK cross compiler
install:
	cp libpkg.a $(GCCSDK_INSTALL_ENV)/lib/
	mkdir -p $(GCCSDK_INSTALL_ENV)/include/libpkg/os
	cp libpkg/*.h libpkg/os/*.h $(GCCSDK_INSTALL_ENV)/include/libpkg
	cp libpkg/os/*.h $(GCCSDK_INSTALL_ENV)/include/libpkg/os

