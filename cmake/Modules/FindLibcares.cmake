# - Try to find libcares
# Once done this will define
#  LIBCARES_FOUND        - System has libcares
#  LIBCARES_INCLUDE_DIRS - The libcares include directories
#  LIBCARES_LIBRARIES    - The libraries needed to use libcares

find_path(LIBCARES_INCLUDE_DIR
  NAMES ares.h
)
find_library(LIBCARES_LIBRARY
  NAMES cares
)

if(LIBCARES_INCLUDE_DIR)
  file(STRINGS "${LIBCARES_INCLUDE_DIR}/ares_version.h"
    LIBCARES_VERSION_MAJOR REGEX "^#define[ \t]+ARES_VERSION_MAJOR[ \t]+[0-9]+")
  file(STRINGS "${LIBCARES_INCLUDE_DIR}/ares_version.h"
    LIBCARES_VERSION_MINOR REGEX "^#define[ \t]+ARES_VERSION_MINOR[ \t]+[0-9]+")
  string(REGEX REPLACE "[^0-9]+" "" LIBCARES_VERSION_MAJOR "${LIBCARES_VERSION_MAJOR}")
  string(REGEX REPLACE "[^0-9]+" "" LIBCARES_VERSION_MINOR "${LIBCARES_VERSION_MINOR}")
  set(LIBCARES_VERSION "${LIBCARES_VERSION_MAJOR}.${LIBCARES_VERSION_MINOR}")
  unset(LIBCARES_VERSION_MINOR)
  unset(LIBCARES_VERSION_MAJOR)
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBCARES_FOUND to TRUE
# if all listed variables are TRUE and the requested version matches.
find_package_handle_standard_args(Libcares REQUIRED_VARS
                                  LIBCARES_LIBRARY LIBCARES_INCLUDE_DIR
                                  VERSION_VAR LIBCARES_VERSION)

if(LIBCARES_FOUND)
  set(LIBCARES_LIBRARIES     ${LIBCARES_LIBRARY})
  set(LIBCARES_INCLUDE_DIRS  ${LIBCARES_INCLUDE_DIR})
endif()

mark_as_advanced(LIBCARES_INCLUDE_DIR LIBCARES_LIBRARY)