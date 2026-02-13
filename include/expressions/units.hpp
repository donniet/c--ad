#ifndef __UNITS_HPP__
#define __UNITS_HPP__

#include "expressions/expression_base.hpp"

#include <cmath>

namespace expressions {
namespace units {

// conversion factors for units
constexpr long double meters_per_inch = 0.0254;
constexpr long double meters_per_foot = meters_per_inch / 12.;
constexpr long double meters_per_mile = meters_per_foot / 5280.;
constexpr long double meters_per_second = 299'792'458; // c

// TODO: undefined and automatic
// struct undefined_t { };
// constexpr const undefined_t undefined = {};
// struct automatic_t { };
// constexpr const automatic_t automatic = {};

/**
 * represents a dimensionless floating point value
 */
using Scalar = StaticExpression< long double >;

/**
 * represents an non-negative integer value
 */
using Cardinal = StaticExpression< unsigned long long >;

/**
 * represents the result of a comparison or logical operation
 */
using Boolean = StaticExpression< bool >;

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
Length operator *( Cardinal const& left, Length const& right )
{ return { (long double)left.value * right.meters }; }
Length operator *( Length const& left, Cardinal const& right )
{ return { left.meters * (long double)right.value }; }
Length operator /( Length const& left, Cardinal const& right )
{ return { left.meters / (long double)right.value }; }

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