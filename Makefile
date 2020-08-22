all: lib build binaries

VERSION=0.2

BINARIES = ov-client
OBJ = ov_client_orlandoviols ov_render_tascar

EXTERNALS = jack libxml++-2.6 liblo sndfile libcurl

BUILD_BINARIES = $(patsubst %,build/%,$(BINARIES))
BUILD_OBJ = $(patsubst %,build/%.o,$(OBJ))


CXXFLAGS = -Wall -Wno-deprecated-declarations -std=c++11 -pthread	\
-ggdb -fno-finite-math-only -fext-numeric-literals

GITMODIFIED=$(shell test -z "`git status --porcelain -uno`" || echo "-modified")
COMMITHASH=$(shell git log -1 --abbrev=7 --pretty='format:%h')
COMMIT_SINCE_MASTER=$(shell git log --pretty='format:%h' origin/master.. | wc -w)

FULLVERSION=$(VERSION).$(COMMIT_SINCE_MASTER)-$(COMMITHASH)$(GITMODIFIED)

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

build/%: src/%.cc
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

#$(BUILD_BINARIES): $(wildcard libov/build/*.o)

build/ov-client: $(wildcard src/*.h)

build/ov-client: $(BUILD_OBJ)

build/%.o: src/%.cc src/%.h

build/%.o: src/%.cc
	$(CXX) $(CXXFLAGS) -c $^ -o $@

clangformat:
	clang-format-9 -i $(wildcard src/*.cc) $(wildcard src/*.h)

clean:
	rm -Rf build src/*~
