#!/usr/bin/env bash
set -euo pipefail

OPENSSL_ROOT=$(brew --prefix openssl) CPPREST_ROOT=$(brew --prefix cpprestsdk) BOOST_ROOT=$(brew --prefix boost) NLOHMANN_JSON_ROOT=$(brew --prefix nlohmann-json) make
(cd zita/zita-resampler-1.6.2 && cmake . && make)
(cd zita/zita-njbridge-0.4.4 && cmake . && make && cp zita-n2j ../../ && cp zita-j2n ../../)

./start_ov-client.sh -f ds
