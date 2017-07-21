# - Try to find emn
# Once done this will define
#  EMN_FOUND        - System has emn
#  EMN_INCLUDE_DIRS - The emn include directories
#  EMN_LIBRARIES    - The libraries needed to use emn

find_path(EMN_INCLUDE_DIR
  NAMES emn.h
)
find_library(EMN_LIBRARY
  NAMES emn
)

if(EMN_INCLUDE_DIR)
  file(STRINGS "${EMN_INCLUDE_DIR}/emn_config.h"
    EMN_VERSION_MAJOR REGEX "^#define[ \t]+EMN_VERSION_MAJOR[ \t]+[0-9]+")
  file(STRINGS "${EMN_INCLUDE_DIR}/emn_config.h"
    EMN_VERSION_MINOR REGEX "^#define[ \t]+EMN_VERSION_MINOR[ \t]+[0-9]+")
  string(REGEX REPLACE "[^0-9]+" "" EMN_VERSION_MAJOR "${EMN_VERSION_MAJOR}")
  string(REGEX REPLACE "[^0-9]+" "" EMN_VERSION_MINOR "${EMN_VERSION_MINOR}")
  set(EMN_VERSION "${EMN_VERSION_MAJOR}.${EMN_VERSION_MINOR}")
  unset(EMN_VERSION_MINOR)
  unset(EMN_VERSION_MAJOR)
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set EMN_FOUND to TRUE
# if all listed variables are TRUE and the requested version matches.
find_package_handle_standard_args(Emn REQUIRED_VARS
                                  EMN_LIBRARY EMN_INCLUDE_DIR
                                  VERSION_VAR EMN_VERSION)

if(EMN_FOUND)
  set(EMN_LIBRARIES     ${EMN_LIBRARY})
  set(EMN_INCLUDE_DIRS  ${EMN_INCLUDE_DIR})
endif()

mark_as_advanced(EMN_INCLUDE_DIR EMN_LIBRARY)