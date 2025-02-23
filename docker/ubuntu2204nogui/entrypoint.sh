#!/bin/sh
cd /build/ov-client
# build and package command line tools:
make -j $(nproc) cli && make -C packaging/deb clipack
