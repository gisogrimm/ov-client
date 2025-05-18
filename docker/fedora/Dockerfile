FROM fedora

RUN yum -y install git make gcc g++ pkg-config libxml++-devel jack-audio-connection-kit-devel liblo-devel libsndfile-devel fftw-devel gsl-devel eigen3-devel boost-devel libsamplerate-devel alsa-lib-devel libcurl-devel xerces-c-devel json-devel gtkmm30-devel cairomm-devel cmake cunit-devel libltc-devel libsodium-devel xxd matio-devel

COPY entrypoint.sh /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
