# ----------------------------------------------------------------------------
#
#  Copyright (C) 2006-2014 Fons Adriaensen <fons@linuxaudio.org>
#    
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# ----------------------------------------------------------------------------


# Modify as required.
#
PREFIX = /usr/local
#SUFFIX := $(shell uname -m | sed -e 's/^unknown/$//' -e 's/^i.86/$//' -e 's/^x86_64/$/64/')
LIBDIR = lib$(SUFFIX)

MAJVERS = 1
MINVERS = 6.0
VERSION = $(MAJVERS).$(MINVERS)
DISTDIR = zita-resampler-$(VERSION)


CPPFLAGS += -I. -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS
CXXFLAGS += -Wall -dynamic -O2 -ffast-math
CXXFLAGS += -march=native
LDFLAGS += 
LDLIBS +=

ZITA-RESAMPLER_l = libzita-resampler
ZITA-RESAMPLER_SO = $(ZITA-RESAMPLER_l).dylib
ZITA-RESAMPLER_MAJ = $(ZITA-RESAMPLER_l).$(MAJVERS).dylib
ZITA-RESAMPLER_MIN = $(ZITA-RESAMPLER_l).$(MAJVERS).$(MINVERS).dylib
ZITA-RESAMPLER_DEP =
ZITA-RESAMPLER_O = resampler.o vresampler.o resampler-table.o cresampler.o
ZITA-RESAMPLER_H = zita-resampler/resampler.h zita-resampler/resampler-table.h \
	zita-resampler/vresampler.h zita-resampler/cresampler.h


$(ZITA-RESAMPLER_MIN): $(ZITA-RESAMPLER_O)
	g++ -dynamiclib -flat_namespace \
	-current_version $(MAJVERS).$(MINVERS) \
	-compatibility_version $(MAJVERS) \
	-o $(ZITA-RESAMPLER_MIN) \
	$(ZITA-RESAMPLER_O) $(ZITA-RESAMPLER_DEP) $(LDFLAGS)	


$(ZITA-RESAMPLER_O):	$(ZITA-RESAMPLER_H)


install:	$(ZITA-RESAMPLER_MIN)
	install -d $(DESTDIR)$(PREFIX)/include/zita-resampler
	install -d $(DESTDIR)$(PREFIX)/$(LIBDIR)
	install -m 644 $(ZITA-RESAMPLER_H) $(DESTDIR)$(PREFIX)/include/zita-resampler
	install -m 644 $(ZITA-RESAMPLER_MIN) $(DESTDIR)$(PREFIX)/$(LIBDIR)
	ln -sf $(ZITA-RESAMPLER_MIN) $(DESTDIR)$(PREFIX)/$(LIBDIR)/$(ZITA-RESAMPLER_MAJ)
	ln -sf $(ZITA-RESAMPLER_MIN) $(DESTDIR)$(PREFIX)/$(LIBDIR)/$(ZITA-RESAMPLER_SO)

uninstall:
	/bin/rm -rf $(DESTDIR)$(PREFIX)/include/zita-resampler
	/bin/rm -rf $(DESTDIR)$(PREFIX)/$(LIBDIR)/libzita-resampler*

clean:
	/bin/rm -rf *~ *.o *.a *.d *.so.* *.so *.dylib zita-resampler/*~ 

