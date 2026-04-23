#ifndef __GEOMETRY_ORIENT_HPP__
#define __GEOMETRY_ORIENT_HPP__

#include "geometry/space.hpp"

namespace geometry {

/// @brief an oriented object
/// @tparam ObjectT the type of the object
template< typename ObjT > //, typename OrientationT >
struct Orientation: Object
{
    using object_type = ObjT;
    static constexpr size_t dimensions() { return object_type::dimensions(); }

    static string object_name() 
    { return "orientation_" + object_type::object_name(); }

    constexpr object_type object() { return _object; }


    object_type _object;
};

template< typename ObjT >
struct IsEmpty< Orientation< ObjT >>: IsEmpty< ObjT > { };

template< typename ObjT >
Orientation< ObjT > orient( ObjT object )
{ return { object }; }

} // namespace geometry 

#endif