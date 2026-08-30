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
EXTRA_DIST = munit/COPYING munit/munitxx.h

check_PROGRAMS = main

OBJECTS = \
	dwnx_conn_test.c \
	dwnx_frame_test.c \
	dwnx_log_test.c \
	dwnx_test_helper.c \
	dwnx_transport_params_test.c \
	main.c \
	munit/munit.c

HFILES = \
	dwnx_conn_test.h \
	dwnx_frame_test.h \
	dwnx_log_test.h \
	dwnx_test_helper.h \
	dwnx_transport_params_test.h \
	munit/munit.h

main_SOURCES = $(HFILES) $(OBJECTS)

# With static lib disabled and symbol hiding enabled, we have to link
# object files directly because the tests use symbols not included in
# public API.
if ENABLE_SHARED
main_LDADD = ${top_builddir}/lib/.libs/*.o
else
main_LDADD = ${top_builddir}/lib/.libs/libdwnx.la
endif
main_LDFLAGS = -static

AM_CFLAGS = $(WARNCFLAGS) \
	-I${top_srcdir}/lib \
	-I${top_srcdir}/lib/includes \
	-I${top_srcdir}/tests/munit \
	-I${top_builddir}/lib/includes \
	-DBUILDING_DWNX \
	@DEFS@
AM_LDFLAGS = -no-install

TESTS = main
