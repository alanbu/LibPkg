# This file is part of !RiscPkg.
# Copyright © 2003 Graham Shaw.            
# Distribution and use are subject to the GNU General Public License,
# a copy of which may be found in the file !RiscPkg.Copyright.

CXX = gcc
LD = gcc
FIXDEPS = fixdeps

CPPFLAGS = -Irtk: -Ilibcurl: -Ilibpkg:
CXXFLAGS = -mthrowback -munixlib -mpoke-function-name -Wall -W -Wno-unused -O2

.SUFFIXES: .o .cc .d .dd

.cc.o:
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<

.cc.dd:
	$(CXX) -MM $(CPPFLAGS) $< > $@

.dd.d:
	$(FIXDEPS) $* < $< > $@

.DELETE_ON_ERROR:
