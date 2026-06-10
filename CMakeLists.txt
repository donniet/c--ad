cmake_minimum_required(VERSION 4.2)
project(c++ad VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_COLOR_MAKEFILE=ON)

add_compile_options("-ftemplate-backtrace-limit=0")
add_compile_options("-fconstexpr-backtrace-limit=0")
#add_compile_options("-ferror-limit=1")
#add_compile_options("-fno-inline")
add_compile_options("-fdiagnostics-show-template-tree")
add_compile_options("-fcolor-diagnostics")
add_compile_options("-fansi-escape-codes")

# add_compile_options("-fexperimental-library")
# add_compile_options("-stdlib=libstdc++")
# add_compile_options("-stdlib=libc++" "-v")
# add_link_options("-static")
# link_libraries("c++abi" "c++")
include_directories(include ../vcglib/)

## Dependencies
find_package(TBB REQUIRED)
find_package(Eigen3 3.4 REQUIRED)

file(GLOB_RECURSE LIB_MODULE_FILES
    *.cppm
)

message("modules file list: ${LIB_MODULE_FILES}")

# add_compile_options(-fmodules -fbuiltin-module-map -stdlib=libc++)
# add_compile_options(-stdlib=libc++)

add_subdirectory(lib)

enable_testing()
add_subdirectory(test)

add_subdirectory(experiments)
 
add_executable(c++ad test.cpp)
target_compile_definitions(c++ad PRIVATE PARALLEL)
target_link_libraries(c++ad PRIVATE TBB::tbb volume linalg)

