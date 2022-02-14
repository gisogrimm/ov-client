#!/bin/sh
PKG="$1"
VER="$2"
if test -z "${PKG}"; then
    echo "empty package name"
    return 1
fi
if test -z "${VER}"; then
    echo "empty version tag"
    return 1
fi
if ! test -e "${PKG}.csv"; then
    echo "package definition ${PKG}.csv not found"
    return 1
fi
rm -Rf "${PKG}.app" "${PKG}_${VER}.tgz"
mkdir "${PKG}.app"
cat "${PKG}.csv" | sed -e "s/[^,]*,[[:blank:]]*/${PKG}.app\//1" -e 's/\/\.\//\//1' -e 's/\/$//1' |sort -u|xargs -L 1 -- mkdir -p
cat "${PKG}.csv" | sed -e "s/\([^,]*\),[[:blank:]]*/..\/..\/\1 ${PKG}.app\//1" -e 's/\/\.\//\//1' -e 's/\/$//1'|xargs -I % -- sh -c "cp -a %"
tar czf "${PKG}_${VER}.tgz" "${PKG}.app"
rm -Rf "${PKG}.app"
