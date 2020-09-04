# This file is part of the LibPkg.
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
