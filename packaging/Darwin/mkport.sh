#!/bin/sh
BIN="$1"
BINDIR=$(dirname "$BIN")
SCRIPT=`pwd`/$0
SCRIPT=$(echo $SCRIPT | sed -e 's/.*\/\//\//1' -e 's/\/\.\//\//1')
echo $SCRIPT
(
    cd ${BINDIR}
    echo "cd  ${BINDIR}; current dir:"
    pwd
    BINBASE=$(echo "$BIN" | awk -F'/' '{print $NF}')
#    mkdir -p lib
    for lib in `otool -L $BINBASE | sed -e "/dylib/! d" -e "/@loader_path/ d" -e "s/[[:blank:]]*\//\//1" -e "s/dylib .*/dylib/1" -e "/\//! d"`; do
	OBJ_LEAF_NAME=$(echo "$lib" | awk -F'/' '{print $NF}')
	if test "$OBJ_LEAF_NAME" != "$BINBASE"; then
	    if test ! -e "${OBJ_LEAF_NAME}"; then
		echo "copying $lib to ./"
		cp $lib ./
		if test -e "${OBJ_LEAF_NAME}"; then
		    echo "replacing  $lib by ${OBJ_LEAF_NAME} in ${BINBASE}"
		    install_name_tool -change "$lib" "${OBJ_LEAF_NAME}" "${BINBASE}"
		    $SCRIPT "${OBJ_LEAF_NAME}"
		fi
	    fi
	fi
    done
)
