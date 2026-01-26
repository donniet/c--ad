## c++ad 

c++ syntax -> dimensional objects and drawings

1) All dimensional objects should inherit `virtual Object`
2) Objects must be managed by a Universe, and are constructed using placement new `auto box_ptr = new ( uninvers ) Box{ };`
3) The universe will delete all managed objects when it is deleted
4) Templates wrap the operations used to create compound shapes and parse expressions.
5) Inheritance is used to add capabilities to an object
6) Dimensions are evaluated lazily (act as `unit(*)()`) and auto differentiated
    - act as a function pointer `unit(*)()`
    - can be used in Expressions
    - provide a `units::d` overload for differentiation
    - the `operator=` assignment operator must accept any another Dimension-type object

Design Principles:
* COMPILE-TIME verification of units and autodifferentiation of _Expressions_.
* _Expresssions_ are assigned to _Dimensions_
* _Constraints_ relate _Dimensions_
* EXECUTION-TIME verification of _Constraints_ and identifiation of acceptable values for _Dimensions_
* _Objects_ reference other _Objects_ and define _Constraints_
* A _Unit_ is a numeric type imbued with a real-world dimensional unit


Definitions:
* An _Object Predicate_ is the `long double operator()` required on every object
* A _Domain_ is the list of numeric types that can be passed to an _Object Predicate_

Capability Classes:
- `Object` handles allocation/deallocation and connection to a Universe. All CAD objects must inherit `virtual Object`.
- `Dimensions<...>` are a tuple of nullary functions
- `DimensionAlias<...>` acts as an alias to an element of `Dimensions<...>`

Basic Objects:
- Point: empty object representing a single point filling a 0-dimensional space
- Segment: an extrusion of a point a given amount