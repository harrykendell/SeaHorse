cmake_minimum_required(VERSION 3.21)

project(seahorse_next LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_VARIABLE nanobind_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY
)

list(APPEND CMAKE_PREFIX_PATH "${nanobind_DIR}")
find_package(nanobind CONFIG REQUIRED)

set(SEAHORSE_NEXT_EIGEN_DIR "${PROJECT_SOURCE_DIR}/../libs/eigen")
set(SEAHORSE_NEXT_SPECTRA_DIR "${PROJECT_SOURCE_DIR}/../libs/spectra/include")

add_library(seahorse_core STATIC
  src/cpp/core/grid.cpp
  src/cpp/core/objective.cpp
  src/cpp/core/potential.cpp
  src/cpp/core/propagator.cpp
  src/cpp/core/spectrum.cpp
)

target_include_directories(seahorse_core
  PUBLIC
    "${PROJECT_SOURCE_DIR}/src/cpp/core"
    "${SEAHORSE_NEXT_EIGEN_DIR}"
    "${SEAHORSE_NEXT_SPECTRA_DIR}"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(seahorse_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

nanobind_add_module(_core
  src/cpp/bindings/module.cpp
)

target_link_libraries(_core PRIVATE seahorse_core)
target_include_directories(_core PRIVATE
  "${PROJECT_SOURCE_DIR}/src/cpp/core"
  "${SEAHORSE_NEXT_EIGEN_DIR}"
  "${SEAHORSE_NEXT_SPECTRA_DIR}"
)

install(TARGETS _core LIBRARY DESTINATION seahorse)
install(
  DIRECTORY src/python/seahorse/
  DESTINATION seahorse
  FILES_MATCHING PATTERN "*.py"
)

