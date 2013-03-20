#!/bin/sh

set -e

/bin/cp foo.orig.h foo.h
/bin/cp foo.orig.cpp foo.cpp
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON .

clang-refactor < test.yml

diff -urN foo.orig.h foo.h
diff -urN foo.orig.cpp foo.cpp
