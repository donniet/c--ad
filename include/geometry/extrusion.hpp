#ifndef __GEOMETRY_EXTRUSION_HPP__
#define __GEOMETRY_EXTRUSION_HPP__

#include "geometry/space.hpp"

namespace geometry { 

template< typename ObjT, typename U = long double >
struct Extrusion 
{ 
    using object_type = ObjT;
    using parameter_type = U;
    using space_type = ExtrudeSpace< space_of< object_type >, 
        unit_for< parameter_type >>::space_type;

    static constexpr string object_name()
    { return "extrusion_" + object_type::object_name(); }

    constexpr object_type object() { return _object; }
    constexpr parameter_type parameter() { return _parameter; }

    object_type _object;
    parameter_type _parameter;
};

template< typename ObjT, typename U >
struct IsEmpty< Extrusion< ObjT, U >>: 
    integral_constant< bool, IsEmpty< ObjT >::value > { };

template< typename U, typename Point >
Extrusion< Point, U > extrude( Point point )
{ return { point }; }

template< typename U, typename CollectionT, size_t... Is >
auto extrude_collection_helper( CollectionT col, seq< Is... > )
{ return collection( extrude< U >( get< Is >( col ))... ); }

template< typename U, typename... Objects >
requires( not ( IsEmpty< Objects > and ... ))
auto extrude( Collection< Objects... > col )
{ return extrude_collection_helper< U >( col, 
    make_seq< sizeof...( Objects )>{} ); }



template< typename U, typename ComponentsT, typename Objects... >
auto extrude( );

} // namespace geometry 


#endif