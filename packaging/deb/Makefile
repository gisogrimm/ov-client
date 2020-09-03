all:

CODENAME:=$(shell lsb_release -sc)
REPO=../..
TSCVER:=$(shell $(REPO)/build/ovc_tascar_ver)
PACKVER:=$(VERSION).$(MINORVERSION)-$(TSCVER)-$(COMMIT)$(COMMITMOD)-$(shell lsb_release -si)$(shell lsb_release -sr)
#DEBS:=$($(wildcard *.csv):.csv=)
DEBS:=$(subst .csv,,$(wildcard *.csv))
STORAGE:=debian/$(CODENAME)

pack:
	test -n "$(VERSION)"
	test -n "$(MINORVERSION)"
	@echo "Packaging:"
	@echo " - TASCAR version: $(TSCVER)"
	@echo " - Package version: $(PACKVER)"
	@echo " - Packages: $(DEBS)"
	rm -Rf debian
	mkdir -p $(STORAGE)
	for p in $(DEBS); do rm -f $(REPO)/$${p}*.deb; (cd $(REPO); TSCVER=$(TSCVER) mhamakedeb packaging/deb/$${p}.csv $(PACKVER)) && mv $(REPO)/$${p}*.deb $(STORAGE)/;done
