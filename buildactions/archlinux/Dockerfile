# use archlinux:
FROM archlinux

# install build dependencies:
RUN pacman --noconfirm -Syu git make gcc pkg-config libxml++2.6 jack2 liblo libsndfile fftw gsl eigen gtkmm3 boost libltc xxd gtksourceviewmm webkit2gtk

COPY entrypoint.sh /entrypoint.sh
RUN chmod a+x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
