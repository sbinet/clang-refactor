#!/bin/sh

set -e

/bin/cp foo.orig.h foo.h
/bin/cp foo.orig.cpp foo.cpp
cmake \
    -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON \
    .
make
make rebuild_cache
make clean

clang-refactorial < test.yml

set +e
diff -urN foo.orig.h foo.h
diff -urN foo.orig.cpp foo.cpp

set -e
touch foo.h foo.cpp
make
