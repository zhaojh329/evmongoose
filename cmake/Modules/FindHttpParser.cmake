# - Try to find http-parser
# Once done this will define
#  HTTP_PARSER_FOUND        - System has http-parser
#  HTTP_PARSER_INCLUDE_DIRS - The http-parser include directories
#  HTTP_PARSER_LIBRARIES    - The libraries needed to use http-parser

find_path(HTTP_PARSER_INCLUDE_DIR
  NAMES http_parser.h
)
find_library(HTTP_PARSER_LIBRARY
  NAMES http_parser
)

if(HTTP_PARSER_INCLUDE_DIR)
  file(STRINGS "${HTTP_PARSER_INCLUDE_DIR}/http_parser.h"
    HTTP_PARSER_VERSION_MAJOR REGEX "^#define[ \t]+HTTP_PARSER_VERSION_MAJOR[ \t]+[0-9]+")
  file(STRINGS "${HTTP_PARSER_INCLUDE_DIR}/http_parser.h"
    HTTP_PARSER_VERSION_MINOR REGEX "^#define[ \t]+HTTP_PARSER_VERSION_MINOR[ \t]+[0-9]+")
  string(REGEX REPLACE "[^0-9]+" "" HTTP_PARSER_VERSION_MAJOR "${HTTP_PARSER_VERSION_MAJOR}")
  string(REGEX REPLACE "[^0-9]+" "" HTTP_PARSER_VERSION_MINOR "${HTTP_PARSER_VERSION_MINOR}")
  set(HTTP_PARSER_VERSION "${HTTP_PARSER_VERSION_MAJOR}.${HTTP_PARSER_VERSION_MINOR}")
  unset(HTTP_PARSER_VERSION_MINOR)
  unset(HTTP_PARSER_VERSION_MAJOR)
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set HTTP_PARSER_FOUND to TRUE
# if all listed variables are TRUE and the requested version matches.
find_package_handle_standard_args(HttpParser REQUIRED_VARS
                                  HTTP_PARSER_LIBRARY HTTP_PARSER_INCLUDE_DIR
                                  VERSION_VAR HTTP_PARSER_VERSION)

if(HTTP_PARSER_FOUND)
  set(HTTP_PARSER_LIBRARIES     ${HTTP_PARSER_LIBRARY})
  set(HTTP_PARSER_INCLUDE_DIRS  ${HTTP_PARSER_INCLUDE_DIR})
endif()

mark_as_advanced(HTTP_PARSER_INCLUDE_DIR HTTP_PARSER_LIBRARY)