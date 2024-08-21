all:

VERSION=$(shell cat ../../libov/.ov_version)
MINORVERSION=$(shell cat ../../libov/.ov_minor_version)
COMMITVER=$(shell cat ../../libov/.ov_commitver)
PACKCNT=$(shell git rev-list --count $(shell git log --follow -1 --pretty=%H ../../libov)..HEAD)
FULLVERSION=$(VERSION).$(MINORVERSION)-$(PACKCNT)-$(COMMITVER)
#FULLVERSION=$(shell cat ../../libov/.ov_full_version)
PACKVER:=$(FULLVERSION)_$(shell sw_vers -productName)-$(shell sw_vers -productVersion)_$(shell uname -m)

.PHONY : pack

BINS = ov-client ovbox ovzita-j2n ovzita-n2j ovrealpath
LIBS = libovclienttascar.0.dylib libovclienttascargui.0.dylib
PLUGS = $(shell (cd ../../libov/tascar/plugins/build;echo ovclient*.dylib))

plug:
	echo $(PLUGS)

portable:
	rm -Rf bin;mkdir -p bin/lib ;\
	for i in $(BINS); do cp ../../build/$$i bin/ ;./mkport.sh ./bin/$$i; done; \
	for i in $(LIBS); do cp ../../libov/tascar/libtascar/build/$$i bin/lib/ ;./mkport.sh ./bin/lib/$$i; done; \
	for i in $(PLUGS); do cp ../../libov/tascar/plugins/build/$$i bin/lib/ ;./mkport.sh ./bin/lib/$$i ./bin/lib/; done; \
	mv bin/ovbox bin/ovbox.bin; \
	cp ovboxwrapper bin/ovbox; \
	chmod a+x bin/ovbox

pack: portable
	./pack.sh ovbox $(PACKVER)
