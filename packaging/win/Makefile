ALL_CSV=$(wildcard *.csv)

FULLVERSION=$(shell cd ../../libov && ./get_version.sh)
PACKVERSION=$(FULLVERSION)-windows

all:
	for pkg in $(ALL_CSV); do \
		./copy_files.sh $${pkg} $(PACKVERSION); \
	done

#\
#	(cd tascar; find /d/ -name "Adwaita*"; find /d/ -name "glib-2.0"; find /d/ -name "hicolor"; find /d/ -name "librsvg-2-2.dll";echo "---";echo "---";ls /ucrt64/;echo "packaging completed.";)

ver:
	@echo "Packaging: $(PACKVERSION)"

