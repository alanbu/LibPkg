# This file is part of LibPkg.
#
# Copyright 2003-2020 Graham Shaw
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
	cp -f libpkg.a $(GCCSDK_INSTALL_ENV)/lib/
	mkdir -p $(GCCSDK_INSTALL_ENV)/include/libpkg/os
	cp -f libpkg/*.h libpkg/os/*.h $(GCCSDK_INSTALL_ENV)/include/libpkg
	cp -f libpkg/os/*.h $(GCCSDK_INSTALL_ENV)/include/libpkg/os

