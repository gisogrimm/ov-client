all:

VERSION=$(shell cat ../../libov/.ov_version)
MINORVERSION=$(shell cat ../../libov/.ov_minor_version)
COMMITVER=$(shell cat ../../libov/.ov_commitver)
PACKCNT=$(shell git rev-list --count $(shell git log --follow -1 --pretty=%H ../../libov)..HEAD)
FULLVERSION=$(VERSION).$(MINORVERSION)-$(PACKCNT)-$(COMMITVER)
#FULLVERSION=$(shell cat ../../libov/.ov_full_version)
PACKVER:=$(FULLVERSION)_$(shell uname -s)_$(shell uname -m)

.PHONY : pack

BINS = ov-client ovbox zita-j2n zita-n2j

portable:
	for i in $(BINS); do ./mkport.sh ../../build/$$i; done

pack: portable
	./pack.sh ov-client $(PACKVER)
