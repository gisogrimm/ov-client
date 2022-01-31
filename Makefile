all: build lib binaries

BINARIES = ov-client ov-client_hostname ov-client_listsounddevs ovbox test_exec

EXTERNALS = jack liblo sndfile libcurl gsl samplerate fftw3f xerces-c

BUILD_BINARIES = $(patsubst %,build/%,$(BINARIES))


CXXFLAGS = -Wall -Wno-deprecated-declarations -std=c++17 -pthread	\
-ggdb -fno-finite-math-only

ifeq "$(ARCH)" "x86_64"
CXXFLAGS += -msse -msse2 -mfpmath=sse -ffast-math
endif

CPPFLAGS = -std=c++11
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
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		OSFLAG += -D LINUX
		CXXFLAGS += -fext-numeric-literals
		LDLIBS += -lasound
	endif
	ifeq ($(UNAME_S),Darwin)
		OSFLAG += -D OSX
		LDFLAGS += -framework IOKit -framework CoreFoundation
		LDLIBS += -lfftw3f -lsamplerate -lc++ -lcpprest -lcrypto -lssl -lboost_filesystem -lsoundio
		CXXFLAGS += -I`brew --prefix libsoundio`/include
		LDFLAGS += -L`brew --prefix libsoundio`/lib
		OPENSSL_ROOT=$(shell brew --prefix openssl)
		CPPREST_ROOT=$(shell brew --prefix cpprestsdk)
		BOOST_ROOT=$(shell brew --prefix boost)
		NLOHMANN_JSON_ROOT=$(shell brew --prefix nlohmann-json)
		CXXFLAGS += -I$(OPENSSL_ROOT)/include/openssl -I$(OPENSSL_ROOT)/include -I$(BOOST_ROOT)/include -I$(NLOHMANN_JSON_ROOT)/include
		LDFLAGS += -L$(OPENSSL_ROOT)/lib -L$(CPPREST_ROOT)/lib -L$(BOOST_ROOT)/lib
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

build/ov-client build/ovbox: $(ZITATARGET)

lib:
	$(MAKE) -C libov all unit-tests

libov/build/libov.a: lib

libov/Makefile:
	git submodule update --init --recursive

build: build/.directory

%/.directory:
	mkdir -p $* && touch $@

binaries: $(BUILD_BINARIES)

$(BUILD_BINARIES): libov/build/libov.a

build/%: src/%.cc
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@


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

.PHONY: packaging

ifeq ($(UNAME_S),Linux)
packaging:
	$(MAKE) -C packaging/deb pack
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
ICON_PATH_REPLACEMENT = -e 's|usr/share/icons/hicolor/48x48/apps/|./|'
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

zita: build/.directory
	(cd build && cmake ../zita-njbridge && make)

gitupdate:
	git fetch --recurse-submodules ; git submodule update --init --recursive
