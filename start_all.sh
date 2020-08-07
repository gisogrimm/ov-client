#!/bin/bash
CODIR=$(dirname `which $0`)
(
    echo $CODIR
    cd $CODIR
    export LD_LIBRARY_PATH=/usr/local/lib
    # make sure this is the first step, so we can fix anything later remotely:
    git pull || (sleep 20 ; git pull)
    # compile binary tools:
    make
		./build/ov-client
)
