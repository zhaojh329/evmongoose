# - Try to find udns
# Once done this will define
#  UDNS_FOUND        - System has udns
#  UDNS_INCLUDE_DIRS - The udns include directories
#  UDNS_LIBRARIES    - The libraries needed to use udns

find_path(UDNS_INCLUDE_DIR
  NAMES udns.h
)
find_library(UDNS_LIBRARY
  NAMES udns
)

if(UDNS_INCLUDE_DIR)
  file(STRINGS "${UDNS_INCLUDE_DIR}/udns.h" 
	UDNS_VERSION REGEX "^#define[ \t]+UDNS_VERSION[ \t]+\"[0-9]+\\.[0-9]+\"$")
  string(REGEX REPLACE "[^0-9\\.]+" "" UDNS_VERSION "${UDNS_VERSION}")
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set UDNS_FOUND to TRUE
# if all listed variables are TRUE and the requested version matches.
find_package_handle_standard_args(UDNS REQUIRED_VARS
                                  UDNS_LIBRARY UDNS_INCLUDE_DIR
                                  VERSION_VAR UDNS_VERSION)

if(UDNS_FOUND)
  set(UDNS_LIBRARIES     ${UDNS_LIBRARY})
  set(UDNS_INCLUDE_DIRS  ${UDNS_INCLUDE_DIR})
endif()

mark_as_advanced(UDNS_INCLUDE_DIR UDNS_LIBRARY)