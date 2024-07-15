#!/bin/bash
(
    # minimal error handling:
    trap "echo An error occured.;exit 1" ERR

    export DEBIAN_FRONTEND=noninteractive

    echo "update apt database:"
    sudo -E apt-get update --assume-yes

    echo "Install CA certificates:"
    sudo -E apt-get install --no-install-recommends --assume-yes ca-certificates

    echo "install git, build-essential and other basic stuff:"
    sudo -E apt-get install --no-install-recommends --assume-yes git build-essential xxd gettext-base lsb-release

    echo "install dependencies of cli tool:"
    sudo -E apt-get install --no-install-recommends --assume-yes liblo-dev libcurl4-openssl-dev libasound2-dev libeigen3-dev libfftw3-dev libfftw3-single3 libgsl-dev libjack-jackd2-dev libltc-dev libmatio-dev libsndfile1-dev libsamplerate0-dev nlohmann-json3-dev libxerces-c-dev libgtkmm-3.0-dev libcairomm-1.0-dev

    echo "clone or update git repo:"
    (cd ov-client && git clean -ffx && git pull) || git clone https://github.com/gisogrimm/ov-client

    echo "move to development version:"
    (cd ov-client && git clean -ffx && git checkout development && git pull)

    echo "update submodules:"
    make -C ov-client gitupdate

    echo "clean repo:"
    make -C ov-client clean

    echo "build tools:"
    make -j 5 -C ov-client

    echo "package tools:"
    make -C ov-client packaging

    echo "install new debian packages:"
    sudo -E apt-get install --assume-yes --no-install-recommends ./ov-client/packaging/deb/debian/*/ovbox-cli_*.deb

    echo "successfully installed ovbox system!"
)
