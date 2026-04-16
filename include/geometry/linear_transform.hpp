#ifndef __GEOMETRY_LINEAR_TRANSFORM_HPP__
#define __GEOMETRY_LINEAR_TRANSFORM_HPP__

#include "geometry/space.hpp"

namespace geometry {

template< typename ObjectT >
struct LinearTransformation
{
    using scalar_type = long double;
    using object_type = ObjectT;
    using space_type = space_of< object_type >;
    static constexpr size_t dim = dimensions_of_v< space_type >;
    using matrix_type = uniform_tensor_t< Shape< dim, dim >, scalar_type >;

    static string object_name() 
    { return "lineartransformation_" + object_type::object_name(); }

    object_type object;
    matrix_type transform;
};

template< typename ObjT >
struct IsEmpty< LinearTransformation< ObjT >>
{ static constexpr bool value = IsEmpty< ObjT >::value; };

/// @brief linear transformation
template< typename ObjT >
constexpr LinearTransformation< ObjT > 
transform_linear( ObjT const& obj, 
    typename LinearTransformation< ObjT >::matrix_type const& transform )
{ return { obj, transform }; }

template< typename MatT, size_t Axis0, size_t Axis1 >
constexpr MatT rotate_plane_matrix( long double angle )
{
    auto at = typename MatT::shape_type{};
    static constexpr size_t I = Axis0;
    static constexpr size_t J = Axis1;

    auto rotation_matrix = identity_matrix< MatT >();
    
    tensor_get< at( I, I )>( rotation_matrix ) = cos( angle );
    tensor_get< at( I, J )>( rotation_matrix ) = -sin( angle );
    tensor_get< at( J, I )>( rotation_matrix ) = sin( angle );
    tensor_get< at( J, J )>( rotation_matrix ) = cos( angle );

    return rotation_matrix;
}

template< size_t Axis0, size_t Axis1, typename ObjT >
requires( Axis0 != Axis1 and isless( Axis0, dimensions_of_v< space_of< ObjT >> ) 
    and isless( Axis1, dimensions_of_v< space_of< ObjT >> ))
constexpr LinearTransformation< ObjT >
rotate_plane( ObjT const& obj, long double angle )
{ 
    using matrix_type = LinearTransformation< ObjT >::matrix_type;

    return transform_linear( obj, 
        rotate_plane_matrix< matrix_type, Axis0, Axis1 >( angle ));
}


} // namespace geometry 

#endif