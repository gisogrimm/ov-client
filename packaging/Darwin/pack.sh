#!/bin/sh
PKG="$1"
if test -z "${PKG}"; then
    echo "empty package name"
    return 1
fi
if ! test -e "${PKG}.csv"; then
    echo "package definition ${PKG}.csv not found"
    return 1
fi
rm -Rf "${PKG}" "${PKG}_`uname -s`_`uname -m`.tgz"
mkdir "${PKG}"
cat "${PKG}.csv" | sed -e "s/[^,]*,[[:blank:]]*/${PKG}\//1" -e 's/\/\.\//\//1' -e 's/\/$//1' |sort -u|xargs -L 1 -- mkdir -p
cat "${PKG}.csv" | sed -e "s/\([^,]*\),[[:blank:]]*/..\/..\/\1 ${PKG}\//1" -e 's/\/\.\//\//1' -e 's/\/$//1'|xargs -I % -- sh -c "cp -ar %"
tar czf "${PKG}_`uname -s`_`uname -m`.tgz" "${PKG}"
rm -Rf "${PKG}"
