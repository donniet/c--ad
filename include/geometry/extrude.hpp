#ifndef __GEOMETRY_EXTRUSION_HPP__
#define __GEOMETRY_EXTRUSION_HPP__

#include "geometry/space.hpp"

namespace geometry { 

template< typename ObjT, typename FromT, typename ToT >
struct Extrusion: Object
{ 
    using object_type = ObjT;
    using from_type = FromT;
    using to_type = ToT;

    static constexpr size_t dimensions() 
    { return object_type::dimensions() + 1; }

    static constexpr size_t parameters() 
    { object_type::parameters() + 1; };

    static constexpr string object_name()
    { return "extrusion_" + object_type::object_name(); }

    constexpr object_type object() { return _object; }
    constexpr from_type from() { return _from; }
    constexpr to_type to() { return _to; }

    constexpr Extrusion( object_type object, 
        from_type from = static_cast< from_type >( 0 ),
        to_type to = static_cast< to_type >( 1 )):
            _object{ object }, _from{ from }, _to{ to } { }

    object_type _object;
    from_type _from;
    to_type _to;
};

template< typename ObjT, typename FromT, typename ToT >
struct IsEmpty< Extrusion< ObjT, FromT, ToT >>: IsEmpty< ObjT > { };

template< typename ObjT, typename FromT = scalar_type, typename ToT = FromT >
requires( is_empty< ObjT >)
constexpr Empty< ObjT::dimensions() > extrude( ObjT, 
    FromT from = static_cast< FromT >( 0 ), ToT to = static_cast< ToT >( 1 ))
{ return { }; }

template< typename ObjT, typename FromT = scalar_type, typename ToT = FromT >
requires( not is_empty< ObjT >)
constexpr Extrusion< ObjT, FromT, ToT > extrude( ObjT object, 
    FromT from = static_cast< FromT >( 0 ), ToT to = static_cast< ToT >( 1 ))
{ return { object, from, to }; }

template< typename CollectionT, typename FromT, typename ToT, size_t... Is >
auto extrude_collection_helper( CollectionT col, seq< Is... >, FromT from, ToT to )
{ return collection( extrude( get< Is >( col ), from, to )... ); }

template< typename FromT, typename ToT, typename... Objects >
requires( not ( is_empty< Objects > and ... ))
auto extrude( Collection< Objects... > col, FromT from, ToT to )
{ return extrude_collection_helper( col, from, to, make_seq< sizeof...( Objects )>{} ); }

// template< typename U, typename ComponentsT, typename... Objects >
// auto extrude( );

} // namespace geometry 


#endif