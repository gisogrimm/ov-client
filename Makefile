PREFIX=/usr/local
LIBDIR=$(PREFIX)/lib
BINDIR=$(PREFIX)/bin
SHAREDIR=$(PREFIX)/share/ovclient
DESTDIR=

# build all tools:
all: build lib binaries

# build only command line tools, should work without gtkmm and cairomm:
cli: build libcli clibinaries

# build only GUI tools:
gui: build lib guibinaries


UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CMD_INSTALL=install
	LIB_EXT=so
	CMD_LD=ldconfig -n $(DESTDIR)$(LIBDIR)
endif
ifeq ($(UNAME_S),Darwin)
	CMD_INSTALL=ginstall
	LIB_EXT=dylib
	CMD_LD=
endif

BIN_OLD_CLI = ov-client

BIN_CLI = ovbox_cli ov-client_hostname ov-client_listsounddevs	\
  ovrealpath ovbox_version ovbox_sendlog

BIN_GUI = ovbox

BINARIES = $(BIN_OLD_CLI) $(BIN_CLI) $(BIN_GUI)

BUILD_BINARIES = $(patsubst %,build/%,$(BINARIES))
BUILD_CLI = $(patsubst %,build/%,$(BIN_CLI))
BUILD_GUI = $(patsubst %,build/%,$(BIN_GUI))

# external library dependencies:
EXTERNALS = jack liblo sndfile libcurl gsl samplerate fftw3f xerces-c libsodium


CXXFLAGS = -Wall -Wno-deprecated-declarations -std=c++17 -pthread	\
-ggdb -fno-finite-math-only -Wno-psabi

CFLAGS = -Wall -Wno-deprecated-declarations

ifneq ($(HOMEBREW_OVBOX_TAG),)
  CXXFLAGS+=-DHOMEBREW_OVBOX_TAG=\"$(HOMEBREW_OVBOX_TAG)\"
endif

ifeq "$(ARCH)" "x86_64"
CXXFLAGS += -msse -msse2 -mfpmath=sse -ffast-math
endif

#CPPFLAGS = -std=c++17
PREFIX = /usr/local
BUILD_DIR = build
SOURCE_DIR = src

# libcpprest dependencies:
#LDLIBS += -lcrypto -lboost_filesystem -lboost_system -lcpprest
#LDLIBS += -lcrypto -lcpprest

#libov submodule:
CXXFLAGS += -Ilibov/src
LDLIBS += -lov -lovclienttascar
LDFLAGS += -Llibov/build -Llibov/tascar/libtascar/build

LDLIBS += `pkg-config --libs $(EXTERNALS)`
CXXFLAGS += `pkg-config --cflags $(EXTERNALS)`
LDLIBS += -ldl


HEADER := $(wildcard src/*.h) $(wildcard libov/src/*.h) tscver

CXXFLAGS += -Ilibov/tascar/libtascar/build

OSFLAG :=
UNAME_S :=
ifeq ($(OS),Windows_NT)
	OSFLAG += -D WIN32
	ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
		OSFLAG += -D AMD64
	endif
	ifeq ($(PROCESSOR_ARCHITECTURE),x86)
		OSFLAG += -D IA32
	endif
	EXTERNALS += portaudio-2.0

	LDLIBS += -lrpcrt4 -lole32 -lshell32
	ZITATARGET = zita

else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		OSFLAG += -D LINUX
		CXXFLAGS += -fext-numeric-literals
		LDLIBS += -lasound
		ZITATARGET = zita
	endif
	ifeq ($(UNAME_S),Darwin)
		OSFLAG += -D OSX
		LDFLAGS += -framework IOKit -framework CoreFoundation -headerpad_max_install_names
		LDLIBS += -lfftw3f -lsamplerate -lc++ -lcpprest -lcrypto -lssl -lsoundio
		SOUNDIO_ROOT:=$(shell brew --prefix libsoundio)
		OPENSSL_ROOT:=$(shell brew --prefix openssl)
		CPPREST_ROOT:=$(shell brew --prefix cpprestsdk)
		BOOST_ROOT=$(shell brew --prefix boost)
		NLOHMANN_JSON_ROOT=$(shell brew --prefix nlohmann-json)
		CXXFLAGS += -I$(SOUNDIO_ROOT)/include -I$(OPENSSL_ROOT)/include/openssl -I$(OPENSSL_ROOT)/include -I$(BOOST_ROOT)/include -I$(NLOHMANN_JSON_ROOT)/include
		LDFLAGS += -L$(SOUNDIO_ROOT)/lib -L$(OPENSSL_ROOT)/lib -L$(CPPREST_ROOT)/lib -L$(BOOST_ROOT)/lib
#		EXTERNALS += nlohmann-json
		ZITATARGET = zita
	endif
		UNAME_P := $(shell uname -p)
	ifeq ($(UNAME_P),x86_64)
		OSFLAG += -D AMD64
	endif
		ifneq ($(filter %86,$(UNAME_P)),)
	OSFLAG += -D IA32
		endif
	ifneq ($(filter arm%,$(UNAME_P)),)
		OSFLAG += -D ARM
	endif
endif

CXXFLAGS += $(OSFLAG)

build/ov-client build/ovbox build/ovbox_cli: $(ZITATARGET)

lib: libcli
	$(MAKE) -C libov all

libcli: build
	$(MAKE) -C libov cli

libtest:
	$(MAKE) -C libov unit-tests

libov/build/libov.a: libcli

libov/Makefile:
	git submodule update --init --recursive

build: build/.directory

%/.directory:
	mkdir -p $* && touch $@

binaries: $(BUILD_BINARIES)
guibinaries: $(BUILD_GUI) $(BUILD_CLI)
clibinaries: $(BUILD_CLI)

$(BUILD_BINARIES): libov/build/libov.a

build/%: src/%.cc
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@

build/ovrealpath: src/ovrealpath.c
	$(CC) $(CFLAGS) $< -o $@

build/%.o: src/%.cc $(HEADER)
	$(CXX) $(CXXFLAGS) -c $< -o $@


## ZITA stuff:
zita-resampler-build:
	$(MAKE) -C zita/zita-resampler-1.6.2/source
zita-njbridge-build:
	$(MAKE) -C zita/zita-njbridge-0.4.4/source
##TODO: @alessandro: Let zita-njbridge build binaries into build dir

clangformat:
	clang-format-9 -i $(wildcard src/*.cc) $(wildcard src/*.h)

clean:
	rm -Rf build src/*~ ovclient*.deb
	$(MAKE) -C libov clean
	-cd zita-njbridge && git clean -ffdx
	-cd zita-njbridge/zita-resampler && git clean -ffdx

.PHONY: packaging

ifeq ($(UNAME_S),Linux)
packaging:
	$(MAKE) -C packaging/deb pack
clipackaging:
	$(MAKE) -C packaging/deb clipack
endif
ifeq ($(UNAME_S),Darwin)
packaging:
	$(MAKE) -C packaging/Darwin pack
endif

.PHONY : doc

doc:
	(cd doc && doxygen doxygen.cfg)

ver:
	(cd libov && ./get_version.sh)

ifeq ($(OS),Windows_NT)
ICON_PATH_REPLACEMENT = -e 's|usr/share/icons/hicolor/48x48/apps/|./|'
LDFLAGS += -mwindows
else
ifeq ($(UNAME_S),Darwin)
ICON_PATH_REPLACEMENT = -e 's|usr/share/icons/hicolor/.*/apps/||'
else
ICON_PATH_REPLACEMENT = -e 's/>usr/>\/usr/1'
endif
endif

build/%_glade.h: src/%.glade build/.directory
	cat $< | sed -e 's/tascarversion/$(TASCARVERSION)/g' -e '/name="authors"/ r ../contributors' $(ICON_PATH_REPLACEMENT) >$*_glade
	echo "#include <string>" > $@
	xxd -i $*_glade >> $@
	echo "std::string ui_"$*"((const char*)"$*"_glade,(size_t)"$*"_glade_len);" >> $@
	rm -f $*_glade

build/%.res.c: src/%.res
	(cd src && glib-compile-resources --generate-source --target=../$@ ../$<)

build/ovbox: EXTERNALS += gtkmm-3.0
build/ovbox: CXXFLAGS += -I./build
build/ovbox: build/ovbox_glade.h
build/ovbox: build/ovbox.res.c

ifeq ($(UNAME_S),Darwin)
zita: build/.directory
	$(MAKE) -C zita-njbridge/zita-resampler/source libzita-resampler.a && \
	$(MAKE) -C zita-njbridge/source CXXFLAGS+="$(shell pkg-config --cflags jack) -I../zita-resampler/source" LDFLAGS+="$(shell pkg-config --libs jack) -L../zita-resampler/source" -f Makefile-osx && \
	cp zita-njbridge/source/zita-n2j build/ovzita-n2j && \
	cp zita-njbridge/source/zita-j2n build/ovzita-j2n
else
ifeq ($(OS),Windows_NT)
zita: build/.directory
	$(MAKE) -C zita-njbridge/zita-resampler/source libzita-resampler.a && \
	$(MAKE) -C zita-njbridge/source CXXFLAGS+="-I../zita-resampler/source" LDFLAGS+="-L../zita-resampler/source" -f Makefile-win && \
	cp zita-njbridge/source/zita-n2j.exe build/ovzita-n2j.exe && \
	cp zita-njbridge/source/zita-j2n.exe build/ovzita-j2n.exe
else
zita: build/.directory
	$(MAKE) -C zita-njbridge/zita-resampler/source libzita-resampler.a && \
	$(MAKE) -C zita-njbridge/source CXXFLAGS+="$(shell pkg-config --cflags jack) -I../zita-resampler/source" LDFLAGS+="$(shell pkg-config --libs jack) -L../zita-resampler/source" -f Makefile-linux && \
	cp zita-njbridge/source/zita-n2j build/ovzita-n2j && \
	cp zita-njbridge/source/zita-j2n build/ovzita-j2n
endif
endif
#	(cd build && cmake ../zita-njbridge && make)

gitupdate:
	git fetch --recurse-submodules ; git submodule update --init --recursive

install: all
	$(CMD_INSTALL) -D libov/tascar/libtascar/build/lib*.$(LIB_EXT) -t $(DESTDIR)$(LIBDIR)
	$(CMD_INSTALL) -D libov/tascar/plugins/build/*.$(LIB_EXT) -t $(DESTDIR)$(LIBDIR)
	$(CMD_INSTALL) -D build/ovbox -t $(DESTDIR)$(BINDIR)
	$(CMD_INSTALL) -D build/ovbox_version -t $(DESTDIR)$(BINDIR)
	$(CMD_INSTALL) -D build/ovbox_sendlog -t $(DESTDIR)$(BINDIR)
	$(CMD_INSTALL) -D build/ovbox_cli -t $(DESTDIR)$(BINDIR)
	$(CMD_INSTALL) -D build/ovzita* -t $(DESTDIR)$(BINDIR)
	mkdir -p  $(DESTDIR)$(SHAREDIR) && cp -r node_modules $(DESTDIR)$(SHAREDIR)
	$(CMD_INSTALL) -D ovclient.css -t $(DESTDIR)$(SHAREDIR)
	$(CMD_INSTALL) -D ovclient.js -t $(DESTDIR)$(SHAREDIR)
	$(CMD_INSTALL) -D webmixer.js -t $(DESTDIR)$(SHAREDIR)
	$(CMD_INSTALL) -D jackrec.html -t $(DESTDIR)$(SHAREDIR)
	$(CMD_INSTALL) -D sounds/2138735723541465742.flac -t $(DESTDIR)$(SHAREDIR)/sounds
	$(CMD_INSTALL) -D sounds/4180150583.flac -t $(DESTDIR)$(SHAREDIR)/sounds
	$(CMD_LD)


#install:
#	cat packaging/deb/*.csv |sed -e 's/,usr/,$${PREFIX}/1' | PREFIX=$(PREFIX) envsubst |sed -e 's/.*,//1' | sort -u | xargs -L 1 -- mkdir -p && cat packaging/deb/*.csv |sed -e 's/,usr/ $${PREFIX}/1' | PREFIX=$(PREFIX) envsubst | xargs -L 1 -I % sh -c "cp --preserve=links -r %"

homebrew:
	$(MAKE) -C packaging/homebrew install

testendianess:
	$(MAKE) build/test_endianess && ./start_env.sh ./build/test_endianess

#	$(MAKE) build/test_endianess && (cd ./build/ && ./test_endianess)
