all: build lib binaries

BINARIES = ov-client ov-client_hostname ov-client_listsounddevs

EXTERNALS = jack liblo sndfile libcurl gsl samplerate fftw3f xerces-c

BUILD_BINARIES = $(patsubst %,build/%,$(BINARIES))


CXXFLAGS = -Wall -Wno-deprecated-declarations -std=c++11 -pthread	\
-ggdb -fno-finite-math-only

ifeq "$(ARCH)" "x86_64"
CXXFLAGS += -msse -msse2 -mfpmath=sse -ffast-math
endif

CPPFLAGS = -std=c++11
PREFIX = /usr/local
BUILD_DIR = build
SOURCE_DIR = src

LDLIBS += `pkg-config --libs $(EXTERNALS)`
CXXFLAGS += `pkg-config --cflags $(EXTERNALS)`
LDLIBS += -ldl

# libcpprest dependencies:
#LDLIBS += -lcrypto -lboost_filesystem -lboost_system -lcpprest
#LDLIBS += -lcrypto -lcpprest

#libov submodule:
CXXFLAGS += -Ilibov/src
LDLIBS += -lov
LDFLAGS += -Llibov/build

HEADER := $(wildcard src/*.h) $(wildcard libov/src/*.h) tscver

CXXFLAGS += -Ilibov/tascar/libtascar/build

OSFLAG :=
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
		LDLIBS += -lfftw3f -lsamplerate -lc++ -lcpprest -lcrypto -lssl -lboost_filesystem
		CXXFLAGS += -I$(OPENSSL_ROOT)/include/openssl -I$(OPENSSL_ROOT)/include -I$(BOOST_ROOT)/include -I$(NLOHMANN_JSON_ROOT)/include
		LDFLAGS += -L$(OPENSSL_ROOT)/lib -L$(CPPREST_ROOT)/lib -L$(BOOST_ROOT)/lib
		EXTERNALS += nlohmann-json
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


lib: libov/Makefile
	$(MAKE) -C libov all unit-tests

libov/build/libov.a: lib

libov/Makefile:
	git submodule init && git submodule update

build: build/.directory

%/.directory:
	mkdir -p $* && touch $@

binaries: $(BUILD_BINARIES)

$(BUILD_BINARIES): libov/build/libov.a

build/%: src/%.cc
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@


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

packaging:
	$(MAKE) -C packaging/deb pack

.PHONY : doc

doc:
	(cd doc && doxygen doxygen.cfg)
