#!/bin/bash -eu

FUZZERS=(
    read_write
)

for fuzzer in "${FUZZERS[@]}"; do
    $CXX $CXXFLAGS -std=c++20 -Ilib/includes \
         fuzz/${fuzzer}.cc -o $OUT/${fuzzer} \
         $LIB_FUZZING_ENGINE lib/.libs/libdwnx.a
done
