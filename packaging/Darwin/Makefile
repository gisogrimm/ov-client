all:

.PHONY : pack

BINS = ov-client ovbox zita-j2n zita-n2j

portable:
	for i in $(BINS); do ./mkport.sh ../../build/$$i; done

pack: portable
	./pack.sh ov-client
