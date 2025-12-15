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
CPMAddPackage("gh:wolfpld/tracy@0.13.0")

if(${TRACY_ENABLE})
  message("💀💀📊 Tracy client is enabled!")
  add_definitions(-DTRACY_ENABLE)
endif()

if(LOCAL_DEPENDENCIES)
  message("💀💀💀 ${PROJECT_NAME} is using local library versions!")
  CPMAddPackage(NAME common SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../common/logic-link-common)
else()
  CPMAddPackage(NAME common GIT_REPOSITORY "https://${GITHUB_USERNAME}:${GITHUB_TOKEN}@github.com/iwasz/logic-link-common.git" GIT_TAG v0.0.2)
endif()

# +------------------------------------------+
# | Interface target (for common properties) |
# +------------------------------------------+

add_library(interface INTERFACE)
# add_compile_options("-fsanitize=address")
# add_link_options("-fsanitize=address")

set_target_properties(interface PROPERTIES CXX_STANDARD 23)
target_include_directories(interface INTERFACE
  ${GSL_SOURCE_DIR}/include
  ${common_SOURCE_DIR}
  ${tracy_SOURCE_DIR}/public/tracy
)

target_include_directories(
  interface INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include/link-${PROJECT_VERSION}>
)

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(interface INTERFACE "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
target_compile_options(interface INTERFACE "-fPIC")

include (FindPkgConfig)
pkg_check_modules (LIBUSBX REQUIRED "libusb-1.0")
target_include_directories (interface INTERFACE ${LIBUSBX_INCLUDE_DIRS})
target_link_directories(interface INTERFACE ${LIBUSBX_LIBRARY_DIRS})
target_link_libraries(interface INTERFACE ${LIBUSBX_LIBRARIES})

# Enable warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_options(interface INTERFACE -Wall -Werror -Wno-unused-function)
elseif(MSVC)
  target_compile_options(interface INTERFACE /W4 /WX)
  target_compile_options(interface INTERFACE PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
endif()

# +--------------------------------------+
# | Library target                       |
# +--------------------------------------+

# Some algoithms simply can't keep up when compiled with -O0. Will have to optimize them.
set_source_files_properties(
      src/processing/analysis.cc
      src/processing/decompress.cc
      src/processing/downsample.cc
      src/processing/downsample.ccm
      src/processing/generate.cc
      src/processing/generate.ccm
      src/processing/polyPoints.ccm
      src/processing/processing.ccm
      src/processing/rearrange.cc
    PROPERTIES
    COMPILE_OPTIONS "-O3"
)

# add_library (processing STATIC)
# set_target_properties(processing PROPERTIES COMPILE_OPTIONS "-O3")
# target_link_libraries (processing PUBLIC interface)

add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
  PUBLIC FILE_SET CXX_MODULES FILES
    src/logiclink.ccm
)
target_link_libraries (${PROJECT_NAME} PUBLIC interface)
add_subdirectory(src)

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
