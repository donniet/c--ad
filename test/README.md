## c++ad 

c++ syntax -> dimensional objects and drawings

1) All dimensional objects should inherit `virtual Object`
2) Objects must be managed by a Universe, and are constructed using placement new `auto box_ptr = new ( uninvers ) Box{ };`
3) The universe will delete all managed objects when it is deleted
4) Templates wrap the operations used to create compound shapes and parse expressions.
5) Inheritance is used to add capabilities to an object
6) Dimensions are evaluated lazily and auto differentiated

Capability Classes:
- _Object_: handles allocation/deallocation and connection to a Universe. All CAD objects must inherit `virtual Object`.
- Positioned: imbues an object with a position `dim<unit,...>`
- Oriented: imbues an object with an orientation `dim<unit,...>`
- BoundedBy: imbues an object with a bounday `Array<Obj,...>`

Basic Objects:
- Point: empty object representing a single point filling a 0-dimensional space
- Segment: an extrusion of a point a given amount