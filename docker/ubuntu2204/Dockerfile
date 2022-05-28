FROM ubuntu:22.04

RUN DEBIAN_FRONTEND=noninteractive apt update --assume-yes

RUN DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends --assume-yes gpgv

RUN DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends --assume-yes build-essential git ca-certificates make xxd wget

RUN DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends --assume-yes git zita-njbridge liblo-dev nodejs libcurl4-openssl-dev build-essential libasound2-dev libeigen3-dev libfftw3-dev libfftw3-double3 libfftw3-single3 libgsl-dev libjack-jackd2-dev libltc-dev libmatio-dev libsndfile1-dev libsamplerate0-dev libboost-all-dev libxml++2.6-dev libcpprest-dev nlohmann-json3-dev libgtkmm-3.0-dev libcairomm-1.0-dev

RUN DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends --assume-yes libxerces-c-dev cmake gnupg2 software-properties-common

RUN wget -qO- https://apt.hoertech.de/openmha-packaging.pub | apt-key add - && apt-add-repository 'deb http://apt.hoertech.de/ jammy universe'

RUN DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends --assume-yes mhamakedeb

RUN mkdir /build
RUN cd /build && git clone https://github.com/gisogrimm/ov-client
RUN cd /build/ov-client && git submodule update --init --recursive

COPY entrypoint.sh /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
