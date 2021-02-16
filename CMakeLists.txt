cmake_minimum_required(VERSION 3.16.0)

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
include(GNUInstallDirs)


## SOURCE FILES
set(OVCLIENT_HEADERS
    ${HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ov_ds_service.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ov_client_digitalstage.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ov_client_orlandoviols.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ov_render_tascar.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ov_tools.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/soundcardtools.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spawn_process.h
)
set(OVCLIENT_SOURCE
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ov_ds_service.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ov_client_digitalstage.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ov_client_orlandoviols.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ov_render_tascar.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ov_tools.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/soundcardtools.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spawn_process.cc
)


# DEPENDENCIES
find_package(CURL REQUIRED)
find_package(cpprestsdk REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
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
# set(PLUGINPREFIX "ovclient")
add_subdirectory(libov)
add_subdirectory(tascar/libtascar ${CMAKE_BINARY_DIR}/libtascar)
add_subdirectory(tascar/plugins ${CMAKE_BINARY_DIR}/plugins)
add_subdirectory(zita-resampler-1.8.0)
#add_subdirectory(zita-njbridge-0.4.4)


# COMPILER ADDONS
add_compile_definitions(OVBOXVERSION="${OVBOXVERSION}")
if(APPLE)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework IOKit -framework CoreFoundation")
endif()

# TARGET
# libovclient
add_library(ovclient ${OVCLIENT_HEADERS} ${OVCLIENT_SOURCE})
target_include_directories(ovclient
    PUBLIC
    libov/src
    tascar/libtascar/build
    ${JACK_INCLUDE_DIRS}
    ${LIBXML++_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)
target_link_libraries(ovclient
PUBLIC
    ov
    tascar
    ${JACK_LINK_LIBRARIES}
    OpenSSL::Crypto
    ${CURL_LIBRARIES}
    nlohmann_json::nlohmann_json
    cpprestsdk::cpprest
)
# ov-client
add_executable(ov-client ${CMAKE_CURRENT_SOURCE_DIR}/src/ov-client.cc)
target_link_libraries(ov-client PUBLIC ovclient)
# ov-headtracker
add_executable(ov-headtracker ${CMAKE_CURRENT_SOURCE_DIR}/src/ov-headtracker.cc)
target_link_libraries(ov-headtracker PUBLIC ovclient ${LIBLO_LINK_LIBRARIES})
# ov-client_hostname
add_executable(ov-client_hostname ${CMAKE_CURRENT_SOURCE_DIR}/src/ov-client_hostname.cc)
target_link_libraries(ov-client_hostname PUBLIC ovclient)

# zita-j2n
add_executable(zita-j2n ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/zita-j2n.cc)
target_compile_definitions(zita-j2n PRIVATE VERSION="0.4.4")
target_sources(zita-j2n 
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/jacktx.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/jacktx.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/nettx.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/nettx.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/lfqueue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/lfqueue.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/netdata.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/netdata.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/zsockets.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/zsockets.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/pxthread.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/pxthread.cc
)
target_include_directories(zita-j2n 
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source
    PUBLIC 
    ${JACK_INCLUDE_DIRS}
)
target_link_libraries(zita-j2n 
    PUBLIC 
    ${JACK_LINK_LIBRARIES}
)
# # zita-n2j
add_executable(zita-n2j ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/zita-n2j.cc)
target_compile_definitions(zita-n2j PRIVATE VERSION="0.4.4")
target_sources(zita-n2j
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/jackrx.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/jackrx.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/netrx.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/netrx.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/syncrx.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/syncrx.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/syncrx.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/syncrx.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/lfqueue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/lfqueue.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/netdata.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/netdata.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/zsockets.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/zsockets.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/pxthread.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source/pxthread.cc
)
target_include_directories(zita-n2j 
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-njbridge-0.4.4/source
    ${CMAKE_CURRENT_SOURCE_DIR}/zita-resampler-1.8.0/source
    ${JACK_INCLUDE_DIRS}
)
target_link_libraries(zita-n2j
    PRIVATE
    zita-resampler
    PUBLIC
    ${JACK_LINK_LIBRARIES}
)

## INSTALL
install(FILES ${HEADERS} DESTINATION include/libov-client)
