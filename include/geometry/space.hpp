#ifndef __GEOMETRY_SPACE_HPP__
#define __GEOMETRY_SPACE_HPP__

#include "tensors.hpp"

#include <string>
#include <vector>
#include <iostream>

namespace geometry {

using namespace tensors;

template< vector T >
struct Space 
{
    using vector_type = T;
    static constexpr size_t dimensions();
};

template< typename S >
struct IsSpace: integral_constant< bool, false > { };

template< vector X >
struct IsSpace< Space< X >>: integral_constant< bool, true > { };

template< typename S >
concept space = IsSpace< S >::value;

using null_space = Space< Tensor< Shape< >>>;

template< typename T >
struct DimensionsOf;

template< size_t D, typename... Ts >
struct DimensionsOf< Space< Tensor< Shape< D >, Ts... >>>:
    integral_constant< size_t, D > { };

template< >
struct DimensionsOf< Space< Tensor< Shape <> >>>:
    integral_constant< size_t, 0 > { };

template< typename T >
constexpr size_t dimensions_of_v = DimensionsOf< T >::value;

template< vector T >
constexpr size_t Space< T >::dimensions()
{ return dimensions_of_v< Space< T >>; }

template< typename T >
struct SpaceOf
{ using type = T::space_type; };

template< typename T >
using space_of = SpaceOf< T >::type;

template< typename T >
using vector_for = space_of< T >::vector_type;

/// @brief represents empty space
/// @tparam SpaceT 
template< typename SpaceT >
struct Empty
{ 
    static string object_name() { return "empty"; }
    using space_type = SpaceT; 
};

template< typename T >
struct IsEmpty: integral_constant< bool, false > { };

template< typename SpaceT >
struct IsEmpty< Empty< SpaceT >>: integral_constant< bool, true > { };

template< typename T >
constexpr bool is_empty = IsEmpty< T >::value;

/// @brief object
struct Object
{ using space_type = null_space; };

// TODO: i don't know if an Object by itself is empty or not...
// template< >
// struct IsEmpty< Object >: integral_constant< bool, false > { };
// template< >
// struct IsEmpty< Object >: integral_constant< bool, true > { };

template< typename T >
struct IsObject: integral_constant< bool, is_convertible_v< T, Object >> { };

template< typename T >
concept object = IsObject< T >::value;

////////////////////////////
/// space helper classes ///
////////////////////////////

template< typename T, typename U >
struct ExtrudeSpace;

template< typename U >
struct ExtrudeSpace< Space< Tensor< Shape<> >>, U >
{
    using unit_type = U;
    using vector_type = Tensor< Shape< 1 >, unit_type >;
    using matrix_type = Tensor< Shape< 1, 1 >, unit_type >;
    using space_type = Space< vector_type >;

    static constexpr vector_type base( Tensor< Shape<> > )
    { return { static_cast< unit_type >( 0 ) }; }

    static constexpr vector_type extruded( Tensor< Shape<> >, unit_type u )
    { return { u }; }
};

template< size_t D, typename... Ts, typename U >
struct ExtrudeSpace< Space< Tensor< Shape< D >, Ts... >>, U >
{ 
    using vector_type = Tensor< Shape< D + 1 >, Ts..., U >;
    using space_type = Space< vector_type >; 

    template< size_t... Is >
    static constexpr vector_type vector_helper( 
        Tensor< Shape< D >, Ts... > const& v, U val, seq< Is... > )
    { return { tensor_get< Is >( v )..., val }; }

    static constexpr vector_type base( 
        Tensor< Shape< D >, Ts... > const& v )
    { return vector_helper( v, static_cast< U >( 0 ), make_seq< D >{} ); }

    static constexpr vector_type extruded(
        Tensor< Shape< D >, Ts... > const& v, U u )
    { return vector_helper( v, u, make_seq< D >{} ); }
};

template< typename SpaceT, typename U >
using extrude_space_t = ExtrudeSpace< SpaceT, U >::space_type;

template< typename T, size_t I = ( dimensions_of_v< T > - 1 )>
struct IntrudedSpace;

template< size_t D, typename... Ts, size_t I >
struct IntrudedSpace< Space< Tensor< Shape< D >, Ts... >>, I >
{
    template< typename Seq >
    struct VectorHelper;

    template< size_t... Is >
    struct VectorHelper< seq< Is... >>
    { using type = Tensor< Shape< D - 1 >, 
        tensor_element_t<( isless( Is, I ) ? Is : Is + 1 ), 
            Tensor< Shape< D >, Ts... >>... >; };

    using vector_type = VectorHelper< make_seq< D - 1 >>::type;
    using type = Space< vector_type >;

    template< size_t... Is >
    static constexpr vector_type vector_helper(
        Tensor< Shape< D >, Ts... > const& v, seq< Is... > )
    { return { tensor_get< isless( Is, I ) ? Is : Is + 1 >( v )... }; }

    static constexpr vector_type intruded( Tensor< Shape< D >, Ts... > const& v )
    { return vector_helper( v, make_seq< D - 1 >{} ); }
};

/// divisions for both integral and floating point types
///

template< typename U, size_t Sections, size_t... Is >
constexpr U divisions_helper( U amount, std::array< U, Sections >& ret, 
    seq< Is... > )
{ return amount - (( ret[ Is ] = amount / Sections ) + ... ); }

template< typename U, size_t Sections >
struct IgnoreRemainder
{ constexpr void operator()( U remainder, std::array< U, Sections >& divisions )
    { return; } };

template< typename U, size_t Sections >
struct PadLast
{ constexpr void operator()( U remainder, std::array< U, Sections >& divisions )
    { divisions.last() += remainder; } };

template< typename U, size_t Sections >
struct PadFirst
{ constexpr void operator()( U remainder, std::array< U, Sections >& divisions )
    { divisions.first() += remainder; } };

/// @brief divide a measurement into an array such that the sum of the array 
/// equals the amount
template< size_t Sections, typename U, 
    typename RemainderStrategy = IgnoreRemainder< U, Sections >>
requires( isgreater( Sections, 0 ))
constexpr std::array< U, Sections > divisions( U amount, 
    RemainderStrategy handle_remainder = RemainderStrategy{} )
{ 
    std::array< U, Sections > ret;
    U rem = divisions_helper( amount, ret, make_seq< Sections >{} );
    handle_remainder( rem, ret );
    return ret;
}

/// @brief the primary geometric object
struct Point: Object
{ 
    using space_type = null_space;

    static string object_name() { return "point"; }
};

template< >
struct IsEmpty< Point >
{ static constexpr bool value = false; };


} // namespace geometry 

#endif