FROM ubuntu:20.10

RUN DEBIAN_FRONTEND=noninteractive apt update --assume-yes
RUN DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends --assume-yes build-essential git ca-certificates make xxd wget
RUN DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends --assume-yes git zita-njbridge liblo-dev nodejs libcurl4-openssl-dev build-essential libasound2-dev libeigen3-dev libfftw3-dev libfftw3-double3 libfftw3-single3 libgsl-dev libjack-jackd2-dev libltc-dev libmatio-dev libsndfile1-dev libsamplerate0-dev libboost-all-dev libxml++2.6-dev libcpprest-dev nlohmann-json3-dev libxerces-c-dev

RUN mkdir /build
RUN cd /build && git clone https://github.com/gisogrimm/ov-client
RUN cd /build/ov-client && git submodule update --init --recursive

COPY entrypoint.sh /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
