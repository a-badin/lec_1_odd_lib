cmake_minimum_required(VERSION 3.10)

project(adv_cpp)

add_subdirectory(odd_lib)
#add_subdirectory(ipc)
#add_subdirectory(movable_function)

find_package(GTest REQUIRED)
add_executable(test test.cpp)
target_link_libraries(test GTest::Main)
