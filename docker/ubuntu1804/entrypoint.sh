#!/bin/sh
cd /build/ov-client
# build and package command line tools:
make -j $(nproc) && make -C packaging/deb pack
