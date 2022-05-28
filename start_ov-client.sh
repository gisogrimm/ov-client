#!/bin/sh
PLUGINDIR=$(realpath ./libov/tascar/plugins/build/)
TSCLIBDIR=$(realpath ./libov/tascar/libtascar/build/)
export LD_LIBRARY_PATH="${TSCLIBDIR}:${PLUGINDIR}:${LD_LIBRARY_PATH}"
export DYLD_LIBRARY_PATH="${TSCLIBDIR}:${PLUGINDIR}:${DYLD_LIBRARY_PATH}"
PATH=./build:$PATH $DBGTOOL ./build/ov-client $*
