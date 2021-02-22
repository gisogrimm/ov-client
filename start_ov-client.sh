#!/bin/sh
PATH=./build:$PATH LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./build/plugins ./build/ov-client $*
