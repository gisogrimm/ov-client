cmake_minimum_required(VERSION 3.18.0)

## GLOBAL CONFIGURATION
project(ov-client
    LANGUAGES CXX 
    VERSION 0.5 
    DESCRIPTION "Headless clients to share and receive spatial realtime audio on Linux, MacOS and Windows hosts using JACK, zita-njbridge and TASCAR"
)
if(NOT DEFINED OVBOXVERSION)
    set(OVBOXVERSION ${PROJECT_VERSION})
endif()
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


## SOURCE FILES
file(GLOB
    SOURCE_FILES
    src/*.h
    src/*.cc
)
# Remove main entry for library
list(REMOVE_ITEM SOURCE_FILES src/ov-client.cc)


# DEPENDENCIES
find_package(CURL REQUIRED)
find_package(cpprestsdk REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSNDFILE REQUIRED sndfile)
pkg_check_modules(LIBSAMPLERATE REQUIRED samplerate)
pkg_check_modules(LIBXML++ REQUIRED libxml++-2.6)
pkg_check_modules(JACK REQUIRED jack)
pkg_check_modules(LIBLO REQUIRED liblo)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules(EIGEN3 REQUIRED eigen3)
pkg_check_modules(GSL REQUIRED gsl)
pkg_check_modules(FFTW REQUIRED fftw3f)
add_subdirectory(libov)
add_subdirectory(tascar/libtascar)


# COMPILER ADDONS
add_compile_definitions(OVBOXVERSION="${OVBOXVERSION}")
if(APPLE)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework IOKit -framework CoreFoundation")
endif()

# TARGET
add_library(ov-client STATIC ${SOURCE_FILES})
target_include_directories(ov-client 
	PUBLIC
    libov/src
    tascar/libtascar/build
    ${JACK_INCLUDE_DIRS}
	${LIBXML++_INCLUDE_DIRS}
	${CURL_INCLUDE_DIRS}
)
target_link_libraries(ov-client
	PUBLIC
    ov
	tascar
    ${JACK_LINK_LIBRARIES}
	OpenSSL::Crypto
	${CURL_LIBRARIES}
    #nlohmann_json::nlohmann_json
    #cpprestsdk::cpprest
)

add_executable(ov-client-exec src/ov-client.cc)
#target_include_directories(ov-client-exec 
#	PUBLIC
#)
target_link_libraries(ov-client-exec
	PUBLIC
	ov-client
)