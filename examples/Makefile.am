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
AM_CFLAGS = $(WARNCFLAGS) $(DEBUGCFLAGS)
AM_CXXFLAGS = $(WARNCXXFLAGS) $(DEBUGCFLAGS)
AM_CPPFLAGS = \
	-I$(top_srcdir)/lib/includes \
	-I$(top_builddir)/lib/includes \
	-I$(top_srcdir)/third-party/urlparse \
	@LIBEV_CFLAGS@ \
	@LIBNGHTTP3_CFLAGS@ \
	@OPENSSL_CFLAGS@ \
	@JEMALLOC_CFLAGS@ \
	@DEFS@ \
	@EXTRA_DEFS@
AM_LDFLAGS = -no-install \
	@LIBTOOL_LDFLAGS@
LDADD = $(top_builddir)/lib/libdwnx.la \
	$(top_builddir)/third-party/liburlparse.la \
	@LIBEV_LIBS@ \
	@LIBNGHTTP3_LIBS@ \
	@OPENSSL_LIBS@ \
	@JEMALLOC_LIBS@

SERVER_SRCS = \
	server_base.cc server_base.h \
	template.h \
	debug.cc debug.h \
	util.cc util.h \
	util_openssl.cc \
	shared.cc shared.h \
	http.cc http.h \
	network.h

CLIENT_BASE_SRCS = \
	client_base.cc client_base.h \
	template.h \
	debug.cc debug.h \
	util.cc util.h \
	util_openssl.cc \
	shared.cc shared.h \
	network.h

noinst_PROGRAMS = h3client h3server hqclient hqserver

h3server_CPPFLAGS = ${AM_CPPFLAGS} -DWITH_EXAMPLE_HTTP3_PROTO_CODEC
h3server_SOURCES = \
	${SERVER_SRCS} \
	server.cc server.h \
	http3_server_proto_codec.cc http3_server_proto_codec.h

h3client_CPPFLAGS = ${AM_CPPFLAGS} -DWITH_EXAMPLE_HTTP3_PROTO_CODEC
h3client_SOURCES = \
	${CLIENT_BASE_SRCS} \
	client.cc client.h \
	http3_client_proto_codec.cc http3_client_proto_codec.h

hqserver_CPPFLAGS = ${AM_CPPFLAGS} -DWITH_EXAMPLE_HQ_PROTO_CODEC \
	-I$(top_srcdir)/third-party
hqserver_LDADD = ${LDADD} \
	$(top_builddir)/third-party/libhttp-parser.la
hqserver_SOURCES = \
	${SERVER_SRCS} \
	server.cc server.h \
	hq_server_proto_codec.cc hq_server_proto_codec.h

hqclient_CPPFLAGS = ${AM_CPPFLAGS} -DWITH_EXAMPLE_HQ_PROTO_CODEC
hqclient_SOURCES = \
	${CLIENT_BASE_SRCS} \
	client.cc client.h \
	hq_client_proto_codec.cc hq_client_proto_codec.h
