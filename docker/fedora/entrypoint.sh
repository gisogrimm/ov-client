#!/bin/sh
cd /build
# update / clone ov-client:
test -e ov-client || git clone https://github.com/gisogrimm/ov-client
(
    cd ov-client
    git pull
    git checkout $1
    make
)
