#!/bin/sh
PATH=./build:./build/zita:$PATH LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./build/plugins:./tascar/plugins/build/ ./build/ov-client $*