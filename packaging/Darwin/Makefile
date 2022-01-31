all:

VERSION=$(shell cat ../../libov/.ov_version)
MINORVERSION=$(shell cat ../../libov/.ov_minor_version)
FULLVERSION=$(shell cat ../../libov/.ov_full_version)
PACKVER:=$(FULLVERSION)_$(shell uname -s)_$(shell uname -m)

.PHONY : pack

BINS = ov-client ovbox zita-j2n zita-n2j

portable:
	for i in $(BINS); do ./mkport.sh ../../build/$$i; done

pack: portable
	./pack.sh ov-client $(PACKVER)
