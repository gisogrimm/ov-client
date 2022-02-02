#!/bin/sh
BIN="$1"
BINDIR=$(realpath $(dirname "$BIN"))
if test -z "$2"; then
    LIBDIR=${BINDIR}/lib
    echo mkdir -p "${LIBDIR}"
    mkdir -p "${LIBDIR}"
else
    LIBDIR="$2"
fi
SCRIPT=$(realpath $(pwd)/$0)
BINBASE=$(echo "$BIN" | awk -F'/' '{print $NF}')
#
for lib in `otool -L $BINBASE | sed -e "/dylib/! d" -e "/@loader_path/ d" -e "s/[[:blank:]]*\//\//1" -e "s/dylib .*/dylib/1" -e "/\//! d"`; do
    OBJ_LEAF_NAME=$(echo "$lib" | awk -F'/' '{print $NF}')
    if test "$OBJ_LEAF_NAME" != "$BINBASE"; then
	if test ! -e "${LIBDIR}/${OBJ_LEAF_NAME}"; then
            if test -e "${lib}"; then
	        echo "copying $lib to ${LIBDIR}"
	        cp "$lib" "${LIBDIR}"
	        $SCRIPT "${LIBDIR}/${OBJ_LEAF_NAME}" "${LIBDIR}"
            fi
	fi
	if test -e "${LIBDIR}/${OBJ_LEAF_NAME}"; then
	    echo "replacing $lib by @loader_path/lib/${OBJ_LEAF_NAME} in ${BINBASE}"
	    install_name_tool -change "$lib" "@loader_path/lib/${OBJ_LEAF_NAME}" "${BINBASE}"
	fi
    fi
done
