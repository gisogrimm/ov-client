cmake_minimum_required(VERSION 3.8)
project (njbridge)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JACK REQUIRED jack)
add_subdirectory(zita-resampler)

add_definitions( -DVERSION="0.4.4" )

set(CMAKE_CPPFLAGS_FLAGS "${CMAKE_CPPFLAGS_FLAGS} -I. -MMD -MP")
set(CMAKE_CXXFLAGS_FLAGS "${CMAKE_CXXFLAGS_FLAGS} -O2 -Wall")

set(J2N_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/source/netdata.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/source/jacktx.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/source/nettx.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/source/pxthread.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/source/lfqueue.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/source/zsockets.cc)

set(N2J_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/source/zita-n2j.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/source/netdata.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/source/jackrx.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/source/netrx.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/source/pxthread.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/source/lfqueue.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/source/zsockets.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/source/syncrx.cc)

add_executable( zita-j2n ${CMAKE_CURRENT_SOURCE_DIR}/source/zita-j2n.cc ${J2N_SOURCES} )
add_executable( zita-n2j ${CMAKE_CURRENT_SOURCE_DIR}/source/zita-n2j.cc ${N2J_SOURCES} )
target_include_directories(zita-j2n
    PUBLIC
    ${JACK_INCLUDE_DIRS}
)
target_link_libraries(zita-j2n
    PUBLIC
    ${JACK_LINK_LIBRARIES}
)
target_include_directories(zita-n2j
    PUBLIC
    ${JACK_INCLUDE_DIRS}
)
target_link_libraries(zita-n2j
    PUBLIC
    zita-resampler
    ${JACK_LINK_LIBRARIES}
)

install(TARGETS zita-j2n zita-n2j DESTINATION bin)