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
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

# DEPENDENCIES
find_package(Threads REQUIRED)
find_package(XercesC REQUIRED)
find_package(Jack REQUIRED)
find_package(LibSndFile REQUIRED)
find_package(LibSamplerate REQUIRED)
find_package(SoundIo REQUIRED)
find_package(FFTW REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(liblo REQUIRED)
find_package(GSL REQUIRED)
#find_package(LibXml++ REQUIRED)
set(PLUGINPREFIX "ovclient")
add_subdirectory(libov)
list(APPEND RECEIVERS
        ortf
        hrtf
        simplefdn
        omni
        )
list(APPEND TASCARMODS
        system
        touchosc
        waitforjackport
        route
        jackrec
        )
list(APPEND SOURCEMODS
        omni
        cardioidmod
        )
list(APPEND AUDIOPLUGINS
        sndfile
        delay
        metronome
        )
add_subdirectory(libov/tascar/plugins ${PROJECT_BINARY_DIR}/plugins)
get_property(TASCAR_PLUGIN_LIBRARIES GLOBAL PROPERTY TASCAR_PLUGIN_LIBRARIES)
add_subdirectory(zita-njbridge ${PROJECT_BINARY_DIR}/zita)
set(EXTERNAL_INCLUDES
        PRIVATE
        libov/src
        ${JACK_INCLUDE_DIRS}
        ${LIBXML++_INCLUDE_DIRS}
        ${LIBSNDFILE_INCLUDE_DIRS}
        ${LIBSAMPLERATE_INCLUDE_DIRS}
        ${SOUNDIO_INCLUDE_DIRS}
        ${LIBLO_INCLUDE_DIRS}
        ${FFTW_INCLUDE_DIRS})
set(EXTERNAL_LIBS
        ${JACK_LIBRARIES}
        ${LIBXML++_LIBRARIES}
        ${LIBSNDFILE_LIBRARIES}
        ${LIBSAMPLERATE_LIBRARIES}
        ${SOUNDIO_LIBRARIES}
        ${LIBLO_LIBRARIES}
        ${FFTW_LIBRARIES})

message("Tascar plugin: ${TASCAR_PLUGIN_LIBRARIES}")

# COMPILER ADDONS
add_compile_definitions(OVBOXVERSION="${OVBOXVERSION}")

# TARGET
# libovclient
# ov-client
add_executable(ov-client 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ov-client.cc
        )
target_link_libraries(ov-client
        PRIVATE
        ov
        ${EXTERNAL_LIBS}
        PUBLIC
        ${TASCAR_PLUGIN_LIBRARIES}
        )
target_include_directories(ov-client
        PRIVATE
        libov/src 
        ${EXTERNAL_INCLUDES}
        )
# ov-client_hostname
add_executable(ov-client_hostname ${CMAKE_CURRENT_SOURCE_DIR}/src/ov-client_hostname.cc)
target_link_libraries(ov-client_hostname PRIVATE ov ${EXTERNAL_LIBS})
target_include_directories(ov-client_hostname PRIVATE libov/src ${EXTERNAL_INCLUDES})
# ov-client_listsounddevs
add_executable(ov-client_listsounddevs ${CMAKE_CURRENT_SOURCE_DIR}/src/ov-client_listsounddevs.cc)
target_link_libraries(ov-client_listsounddevs PRIVATE ov ${EXTERNAL_LIBS})
target_include_directories(ov-client_listsounddevs PRIVATE libov/src ${EXTERNAL_INCLUDES})

set(CPACK_PACKAGE_EXECUTABLES ov-client)
if(APPLE AND BUILD_APP)
        set(APP "${PROJECT_BINARY_DIR}/OVClient.app")
        install(FILES
                ${PROJECT_SOURCE_DIR}/icon.icns
                DESTINATION
                ${APP}/Contents/Resources/)
        install(FILES
                ${PROJECT_BINARY_DIR}/zita/zita-n2j
                DESTINATION
                ${APP}/Contents/MacOS)
        install(FILES
                ${PROJECT_BINARY_DIR}/zita/zita-j2n
                DESTINATION
                ${APP}/Contents/MacOS)
        set_target_properties(ov-client PROPERTIES
                MACOSX_BUNDLE TRUE
                MACOSX_BUNDLE_GUI_IDENTIFIER com.orlandoviols.ovclient
                MACOSX_FRAMEWORK_IDENTIFIER com.orlandoviols.ovclient
                MACOSX_BUNDLE_BUNDLE_NAME "OVClient"
                OUTPUT_NAME "OVClient"
                MACOSX_BUNDLE_ICON_FILE "icon.icns"
                )
        install(FILES "${PROJECT_BINARY_DIR}/zita/zita-n2j" "${PROJECT_BINARY_DIR}/zita/zita-j2n"
                DESTINATION "${PROJECT_BINARY_DIR}/${APP}/MacOS"
                )
        install(TARGETS ov-client
                BUNDLE 
                        DESTINATION ${PROJECT_BINARY_DIR}
                        COMPONENT Runtime
                )
        set(DIRS 
                ${CMAKE_BINARY_DIR}/plugins
                ${CMAKE_BINARY_DIR}/libov
                ${CMAKE_BINARY_DIR}/libov/libtascar
        )
        install(CODE "include(BundleUtilities)
                fixup_bundle(\"${APP}\" \"\" \"${DIRS}\")
                verify_app(\"${APP}\")
                ")
        # CPACK
        set(CPACK_GENERATOR "DragNDrop")
        set(CPACK_BUNDLE_NAME "OVClient")
        set(CPACK_DMG_VOLUME_NAME "InstallOvClient")
        include(CPack)
        message("Use cpack to generate an app installer!")
endif()
