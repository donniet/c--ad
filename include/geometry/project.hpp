#ifndef __GEOMETRY_PROJECT_HPP__
#define __GEOMETRY_PROJECT_HPP__

#include "geometry/space.hpp"
#include "expressions/expressions.hpp"

namespace geometry {

using namespace expressions;

/// @brief projects the object directly at coordinates
/// @tparam ...Us 
template< size_t From, typename... Us >
struct ProjectAt
{
    using coordinates_type = tuple< Us... >;
    static constexpr size_t from_dimensions = From;
    static constexpr size_t to_dimensions = From + sizeof...( Us );

    template< vector V >
    constexpr extend_vector_t< V, Us... > operator()( V point ) 
    { return extend_helper( point, make_seq< sizeof...( Us )>{} ); }

    constexpr ProjectAt( Us... coords ): _here{ coords... } { }
    constexpr ProjectAt(): _here{ zero() } { }

    static constexpr tuple< Us... > zero()
    { return { static_cast< Us >( 0 )... }; }

    template< vector V, size_t... Is >
    constexpr auto extend_helper( V point, seq< Is... > )
    { return extend_vector( point, get< Is >( _here )... ); }

    coordinates_type _here;
};

/// @brief apply a linear transformation to the object
/// @tparam MatT 
template< matrix MatT >
struct Linear
{
    using matrix_type = MatT;
    using matrix_shape = tensor_shape_t< matrix_type >;
    static constexpr size_t from_dimensions = shape_element_v< 1, matrix_shape >;
    static constexpr size_t to_dimensions = shape_element_v< 0, matrix_shape >;

    template< vector V >
    requires( shape_element_v< 0, tensor_shape_t< V >> == from_dimensions )
    constexpr auto operator()( V point )
    { return matmul( _matrix, point ); }

    matrix_type const& matrix() const
    { return _matrix; }

    matrix_type _matrix;
};

/// @brief wraps a differentiable expression as a geometry transform
/// NOTE: the expression must contain a single vector-typed variable at index 0
/// TODO: enforce the above with a constraint
/// @tparam ExprT 
template< expression ExprT >
requires tensors::vector< result_t< ExprT >>
struct Differentiable
{
    using expression_type = ExprT;
    using result_type = result_t< expression_type >;
    // using input_type = expression_variable_t< 0, ExprT >;
    // static constexpr size_t from_dimensions = shape_element_v< 0, shape_of_t< input_type >>;
    static constexpr auto to_dimensions = tensors::shape_element_v< 0, 
        typename result_type::shape_type >;

    template< vector V >
    constexpr auto operator ()( V point )
    { 
        auto vars = declare_variables( var< V >( "point" ));
        auto [ point_variable ] = vars.all();

        point_variable = point;
        return eval( _expr );
    }

    expression_type const& expression() const
    { return _expr; }

    expression_type _expr;
};

/// @brief projects an object from its space to another space using a linear
/// transformatoin
/// @tparam ObjectT 
/// @tparam U 
template< typename ObjT, typename TransformT, 
    size_t Dim = TransformT::to_dimensions >
struct Projection: Object
{
    using object_type = ObjT;
    using transform_type = TransformT;

    static constexpr size_t from_dimensions = object_type::dimensions();
    static constexpr size_t to_dimensions = Dim;

    static constexpr size_t dimensions() { return Dim; }
    static constexpr size_t parameters() { return object_type::parameters(); }

    static string object_name() 
    { return "projection_" + object_type::object_name(); }

    constexpr object_type object() { return _object; }
    constexpr transform_type transform() { return _transform; };

    constexpr Projection( object_type object, TransformT transform ):
        _object{ object }, _transform{ transform } { }

    object_type _object;
    transform_type _transform;
};

template< typename ObjT, typename TransformT, size_t Dim >
struct IsEmpty< Projection< ObjT, TransformT, Dim >>: IsEmpty< ObjT > { };

/// @brief projecting an object into an extrude space
/// @tparam ObjectT type of the object
/// @tparam U unit of additional dimension
/// @param object instance
/// @param here position of projection
/// @return an object + unit pair in the extrude space
template< typename ObjT, typename... Us >
requires( not is_empty< ObjT > )
constexpr Projection< ObjT, ProjectAt< ObjT::dimensions(), Us... >>
project_at( ObjT object, Us... us )
{ return { object, { us... }}; }

template< typename ObjT, matrix MatT >
requires( not is_empty< ObjT >)
constexpr Projection< ObjT, Linear< MatT >> 
transform_linear( ObjT object, MatT mat )
{ return { object, { mat }}; };

} // namespace geometry  

#endif