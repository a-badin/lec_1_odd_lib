cmake_minimum_required(VERSION 3.10)

project(odd)
add_library(odd SHARED src/odd.cpp)
target_include_directories(odd PUBLIC include)
add_subdirectory(example)
