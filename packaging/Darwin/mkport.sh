#!/bin/sh
BIN="$1"
BINDIR=$(dirname "$BIN")
(
    cd ${BINDIR}
    echo "cd  ${BINDIR}; current dir:"
    pwd
    BINBASE=$(echo "$BIN" | awk -F'/' '{print $NF}')
    mkdir -p lib
    for lib in `otool -L $BINBASE | sed -e "/dylib/! d" -e "/@loader_path/ d" -e "s/[[:blank:]]*\//\//1" -e "s/dylib .*/dylib/1" -e "/\//! d"`; do
	echo "copying $lib to lib"
	cp $lib lib/
	OBJ_LEAF_NAME=$(echo "$lib" | awk -F'/' '{print $NF}')
	if test -e "${LIBDIR}/${OBJ_LEAF_NAME}"; then
	    echo "replacing  $lib by lib/${OBJ_LEAF_NAME} in ${BINBASE}"
	    install_name_tool -change "$lib" "lib/${OBJ_LEAF_NAME}" "${BINBASE}"
	fi
    done
    chmod u+w lib/*
)
