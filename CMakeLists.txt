cmake_minimum_required(VERSION 3.16.0)

## GLOBAL CONFIGURATION
project(ov-client
        LANGUAGES CXX
        VERSION 0.5
        DESCRIPTION "Headless clients to share and receive spatial realtime audio on Linux, MacOS and Windows hosts using JACK, zita-njbridge and TASCAR"
        )
if (NOT DEFINED OVBOXVERSION)
    set(OVBOXVERSION ${PROJECT_VERSION})
endif ()
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(GNUInstallDirs)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

# DEPENDENCIES
set(PLUGINPREFIX "ovclient")
add_subdirectory(libov)
#add_subdirectory(libov/tascar ${PROJECT_BINARY_DIR}/plugins)
add_subdirectory(zita-njbridge ${PROJECT_BINARY_DIR}/zita)

set_target_properties(zita-n2j PROPERTIES
        PROJECT_BINARY_DIR bla
        )

# REQUIREMENTS
find_package(Jack REQUIRED)
find_package(LibSndFile REQUIRED)
find_package(LibSamplerate REQUIRED)
find_package(FFTW REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(liblo REQUIRED)
find_package(GSL REQUIRED)
find_package(GTK3 COMPONENTS gtkmm)
find_package(LibXml++ REQUIRED)

# COMPILER ADDONS
add_compile_definitions(OVBOXVERSION="${OVBOXVERSION}")

# TARGET
# libovclient
# ov-client
add_executable(ov-client ${CMAKE_CURRENT_SOURCE_DIR}/src/ov-client.cc)
target_link_libraries(ov-client
        PRIVATE
        ov
        )
target_include_directories(ov-client
        PRIVATE
        libov/src
        PUBLIC
        ${LIBXML++_INCLUDE_DIRS}
        ${LIBLO_INCLUDE_DIRS}
        ${LIBSNDFILE_INCLUDE_DIRS}
        ${LIBSAMPLERATE_INCLUDE_DIRS}
        ${FFTW_INCLUDE_DIRS}
        ${JACK_INCLUDE_DIRS}
        ${EIGEN3_INCLUDE_DIR}
        ${GSL_INCLUDE_DIR}
        )
# ov-client_hostname
add_executable(ov-client_hostname ${CMAKE_CURRENT_SOURCE_DIR}/src/ov-client_hostname.cc)
target_link_libraries(ov-client_hostname PRIVATE ov)
target_include_directories(ov-client_hostname PRIVATE libov/src ${JACK_INCLUDE_DIRS})
# ov-client_listsounddevs
add_executable(ov-client_listsounddevs ${CMAKE_CURRENT_SOURCE_DIR}/src/ov-client_listsounddevs.cc)
target_link_libraries(ov-client_listsounddevs PRIVATE ov)
target_include_directories(ov-client_listsounddevs PRIVATE libov/src ${JACK_INCLUDE_DIRS})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_PATH}/build)

set_target_properties(zita-n2j PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_PATH}/build
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_PATH}/build
        )
set_target_properties(zita-j2n PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_PATH}/build
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_PATH}/build
        )
