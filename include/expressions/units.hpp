#ifndef __UNITS_HPP__
#define __UNITS_HPP__

#include "expressions/expression_base.hpp"

#include <cmath>
#include <numeric>
#include <array>

namespace expressions {
namespace units {

using std::gcd;
using std::pair;

// conversion factors for units
constexpr long double meters_per_inch = 0.0254;
constexpr long double meters_per_foot = meters_per_inch / 12.;
constexpr long double meters_per_mile = meters_per_foot / 5280.;
constexpr long double meters_per_second = 299'792'458; // c

// NOTE: we allow for a max of 16 units
constexpr size_t total_units = 16;
using ull_t = unsigned long long;

// powers of higher units will be severely limited due to Godel numbering
constexpr std::array< ull_t, total_units > primes = 
/*0  1  2  3  4   5   6   7   8   9   10  11  12  13  14  15 */
{ 2, 3, 5, 7, 11, 13, 17, 19, 23, 31, 37, 41, 47, 53, 61, 67 };

// TODO: undefined and automatic
// struct undefined_t { };
// constexpr const undefined_t undefined = {};
// struct automatic_t { };
// constexpr const automatic_t automatic = {};

/**
 * A unit_id is a fraction of two unsigned integers.  
 * 
 * TODO: handle null units (automatics)
 */
using unit_id_type = pair< unsigned long long, unsigned long long >;

consteval unit_id_type operator* ( unit_id_type left, unit_id_type right )
{ return { left.first / gcd( left.first, right.second ) * right.first / gcd( right.first, left.second ),
    left.second / gcd( left.second, right.first ) * right.second / gcd( right.second, left.first ) }; }

consteval unit_id_type operator/ ( unit_id_type left, unit_id_type right )
{ return { left.first / gcd( left.first, right.first ) * right.second / gcd( right.second, left.first ),
    left.second / gcd( left.second, right.second ) * right.first / gcd( right.first, left.second ) }; }

template< ull_t Num, ull_t Den >
requires (/* ( Num == 0 and Den == 0 ) or */ gcd( Num, Den ) == 1 )
struct UnitId
{
    static constexpr ull_t numerator = Num;
    static constexpr ull_t denominator = Den;
};

template< typename... IDs >
struct UnitProduct;

template< typename... IDs >
using unit_product_t = UnitProduct< IDs... >::type;

template< typename Id >
struct UnitProduct< Id >
{ using type = Id; };

template< typename LeftId, typename RightId >
struct UnitProduct< LeftId, RightId >
{
    using type = UnitId< 
        LeftId::numerator / gcd( LeftId::numerator, RightId::denominator ) *
            RightId::numerator / gcd( RightId::numerator, LeftId::denominator ),
        LeftId::denominator / gcd( LeftId::denominator, RightId::numerator ) *
            RightId::denominator / gcd( RightId::denominator, LeftId::numerator ) >;
};

template< typename First, typename... Rests >
struct UnitProduct< First, Rests... >
{ using type = UnitProduct< First, typename UnitProduct< Rests... >::type >::type; };

static_assert( is_same_v< unit_product_t< UnitId< 2, 1 >, UnitId< 3, 1 >>, UnitId< 6, 1 >> );
static_assert( is_same_v< unit_product_t< UnitId< 2, 1 >, UnitId< 3, 2 >>, UnitId< 3, 1 >> );
static_assert( is_same_v< unit_product_t< UnitId< 4, 1 >, UnitId< 3, 2 >>, UnitId< 6, 1 >> );

template< typename... IDs >
struct UnitQuotient;

template< typename... IDs >
using unit_quotient_t = UnitQuotient< IDs... >::type;

template< typename Id >
struct UnitQuotient< Id >
{ using type = Id; };

template< typename LeftId, typename RightId >
struct UnitQuotient< LeftId, RightId >
{
    using type = UnitId< 
        LeftId::numerator / gcd( LeftId::numerator, RightId::numerator ) *
            RightId::denominator / gcd( RightId::denominator, LeftId::denominator ),
        LeftId::denominator / gcd( LeftId::denominator, RightId::denominator ) *
            RightId::numerator / gcd( RightId::numerator, LeftId::numerator ) >;
};

template< typename First, typename... Rests >
struct UnitQuotient< First, Rests... >
{ using type = UnitQuotient< First, typename UnitQuotient< Rests... >::type >::type; };

static_assert( is_same_v< unit_quotient_t< UnitId< 2, 1 >, UnitId< 5, 1 >>, UnitId< 2, 5 >> );
static_assert( is_same_v< unit_quotient_t< UnitId< 2, 1 >, UnitId< 3, 2 >>, UnitId< 4, 3 >> );
static_assert( is_same_v< unit_quotient_t< UnitId< 4, 1 >, UnitId< 3, 2 >>, UnitId< 8, 3 >> );

/**
 * unit traits are defined on types that are considered units.  Must declare
 * a static ull member `id` which is a prime number.  Godel numbering will be
 * used to create unique ids for powers of units.  quotients of units will be 
 * 
 * This will ensure that compound units will have unique ids.  
 * 
 * the IDs of scalars and cardinal numbers will be 1 so that raised to any power
 * they stay 1. 
 * 
 * TODO: IDs of automatic expressions will be 0
 */
template< typename T >
struct unit_traits;

template< typename T >
concept unit = requires
{ typename unit_traits< T >; };

template< unit... Us >
struct unit_product
{ using units_tuple = tuple< Us... >; };

template< unit... Us >
struct unit_traits< unit_product< Us... >>
{ static constexpr unit_id_type id = ( ... * unit_traits< Us >::id ); };

template< unit... Us >
struct unit_quotient
{ using units_tuple = tuple< Us... >; };

template< unit... Us >
struct unit_traits< unit_quotient< Us... >>
{ static constexpr unit_id_type id = ( ... / unit_traits< Us >::id ); };

/**
 * represents the result of a comparison or logical operation
 */
using Boolean = StaticExpression< bool >;

template< >
struct unit_traits< Boolean >
{ 
    static constexpr unit_id_type id = { 1, 1 }; 
    using scalar_type = Boolean;
};

/**
 * represents a dimensionless floating point value
 */
using Scalar = StaticExpression< long double >;

template< >
struct unit_traits< Scalar >
{ 
    static constexpr unit_id_type id = { 1, 1 }; 
    using scalar_type = Scalar;
};

/**
 * represents an non-negative integer value
 */
using Cardinal = StaticExpression< unsigned long long >;

template< >
struct unit_traits< Cardinal >
{ 
    static constexpr unit_id_type id = { 1, 1 }; 
    using scalar_type = Cardinal;
};

// true and false expressions
constexpr Boolean true_bool = Boolean{ true };
constexpr Boolean false_bool = Boolean{ false };

// literals and construction methods for scalars
Scalar operator ""_scalar( long double x )
{ return { x }; }
Scalar operator ""_scalar( unsigned long long x )
{ return { (long double)x }; }
Scalar scalar( long double x )
{ return { x }; }
Scalar operator ""_percent( long double x )
{ return { 0.01 * x }; }
Scalar operator ""_percent( unsigned long long x )
{ return { 0.01 * (long double)x }; }
Scalar percent( Cardinal n )
{ return { 0.01 * (long double)n.value }; }
Scalar percent( unsigned long long n )
{ return { 0.01 * (long double)n }; }
Scalar percent( long double x )
{ return { 0.01 * x }; }

// literals and construction methods for cardinals
Cardinal operator ""_cardinal( unsigned long long n )
{ return { n }; }
Cardinal cardinal( Scalar x )
{ return { (unsigned long long)x }; }
Cardinal cardinal( long double x )
{ return { (unsigned long long)x }; }
Cardinal cardinal( unsigned long long n )
{ return { n }; }

// logical operations
Boolean operator and( Boolean const& left, Boolean const& right )
{ return { left.value and right.value }; }
Boolean operator or( Boolean const& left, Boolean const& right )
{ return { left.value or right.value }; }
Boolean operator not( Boolean const& arg )
{ return { not arg.value }; }

// scalar arithmetic
Scalar operator +( Scalar const& left, Scalar const& right )
{ return { left.value + right.value }; }
Scalar operator -( Scalar const& left, Scalar const& right )
{ return { left.value - right.value }; }
Scalar operator -( Scalar const& arg )
{ return { -arg.value }; }
Scalar operator *( Scalar const& left, Scalar const& right )
{ return { left.value * right.value }; }
Scalar operator /( Scalar const& left, Scalar const& right )
{ return { left.value / right.value }; }

// TODO: add exceptions for negative numbers and infinity
Cardinal operator +( Cardinal const& left, Cardinal const& right )
{ return { left.value + right.value }; }
Cardinal operator -( Cardinal const& left, Cardinal const& right )
{ return { left.value - right.value }; }
Cardinal operator *( Cardinal const& left, Cardinal const& right )
{ return { left.value * right.value }; }
Cardinal operator /( Cardinal const& left, Cardinal const& right )
{ return { left.value / right.value }; }
Cardinal operator %( Cardinal const& left, Cardinal const& right )
{ return { left.value % right.value }; }
Cardinal operator ~( Cardinal const& arg )
{ return { ~arg.value }; }
Cardinal operator |( Cardinal const& left, Cardinal const& right )
{ return { left.value | right.value }; }
Cardinal operator &( Cardinal const& left, Cardinal const& right )
{ return { left.value & right.value }; }
Cardinal operator ^( Cardinal const& left, Cardinal const& right )
{ return { left.value ^ right.value }; }
Cardinal operator >>( Cardinal const& left, Cardinal const& right )
{ return { left.value >> right.value }; }
Cardinal operator <<( Cardinal const& left, Cardinal const& right )
{ return { left.value << right.value }; }
Cardinal operator >>( Cardinal const& left, unsigned long long bits )
{ return { left.value >> bits }; }
Cardinal operator <<( Cardinal const& left, unsigned long long bits )
{ return { left.value << bits }; }

// scalar comparison
Boolean operator ==( Scalar const& left, Scalar const& right )
{ return { left.value == right.value }; }
Boolean operator !=( Scalar const& left, Scalar const& right )
{ return { left.value != right.value }; }
Boolean operator <( Scalar const& left, Scalar const& right )
{ return { left.value < right.value }; }
Boolean operator <=( Scalar const& left, Scalar const& right )
{ return { left.value <= right.value }; }
Boolean operator >( Scalar const& left, Scalar const& right )
{ return { left.value > right.value }; }
Boolean operator >=( Scalar const& left, Scalar const& right )
{ return { left.value >= right.value }; }

// cardinal comparison
Boolean operator ==( Cardinal const& left, Cardinal const& right )
{ return { left.value == right.value }; }
Boolean operator !=( Cardinal const& left, Cardinal const& right )
{ return { left.value != right.value }; }
Boolean operator <( Cardinal const& left, Cardinal const& right )
{ return { left.value < right.value }; }
Boolean operator <=( Cardinal const& left, Cardinal const& right )
{ return { left.value <= right.value }; }
Boolean operator >( Cardinal const& left, Cardinal const& right )
{ return { left.value > right.value }; }
Boolean operator >=( Cardinal const& left, Cardinal const& right )
{ return { left.value >= right.value }; }

// cardinal to scalar comparison
Boolean operator ==( Cardinal const& left, Scalar const& right )
{ return { (long double)left.value == right.value }; }
Boolean operator !=( Cardinal const& left, Scalar const& right )
{ return { (long double)left.value != right.value }; }
Boolean operator <( Cardinal const& left, Scalar const& right )
{ return { (long double)left.value < right.value }; }
Boolean operator <=( Cardinal const& left, Scalar const& right )
{ return { (long double)left.value <= right.value }; }
Boolean operator >( Cardinal const& left, Scalar const& right )
{ return { (long double)left.value > right.value }; }
Boolean operator >=( Cardinal const& left, Scalar const& right )
{ return { (long double)left.value >= right.value }; }
Boolean operator ==( Scalar const& left, Cardinal const& right )
{ return { left.value == (long double)right.value }; }
Boolean operator !=( Scalar const& left, Cardinal const& right )
{ return { left.value != (long double)right.value }; }
Boolean operator <( Scalar const& left, Cardinal const& right )
{ return { left.value < (long double)right.value }; }
Boolean operator <=( Scalar const& left, Cardinal const& right )
{ return { left.value <= (long double)right.value }; }
Boolean operator >( Scalar const& left, Cardinal const& right )
{ return { left.value > (long double)right.value }; }
Boolean operator >=( Scalar const& left, Cardinal const& right )
{ return { left.value >= (long double)right.value }; }

// scalar and cardinal arithmetic
Scalar operator +( Scalar const& left, Cardinal const& right )
{ return { left.value + (long double)right.value }; }
Scalar operator +( Cardinal const& left, Scalar const& right )
{ return { (long double)left.value + right.value }; }
Scalar operator -( Scalar const& left, Cardinal const& right )
{ return { left.value - (long double)right.value }; }
Scalar operator -( Cardinal const& left, Scalar const& right )
{ return { (long double)left.value - right.value }; }
Scalar operator *( Scalar const& left, Cardinal const& right )
{ return { left.value * (long double)right.value }; }
Scalar operator *( Cardinal const& left, Scalar const& right )
{ return { (long double)left.value * right.value }; }
Scalar operator /( Scalar const& left, Cardinal const& right )
{ return { left.value / (long double)right.value }; }
Scalar operator /( Cardinal const& left, Scalar const& right )
{ return { (long double)left.value / right.value }; }
// TODO: trig, pow and log functions of Scalars and Cardinals


/**
 * represents a length in meters
 */
struct Length
{ 
    Length& operator=( Length const& other )
    { meters = other.meters; return *this; }
    constexpr Length() : meters{0.} { }
    constexpr Length( Length const& other ) : meters{ other.meters } { }
    constexpr Length( long double meters ) : meters{ meters } { }

    long double meters;
};

template< >
struct unit_traits< Length >
{ static constexpr unsigned long long id = 2; };

// length measurement units
Length operator""_in(long double inches)
{ return { inches * meters_per_inch }; }
Length operator""_in(unsigned long long inches)
{ return { (long double)inches * meters_per_inch }; }
Length operator""_ft(long double feet)
{ return { feet * meters_per_foot }; }
Length operator""_ft(unsigned long long feet)
{ return { (long double)feet * meters_per_foot }; }
Length operator""_mi(long double miles)
{ return { miles * meters_per_mile }; }
Length operator""_mi(unsigned long long miles)
{ return { (long double)miles * meters_per_mile }; }
Length operator""_m(long double meters)
{ return { meters }; }
Length operator""_m(unsigned long long meters)
{ return { (long double)meters }; }
Length operator""_mm(long double millimeters)
{ return { 0.001 * millimeters }; }
Length operator""_mm(unsigned long long millimeters)
{ return { 0.001 * (long double)millimeters }; }
Length operator""_km(long double kilometers)
{ return { 1000. * kilometers }; }
Length operator""_km(unsigned long long kilometers)
{ return { 1000. * (long double)kilometers }; } 

// length arithmetic
Length operator +( Length const& left, Length const& right )
{ return { left.meters + right.meters }; }
Length operator -( Length const& left, Length const& right )
{ return { left.meters - right.meters }; }
Scalar operator /( Length const& left, Length const& right )
{ return { left.meters / right.meters }; }
Scalar operator %( Length const& left, Length const& right )
{ return { left.meters - 
    ( right.meters * std::floor( left.meters / right.meters )) }; }

// length scaling
Length operator *( Scalar const& left, Length const& right )
{ return { left.value * right.meters }; }
Length operator *( Length const& left, Scalar const& right )
{ return { left.meters * right.value }; }
Length operator /( Length const& left, Scalar const& right )
{ return { left.meters / right.value }; }

// length comparison
Boolean operator ==( Length const& left, Length const& right )
{ return left.meters == right.meters; }
Boolean operator !=( Length const& left, Length const& right )
{ return left.meters != right.meters; }
Boolean operator <( Length const& left, Length const& right )
{ return left.meters < right.meters; }
Boolean operator <=( Length const& left, Length const& right )
{ return left.meters <= right.meters; }
Boolean operator >( Length const& left, Length const& right )
{ return left.meters > right.meters; }
Boolean operator >=( Length const& left, Length const& right )
{ return left.meters >= right.meters; }

} // namespace units

/**
 * Traits for the units
 */
template<>
struct expression_traits< units::Length >
{
    using result_type = units::Length;
    // by default the expression is a static one
    static constexpr bool is_static_expression = true;
    static constexpr bool is_compound_expression = false;
    static constexpr bool is_variable = false;
    using dependent_types = tuple<>;
};

} // namespace expressions

#endif