#!/bin/sh
PATH=./build:$PATH LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./build/plugins/:libov/tascar/plugins/build/ $DBGTOOL ./build/ov-client $*
