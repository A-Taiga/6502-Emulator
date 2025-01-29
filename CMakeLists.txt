cmake_minimum_required(VERSION 3.10)
project(6502 VERSION 0.1)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -g")


add_executable(Emulator main.cpp)

add_subdirectory(cpu)
add_subdirectory(bus)
add_subdirectory(ui)

target_link_libraries(Emulator BUS)
target_link_libraries(Emulator CPU)
target_link_libraries(Emulator GUI)