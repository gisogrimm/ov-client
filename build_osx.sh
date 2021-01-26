#!/usr/bin/env bash

OPENSSL_ROOT=$(brew --prefix openssl) CPPREST_ROOT=$(brew --prefix cpprestsdk) BOOST_ROOT=$(brew --prefix boost) make
