#!/bin/sh
PATH=build:$PATH LD_LIBRARY_PATH=./tascar/plugins/build/ ./build/ov-client $*
