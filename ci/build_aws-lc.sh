#!/bin/sh -e
# build aws-lc (for GitHub workflow)

git clone --depth 1 -b "${AWSLC_VERSION}" https://github.com/aws/aws-lc
cd aws-lc
cmake -B work \
      -DDISABLE_GO=ON \
      --install-prefix $PWD/build
make -j"$(nproc 2> /dev/null || sysctl -n hw.ncpu)" -C work
cmake --install work
