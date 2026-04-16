#ifndef __GEOMETRY_PROJECT_HPP__
#define __GEOMETRY_PROJECT_HPP__

#include "geometry/space.hpp"

namespace geometry {

/// @brief projects an object from its space to another space using a linear
/// transformatoin
/// @tparam ObjectT 
/// @tparam U 
template< typename ObjT, typename MatT >
struct Projection
{
    using object_type = ObjT;
    using transform_type = MatT;
    using input_vector_type = space_of< ObjT >::vector_type;
    using output_vector_type = decltype( matmul( MatT(), input_vector_type{} ));
    using space_type = Space< output_vector_type >;

    static constexpr size_t from_dimensions = dimensions_of_v< space_of< ObjT >>;
    static constexpr size_t to_dimensions = dimensions_of_v< space_type >;

    static string object_name() 
    { return "projection_" + object_type::object_name(); }

    constexpr object_type object() { return _object; }
    constexpr transform_type transform() { return _transform; }

    object_type _object;
    transform_type = _transform;
};

template< typename ObjT, typename U >
struct IsEmpty< Projection< ObjT, U >>:
    integral_constant< bool, IsEmpty< ObjT >::value > { };

/// @brief projecting an object into an extrude space
/// @tparam ObjectT type of the object
/// @tparam U unit of additional dimension
/// @param object instance
/// @param here position of projection
/// @return an object + unit pair in the extrude space
template< space SpaceU, typename ObjT >
constexpr Projection< ObjT, SpaceU > project( ObjT const& object )
{ return { object }; }


} // namespace geometry  

#endif