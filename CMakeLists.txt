cmake_minimum_required(VERSION 3.14...3.22)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

project(LogicLink VERSION 1.0 LANGUAGES CXX)

# +--------------------------------------+
# | Include guard                        |
# +--------------------------------------+

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# +------------------------------------------+
# | Add dependencies via CPM (lib and tests) |
# +------------------------------------------+

include(cmake/CPM.cmake)

# PackageProject.cmake will be used to make our target installable
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.8.0")
CPMAddPackage("gh:microsoft/GSL@4.1.0")
CPMAddPackage("gh:TheLartians/Format.cmake@1.7.3")
CPMAddPackage("gh:wolfpld/tracy@0.12.2")

if(LOCAL_DEPENDENCIES)
  message("+-----------------------------------------------+")
  message("| ${PROJECT_NAME}\t\tis using local library versions |")
  message("+-----------------------------------------------+")
  CPMAddPackage(NAME common SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../common/logic-link-common)
else()
  CPMAddPackage(NAME common GIT_REPOSITORY "https://${GITHUB_USERNAME}:${GITHUB_TOKEN}@github.com/iwasz/logic-link-common.git" GIT_TAG v0.0.2)
endif()

# +--------------------------------------+
# | Library target                       |
# +--------------------------------------+

add_library(${PROJECT_NAME})
# add_compile_options("-fsanitize=address")
# add_link_options("-fsanitize=address")

# This is added only to set the BASE_DIRS once an for all. Without it, the first
# target_sources instruction that adds to CXX_MODULES fileset would change the
# BASE_DIRS (they are scaterred all over the place in various folders).
target_sources(${PROJECT_NAME}
  PUBLIC FILE_SET CXX_MODULES FILES
    src/logiclink.ccm
    src/session.ccm
)

# Some algoithms simply can't keep up when compiled with -O0. Will have to optimize them.
set_source_files_properties(
    src/processing/rearrange.cc
    src/processing/processing.ccm
    PROPERTIES
    COMPILE_OPTIONS "-O3"
)

add_subdirectory(src)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23)
target_include_directories(${PROJECT_NAME} PRIVATE
  ${GSL_SOURCE_DIR}/include
  ${common_SOURCE_DIR}
  ${tracy_SOURCE_DIR}/public/tracy
)

target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include/link-${PROJECT_VERSION}>
)

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
target_compile_options(${PROJECT_NAME} PUBLIC "-fPIC")

include (FindPkgConfig)
pkg_check_modules (LIBUSBX REQUIRED "libusb-1.0")
target_include_directories (${PROJECT_NAME} PRIVATE ${LIBUSBX_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${LIBUSBX_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBUSBX_LIBRARIES})

# Enable warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  # add_definitions(-Wall -Wpedantic -Wextra -Werror)
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror -Wno-unused-function)
elseif(MSVC)
  target_compile_options(${PROJECT_NAME} PUBLIC /W4 /WX)
  target_compile_options(${PROJECT_NAME} PUBLIC PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
endif()

# +--------------------------------------+
# | Installable target                   |
# +--------------------------------------+

# this allows users to install and find the library via `find_package()`.
# the location where the project's version header will be placed should match the project's regular
# header paths
string(TOLOWER link/version.h VERSION_HEADER_LOCATION)

# packageProject(
#   NAME ${PROJECT_NAME}
#   VERSION ${PROJECT_VERSION}
#   NAMESPACE ${PROJECT_NAME}
#   BINARY_DIR ${PROJECT_BINARY_DIR}
#   INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
#   INCLUDE_DESTINATION include/link-${PROJECT_VERSION}
#   VERSION_HEADER "${VERSION_HEADER_LOCATION}"
#   COMPATIBILITY SameMajorVersion
#   # DEPENDENCIES "fmt 9.1.0"
# )
