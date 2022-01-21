#!/bin/sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./build/plugins/:./build/libov/tascar/plugins/:libov/tascar/plugins/build/:libov/tascar/libtascar/build/
export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:$LD_LIBRARY_PATH
PATH=./build:$PATH LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./build/plugins/:./build/libov/tascar/plugins/:libov/tascar/plugins/build/:libov/tascar/libtascar/build/ $DBGTOOL ./build/ovbox $*
