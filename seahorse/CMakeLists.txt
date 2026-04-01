cmake_minimum_required(VERSION 3.21)

project(seahorse LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_VARIABLE nanobind_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY
)

list(APPEND CMAKE_PREFIX_PATH "${nanobind_DIR}")
find_package(nanobind CONFIG REQUIRED)

set(SEAHORSE_NEXT_VENDOR_EIGEN_DIR "${PROJECT_SOURCE_DIR}/../libs/eigen")
set(SEAHORSE_NEXT_VENDOR_SPECTRA_DIR "${PROJECT_SOURCE_DIR}/../libs/spectra")

find_path(SEAHORSE_NEXT_EIGEN_INCLUDE_DIR
  NAMES Eigen/Core
  HINTS
    "${SEAHORSE_NEXT_VENDOR_EIGEN_DIR}"
  PATH_SUFFIXES
    ""
    eigen3
)

if(NOT SEAHORSE_NEXT_EIGEN_INCLUDE_DIR)
  message(FATAL_ERROR
    "Could not find Eigen headers (expected Eigen/Core).\n"
    "Looked in vendored dependency paths under '${SEAHORSE_NEXT_VENDOR_EIGEN_DIR}' and standard include paths.\n"
    "If you cloned this repository without submodules, run:\n"
    "  git submodule update --init --recursive"
  )
endif()

find_path(SEAHORSE_NEXT_SPECTRA_INCLUDE_DIR
  NAMES Spectra/SymEigsSolver.h
  HINTS
    "${SEAHORSE_NEXT_VENDOR_SPECTRA_DIR}"
  PATH_SUFFIXES
    include
    ""
)

if(NOT SEAHORSE_NEXT_SPECTRA_INCLUDE_DIR)
  message(FATAL_ERROR
    "Could not find Spectra headers (expected Spectra/SymEigsSolver.h).\n"
    "Looked in vendored dependency paths under '${SEAHORSE_NEXT_VENDOR_SPECTRA_DIR}' and standard include paths.\n"
    "If you cloned this repository without submodules, run:\n"
    "  git submodule update --init --recursive"
  )
endif()

add_library(seahorse_core STATIC
  src/cpp/core/grid.cpp
  src/cpp/core/objective.cpp
  src/cpp/core/potential.cpp
  src/cpp/core/propagator.cpp
  src/cpp/core/spectrum.cpp
)

set_target_properties(seahorse_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(seahorse_core
  PUBLIC
    "${PROJECT_SOURCE_DIR}/src/cpp/core"
    "${SEAHORSE_NEXT_EIGEN_INCLUDE_DIR}"
    "${SEAHORSE_NEXT_SPECTRA_INCLUDE_DIR}"
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
  "${SEAHORSE_NEXT_EIGEN_INCLUDE_DIR}"
  "${SEAHORSE_NEXT_SPECTRA_INCLUDE_DIR}"
)

install(TARGETS _core LIBRARY DESTINATION seahorse)
install(
  DIRECTORY src/python/seahorse/
  DESTINATION seahorse
  FILES_MATCHING PATTERN "*.py"
)
