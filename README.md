# C++ad 

Library for rigorous expression of physical objects using unintrusive syntax
and leveraging only the minimual international standards.  

Currently the only dependencies are a modern C++26 compiler and CMake

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
{ return m * pow<2>( v ) / 2; };

// ERROR: cannot add unit values of different types
// | auto invalid_addition( Mass m, Velocity v )
// | { return m + v; }
// |____________^
```

### Tensors
Header-only C++ library for type-safe vector, matrix, and tensor operations 
on rectangular arrays of arbitrary arithmetic types.

### Expressions
Header-only C++ library for lazily evaluated expressions.  Auto-differentiation
and common solvers are included.

### Geometry
Header-only C++ library for modelling physical objects and systems.

## Other Names

This project is also known as c--ad due to github naming rules and to welcome all C programmers despite their opinions of C++.