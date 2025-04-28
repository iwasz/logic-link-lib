cmake_minimum_required(VERSION 3.14...3.22)

# ---- Project ----

# Note: update this to your new project's name and version
project(
  LogicLink
  VERSION 1.0
  LANGUAGES CXX
)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# ---- Add dependencies via CPM ----

include(cmake/CPM.cmake)

# PackageProject.cmake will be used to make our target installable
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.8.0")
CPMAddPackage("gh:microsoft/GSL@4.1.0")
CPMAddPackage("gh:p-ranav/argparse@3.1")

# Note: for header-only libraries change all PUBLIC flags to INTERFACE and create an interface
# target: add_library(${PROJECT_NAME} INTERFACE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
include_directories(${GSL_SOURCE_DIR}/include)
include_directories("deps")

add_library(link)
set_target_properties(link PROPERTIES CXX_STANDARD 23)

target_include_directories(link PRIVATE
  src/lib
)

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(link PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")

target_include_directories(
  link PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include/link-${PROJECT_VERSION}>
)

include (FindPkgConfig)
pkg_check_modules (LIBUSBX REQUIRED "libusb-1.0")
target_include_directories (link PRIVATE ${LIBUSBX_INCLUDE_DIRS})
target_link_directories(link PRIVATE ${LIBUSBX_LIBRARY_DIRS})
target_link_libraries(link PRIVATE ${LIBUSBX_LIBRARIES})

# ---- Create the cli app ----

add_executable(cli)
set_target_properties(cli PROPERTIES CXX_STANDARD 23)
target_link_libraries(cli link)
target_include_directories(cli PRIVATE
  src
  src/lib
  ${argparse_SOURCE_DIR}/include
)
add_subdirectory(src)

# ---- Create an installable target ----
# this allows users to install and find the library via `find_package()`.

# the location where the project's version header will be placed should match the project's regular
# header paths
string(TOLOWER link/version.h VERSION_HEADER_LOCATION)

packageProject(
  NAME link
  VERSION ${PROJECT_VERSION}
  NAMESPACE link
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/link-${PROJECT_VERSION}
  VERSION_HEADER "${VERSION_HEADER_LOCATION}"
  COMPATIBILITY SameMajorVersion
  DEPENDENCIES "fmt 9.1.0"
)

add_custom_target(sendToTestPC DEPENDS cli)

add_custom_command(
  TARGET sendToTestPC
  POST_BUILD
  COMMAND scp ${CMAKE_BINARY_DIR}/cli tadek@192.168.0.129:Downloads/
  COMMENT "Send the binary to a test PC..."
)