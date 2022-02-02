#!/bin/sh
PLUGINDIR=$(realpath ./libov/tascar/plugins/build/)
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:libov/tascar/libtascar/build/:${PLUGINDIR}
export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:$LD_LIBRARY_PATH
PATH=./build:$PATH LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./build/plugins/:./build/libov/tascar/plugins/:libov/tascar/plugins/build/:libov/tascar/libtascar/build/ $DBGTOOL ./build/ovbox $*
