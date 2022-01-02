FROM archlinux

# WORKAROUND for glibc 2.33 and old Docker
# See https://github.com/actions/virtual-environments/issues/2658
# Thanks to https://github.com/lxqt/lxqt-panel/pull/1562
RUN patched_glibc=glibc-linux4-2.33-4-x86_64.pkg.tar.zst && \
    curl -LO "https://repo.archlinuxcn.org/x86_64/$patched_glibc" && \
    bsdtar -C / -xvf "$patched_glibc"

RUN pacman --noconfirm -Syu git make gcc pkg-config jack2 liblo libsndfile fftw gsl eigen boost xerces-c nlohmann-json libxml++2.6 gtkmm3 cmake

RUN mkdir /build
RUN cd /build && git clone https://github.com/gisogrimm/ov-client
RUN cd /build/ov-client && git submodule update --init --recursive

COPY entrypoint.sh /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
