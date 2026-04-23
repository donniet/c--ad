#ifndef __GEOMETRY_SPACE_HPP__
#define __GEOMETRY_SPACE_HPP__

#include "tensors.hpp"

#include <string>
#include <vector>
#include <iostream>

namespace geometry {

using namespace tensors;

using scalar_type = long double;

/// @brief object
struct Object
{ static string object_name() { return "object"; }};

template< typename T >
struct IsObject: integral_constant< bool, is_convertible_v< T, Object >> { };

template< typename T >
concept object = IsObject< T >::value;

/// @brief an object with no substance
/// @tparam VectorT vector space
template< size_t Dimensions = 0 >
struct Empty: Object
{ 
    static constexpr size_t dimensions = Dimensions;
    static string object_name() { return "empty"; }
};

template< typename T >
struct IsEmpty: integral_constant< bool, false > { };

template< size_t Dimensions >
struct IsEmpty< Empty< Dimensions >>: 
    integral_constant< bool, true > { };

template< typename T >
constexpr bool is_empty = IsEmpty< T >::value;

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
    static constexpr size_t dimensions() { return 0; }
    static constexpr size_t parameters() { return 0; }
    static string object_name() { return "point"; } 
};

template< >
struct IsEmpty< Point >: integral_constant< bool, false > { };


} // namespace geometry 

#endif