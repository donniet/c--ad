cmake_minimum_required(VERSION 4.2)
project(c++ad VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)


add_compile_options("-fexperimental-library")
add_compile_options("-stdlib=libc++")
include_directories(include)

## Dependencies
find_package(TBB REQUIRED)

file(GLOB_RECURSE LIB_MODULE_FILES
    *.cppm
)

message("modules file list: ${LIB_MODULE_FILES}")

# add_compile_options(-fmodules -fbuiltin-module-map -stdlib=libc++)
# add_compile_options(-stdlib=libc++)

add_subdirectory(lib)
add_subdirectory(experiments)
 
add_executable(c++ad test.cpp)
target_compile_definitions(c++ad PRIVATE PARALLEL)
target_link_libraries(c++ad PRIVATE TBB::tbb volume linalg)
