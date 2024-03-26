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
# copy files:
cat "$1"|(cd ../.. && sed -e "/#.*/ d" -e "s|\(.*\),\(.*\)|cp -r \1 ${STARTDIR}/${BASENAME}/\2|1")
cat "$1"|(cd ../.. && sed -e "/#.*/ d" -e "s|\(.*\),\(.*\)|cp -r \1 ${STARTDIR}/${BASENAME}/\2|1"|bash)
# package files:
zip -r "${BASENAME}-$2.zip" "${BASENAME}"
