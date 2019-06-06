cmake_minimum_required(VERSION 3.10.2)
project(Variant)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(test_lib test/test_lib.cpp)
target_link_libraries(test_lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libtotal_variant.a)

add_executable(test_header test/test_header.cpp)

add_library(total_variant STATIC src/total_variant.cpp)