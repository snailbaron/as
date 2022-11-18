cmake_minimum_required(VERSION 3.12)

project(As)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

add_library(as INTERFACE)
target_include_directories(as INTERFACE include)

add_subdirectory(examples)
