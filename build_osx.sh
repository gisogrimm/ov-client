#!/usr/bin/env bash

set -e
git submodule update --init --recursive
mkdir -p build
cd build
OPENSSL_ROOT_DIR=$(brew --prefix openssl) CPPREST_ROOT=$(brew --prefix cpprestsdk) BOOST_ROOT=$(brew --prefix boost) NLOHMANN_JSON_ROOT=$(brew --prefix nlohmann-json) cmake ./..
make
mv zita/zita-n2j .
mv zita/zita-j2n .