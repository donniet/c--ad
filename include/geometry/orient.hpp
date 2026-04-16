#ifndef __GEOMETRY_ORIENT_HPP__
#define __GEOMETRY_ORIENT_HPP__

#include "geometry/space.hpp"

namespace geometry {

/// @brief an oriented object
/// @tparam ObjectT the type of the object
template< typename ObjT >
struct Orientation
{
    using object_type = ObjT;
    using space_type = space_of< object_type >;
    using vector_type = space_type::vector_type;

    static string object_name() 
    { return "orientation_" + object_type::object_name(); }

    object_type object;
    vector_type orientation;
};

template< typename ObjT >
struct IsEmpty< Orientation< ObjT >>
{ static constexpr bool value = IsEmpty< ObjT >::value; };

/// @brief orienting an object
/// @tparam ObjectT type of the object
/// @param object instance
/// @param orientation vector
/// @return an object + orientation pair
template< typename ObjectT >
Orientation< ObjectT > orient( ObjectT const& object, 
    typename space_of< ObjectT >::vector_type orientation )
{ return { object, orientation }; }

/// @brief re-orienting an object overrides the orientation
/// @tparam ObjectT type of the object
/// @param oriented instance
/// @param orientation vector
/// @return an object + orientation pair
/// TODO: re-orient? 
template< typename ObjT >
Orientation< ObjT > orient( Orientation< ObjT > const& oriented, 
    typename space_of< ObjT >::vector_type orientation )
{ return { oriented.object(), orientation }; }


template< typename ObjT >
vector_for< ObjT > get_orientation( ObjT const& object )
{ return { }; }

template< typename ObjT >
vector_for< ObjT > get_orientation( Orientation< ObjT > const& object )
{ return object.orientation(); }

template< typename ObjT, typename AttT >
vector_for< ObjT > get_orientation( Attribution< ObjT, AttT > const& attributed )
{ return get_orientation( attributed.object() ); }

template< typename ObjT, typename U >
vector_for< ObjT > get_orientation( Projection< ObjT, U > const& projected )
{ return ExtrudeSpace< space_of< ObjT >, U >::base( get_orientation( 
    projected.object() )); }

// template< typename ObjT, typename U, size_t Steps >
// vector_for< ObjT > get_orientation( Extrusion< ObjT, U, Steps > const& extruded )
// { return ExtrudeSpace< space_of< ObjT >, U >::base( get_orientation( 
//     extruded.object() )); }


} // namespace geometry 

#endif