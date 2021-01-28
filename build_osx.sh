#!/usr/bin/env bash

(cd zita/zita-resampler-1.6.2 && cmake . && make)
(cd zita/zita-njbridge-0.4.4 && cmake . && make && cp zita-n2j ../../build/ && cp zita-j2n ../../build/)

OPENSSL_ROOT=$(brew --prefix openssl) CPPREST_ROOT=$(brew --prefix cpprestsdk) BOOST_ROOT=$(brew --prefix boost) make