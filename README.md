# C++ad 

Library for rigorous expression of physical objects using unintrusive syntax
and leveraging only the minimual international standards.  

Currently the only dependencies are a modern C++26 compiler (clang++-21), the ninja build system (for c++ modules) and cmake

## Build Environment

Build works using clang++-21, cmake (4.2+), and ninja-build. See the [.github/workflows/cmake-single-platform.yml] for all expected dependencies and an example build environment setup.  If you just want to get it to compile as it stands currently below are instructions for linux+bash, mac+zsh, and windows+bat.

### `setup.sh` - Linux Bash Build Environment Setup

```bash
# install prerequisites
wget -O - https://apt.llvm.org/llvm.sh | sudo bash -s -- 21
sudo apt update && sudo apt install cmake ninja-build clang-21 clang++-21

# setup build folder
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++-21 --fresh

# compile all targets
cmake --build build

# run tests
ctest --test-dir build
```

### `setup.zsh` - MacOS Zsh Build Environment Setup

```zsh
# install prerequisites
brew install cmake ninja llvm@21
export PATH="/opt/homebrew/opt/llvm@21/bin:$PATH"

# setup build folder
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++-21 --fresh

# compile all targets
cmake --build build

# run tests
ctest --test-dir build
```

### `setup.bat` - Windows Build Environment Setup (credit: Google Gemini)

```bat
echo Installing CMake, Ninja, and LLVM...
winget install Kitware.CMake --silent --accept-source-agreements --accept-package-agreements
winget install Ninja-build.Ninja --silent --accept-source-agreements --accept-package-agreements
winget install LLVM.LLVM --silent --accept-source-agreements --accept-package-agreements

echo Close and Re-open Command Window to Refresh PATH...

echo setup build folder
cmake -S . -B build/windows -G Ninja -DCMAKE_CXX_COMPILER=clang++-21 --fresh

echo compile all targets
cmake --build build/windows

echo run tests
ctest --test-dir build
```

## Goals

### 1. Expressability
Rigorous expression of quantifiable physical systems. Code in this library must be spare of extraneous logic and implement only what is required by the goals listed here.

### 2. Validation
Descriptive validation of physical expressions

### 3. Interopability
Simple generation of models for external tools, applications, and libraries.  Direct interopability for common physics and graphics libraries like OpenGL.

### 4. Linking
Simple linking of library capabilities to external applications and libraries

### 5. Dependencies
Minimal build and run dependencies.  Building and linking must use common C++ tooling such as CMake and Make. Should be buildable with common, open-source, C++ compilers including clang++ and g++.

### 6. Simplicity
Semantics of geometric and physical expressions and constraints accessible to novice C++ programmers

### 7. Componentization
Components should be loosely interdependent allowing for "copy-paste" re-use of individual components where possible.

## Components
C++ad is made from loosely-dependent component libraries.

### Units
Header-only C++ library for type-safe physical units such as length, time and mass. Arithmetic operations on unit types validate expressions automatically, and convert unit types simuntaneous with unit values. 

```
Energy kinetic( Mass m, Velocity v )
{ return m * pow<2>( v ) / 2; }

// ERROR: cannot add unit values of different types
// auto invalid_addition( Mass m, Velocity v )
// { return m + v; }
```

### Tensors
Header-only C++ library for type-safe vector, matrix, and tensor operations 
on rectangular arrays of arbitrary arithmetic types.

### Expressions
Header-only C++ library for lazily evaluated expressions.  Auto-differentiation
and common solvers are included.

### Geometry
Header-only C++ library for modelling physical objects and systems.

## Definition of Terms

### `trait`
A templated class with static data members used to ascribe named characteristics to other classes.

### `type maniuplator`
A templated class with a using declaration `type` and a static method `value( T const& )` used to transform the type and/or instance of one class into another.

### `tuple-like`
A templated class, often variadic, that contains values of the template types.  It must conform to the following guidelines:
- be default constructable
- be constructable from a parameter pack of it's template parameters
- support std::tuple_size_v, std::tuple_element_t, and std::get


## Other Names

This project is also known as c--ad due to github naming rules and to welcome all C programmers despite their opinions of C++.
