FROM archlinux

RUN pacman --noconfirm -Syu git make gcc pkg-config libxml++2.6 jack2 liblo libsndfile fftw gsl eigen boost

RUN mkdir /build
RUN cd /build && git clone https://github.com/gisogrimm/ov-client

COPY entrypoint.sh /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
