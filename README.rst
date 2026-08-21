dwnx
====

dwnx (pronounced "down-mix") is a `QMux
<https://datatracker.ietf.org/doc/html/draft-ietf-quic-qmux>`_
implementation in C.

It closely follows the API design of the original QUIC implementation
`ngtcp2 <https://github.com/ngtcp2/ngtcp2>`_.

Documentation
-------------

`Online documentation <https://nghttp2.org/dwnx/>`_ is available.

Requirements
------------

The libdwnx C library itself does not depend on any external
libraries.  It requires a C11 compiler to build.  The example client,
and server are written in C++23, and should compile with the modern
C++ compilers.

The following packages are required to configure the build system:

- pkg-config >= 0.20
- autoconf
- automake
- autotools-dev
- libtool

To build sources under the examples directory, libev, a TLS library
(aws-lc or OpenSSL), and nghttp3 are required:

- libev
- `nghttp3 <https://github.com/ngtcp2/nghttp3>`_ for HTTP/3
- `aws-lc <https://github.com/aws/aws-lc>`_ or `OpenSSL
  <https://github.com/openssl/openssl/>`_.

Build
-----

.. code-block:: shell

   $ git clone --recursive https://github.com/ngtcp2/nghttp3
   $ cd nghttp3
   $ autoreconf -i
   $ ./configure --prefix=$PWD/build --enable-lib-only
   $ make -j$(nproc) check
   $ make install
   $ cd ..
   $ git clone --recursive https://github.com/ngtcp2/dwnx
   $ cd dwnx
   $ autoreconf -i
   $ # For Mac users who have installed libev with MacPorts, append
   $ # LIBEV_CFLAGS="-I/opt/local/include" LIBEV_LIBS="-L/opt/local/lib -lev"
   $ ./configure PKG_CONFIG_PATH=$PWD/../nghttp3/build/lib/pkgconfig
   $ make -j$(nproc) check

License
-------

The MIT License

Copyright (c) 2026 dwnx contributors
