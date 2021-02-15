cmake_minimum_required(VERSION 3.18.0)

## GLOBAL CONFIGURATION
project(ovclient
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
set(HEADERS
    ${HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ov_client_digitalstage.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ov_client_orlandoviols.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ov_render_tascar.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ov_tools.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/soundcardtools.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spawn_process.h
)
set(SOURCE
    ${SOURCE}
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
add_subdirectory(libov)
# set(PLUGINPREFIX "ovclient")
add_subdirectory(tascar/libtascar ${CMAKE_BINARY_DIR}/libtascar)
add_subdirectory(tascar/plugins ${CMAKE_BINARY_DIR}/plugins)
add_subdirectory(zita/zita-njbridge-0.4.4 ${CMAKE_BINARY_DIR}/zita)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# COMPILER ADDONS
add_compile_definitions(OVBOXVERSION="${OVBOXVERSION}")
if(APPLE)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework IOKit -framework CoreFoundation")
endif()

# TARGET
add_library(ovclient STATIC ${HEADERS} ${SOURCE})
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
## INSTALL
install(FILES ${HEADERS} DESTINATION include/libov-client)

add_executable(ov-client src/ov-client.cc)
target_link_libraries(ov-client PUBLIC ovclient)
add_executable(ov-headtracker src/ov-headtracker.cc)
target_link_libraries(ov-headtracker PUBLIC ovclient ${LIBLO_LINK_LIBRARIES})
add_executable(ov-client_hostname src/ov-client_hostname.cc)
target_link_libraries(ov-client_hostname PUBLIC ovclient)


add_custom_command(
        TARGET ov-client POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_BINARY_DIR}/zita/zita-j2n
                ${CMAKE_BINARY_DIR}/zita-j2n)
add_custom_command(
    TARGET ov-client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/zita/zita-n2j
            ${CMAKE_BINARY_DIR}/zita-n2j)