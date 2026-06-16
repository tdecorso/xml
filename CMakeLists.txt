cmake_minimum_required(VERSION 3.31)

project(xml)

set(CMAKE_BUILD_TYPE DEBUG)

add_compile_options(-Wall -Wextra -Wpedantic)

include_directories(include)

add_library(xml src/xml.c)

add_executable(test tests/test.c)

target_link_libraries(test PRIVATE xml)
