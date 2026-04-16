#ifndef __GEOMETRY_BOUNDARY_HPP__
#define __GEOMETRY_BOUNDARY_HPP__

#include "geometry/space.hpp"
#include "geometry/attribute.hpp"
#include "geometry/collection.hpp"
#include "geometry/compound.hpp"
#include "geometry/linear_transform.hpp"
#include "geometry/orient.hpp"
#include "geometry/project.hpp"
#include "geometry/translate.hpp"
#include "geometry/extrusion.hpp"

namespace geometry { 

/// @brief boundary of a point is empty
/// @param Point parameter
/// @return empty space
constexpr Empty< null_space > boundary( Point ) { return {}; }

/// @brief boundary of empty space is empty
/// @param ObjT type of empty object
/// @return an empty in the space of ObjT
template< typename ObjT >
requires( is_empty< ObjT > )
constexpr Empty< space_of< ObjT >> boundary( ObjT const& ) { return {}; }

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
{ return orient( boundary( ori.object() ), ori.orientation() ); }

template< typename CollectionT, size_t... Is >
constexpr auto collection_boundary_helper( CollectionT col, 
    seq< Is... > )
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

template< typename ObjT, space SpaceU >
requires( not is_empty< ObjT > )
auto boundary( Projection< ObjT, SpaceU > projected )
{ return project< SpaceU >( boundary( projected.object() )); }

/// @brief boundary components of an extrusion
struct extrusion {
    struct base { }; 
    struct shell { };
    struct cap { };
};

/// @brief 
/// @tparam ObjT 
/// @tparam U 
/// @param extruded 
/// @return 
template< typename ObjT, typename U >
requires( not is_empty< ObjT > )
auto boundary( Extrusion< ObjT, U > extruded )
{ 
    using extruded_space = extrude_space_t< space_of< ObjT >, U >;

    MakeCompound< 
        extrusion::base,
        extrusion::shell,
        extrusion::cap >::make(
    /* base  */ project< extruded_space >( extruded.object() ),
    /* shell */ extrude< U >( boundary( extruded.object(), extruded.parameter() )),
    /* cap   */ translate( project< extruded_space >( extruded.object() ), 
                    extruded_space::displacement( extruded.parameter() )));
}

// template< typename ObjT >
// requires( not is_empty< ObjT > )
// auto boundary( Component< extrusion_boundary::base, ObjT > base )
// { return boundary(  ); }


// template< typename ObjT >
// requires( not is_empty< ObjT > )
// Boundary< Padding< ObjT >> boundary( Padding< ObjT > padded )
// { return { padded }; }

} // namespace geometry

#endif