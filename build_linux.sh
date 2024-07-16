#! /bin/sh

mkdir -p build
cmake -B build $@
cmake --build build --config Release
