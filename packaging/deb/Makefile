all:

VERSION=$(shell cat ../../libov/.ov_version)
MINORVERSION=$(shell cat ../../libov/.ov_minor_version)
FULLVERSION=$(shell cat ../../libov/.ov_full_version)
CODENAME:=$(shell lsb_release -sc)
REPO=../..
PACKVER:=$(FULLVERSION)-$(shell lsb_release -si)$(shell lsb_release -sr)
DEBS:=$(subst .csv,,$(wildcard *.csv))
STORAGE:=debian/$(CODENAME)

pack:
	test -n "$(VERSION)"
	test -n "$(MINORVERSION)"
	test -n "$(FULLVERSION)"
	@echo "Packaging:"
	@echo " - Package version: $(PACKVER)"
	@echo " - Packages: $(DEBS)"
	rm -Rf debian
	mkdir -p $(STORAGE)
	for p in $(DEBS); do rm -f $(REPO)/$${p}*.deb; (cd $(REPO); mhamakedeb packaging/deb/$${p}.csv $(PACKVER)) && mv $(REPO)/$${p}*.deb $(STORAGE)/;done
