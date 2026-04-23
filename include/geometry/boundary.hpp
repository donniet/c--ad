#ifndef __GEOMETRY_BOUNDARY_HPP__
#define __GEOMETRY_BOUNDARY_HPP__

#include "geometry/space.hpp"
#include "geometry/attribute.hpp"
#include "geometry/collection.hpp"
#include "geometry/compound.hpp"
#include "geometry/orient.hpp"
#include "geometry/project.hpp"
#include "geometry/extrude.hpp"
#include "geometry/translate.hpp"

#include <numeric>

namespace geometry { 

template< typename ObjT >
struct Boundary
{
    using object_type = ObjT;
    static constexpr size_t dimensions() { return object_type::dimensions(); }
    static constexpr size_t parameters() { return object_type::parameters() - 1; }

    constexpr object_type object() { return _object; }

    constexpr Boundary( object_type object ): _object{ object } { }

    object_type _object;
};

/// @brief boundary of a point is empty
/// @param Point parameter
/// @return empty space
constexpr Empty<> boundary( Point ) { return {}; }

/// @brief boundary of empty space is empty
/// @param ObjT type of empty object
/// @return an empty in the space of ObjT
template< typename ObjT >
requires( is_empty< ObjT > )
constexpr Empty< ObjT::dimensions() > boundary( ObjT const& ) { return {}; }

/// @brief boundary of an attributed object attributes the boundary
/// @tparam ObjT attributed object type
/// @tparam T attribute type
/// @param attributed attribute object
/// @return the attributed boundary object
template< typename ObjT, typename T >
requires( not is_empty< ObjT > )
constexpr auto boundary( Attribution< ObjT, T > attributed )
{ return attribute( boundary( attributed.object() ), attributed.attribute() ); }

/// @brief boundary of an oriented object is the orientation of the boundary
/// @tparam ObjT oriented object type
/// @param ori oriented object
/// @return the boundary of the oriented object re-oriented
template< typename ObjT >
requires( not is_empty< ObjT > )
constexpr auto boundary( Orientation< ObjT > ori )
{ return orient( boundary( ori.object() )); }

template< typename CollectionT, size_t... Is >
constexpr auto collection_boundary_helper( CollectionT col, seq< Is... > )
{ return collection( boundary( get< Is >( col ))... ); }

/// @brief the boundary of a collection is a collection of all the boundaries
/// @tparam ...Objects
/// @param collection 
/// @return 
template< typename... Objects >
requires( not is_empty< Collection< Objects... >> )
constexpr auto boundary( Collection< Objects... > col )
{ return collection_boundary_helper( col, 
    make_seq< sizeof...( Objects )>{} ); }

/// TODO: I think we need something more sophisticated here...
template< typename ObjT, typename TransformT, size_t Dim >
requires( not is_empty< ObjT > )
auto boundary( Projection< ObjT, TransformT, Dim > projected )
{ return project< Dim >( boundary( projected.object() ), 
    projected.transform() ); }

/// @brief boundary components of an extrusion
struct extrusion {
    struct base { }; 
    struct shell { };
    struct cap { };
};

/*

Example of matrix multiplication to a higher dimension
------------------
1 0  0.25    0.25
0 1  0.125 = 0.125
2 4          1. 

And an example where this doesn't work
--------------------
1 0  0        0
0 1  0     =  0
? ?           ?

We might need to add translation...
or we could have special projection transforms like a ProjectAt transform
that sets the additional dimensions of the projection to constants?

*/

/// @brief 
/// @tparam ObjT 
/// @tparam U 
/// @param extruded 
/// @return 
template< typename ObjT, typename FromT, typename ToT >
requires( not is_empty< ObjT > )
auto boundary( Extrusion< ObjT, FromT, ToT > extruded )
{ 
    auto base = project_at( extruded.object(), extruded.from() );
    auto shell = extrude( boundary( extruded.object() ),
        extruded.from(), extruded.to() );
    auto cap = project_at( extruded.object(), extruded.to() );

    return MakeCompound< 
        extrusion::base, extrusion::shell, extrusion::cap >::make(
        base,            shell,            cap );
}

/// @brief boundary of a line segment
template< typename FromT, typename ToT >
auto boundary( Extrusion< Point, FromT, ToT > extruded )
{ 
    auto base = project_at( extruded.object(), extruded.from() );
    auto cap = project_at( extruded.object(), extruded.to() );

    return MakeCompound< 
        extrusion::base, extrusion::cap >::make(
        base,            cap );
}

} // namespace geometry

#endif