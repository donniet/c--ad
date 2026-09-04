cmake_minimum_required(VERSION 4.2)
project(c++ad VERSION 0.1.0 LANGUAGES C CXX)

if(APPLE)
    set(CMAKE_PREFIX_PATH /opt/homebrew/opt/llvm@21)
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -D_LIBCPP_DISABLE_AVAILABILITY)
    link_directories(/opt/homebrew/opt/llvm@21/lib/c++ /opt/homebrew/opt/llvm@21/lib/unwind)
    link_libraries(unwind)
    set(C__AD_PARALLEL_LIBS "")
else()
# add_compile_options("-fexperimental-library")
# add_compile_options("-stdlib=libstdc++")
# add_compile_options("-stdlib=libc++" "-v")
# add_link_options("-static")
# link_libraries("c++abi" "c++")

## Dependencies
#removing dependency on TBB to simplify build
#find_package(TBB REQUIRED)
#set(C__AD_PARALLEL_LIBS TBB:tbb)
endif()

#if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
#    message(FATAL_ERROR "This project requires the Clang compiler. Current compiler: ${CMAKE_CXX_COMPILER_ID}")
#endif()
#if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "21.0")
#    message(FATAL_ERROR "Clang version 21.0 or higher is required. Current version: ${CMAKE_CXX_COMPILER_VERSION}")
#endif()

# DT: removing vcglib to to simplify build
include_directories(include ../vcglib/)
#find_package(Eigen3 REQUIRED)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# DT: disable extensions to test reflection
#set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_COLOR_MAKEFILE=ON)

if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    add_compile_options("-Wnormalized=none")
endif()

# debug options
add_compile_options("-ftemplate-backtrace-limit=0")
add_compile_options("-fdiagnostics-show-template-tree")
#add_compile_options("-fno-inline")

if( CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # debug options (CLANG)
    add_compile_options("-fshow-overloads=all")
    add_compile_options("-fconstexpr-backtrace-limit=0")
    add_compile_options("-ferror-limit=1")
    add_compile_options("-fcolor-diagnostics")
    add_compile_options("-fansi-escape-codes")
elseif( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    # debug options (GNU)
    add_compile_options("-fdiagnostics-color")
    add_compile_options("-fdiagnostics-all-candidates")
    add_compile_options("-fmax-errors=1")
    add_compile_options("-fdiagnostics-color=always")
    #add_compile_options("-ftemplate-depth=0")
endif()

# reflection args
if( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
    # enabling reflection (CLANG)
    add_compile_options("-fexperimental-library")
elseif( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    # enabling reflection (GNU)
    add_compile_options("-freflection")
endif()

# DT: removing modules for now to simplify build
#file(GLOB_RECURSE LIB_MODULE_FILES
#    *.cppm
#)

#message("modules file list: ${LIB_MODULE_FILES}")

# add_compile_options(-fmodules -fbuiltin-module-map -stdlib=libc++)
# add_compile_options(-stdlib=libc++)

#add_subdirectory(lib)

enable_testing()
add_subdirectory(test)

# DT: removing experiments subdirectory to simplify build
#add_subdirectory(experiments)
 
#add_executable(c++ad test.cpp)
#target_compile_definitions(c++ad PRIVATE PARALLEL)
#target_link_libraries(c++ad PRIVATE ${C__AD_PARALLEL_LIBS} volume linalg)

