cmake_minimum_required(VERSION 3.18)
project(TRUtils LANGUAGES CXX)

# Global Requirements
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Options
option(${PROJECT_NAME}_BUILD_TESTS "Build tests" ON)

# Build Artifact
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/)

# Compiler Setup
include(cmake/CompilerWarnings.cmake)

# Dependencies
include(cmake/CPM.cmake)
CPMAddPackage(
  NAME CPMLicenses.cmake 
  GITHUB_REPOSITORY cpm-cmake/CPMLicenses.cmake
  VERSION 0.0.5
)

# Packaging/Installing
# TODO: Adjust the output folder for this
cpm_licenses_create_disclaimer_target(
  write-licenses "${CMAKE_CURRENT_BINARY_DIR}/NOTICE.md" "${CPM_PACKAGES}"
)


# Tests
if(NOT ${PROJECT_NAME}_BUILD_TESTS)
  return()
endif()

message("Configuring Tests For ${PROJECT_NAME}")
include(CTest)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)