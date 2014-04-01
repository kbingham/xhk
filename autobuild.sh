#!/bin/sh

./autogen.sh
automake
mkdir -p build
cd build
../configure
make

