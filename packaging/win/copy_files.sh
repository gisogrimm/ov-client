#!/bin/bash
if ! test -f "$1"; then
    echo "File \"$1\" not found"
    exit
fi
if test -z "$2"; then
    echo "Version name is empty"
    exit
fi
# get base name:
BASENAME=$(basename "$1"| sed 's/\.csv//1')
echo "Packaging into \"${BASENAME}\""
STARTDIR=$(pwd)
rm -Rf "${BASENAME}" "${BASENAME}-$2.zip"
# create subfolders:
cat "$1"|sed -e "/#.*/ d" -e  "s/.*,/${BASENAME}\//1"|sort -u|xargs -l mkdir -p
# show files:
cat "$1"|(cd ../.. && sed -e "/^[ \t]*$/ d" -e "/#.*/ d" -e "s|\(.*\),\(.*\)|cp -r \1 ${STARTDIR}/${BASENAME}/\2|1")
# copy files:
cat "$1"|(cd ../.. && sed -e "/^[ \t]*$/ d" -e "/#.*/ d" -e "s|\(.*\),\(.*\)|cp -r \1 ${STARTDIR}/${BASENAME}/\2|1"|bash)

# make portable package:
MSYSFILES=$(cygcheck ${BASENAME}/bin/* ${BASENAME}/lib/gdk-pixbuf-2.0/*/*.dll 2>/dev/null|grep msys64|sed 's/^ *//1' | sort -bu)
cp ${MSYSFILES} ${BASENAME}/bin/
glib-compile-schemas ${BASENAME}/bin/share/glib-2.0/schemas

# package files:
zip -r "${BASENAME}-$2.zip" "${BASENAME}"
