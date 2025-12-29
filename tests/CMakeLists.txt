cmake_minimum_required(VERSION 3.18)
set(PARENT_PROJECT "${PROJECT_NAME}")
project(${PARENT_PROJECT}_TESTS LANGUAGES CXX)

CPMAddPackage("gh:catchorg/Catch2#v3.11.0")

add_executable(tests slotmap.cpp)
target_link_libraries(tests PRIVATE ${PARENT_PROJECT} Catch2::Catch2WithMain)