FROM fedora

RUN yum -y install git make gcc g++ pkg-config libxml++-devel jack-audio-connection-kit-devel liblo-devel libsndfile-devel fftw-devel gsl-devel eigen3-devel boost-devel libsamplerate-devel alsa-lib-devel libcurl-devel xerces-c-devel json-devel gtkmm30-devel cairomm-devel cmake

RUN mkdir -p /build
RUN cd /build && git clone https://github.com/gisogrimm/ov-client
RUN cd /build/ov-client && git submodule update --init --recursive

COPY entrypoint.sh /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
