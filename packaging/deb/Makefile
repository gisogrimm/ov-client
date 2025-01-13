all:

VERSION=$(shell cat ../../libov/.ov_version)
MINORVERSION=$(shell cat ../../libov/.ov_minor_version)
COMMITVER=$(shell cat ../../libov/.ov_commitver)
PACKCNT=$(shell git rev-list --count $(shell git log --follow -1 --pretty=%H ../../libov)..HEAD)
FULLVERSION=$(VERSION).$(MINORVERSION)-$(PACKCNT)-$(COMMITVER)
#FULLVERSION=$(shell cat ../../libov/.ov_full_version)
CODENAME:=$(shell lsb_release -sc)
REPO=../..
PACKVER:=$(FULLVERSION)-$(shell lsb_release -si)$(shell lsb_release -sr)
DEBS:=$(subst .csv,,$(wildcard *.csv))
STORAGE:=debian/$(CODENAME)

pack:
	@test -n "$(VERSION)" && \
	test -n "$(MINORVERSION)" && \
	test -n "$(COMMITVER)" && \
	test -n "$(FULLVERSION)" && \
	echo "Packaging:" && \
	echo " - Package version: $(PACKVER)" && \
	echo " - Packages: $(DEBS)" && \
	rm -Rf debian && \
	mkdir -p $(STORAGE) && \
	for p in $(DEBS); do rm -f $(REPO)/$${p}*.deb; (cd $(REPO); ./packaging/deb/htchdebian-mkdeb packaging/deb/$${p}.csv $(PACKVER)) && mv $(REPO)/$${p}*.deb $(STORAGE)/;done

clipack:
	@test -n "$(VERSION)" && \
	test -n "$(MINORVERSION)" && \
	test -n "$(COMMITVER)" && \
	test -n "$(FULLVERSION)" && \
	echo "Packaging:" && \
	echo " - Package version: $(PACKVER)" && \
	echo " - Packages: $(DEBS)" && \
	rm -Rf debian && \
	mkdir -p $(STORAGE) && \
	for p in ovbox-cli; do rm -f $(REPO)/$${p}*.deb; (cd $(REPO); ./packaging/deb/htchdebian-mkdeb packaging/deb/$${p}.csv $(PACKVER)) && mv $(REPO)/$${p}*.deb $(STORAGE)/;done

testallpackaged:
	rm -f $@; (cd ../.. && for i in libov/tascar/plugins/build/ovclienttascar*.so; do grep -q -e $$i packaging/deb/*.csv || (touch packaging/deb/$@;echo "$$i is not packaged");done); ! (test -e $@ && rm -f $@)
