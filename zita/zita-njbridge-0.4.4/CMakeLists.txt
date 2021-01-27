cmake_minimum_required (VERSION 3.8)
project (njbridge)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

find_package(JACK)
if(JACK_FOUND)
    set(STATUS_JACK "OK")
    include_directories(${JACK_INCLUDE_DIR})
else()
    set(STATUS_JACK "not found")
    set(SOUNDIO_HAVE_JACK false)
    set(JACK_LIBRARY "")
    message( FATAL_ERROR "Could not found Jack sources, please specify")
endif()

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

set(HEADER_LIST ${CMAKE_CURRENT_SOURCE_DIR}/source/zita-resampler/resampler.h
                ${CMAKE_CURRENT_SOURCE_DIR}/source/zita-resampler/resampler-table.h
                ${CMAKE_CURRENT_SOURCE_DIR}/source/zita-resampler/vresampler.h
                ${CMAKE_CURRENT_SOURCE_DIR}/source/zita-resampler/cresampler.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source
                    ${JACK_INCLUDE_DIRS})

add_library(zita-j2n-lib STATIC ${J2N_SOURCES})
add_library(zita-n2j-lib STATIC ${N2J_SOURCES})

add_executable( zita-j2n ${CMAKE_CURRENT_SOURCE_DIR}/source/zita-j2n.cc )
add_executable( zita-n2j ${CMAKE_CURRENT_SOURCE_DIR}/source/zita-n2j.cc )

target_include_directories( zita-n2j-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../zita-resampler-1.6.2/source)


target_link_directories( zita-n2j PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../zita-resampler-1.6.2 )

target_link_libraries(zita-j2n PUBLIC zita-j2n-lib ${JACK_LIBRARIES} pthread m)
target_link_libraries(zita-n2j PUBLIC zita-n2j-lib zita-resampler ${JACK_LIBRARIES} pthread m)