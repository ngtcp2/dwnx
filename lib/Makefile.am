# dwnx

# Copyright (c) 2026 dwnx contributors

# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:

# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
SUBDIRS = includes

AM_CFLAGS = $(WARNCFLAGS) $(DEBUGCFLAGS) $(EXTRACFLAG)
AM_CPPFLAGS = -I$(srcdir)/includes -I$(builddir)/includes -DBUILDING_DWNX
AM_LDFLAGS = ${LIBTOOL_LDFLAGS}

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdwnx.pc
DISTCLEANFILES = $(pkgconfig_DATA)

lib_LTLIBRARIES = libdwnx.la

OBJECTS = \
	dwnx_conn.c \
	dwnx_transport_params.c \
	dwnx_mem.c \
	dwnx_record_reader.c \
	dwnx_vec.c \
	dwnx_frame.c \
	dwnx_conv.c \
	dwnx_str.c \
	dwnx_unreachable.c \
	dwnx_buf.c

HFILES = \
	dwnx_conn.h \
	dwnx_transport_params.h \
	dwnx_mem.h \
	dwnx_record_reader.h \
	dwnx_vec.h \
	dwnx_frame.h \
	dwnx_conv.h \
	dwnx_str.h \
	dwnx_unreachable.h \
	dwnx_buf.h \
	dwnx_net.h

libdwnx_la_SOURCES = $(HFILES) $(OBJECTS)
libdwnx_la_LDFLAGS = -no-undefined \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
