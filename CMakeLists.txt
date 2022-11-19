cmake_minimum_required(VERSION 3.12)

project(As)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(as
    coro.cpp
)
target_include_directories(as PUBLIC include)

set(AS_SANITIZE "" CACHE STRING "value for -fsanitize compiler option")
if(AS_SANITIZE)
    target_compile_options(as PRIVATE "-fsanitize=${AS_SANITIZE}")
    target_link_options(as PRIVATE "-fsanitize=${AS_SANITIZE}")
endif()

set(AS_BUILD_EXAMPLES FALSE CACHE BOOL "Build examples for as library")
if(AS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
