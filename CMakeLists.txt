cmake_minimum_required(VERSION 3.14...3.22)

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
CPMAddPackage("gh:catchorg/Catch2@3.5.2")
CPMAddPackage("gh:TheLartians/Format.cmake@1.7.3")
CPMAddPackage("gh:microsoft/GSL#v4.0.0")

# +--------------------------------------+
# | Flags and includes common for the    |
# | lib and the tests                    |
# +--------------------------------------+

# Note: for header-only libraries change all PUBLIC flags to INTERFACE and create an interface
# target: add_library(${PROJECT_NAME} INTERFACE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
include_directories(${GSL_SOURCE_DIR}/include)
include_directories("deps")

# +--------------------------------------+
# | Library target                       |
# +--------------------------------------+

add_library(link)
add_subdirectory(src)
set_target_properties(link PROPERTIES CXX_STANDARD 23)
target_include_directories(link PRIVATE src)
target_include_directories(
  link PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include/link-${PROJECT_VERSION}>
)

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(link PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")

include (FindPkgConfig)
pkg_check_modules (LIBUSBX REQUIRED "libusb-1.0")
target_include_directories (link PRIVATE ${LIBUSBX_INCLUDE_DIRS})
target_link_directories(link PRIVATE ${LIBUSBX_LIBRARY_DIRS})
target_link_libraries(link PRIVATE ${LIBUSBX_LIBRARIES})

# Enable warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  # add_definitions(-Wall -Wpedantic -Wextra -Werror)
  add_definitions(-Wall -Werror -Wno-unused-function)
elseif(MSVC)
  add_definitions(/W4 /WX)
  add_definitions(PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
endif()

# +--------------------------------------+
# | Unit tests                           |
# +--------------------------------------+

add_executable(unittest)
add_subdirectory(test/unit)
enable_testing()
add_test(LogicLink unittest)

target_include_directories(unittest PRIVATE ../../src ../../deps ./src ${Catch2_SOURCE_DIR}/src)
target_link_libraries(unittest Catch2::Catch2WithMain)
target_compile_definitions(unittest PRIVATE "-DUNIT_TEST=1")
target_compile_features(unittest PRIVATE cxx_std_23)
set_target_properties(unittest PROPERTIES CXX_STANDARD 23)
set_target_properties(unittest PROPERTIES CXX_EXTENSIONS false)

target_compile_options(unittest PUBLIC -O0 -g -fprofile-arcs -ftest-coverage)
target_link_options(unittest PUBLIC -fprofile-arcs -ftest-coverage)

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
