cmake_minimum_required(VERSION 3.12)

project(As)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(as INTERFACE)
target_include_directories(as INTERFACE include)

set(AS_BUILD_EXAMPLES FALSE CACHE BOOL "Build examples for as library")
if(AS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
