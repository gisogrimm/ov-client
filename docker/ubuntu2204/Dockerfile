FROM ubuntu:22.04

RUN DEBIAN_FRONTEND=noninteractive apt update --assume-yes

RUN DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends --assume-yes gpgv ca-certificates wget

RUN DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends --assume-yes build-essential git make xxd gettext-base lsb-release file grep dpkg-dev sed

RUN DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends --assume-yes zita-njbridge liblo-dev libcurl4-openssl-dev libasound2-dev libeigen3-dev libfftw3-dev libfftw3-double3 libfftw3-single3 libgsl-dev libjack-jackd2-dev libltc-dev libmatio-dev libsndfile1-dev libsamplerate0-dev nlohmann-json3-dev libxerces-c-dev cmake libcunit1-dev libsodium-dev

RUN DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends --assume-yes libgtkmm-3.0-dev libcairomm-1.0-dev

RUN mkdir /build

COPY entrypoint.sh /entrypoint.sh

USER 1000:1000

