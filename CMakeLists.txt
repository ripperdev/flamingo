cmake_minimum_required(VERSION 2.6)

project(flamingo)

find_program(iwyu_path include-what-you-use)
if(NOT iwyu_path)
    message(FATAL_ERROR "Could not find the program: include-what-you-use")
endif()
set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -O0 -Wno-unused-variable -pthread")

add_subdirectory(base)
add_subdirectory(net)
add_subdirectory(utils)
add_subdirectory(ChatServer)
add_subdirectory(FileServer)
add_subdirectory(ImgServer)
