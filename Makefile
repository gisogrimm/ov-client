all: lib build binaries

export VERSION:=$(shell grep -m 1 VERSION libov/Makefile|sed 's/^.*=//g')
export MINORVERSION:=$(shell git rev-list --count release_0_4..HEAD)
export COMMIT:=$(shell git rev-parse --short HEAD)
export COMMITMOD:=$(shell test -z "`git status --porcelain -uno`" || echo "-modified")
export FULLVERSION:=$(VERSION).$(MINORVERSION)-$(COMMIT)$(COMMITMOD)

showver:
	echo $(VERSION)

BINARIES = ov-client ov-headtracker ovc_tascar_ver
OBJ = spawn_process ov_client_orlandoviols ov_render_tascar

EXTERNALS = jack libxml++-2.6 liblo sndfile libcurl gsl

BUILD_BINARIES = $(patsubst %,build/%,$(BINARIES))
BUILD_OBJ = $(patsubst %,build/%.o,$(OBJ))


CXXFLAGS = -Wall -Wno-deprecated-declarations -std=c++11 -pthread	\
-ggdb -fno-finite-math-only -fext-numeric-literals

CXXFLAGS += -DOVBOXVERSION="\"$(FULLVERSION)\""

ifeq "$(ARCH)" "x86_64"
CXXFLAGS += -msse -msse2 -mfpmath=sse -ffast-math
endif

CPPFLAGS = -std=c++11
PREFIX = /usr/local
BUILD_DIR = build
SOURCE_DIR = src

LDLIBS += -lasound

LDLIBS += `pkg-config --libs $(EXTERNALS)`
CXXFLAGS += `pkg-config --cflags $(EXTERNALS)`
LDLIBS += -ldl -ltascar

# libov submodule:
CXXFLAGS += -Ilibov/src
LDLIBS += -lov
LDFLAGS += -Llibov/build

HEADER := $(wildcard src/*.h) $(wildcard libov/src/*.h)

lib: libov/Makefile
	$(MAKE) -C libov

libov/Makefile:
	git submodule init
	git submodule update

build: build/.directory

%/.directory:
	mkdir -p $*
	touch $@

binaries: $(BUILD_BINARIES)

build/ov-client: libov/build/libov.a

build/%: src/%.cc
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@


#$(BUILD_BINARIES): $(wildcard libov/build/*.o)

#build/ov-client: $(wildcard src/*.h)

build/ov-client: $(BUILD_OBJ)

build/%.o: src/%.cc $(HEADER)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clangformat:
	clang-format-9 -i $(wildcard src/*.cc) $(wildcard src/*.h)

clean:
	rm -Rf build src/*~ ovclient*.deb
	$(MAKE) -C libov clean

.PHONY: packaging

packaging: build/ovc_tascar_ver
	$(MAKE) -C packaging/deb pack
