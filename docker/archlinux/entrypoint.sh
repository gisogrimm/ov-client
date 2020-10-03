#!/bin/sh
cd /build
# update / clone TASCAR:
test -e tascar || git clone https://github.com/gisogrimm/tascar
(
    cd tascar
    git pull
    # build TASCAR:
    make -j 4
    # install TASCAR:
    make install
)
# update / clone ov-client:
test -e ov-clientr || git clone https://github.com/gisogrimm/ov-client
(
    cd ov-client
    git pull
    make
)
