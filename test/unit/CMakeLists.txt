cmake_minimum_required(VERSION 3.14...3.22)

project(LibUnitTest LANGUAGES CXX C)

# ---- Options ----

option(ENABLE_TEST_COVERAGE "Enable test coverage" OFF)
option(TEST_INSTALLED_VERSION "Test the version found by find_package" OFF)

# --- Import tools ----

include(../../cmake/tools.cmake)

# ---- Dependencies ----

include(../../cmake/CPM.cmake)

CPMAddPackage("gh:catchorg/Catch2@3.5.2")
CPMAddPackage("gh:TheLartians/Format.cmake@1.7.3")
CPMAddPackage("gh:microsoft/GSL#v4.0.0")

include_directories(${GSL_SOURCE_DIR}/include)
add_definitions("-DUNIT_TEST=1")
# add_definitions("-fsanitize=address")

# ---- Create binary ----

include_directories(../../src/lib)
include_directories(../../deps)
include_directories(./src)
include_directories(${Catch2_SOURCE_DIR}/src)
add_executable(${PROJECT_NAME} ${sources})

target_sources(${PROJECT_NAME} PRIVATE
  src/queue.cc
  src/debugIntegrity.cc
  src/rearrange.cc
  ../../src/lib/analysis/debug/clockSignal.cc
  ../../src/lib/analysis/rearrange.cc
)

target_link_libraries(${PROJECT_NAME} Catch2::Catch2WithMain)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS false)

# enable compiler warnings
if(NOT TEST_INSTALLED_VERSION)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wpedantic -Wextra -Werror -Wno-unused -O0 -g)
  elseif(MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /W4 /WX)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
  endif()
endif()

# ---- Add UnitTest ----

enable_testing()
add_test(${PROJECT_NAME} ${PROJECT_NAME})

# ---- code coverage ----

# if(ENABLE_TEST_COVERAGE)
  target_compile_options(${PROJECT_NAME} PUBLIC -O0 -g -fprofile-arcs -ftest-coverage)
  target_link_options(${PROJECT_NAME} PUBLIC -fprofile-arcs -ftest-coverage)
# endif()
