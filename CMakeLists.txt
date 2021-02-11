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
find_package (PkgConfig REQUIRED)
find_package(cpprestsdk REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
pkg_check_modules(OPENSSL REQUIRED openssl)
pkg_check_modules(LIBSNDFILE REQUIRED sndfile)
pkg_check_modules(LIBSAMPLERATE REQUIRED samplerate)
pkg_check_modules(LIBXML++ REQUIRED libxml++-2.6)
pkg_check_modules (JACK REQUIRED jack)
pkg_check_modules (LIBLO REQUIRED liblo)
pkg_check_modules (GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules (EIGEN3 REQUIRED eigen3)
pkg_check_modules (GSL REQUIRED gsl)
pkg_check_modules (FFTW REQUIRED fftw3f)
add_subdirectory(libov)
#add_subdirectory(tascar)
add_subdirectory(tascar/libtascar)

# COMPILER ADDONS
add_compile_definitions(OVBOXVERSION="${OVBOXVERSION}")


# TARGET
add_library(ov-client STATIC ${SOURCE_FILES})
target_include_directories(ov-client PUBLIC
    libov/src
    tascar/libtascar/build
    ${OPENSSL_INCLUDE_DIRS}
    ${JACK_INCLUDE_DIRS}
	${LIBXML++_INCLUDE_DIRS}
	${LIBLO_INCLUDE_DIRS}
	${LIBSNDFILE_INCLUDE_DIRS}
	#${LIBSAMPLERATE_INCLUDE_DIRS}
	#${GSL_INCLUDE_DIRS}
	#${GTKMM_INCLUDE_DIRS}
	#${EIGEN3_INCLUDE_DIRS}
	${FFTW_INCLUDE_DIRS}
	#${LIBCURL_INCLUDE_DIRS}
)
target_link_libraries(ov-client
	PRIVATE
    ov
	tascar
	PUBLIC
    nlohmann_json::nlohmann_json
    cpprestsdk::cpprest
    ${JACK_LINK_LIBRARIES}
	${LIBXML++_LINK_LIBRARIES}
	${LIBLO_LINK_LIBRARIES}
	#${LIBSNDFILE_LINK_LIBRARIES}
	#${LIBSAMPLERATE_LINK_LIBRARIES}
	#${GTEST_LINK_LIBRARIES}
	#${GTKMM_LINK_LIBRARIES}
	#${FFTW_LINK_LIBRARIES}
	#${GSL_LINK_LIBRARIES}
	#${EIGEN3_LINK_LIBRARIES}
)

#add_executable(run-ov-client src/ov-client.cc)
#target_link_libraries(run-ov-client STATIC run-ov-client)