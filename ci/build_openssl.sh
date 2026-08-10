#!/bin/sh -e
# build patched openssl (for GitHub workflow)

BRANCH="openssl-${OSSL_VERSION}"
REPO="https://github.com/openssl/openssl"

git clone --depth 1 -b "${BRANCH}" "${REPO}" ossl
cd ossl
./config --prefix=$PWD/build --openssldir=/etc/ssl
make -j"$(nproc 2> /dev/null || sysctl -n hw.ncpu)"
make install_sw
