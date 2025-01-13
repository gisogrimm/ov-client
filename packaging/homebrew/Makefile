.PHONY: install build

#ovbox project directory
REPO=../..

#include $(REPO)/config.mk

install: copyfiles

build:
	rm -rf homebrew-build-dir
	mkdir homebrew-build-dir
	$(MAKE) -C "$(REPO)" "DESTDIR=$$(realpath homebrew-build-dir)" install
	find "homebrew-build-dir/$(PREFIX)/" -type f -exec chmod 755 \{\} \;

copyfiles: build
	mkdir -p "$(DESTDIR)$(PREFIX)"
	cp -r "homebrew-build-dir/$(PREFIX)/"* "$(DESTDIR)$(PREFIX)"

clean:
	@rm -rf homebrew-build-dir
