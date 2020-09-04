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


ifeq ($(GCCSDK_INSTALL_ENV),)
# Native compile
CPPFLAGS = -Izlib:zlib -Ilibcurl: -Ilibpkg_build:  
else
# Cross compiling with GCCSDK Autobuilder
CPPFLAGS = -I..
endif

CXXFLAGS = -mthrowback -munixlib -mpoke-function-name \
 -Wall -W -Wno-unused -Wno-uninitialized -O2 -std=c++0x

ifeq ($(GCCSDK_INSTALL_ENV),)
include fixdeps:Rules/make
else
%.dd: %.cc
	$(CC) -M $(CPPFLAGS) $< > $@

%.d: %.dd
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $< > $@
endif

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
 commit.o \
 log.o \
 component.o \
 component_update.o \
 boot_options_file.o \
 module_info.o \
 trigger.o \
 triggers.o \
 env_checker.o \
 env_checks.o \
 env_packages_table.o


.PHONY: all clean

all: timestamp

timestamp: $(OBJ)
	touch timestamp

clean:
	rm -f $(OBJ) $(OBJ:.o=.d)

include $(OBJ:.o=.d)
