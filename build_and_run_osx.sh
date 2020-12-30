#!/usr/bin/env bash
set -euo pipefail

OPENSSL_ROOT=$(brew --prefix openssl) make
./start_ov-client.sh -f ds

