cmake_minimum_required(VERSION 3.14...3.22)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

project(LogicLinkTests VERSION 1.0 LANGUAGES CXX)
option(ENABLE_TEST_COVERAGE "Enable test coverage" OFF)
# add_compile_options("-fsanitize=address")
# add_link_options("-fsanitize=address")

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

include(../../cmake/CPM.cmake)
include(../../cmake/tools.cmake)

# PackageProject.cmake will be used to make our target installable
CPMAddPackage("gh:microsoft/GSL@4.1.0")
CPMAddPackage("gh:catchorg/Catch2@3.5.2")
CPMAddPackage("gh:TheLartians/Format.cmake@1.7.3")

if(LOCAL_DEPENDENCIES)
  message("+---------------------------------------------------+")
  message("| ${PROJECT_NAME}\tis using local library versions |")
  message("+---------------------------------------------------+")
  CPMAddPackage(NAME common SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../common/logic-link-common)
  CPMAddPackage(NAME LogicLink SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
else()
  CPMAddPackage(NAME common GIT_REPOSITORY "https://${GITHUB_USERNAME}:${GITHUB_TOKEN}@github.com/iwasz/logic-link-common.git" GIT_TAG v0.0.2)
  CPMAddPackage(NAME LogicLink GIT_REPOSITORY "https://${GITHUB_USERNAME}:${GITHUB_TOKEN}@github.com/iwasz/logic-link-lib.git" GIT_TAG v0.0.2)
endif()

# +--------------------------------------+
# | Unit tests                           |
# +--------------------------------------+

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
  PRIVATE
    backend.cc
    bitSpan.cc
    debugIntegrity.cc
    eventQueue.cc
    frontend.cc
    generate.cc
    queue.cc
    rearrange.cc
    uart.cc
    rangeBitSpan.cc
    downsample.cc
    rangeWordSpan.cc
    downsamplePopcount.cc

  PUBLIC FILE_SET CXX_MODULES FILES
    utils.ccm
)

enable_testing()
add_test(${PROJECT_NAME} ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE
  ${Catch2_SOURCE_DIR}/src
  ${GSL_SOURCE_DIR}/include
  ${common_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} Catch2::Catch2WithMain LogicLink fmt Tracy::TracyClient)
# target_compile_definitions(${PROJECT_NAME} PRIVATE "-DUNIT_TEST=1")
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS false)

# Enable warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wno-unused-function)
elseif(MSVC)
  target_compile_options(${PROJECT_NAME} PUBLIC /W4 /WX)
  target_compile_options(${PROJECT_NAME} PUBLIC PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
endif()

if(ENABLE_TEST_COVERAGE)
  target_compile_options(LogicLink PUBLIC -O0 -g -fprofile-arcs -ftest-coverage)
  target_link_options(LogicLink PUBLIC -fprofile-arcs -ftest-coverage)
endif()
