add_subdirectory(SDL)

add_executable(as-example-02-tree-eater 02-tree-eater.cpp)
target_link_libraries(as-example-02-tree-eater as SDL2::SDL2)
if(MSVC)
    target_compile_options(as-example-02-tree-eater PRIVATE /W4 /WX)
else()
    target_compile_options(as-example-02-tree-eater PRIVATE -Wall -Wextra -pedantic -Werror)
endif()
