#ifndef __UNITS_HPP__
#define __UNITS_HPP__

#include <cmath>
#include <numeric>
#include <array>

// formatting
#include <algorithm>
#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace units {

using std::gcd;
using std::pair, std::tuple;
using std::string;

// conversion factors for units
constexpr long double meters_per_inch = 0.0254;
constexpr long double meters_per_foot = meters_per_inch * 12.;
constexpr long double meters_per_mile = meters_per_foot *- 5280.;
constexpr long double meters_per_second = 299'792'458; // c

// NOTE: we allow for a max of 16 units
constexpr size_t total_units = 16;
using ull_t = unsigned long long;

// powers of higher units will be severely limited due to Godel numbering
constexpr std::array< ull_t, total_units > primes = 
/*0  1  2  3  4   5   6   7   8   9   10  11  12  13  14  15 */
{ 2, 3, 5, 7, 11, 13, 17, 19, 23, 31, 37, 41, 47, 53, 61, 67 };

template< size_t I >
struct prime_unit;

template< size_t I >
using prime_unit_t = prime_unit< I >::type;

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
{ return left * unit_id_type{ right.second, right.first }; }

static_assert( unit_id_type{ 2, 1 } * unit_id_type{ 3, 2 } == unit_id_type{ 3, 1 } );
static_assert( unit_id_type{ 2, 1 } * unit_id_type{ 3, 1 } == unit_id_type{ 6, 1 } );
static_assert( ( unit_id_type{ 2, 1 } / unit_id_type{ 3, 1 } ) == unit_id_type{ 2, 3 } );
static_assert( ( unit_id_type{ 2, 1 } / unit_id_type{ 3, 2 } ) == unit_id_type{ 4, 3 } );

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
{ 
    using units_tuple = tuple< Us... >; 
    using scalar_type = std::common_type_t< typename unit_traits< Us >::scalar_type... >;

    constexpr unit_product& operator=( unit_product const& other )
    { value = other.value; return *this; }
    constexpr unit_product& operator=( scalar_type const& other )
    { value = other; return *this; }
    constexpr unit_product() : value{0.} { }
    constexpr unit_product( unit_product const& other ) : value{ other.value } { }
    constexpr unit_product( scalar_type value ) : value{ value } { }

    scalar_type value;
};

template< unit... Us >
struct unit_traits< unit_product< Us... >>
{ 
    static constexpr unit_id_type id = ( unit_traits< Us >::id * ... ); 
    using scalar_type = std::common_type_t< typename unit_traits< Us >::scalar_type... >;
    static constexpr bool is_discrete = std::is_integral_v< scalar_type >;
    static constexpr bool is_continuous = not is_discrete;
};

template< unit... Us >
struct unit_quotient
{ 
    using units_tuple = tuple< Us... >; 
    using scalar_type = std::common_type_t< typename unit_traits< Us >::scalar_type... >;

    constexpr unit_quotient& operator=( unit_quotient const& other )
    { value = other.value; return *this; }
    constexpr unit_quotient& operator=( scalar_type const& other )
    { value = other; return *this; }
    constexpr unit_quotient() : value{0.} { }
    constexpr unit_quotient( unit_quotient const& other ) : value{ other.value } { }
    constexpr unit_quotient( scalar_type value ) : value{ value } { }

    scalar_type value;
};

template< unit... Us >
struct unit_traits< unit_quotient< Us... >>
{ 
    static constexpr unit_id_type id = ( unit_traits< Us >::id / ... ); 
    using scalar_type = std::common_type_t< typename unit_traits< Us >::scalar_type... >;
    static constexpr bool is_discrete = std::is_integral_v< scalar_type >;
    static constexpr bool is_continuous = not is_discrete;
};

template< unit LeftU, unit RightU >
constexpr unit_product< LeftU, RightU > operator* ( LeftU left, RightU right )
{  
    
}

/**
 * represents a dimensionless floating point value
 */
struct Scalar 
{
    using scalar_type = long double;

    constexpr Scalar& operator=( Scalar const& other )
    { value = other.value; return *this; }
    constexpr Scalar& operator=( scalar_type other )
    { value = other; return *this; }
    constexpr Scalar() : value{0.} { }
    constexpr Scalar( Scalar const& other ) : value{ other.value } { }
    constexpr Scalar( scalar_type value ) : value{ value } { }

    scalar_type value;
};

template< >
struct unit_traits< Scalar >
{ 
    static constexpr unit_id_type id = { 1, 1 }; 
    using scalar_type = long double;
    static constexpr bool is_discrete = false;
    static constexpr bool is_continuous = true;
};

/**
 * represents an non-negative integer value
 */
struct Cardinal
{
    using scalar_type = unsigned long long;

    constexpr Cardinal& operator=( Cardinal const& other )
    { value = other.value; return *this; }
    constexpr Cardinal& operator=( scalar_type other )
    { value = other; return *this; }
    constexpr Cardinal() : value{0} { }
    constexpr Cardinal( Cardinal const& other ) : value{ other.value } { }
    constexpr Cardinal( scalar_type value ) : value{ value } { }

    scalar_type value;
};

template< >
struct unit_traits< Cardinal >
{ 
    static constexpr unit_id_type id = { 1, 1 }; 
    using scalar_type = unsigned long long;
    static constexpr bool is_discrete = true;
    static constexpr bool is_continuous = false;
};

/**
 * represents the result of a comparison or logical operation
 */
struct Boolean
{
    using scalar_type = bool;

    constexpr Boolean& operator=( Boolean const& other )
    { value = other.value; return *this; }
    constexpr Boolean& operator=( scalar_type other )
    { value = other; return *this; }
    constexpr Boolean() : value{ false } { }
    constexpr Boolean( Boolean const& other ) : value{ other.value } { }
    constexpr Boolean( scalar_type value ) : value{ value } { }

    scalar_type value;
};

template< >
struct unit_traits< Boolean >
{ 
    static constexpr unit_id_type id = { 1, 1 }; 
    using scalar_type = bool;
    static constexpr bool is_discrete = true;
    static constexpr bool is_continuous = false;
};

// true and false expressions
constexpr Boolean true_bool = true;
constexpr Boolean false_bool = false;

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
{ return { (unsigned long long)x.value }; }
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
    using scalar_type = long double;

    constexpr Length& operator=( Length const& other )
    { meters = other.meters; return *this; }
    constexpr Length& operator=( scalar_type other )
    { meters = other; return *this; }
    constexpr Length() : meters{0.} { }
    constexpr Length( Length const& other ) : meters{ other.meters } { }
    constexpr Length( long double meters ) : meters{ meters } { }

    scalar_type meters;
};

template< >
struct unit_traits< Length >
{ 
    static constexpr unit_id_type id = { 2, 1 }; 
    using scalar_type = Length::scalar_type;
    static constexpr bool is_discrete = false;
    static constexpr bool is_continuous = true;
};

static_assert( unit_traits< unit_product< Length, Length >>::id == unit_id_type{ 4, 1 } );
static_assert( unit_traits< unit_quotient< Length, Length >>::id == unit_traits< Scalar >::id );

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

enum class length_unit : size_t
{ meters = 0, millimeters, kilometers, inches, feet, miles };

static constexpr string length_unit_names[] = 
{ "m", "mm", "km", "in", "ft", "mi" };

static constexpr string length_unit_long_names[] = 
{ "meters", "millimeters", "kilometers", "inches", "feet", "miles" };

constexpr long double length_value( Length length, length_unit u )
{
    switch( u ) {
    case length_unit::meters: return length.meters;
    case length_unit::millimeters: return 1000. * length.meters;
    case length_unit::kilometers: return 0.001 * length.meters;
    case length_unit::inches: return length.meters / meters_per_inch;
    case length_unit::feet: return length.meters / meters_per_foot;
    case length_unit::miles: return length.meters / meters_per_mile;
    }
}

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

// formating of units
namespace std {
    
template<>
struct formatter< units::Boolean, char >
{
    bool as_integer = false;

    template< class ParseContext >
    constexpr ParseContext::iterator parse( ParseContext& ctx )
    {
        auto it = ctx.begin();
        if (it == ctx.end())
            return it;
 
        if (*it == 'i')
        {
            as_integer = true;
            ++it;
        }
        if (it != ctx.end() && *it != '}')
            throw format_error("Invalid format args for Boolean.");
 
        return it;
    }
 
    template< class FmtContext >
    FmtContext::iterator format( units::Boolean s, FmtContext& ctx ) const
    {
        ostringstream out;
        if( as_integer )
            out << s.value;
        else
            out << boolalpha << s.value;
 
        return ranges::copy( std::move(out).str(), ctx.out() ).out;
    }
};

// scalars and cardinals use the appropriate std::formatter
template<>
struct formatter< units::Scalar, char > : formatter< long double, char >
{
    template< class ParseContext >
    constexpr ParseContext::iterator parse( ParseContext& ctx )
    { return formatter< long double, char >::parse( ctx ); }
    
    template< class FmtContext >
    FmtContext::iterator format( units::Scalar s, FmtContext& ctx ) const
    { return formatter< long double, char >::format( s.value, ctx ); }
};

template<>
struct formatter< units::Cardinal, char > : formatter< unsigned long long, char >
{
    template< class ParseContext >
    constexpr ParseContext::iterator parse( ParseContext& ctx )
    { return formatter< unsigned long long, char >::parse( ctx ); }
    
    template< class FmtContext >
    FmtContext::iterator format( units::Cardinal s, FmtContext& ctx ) const
    { return formatter< unsigned long long, char >::format( s.value, ctx ); }
};

template<>
struct formatter< units::Length, char > : formatter< long double, char >
{
    units::length_unit format_unit = units::length_unit::meters;
    bool use_long_name = false;

    constexpr void set_unit_name( string name )
    {
        if( name == "")
            return;

        auto i = find( begin( units::length_unit_names ), 
            end( units::length_unit_names ), name );
        if( i != end(units::length_unit_names) )
        {
            format_unit = (units::length_unit)distance( 
                begin( units::length_unit_names), i );
            use_long_name = false;
        }
        else 
        {
            auto j = find( begin( units::length_unit_long_names ), 
                end( units::length_unit_long_names ), name );
            
            // this is a consteval function so we can't throw exceptions
            // if ( j == end( units::length_unit_long_names ))
            //     throw format_error( "invalid format args for Length " );

            format_unit = (units::length_unit)distance( 
                begin( units::length_unit_long_names), j );
            use_long_name = true;
        }
    }

    template< class ParseContext >
    constexpr ParseContext::iterator parse( ParseContext& ctx )
    { 
        auto it = ctx.begin();
        string u = "";
        while( it != ctx.end() and *it != '}' and *it != ':' )
            u += *it++;
        
        set_unit_name( u );
        if( it != ctx.end() and *it == ':' )
            it++;

        ctx.advance_to( it );
        return formatter< long double, char >::parse( ctx ); 
    }

    // TODO: write non-consteval version which throws an exception
    
    template< class FmtContext >
    FmtContext::iterator format( units::Length const& s, FmtContext& ctx ) const
    { 
        formatter< long double, char >::format( 
            units::length_value( s.meters, format_unit ), ctx );

        if( use_long_name )
            return ranges::copy( units::length_unit_long_names[ (size_t)format_unit ], 
                ctx.out() ).out;

        return ranges::copy( units::length_unit_names[ (size_t)format_unit ], 
            ctx.out() ).out;
    }
};

} // namespace std


#endif