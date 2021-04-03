# - Try to find soundio
# Once done, this will define
#
#  SOUNDIO_FOUND - system has soundio
#  SOUNDIO_INCLUDE_DIRS - the soundio include directories
#  SOUNDIO_LIBRARIES - link these to use soundio

# Use pkg-config to get hints about paths

if (SOUNDIO_LIBRARIES AND SOUNDIO_INCLUDE_DIRS)
    set(SOUNDIO_FOUND TRUE)
else ()
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(SOUNDIO_PKGCONF soundio)
    endif (PKG_CONFIG_FOUND)

    # Include dir
    find_path(SOUNDIO_INCLUDE_DIR
            NAMES soundio/soundio.h
            PATHS ${SOUNDIO_PKGCONF_INCLUDE_DIRS}
            )

    # Library
    find_library(SOUNDIO_LIBRARY
            NAMES soundio
            PATHS ${SOUNDIO_PKGCONF_LIBRARY_DIRS}
            )

    find_package(PackageHandleStandardArgs)
    find_package_handle_standard_args(Soundio DEFAULT_MSG SOUNDIO_LIBRARY SOUNDIO_INCLUDE_DIR)

    if (SOUNDIO_FOUND)
        set(SOUNDIO_LIBRARIES ${SOUNDIO_LIBRARY})
        set(SOUNDIO_INCLUDE_DIRS ${SOUNDIO_INCLUDE_DIR})
    endif (SOUNDIO_FOUND)

    # Mark the singular variables as advanced, to hide them of the GUI by default
    mark_as_advanced(SOUNDIO_LIBRARY SOUNDIO_INCLUDE_DIR)
endif ()
