FROM debian:bullseye

RUN DEBIAN_FRONTEND=noninteractive apt update --assume-yes
RUN DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends --assume-yes build-essential git ca-certificates make xxd
RUN DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends --assume-yes libasound2-dev libboost-all-dev libcairomm-1.0-dev libcurl4-openssl-dev libeigen3-dev libfftw3-dev libfftw3-double3 libfftw3-single3 libgsl-dev libgtkmm-3.0-dev libgtksourceviewmm-3.0-dev libjack-jackd2-dev liblo-dev libltc-dev libmatio-dev libsndfile1-dev libwebkit2gtk-4.0-dev libxml++2.6-dev portaudio19-dev libsamplerate0-dev nlohmann-json3-dev libxerces-c-dev

RUN DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends --assume-yes libxerces-c-dev cmake gnupg2 software-properties-common wget

RUN wget -qO- https://apt.hoertech.de/openmha-packaging.pub | apt-key add - && apt-add-repository 'deb http://apt.hoertech.de/ bionic universe'

RUN DEBIAN_FRONTEND=noninteractive apt update --assume-yes
RUN DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends --assume-yes mhamakedeb

RUN mkdir /build
RUN cd /build && git clone https://github.com/gisogrimm/ov-client
RUN cd /build/ov-client && git submodule update --init --recursive

COPY entrypoint.sh /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
