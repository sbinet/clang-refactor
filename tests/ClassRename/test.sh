#!/bin/sh

set -e

/bin/cp foo.orig.h foo.h
/bin/cp foo.orig.cpp foo.cpp
cmake \
    -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON \
    .
#sed -e 's@clang++@g++@g' -i compile_commands.json 
make
make rebuild_cache
make clean

#/bin/rm -rf compile_commands.json; touch compile_commands.json
set +e
clang-refactor < test.yml

diff -urN foo.orig.h foo.h
diff -urN foo.orig.cpp foo.cpp

touch foo.h foo.cpp
make
