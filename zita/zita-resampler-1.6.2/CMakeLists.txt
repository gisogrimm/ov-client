cmake_minimum_required (VERSION 2.8.11)
project (resampler)

set(CMAKE_CPPFLAGS_FLAGS "${CMAKE_CPPFLAGS_FLAGS} -I. -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS")
set(CMAKE_CXXFLAGS_FLAGS "${CMAKE_CXXFLAGS_FLAGS} -Wall -fPIC -O2 -ffast-math -march=native")

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/source/resampler.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/source/vresampler.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/source/resampler-table.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/source/cresampler.cc)

set(HEADER_LIST ${CMAKE_CURRENT_SOURCE_DIR}/source/zita-resampler/resampler.h
                ${CMAKE_CURRENT_SOURCE_DIR}/source/zita-resampler/resampler-table.h
                ${CMAKE_CURRENT_SOURCE_DIR}/source/zita-resampler/vresampler.h
                ${CMAKE_CURRENT_SOURCE_DIR}/source/zita-resampler/cresampler.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source)

add_library(objlib OBJECT ${SOURCES} ${HEADER_LIST})
set_property(TARGET objlib PROPERTY POSITION_INDEPENDENT_CODE 1)

add_library(zita-resampler-shared SHARED $<TARGET_OBJECTS:objlib>)
add_library(zita-resampler-static STATIC $<TARGET_OBJECTS:objlib>)

set_target_properties(zita-resampler-shared PROPERTIES OUTPUT_NAME "zita-resampler")
#set_target_properties(zita-resampler-static PROPERTIES OUTPUT_NAME "zita-resampler")

install(TARGETS zita-resampler-shared)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/source/zita-resampler DESTINATION include)