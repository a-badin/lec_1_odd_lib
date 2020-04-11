cmake_minimum_required(VERSION 3.2)

project(ipc)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(lib/utils)

add_executable(unix src/unix.cpp)
target_link_libraries(unix utils)

add_executable(mmap src/mmap.cpp)
target_link_libraries(mmap utils pthread)

add_executable(shared src/shared.cpp)
target_link_libraries(shared utils pthread rt)

add_executable(signals src/signals.cpp)
target_link_libraries(signals utils)
