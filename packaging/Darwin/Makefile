all:

VERSION=$(shell cat ../../libov/.ov_version)
MINORVERSION=$(shell cat ../../libov/.ov_minor_version)
COMMITVER=$(shell cat ../../libov/.ov_commitver)
PACKCNT=$(shell git rev-list --count $(shell git log --follow -1 --pretty=%H ../../libov)..HEAD)
FULLVERSION=$(VERSION).$(MINORVERSION)-$(PACKCNT)-$(COMMITVER)
#FULLVERSION=$(shell cat ../../libov/.ov_full_version)
PACKVER:=$(FULLVERSION)_$(shell uname -s)_$(shell uname -m)

.PHONY : pack

BINS = ov-client ovbox ovzita-j2n ovzita-n2j

portable:
	rm -Rf bin;mkdir -p bin ;for i in $(BINS); do cp ../../build/$$i bin/ ;./mkport.sh ./bin/$$i; done

pack: portable
	./pack.sh ovbox $(PACKVER)
