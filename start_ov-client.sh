#!/bin/sh
PATH=./build:$PATH LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./libov/tascar/plugins/build/ ./build/ov-client $*
