#!/bin/sh
SRCPATH=$(realpath $(dirname $0))
echo "$SRCPATH"
PLUGINDIR="${SRCPATH}/libov/tascar/plugins/build/"
TSCLIBDIR="${SRCPATH}/libov/tascar/libtascar/build/"
export LD_LIBRARY_PATH="${TSCLIBDIR}:${PLUGINDIR}:${LD_LIBRARY_PATH}"
export DYLD_LIBRARY_PATH="${TSCLIBDIR}:${PLUGINDIR}:${DYLD_LIBRARY_PATH}"
PATH="${SRCPATH}/build:${PATH}" $DBGTOOL "${SRCPATH}/build/ovbox" $*
