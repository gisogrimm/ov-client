 cmake_minimum_required(VERSION 3.18)
 
 project(ov-client VERSION 1.2.3)

 # Use a directory to collect all DLLs and EXEs
 set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(libov)

add_library(getopt third_party/getopt-for-windows/getopt.c)

set(SOURCES
	src/ov-client.cc
	#src/ov-client_hostname.cc
	#src/ov-client_listsounddevs.cc
	#src/ov-headtracker.cc
	#src/ov_client_digitalstage.cc
	#src/ov_client_digitalstage.h
	#src/ov_client_orlandoviols.cc
	#src/ov_client_orlandoviols.h
	#src/ov_render_tascar.cc
	#src/ov_render_tascar.h
	#src/ov_tools.cc
	#src/ov_tools.h
	#src/soundcardtools.cc
	#src/soundcardtools.h
	#src/spawn_process.cc
	#src/spawn_process.h
)

set(WIN32_LIBS
	Iphlpapi # GetAdaptersAddresses
	Rpcrt4 # UuidCreateSequential 
	Shlwapi # PathMatchSpecW 
)

add_executable(ov-client ${SOURCES})
target_link_libraries(ov-client ov getopt ${WIN32_LIBS})
add_definitions(-D_USE_MATH_DEFINES) # For M_PI

