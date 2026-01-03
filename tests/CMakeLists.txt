cmake_minimum_required(VERSION 3.18)
set(PARENT_PROJECT "${PROJECT_NAME}")
project(${PARENT_PROJECT}_TESTS LANGUAGES CXX)

CPMAddPackage("gh:catchorg/Catch2#v3.11.0")

add_executable(${PARENT_PROJECT}_tests slotmap.cpp type_erased_vector.cpp table.cpp)
target_link_libraries(${PARENT_PROJECT}_tests PRIVATE ${PARENT_PROJECT} Catch2::Catch2WithMain)