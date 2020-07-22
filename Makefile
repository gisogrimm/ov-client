all: build binaries

BINARIES = ov-client

EXTERNALS = jack libxml++-2.6 liblo sndfile

BUILD_BINARIES = $(patsubst %,build/%,$(BINARIES))


CXXFLAGS = -Wall -Wno-deprecated-declarations -std=c++11 -pthread	\
-ggdb -fno-finite-math-only -fext-numeric-literals

ifeq "$(ARCH)" "x86_64"
CXXFLAGS += -msse -msse2 -mfpmath=sse -ffast-math
endif

CPPFLAGS = -std=c++11
PREFIX = /usr/local
BUILD_DIR = build
SOURCE_DIR = src

LDLIBS += `pkg-config --libs $(EXTERNALS)`
CXXFLAGS += `pkg-config --cflags $(EXTERNALS)`
LDLIBS += -ldl -ltascar

build: build/.directory

%/.directory:
	mkdir -p $*
	touch $@

binaries: $(BUILD_BINARIES)

build/%: src/%.cc
	$(CXX) $(CXXFLAGS) $(LDLIBS) $^ -o $@
