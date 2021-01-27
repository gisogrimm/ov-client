#!/usr/bin/env bash

cmake zita/zita-resampler-1.6.2
cd zita/zita-resampler-1.6.2 && cmake . && make


cd zita/zita-njbridge-0.4.4 && cmake . && make

OPENSSL_ROOT=$(brew --prefix openssl) CPPREST_ROOT=$(brew --prefix cpprestsdk) BOOST_ROOT=$(brew --prefix boost) make