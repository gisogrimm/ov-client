all: showver lib tscver tscobj build binaries tscplug

export VERSION:=$(shell grep -m 1 VERSION libov/Makefile|sed 's/^.*=//g')
export MINORVERSION:=$(shell git rev-list --count release_0_5..HEAD)
export COMMIT:=$(shell git rev-parse --short HEAD)
export COMMITMOD:=$(shell test -z "`git status --porcelain -uno`" || echo "-modified")
export FULLVERSION:=$(VERSION).$(MINORVERSION)-$(COMMIT)$(COMMITMOD)

showver: libov/Makefile
	$(MAKE) ver

ver:
	echo $(FULLVERSION)

BINARIES = ov-client ov-headtracker ov-client_hostname ov-client_listsounddevs
OBJ = ov_tools spawn_process ov_client_orlandoviols 	\
  ov_render_tascar soundcardtools

EXTERNALS = jack libxml++-2.6 liblo sndfile libcurl gsl samplerate fftw3f

BUILD_BINARIES = $(patsubst %,build/%,$(BINARIES))
BUILD_OBJ = $(patsubst %,build/%.o,$(OBJ))


CXXFLAGS = -Wall -Wno-deprecated-declarations -std=c++11 -pthread	\
-ggdb -fno-finite-math-only

CXXFLAGS += -DOVBOXVERSION="\"$(FULLVERSION)\""

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

CXXFLAGS += -Itascar/libtascar/build

TASCAROBJECTS = licensehandler.o audiostates.o coordinates.o		\
  audiochunks.o xmlconfig.o dynamicobjects.o sourcemod.o		\
  receivermod.o filterclass.o osc_helper.o pluginprocessor.o		\
  acousticmodel.o scene.o render.o session_reader.o session.o		\
  jackclient.o delayline.o errorhandling.o osc_scene.o ringbuffer.o	\
  jackiowav.o jackrender.o audioplugin.o levelmeter.o serviceclass.o	\
  speakerarray.o spectrum.o fft.o stft.o ola.o

TASCARDMXOBJECTS =

TASCARRECEIVERS = ortf hrtf itu51 simplefdn omni

TASCARSOURCE = omni cardioidmod

TASCARMODULS = system touchosc waitforjackport route jackrec

TASCARAUDIOPLUGS = sndfile delay

#viewport.o sampler.o cli.o irrender.o async_file.o vbap3d.o hoa.o

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
	 	TASCARMODULS += ovheadtracker lightctl
		TASCARDMXOBJECTS += termsetbaud.o serialport.o dmxdriver.o
	endif
	ifeq ($(UNAME_S),Darwin)
		OSFLAG += -D OSX
		LDFLAGS += -framework IOKit -framework CoreFoundation
		LDLIBS += -lfftw3f -lsamplerate -lc++ -lcpprest -lcrypto -lssl -lboost_filesystem
		CXXFLAGS += -I$(OPENSSL_ROOT)/include/openssl -I$(OPENSSL_ROOT)/include
		LDFLAGS += -L$(OPENSSL_ROOT)/lib -L$(OPENSSL_ROOT)/lib
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
	$(MAKE) -C libov

libov/build/libov.a: lib

libov/Makefile:
	git submodule init && git submodule update

tascar/Makefile:
	git submodule init && git submodule update

build: build/.directory

%/.directory:
	mkdir -p $* && touch $@

binaries: $(BUILD_BINARIES)

$(BUILD_BINARIES): libov/build/libov.a

#build/ov-client: libov/build/libov.a

build/%: src/%.cc
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@


#$(BUILD_BINARIES): $(wildcard libov/build/*.o)

#build/ov-client: $(wildcard src/*.h)

build/ov-client_listsounddevs build/ov-client: $(BUILD_OBJ) $(patsubst %,tascar/libtascar/build/%,$(TASCAROBJECTS))

build/%.o: src/%.cc $(HEADER)
	$(CXX) $(CXXFLAGS) -c $< -o $@


## ZITA stuff:
zita-resampler-build:
	$(MAKE) -C zita/zita-resampler-1.6.2/source
zita-njbridge-build:
	$(MAKE) -C zita/zita-njbridge-0.4.4/source
##TODO: @alessandro: Let zita-njbridge build binaries into build dir

## TASCAR stuff:

tascar: tscver tascar/Makefile

tscbuild:
	$(MAKE) -C tascar/libtascar build

$(BUILD_OBJ) $(patsubst %,tascar/libtascar/build/%,$(TASCAROBJECTS)): tscobj

tscver: tscbuild
	$(MAKE) -C tascar/libtascar ver

tscobj: tscver
	$(MAKE) -C tascar/libtascar TSCCXXFLAGS=-DPLUGINPREFIX='\"ovclient\"' $(patsubst %,build/%,$(TASCAROBJECTS))  $(patsubst %,build/%,$(TASCARDMXOBJECTS))

tscplug: tscver tscobj
	 $(MAKE) -C tascar/plugins PLUGINPREFIX=ovclient RECEIVERS="$(TASCARRECEIVERS)" SOURCES="$(TASCARSOURCE)" TASCARMODS="$(TASCARMODULS)" TASCARMODSGUI= AUDIOPLUGINS="$(TASCARAUDIOPLUGS)" GLABSENSORS= TASCARLIB="$(patsubst %,../libtascar/build/%,$(TASCAROBJECTS))" TASCARDMXLIB="$(patsubst %,../libtascar/build/%,$(TASCARDMXOBJECTS))"

clangformat:
	clang-format-9 -i $(wildcard src/*.cc) $(wildcard src/*.h)

clean:
	rm -Rf build src/*~ ovclient*.deb
	$(MAKE) -C libov clean
	$(MAKE) -C tascar clean

.PHONY: packaging

packaging:
	$(MAKE) -C packaging/deb pack
