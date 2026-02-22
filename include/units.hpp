#ifndef __UNITS_HPP__
#define __UNITS_HPP__

#include "utility.hpp"

#include <cmath>
#include <numeric>
#include <array>
#include <type_traits>

// formatting
#include <algorithm>
#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace units {

using std::gcd, std::isgreater, std::isless;
using std::pair, std::tuple;
using std::string;
using std::conditional_t;
using std::is_arithmetic_v;
using std::size_t;

template< typename T >
concept arithmetic = is_arithmetic_v< T >;

// conversion factors for units
constexpr long double meters_per_inch = 0.0254;
constexpr long double meters_per_thou = meters_per_inch * 0.001;
constexpr long double meters_per_foot = meters_per_inch * 12.;
constexpr long double meters_per_mile = meters_per_foot * 5280.;
constexpr long double meters_per_second = 299'792'458; // c
constexpr long double seconds_per_minute = 60.;
constexpr long double seconds_per_hour = seconds_per_minute * 60.;
constexpr long double seconds_per_day = seconds_per_hour * 24.;
constexpr long double seconds_per_week = seconds_per_day * 7.;
constexpr long double seconds_per_year = seconds_per_day * 365.242199;
constexpr long double kilograms_per_ounce = 0.028'349'523'125;
constexpr long double kilograms_per_pound = kilograms_per_ounce * 16.;
static_assert( kilograms_per_pound == 0.453'592'37 );
constexpr long double kilograms_per_long_ton = kilograms_per_pound * 2240.;
constexpr long double kilograms_per_ton = kilograms_per_pound * 2000.;


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

constexpr unit_id_type scalar_unit_id = { 1, 1 };
constexpr unit_id_type length_unit_id = { 2, 1 };
constexpr unit_id_type time_unit_id = { 3, 1 };
constexpr unit_id_type mass_unit_id = { 5, 1 };

consteval unit_id_type operator* ( unit_id_type left, unit_id_type right )
{ return { left.first / gcd( left.first, right.second ) * right.first / gcd( right.first, left.second ),
    left.second / gcd( left.second, right.first ) * right.second / gcd( right.second, left.first ) }; }

consteval unit_id_type operator/ ( unit_id_type left, unit_id_type right )
{ return left * unit_id_type{ right.second, right.first }; }

static_assert( unit_id_type{ 2, 1 } * unit_id_type{ 3, 2 } == unit_id_type{ 3, 1 } );
static_assert( unit_id_type{ 2, 1 } * unit_id_type{ 3, 1 } == unit_id_type{ 6, 1 } );
static_assert( ( unit_id_type{ 2, 1 } / unit_id_type{ 3, 1 } ) == unit_id_type{ 2, 3 } );
static_assert( ( unit_id_type{ 2, 1 } / unit_id_type{ 3, 2 } ) == unit_id_type{ 4, 3 } );

template< unit_id_type Id, arithmetic T >
struct base_unit
{
    static constexpr bool is_unit = true;
    using scalar_type = T;
    static constexpr unit_id_type unit_id = Id;

    constexpr base_unit& operator=( base_unit const& other )
    { value = other.value; return *this; }
    constexpr base_unit& operator=( scalar_type const& other )
    { value = other; return *this; }
    constexpr operator scalar_type() const
    { return value; }
    constexpr base_unit() : value{0.} { }
    constexpr base_unit( base_unit const& other ) : value{ other.value } { }
    constexpr base_unit( scalar_type value ) : value{ value } { }

    constexpr scalar_type get_value() const
    { return value; }

    scalar_type value;
};

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
requires is_arithmetic_v< T >
struct unit_traits< T >
{
    static constexpr unit_id_type unit_id = scalar_unit_id;
    using scalar_type = T;
    static constexpr bool is_discrete = std::is_integral_v< scalar_type >;
    static constexpr bool is_continuous = not is_discrete;
};

template< unit_id_type Id, arithmetic T >
struct unit_traits< base_unit< Id, T >>
{
    static constexpr unit_id_type unit_id = Id; 
    using scalar_type = T;
    static constexpr bool is_discrete = std::is_integral_v< scalar_type >;
    static constexpr bool is_continuous = not is_discrete;
};

template< typename T >
struct is_unit
{ static constexpr bool value = is_arithmetic_v< T >; };

template< unit_id_type Id, arithmetic T >
struct is_unit< base_unit< Id, T >>
{ static constexpr bool value = true; };

template< typename T >
concept unit = is_unit< T >::value;

template< unit... Us >
struct UnitProduct
{ using type = base_unit< ( unit_traits< Us >::unit_id * ... ), 
        std::common_type_t< typename unit_traits< Us >::scalar_type... >>; };

template< unit... Us >
using unit_product = UnitProduct< Us... >::type;

template< unit... Us >
struct UnitQuotient
{ using type = base_unit< ( unit_traits< Us >::unit_id / ... ), 
        std::common_type_t< typename unit_traits< Us >::scalar_type... >>; };

template< unit... Us >
using unit_quotient = UnitQuotient< Us... >::type;

template< unit U >
struct UnitInverse
{ 
    static constexpr unit_id_type unit_id = unit_traits< U >::id;
    using scalar_type = unit_traits< U >::scalar_type;

    using type = base_unit< { unit_id.second, unit_id.first }, scalar_type >;
};

template< unit U >
using unit_inverse = UnitInverse< U >::type;

/**
 * represents a continuous dimensionless value
 */
using Scalar = base_unit< scalar_unit_id, long double >;

/**
 * represents an unsigned discrete dimensionless value
 */
using Cardinal = base_unit< scalar_unit_id, unsigned long long >;

/**
 * represents a true or a false value
 */
using Boolean = base_unit< scalar_unit_id, bool >;

template< unit U, typename Seq >
struct PositivePowerUnitHelper;

template< unit U, size_t... Is >
struct PositivePowerUnitHelper< U, seq< Is... >>
{ using type = unit_product< conditional_t< Is == Is, U, U >... >; };

template< unit U, typename Seq >
struct NegativePowerUnitHelper;

template< unit U, size_t... Is >
struct NegativePowerUnitHelper< U, seq< Is... >>
{ using type = unit_inverse< unit_product< conditional_t< Is == Is, U, U >... >>; };

template< int Power, unit U >
struct PowerUnit;

template< int Power, unit U >
requires( isgreater( Power, 0 ))
struct PowerUnit< Power, U >
{ using type = typename PositivePowerUnitHelper< U, make_seq< (size_t)Power >>::type; };

template< int Power, unit U >
requires( isless( Power, 0 ))
struct PowerUnit< Power, U >
{ using type = typename NegativePowerUnitHelper< U, make_seq< (size_t)-Power >>::type; };

template< unit U >
struct PowerUnit< 0, U >
{ using type = Scalar; };

template< int Power, unit U >
using power_unit_t = PowerUnit< Power, U >::type;

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

// general unit arithmetic
template< unit LeftU, unit RightU >
constexpr unit_product< LeftU, RightU > operator *( LeftU left, RightU right )
{ return { left.get_value() * right.get_value() }; }

template< unit U >
constexpr unit_product< Scalar, U > operator *( long double left, U right )
{ return { left * right.get_value() }; }

template< unit U >
constexpr unit_product< Scalar, U > operator *( long long left, U right )
{ return { left * right.get_value() }; }

template< unit U >
constexpr unit_product< U, Scalar > operator *( U left, long double right )
{ return { left.get_value() * right }; }

template< unit U >
constexpr unit_product< U, Scalar > operator *( U left, long long right )
{ return { left.get_value() * right }; }

template< unit LeftU, unit RightU >
constexpr unit_quotient< LeftU, RightU > operator /( LeftU left, RightU right )
{ return { left.get_value() / right.get_value() }; }

template< unit U >
constexpr unit_quotient< Scalar, U > operator /( long double left, U right )
{ return { left / right.get_value() }; }

template< unit U >
constexpr unit_quotient< Scalar, U > operator /( long long left, U right )
{ return { left / right.get_value() }; }

template< unit U >
constexpr unit_quotient< U, Scalar > operator /( U left, long double right )
{ return { left.get_value() / right }; }

template< unit U >
constexpr unit_quotient< U, Scalar > operator /( U left, long long right )
{ return { left.get_value() / right }; }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::id == unit_traits< RightU >::id )
constexpr LeftU operator +( LeftU const& left, RightU const& right )
{ return { left.get_value() + right.get_value() }; }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::id == unit_traits< RightU >::id )
constexpr LeftU operator -( LeftU const& left, RightU const& right )
{ return { left.get_value() - right.get_value() }; }

template< unit U >
requires( unit_traits< U >::is_continuous )
constexpr U operator-( U const& arg )
{ return { -arg.get_value() }; }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::is_continuous or unit_traits< RightU >::is_continuous )
constexpr unit_quotient< LeftU, RightU > operator %( LeftU left, RightU right )
{ return { left.get_value() - std::floor( left.get_value() / 
    right.get_value()) * right.get_value() }; }

// discrete operators
template< unit U >
requires( unit_traits< U >::is_discrete )
constexpr U operator ~( U const& arg )
{ return { ~arg.get_value() }; }

constexpr Boolean operator ~( Boolean const& arg )
{ return { not arg.get_value() }; }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::is_discrete and unit_traits< RightU >::is_discrete )
constexpr unit_quotient< LeftU, RightU > operator %( LeftU left, RightU right )
{ return { left.get_value() % right.get_value() }; }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::is_discrete and unit_traits< RightU >::is_discrete 
    and unit_traits< LeftU >::id == unit_traits< RightU >::id )
constexpr LeftU operator |( LeftU const& left, RightU const& right )
{ return { left.get_value() | right.get_value() }; }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::is_discrete and unit_traits< RightU >::is_discrete 
    and unit_traits< LeftU >::id == unit_traits< RightU >::id )
constexpr LeftU operator &( LeftU const& left, RightU const& right )
{ return { left.get_value() & right.get_value() }; }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::is_discrete and unit_traits< RightU >::is_discrete 
    and unit_traits< LeftU >::id == unit_traits< RightU >::id )
constexpr LeftU operator ^( LeftU const& left, RightU const& right )
{ return { left.get_value() ^ right.get_value() }; }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::is_discrete and unit_traits< RightU >::is_discrete 
    and unit_traits< LeftU >::id == unit_traits< RightU >::id )
constexpr LeftU operator >>( LeftU const& left, RightU const& right )
{ return { left.get_value() >> right.get_value() }; }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::is_discrete and unit_traits< RightU >::is_discrete 
    and unit_traits< LeftU >::id == unit_traits< RightU >::id )
constexpr LeftU operator <<( LeftU const& left, RightU const& right )
{ return { left.get_value() << right.get_value() }; }

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

// general unit comparison
template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::id == unit_traits< RightU >::id )
Boolean operator ==( LeftU const& left, RightU const& right )
{ return left.get_value() == right.get_value(); }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::id == unit_traits< RightU >::id )
Boolean operator !=( LeftU const& left, RightU const& right )
{ return left.get_value() != right.get_value(); }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::id == unit_traits< RightU >::id )
Boolean operator <( LeftU const& left, RightU const& right )
{ return left.get_value() < right.get_value(); }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::id == unit_traits< RightU >::id )
Boolean operator <=( LeftU const& left, RightU const& right )
{ return left.get_value() <= right.get_value(); }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::id == unit_traits< RightU >::id )
Boolean operator >( LeftU const& left, RightU const& right )
{ return left.get_value() > right.get_value(); }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::id == unit_traits< RightU >::id )
Boolean operator >=( LeftU const& left, RightU const& right )
{ return left.get_value() >= right.get_value(); }

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

// powers (compile time)
template< int Power, unit U >
constexpr power_unit_t< Power, U > pow( U const& u )
{ return { std::pow( u.get_value(), Power )}; }

constexpr Scalar sin( Scalar const& u )
{ return { std::sin( u.get_value() )}; }
constexpr Scalar cos( Scalar const& u )
{ return { std::cos( u.get_value() )}; }
constexpr Scalar tan( Scalar const& u )
{ return { std::tan( u.get_value() )}; }

constexpr Scalar asin( Scalar const& arg )
{ return { std::asin( arg.get_value() ) }; }
constexpr Scalar acos( Scalar const& arg )
{ return { std::acos( arg.get_value() ) }; }
constexpr Scalar atan( Scalar const& arg )
{ return { std::atan( arg.get_value() ) }; }

template< unit NumeratorU, unit DenominatorU >
requires( unit_traits< NumeratorU >::id == unit_traits< DenominatorU >::id )
constexpr Scalar atan2( NumeratorU const& num, DenominatorU const& den )
{ return { std::atan2( num.get_value(), den.get_value()) }; }

/**
 * represents a length in meters
 */
using Length = base_unit< length_unit_id, long double >;

constexpr Length::scalar_type meters( Length length ) 
{ return length.get_value(); }

static_assert( unit_traits< unit_product< Length, Length >>::unit_id == unit_id_type{ 4, 1 } );
static_assert( unit_traits< unit_quotient< Length, Length >>::unit_id == unit_traits< Scalar >::unit_id );

// length measurement units
Length operator""_m(long double meters)
{ return { meters }; }
Length operator""_m(unsigned long long meters)
{ return { (long double)meters }; }
Length operator""_pm(long double picometers)
{ return { 1e-12 * picometers }; }
Length operator""_pm(unsigned long long picometers)
{ return { 1e-12 * (long double)picometers }; }
Length operator""_nm(long double nanometers)
{ return { 1e-9 * nanometers }; }
Length operator""_nm(unsigned long long nanometers)
{ return { 1e-9 * (long double)nanometers }; }
#ifndef NO_UNICODE_COMPILER
Length operator""_μm(long double micrometers)
{ return { 1e-6 * micrometers }; }
Length operator""_μm(unsigned long long micrometers)
{ return { 1e-6 * (long double)micrometers }; }
#endif
Length operator""_mm(long double millimeters)
{ return { 1e-3 * millimeters }; }
Length operator""_mm(unsigned long long millimeters)
{ return { 1e-3 * (long double)millimeters }; }
Length operator""_km(long double kilometers)
{ return { 1e3 * kilometers }; }
Length operator""_km(unsigned long long kilometers)
{ return { 1e3 * (long double)kilometers }; } 
Length operator""_Mm(long double megameters)
{ return { 1e6 * megameters }; }
Length operator""_Mm(unsigned long long megameters)
{ return { 1e6 * (long double)megameters }; } 
Length operator""_Gm(long double gigameters)
{ return { 1e9 * gigameters }; }
Length operator""_Gm(unsigned long long gigameters)
{ return { 1e9 * (long double)gigameters }; } 
Length operator""_Tm(long double terameters)
{ return { 1e12 * terameters }; }
Length operator""_Tm(unsigned long long terameters)
{ return { 1e12 * (long double)terameters }; } 
Length operator""_Pm(long double petameters)
{ return { 1e15 * petameters }; }
Length operator""_Pm(unsigned long long petameters)
{ return { 1e15 * (long double)petameters }; } 
Length operator""_Em(long double exameters)
{ return { 1e18 * exameters }; }
Length operator""_Em(unsigned long long exameters)
{ return { 1e18 * (long double)exameters }; } 
Length operator""_Zm(long double zettameters)
{ return { 1e21 * zettameters }; }
Length operator""_Zm(unsigned long long zettameters)
{ return { 1e21 * (long double)zettameters }; } 
Length operator""_Ym(long double yottameters)
{ return { 1e24 * yottameters }; }
Length operator""_Ym(unsigned long long yottameters)
{ return { 1e24 * (long double)yottameters }; } 
Length operator""_ly(long double light_years)
{ return { light_years * seconds_per_day * meters_per_second }; }
Length operator""_ly(unsigned long long light_years)
{ return { (long double)light_years * seconds_per_day * meters_per_second }; } 
Length operator""_thou(long double thousands_of_an_inch )
{ return { 1e-3 * thousands_of_an_inch * meters_per_inch }; }
Length operator""_thou(unsigned long long thousands_of_an_inch)
{ return { 1e-3 * (long double)thousands_of_an_inch * meters_per_inch }; } 
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

enum class length_unit : size_t
{ meters = 0, picometers, nanometers, micrometers, millimeters, 
    kilometers, megameters, gigameters, terameters, petameters, 
    exameters, zettameters, yottameters, light_years, thou, inches, feet, miles };

static constexpr string length_unit_names[] = 
{ "m", "pm", "nm", "μm", "mm", "km", "Mm", "Gm", "Tm", "Pm", "Em", "Zm", "Ym", 
    "ly", "thou", "in", "ft", "mi" };

static constexpr string length_unit_long_names[] = 
{ "meters", "picometers", "nanometers", "micrometers", "millimeters", 
    "kilometers", "megameters", "gigameters", "terameters", "petameters", 
    "exameters", "zettameters", "yottameters", "light-years", "thou", "inches", 
    "feet", "miles" };

constexpr long double length_value( Length length, length_unit u )
{
    switch( u ) {
    case length_unit::meters: return meters( length );
    case length_unit::picometers: return 1e12 * meters( length );
    case length_unit::nanometers: return 1e9 * meters( length );
    case length_unit::micrometers: return 1e6 * meters( length );
    case length_unit::millimeters: return 1e3 * meters( length );
    case length_unit::kilometers: return 1e-3 * meters( length );
    case length_unit::megameters: return 1e-6 * meters( length );
    case length_unit::gigameters: return 1e-9 * meters( length );
    case length_unit::terameters: return 1e-12 * meters( length );
    case length_unit::petameters: return 1e-15 * meters( length );
    case length_unit::exameters: return 1e-18 * meters( length );
    case length_unit::zettameters: return 1e-21 * meters( length );
    case length_unit::yottameters: return 1e-24 * meters( length );
    case length_unit::light_years: 
        return meters( length ) / meters_per_second / seconds_per_year;
    case length_unit::thou: return 1e-3 * meters( length ) / meters_per_inch;
    case length_unit::inches: return meters( length ) / meters_per_inch;
    case length_unit::feet: return meters( length ) / meters_per_foot;
    case length_unit::miles: return meters( length ) / meters_per_mile;
    }
}

/**
 * represents an amount of time in seconds
 */
// TODO: interop with std::duration
using Time = base_unit< time_unit_id, long double >;

Time::scalar_type seconds( Time time )
{ return time.get_value(); }

// time measurement units
Time operator""_s( long double seconds )
{ return { seconds }; }
Time operator""_s( unsigned long long seconds )
{ return { (long double)seconds }; }
Time operator""_ps( long double picoseconds )
{ return { 1e-12 * picoseconds }; }
Time operator""_ps( unsigned long long picoseconds )
{ return { 1e12 * (long double)picoseconds }; }
Time operator""_ns( long double nanoseconds )
{ return { 1e-9 * nanoseconds }; }
Time operator""_ns( unsigned long long nanoseconds )
{ return { 1e-9 * (long double)nanoseconds }; }
#ifndef NO_UNICODE_COMPILER
Time operator""_μs( long double microseconds )
{ return { 1e-6 * microseconds }; }
Time operator""_μs( unsigned long long microseconds )
{ return { 1e-6 * (long double)microseconds }; }
#endif
Time operator""_ms( long double milliseconds )
{ return { 1e-3 * milliseconds }; }
Time operator""_ms( unsigned long long milliseconds )
{ return { 1e-3 * (long double)milliseconds }; }
Time operator""_min( long double minutes )
{ return { minutes * seconds_per_minute }; }
Time operator""_min( unsigned long long minutes )
{ return { (long double)minutes * seconds_per_minute }; }
Time operator""_h( long double hours )
{ return { hours * seconds_per_hour }; }
Time operator""_h( unsigned long long hours )
{ return { (long double)hours * seconds_per_hour }; }
Time operator""_d( long double days )
{ return { days * seconds_per_day }; }
Time operator""_d( unsigned long long days )
{ return { (long double)days * seconds_per_day }; }
Time operator""_wk( long double weeks )
{ return { weeks * seconds_per_week }; }
Time operator""_wk( unsigned long long weeks )
{ return { (long double)weeks * seconds_per_week }; }
Time operator""_y( long double years )
{ return { years * seconds_per_year }; }
Time operator""_y( unsigned long long years )
{ return { (long double)years * seconds_per_year }; }

enum class time_unit : size_t
{ seconds = 0, picoseconds, nanoseconds, microseconds, milliseconds, minutes, 
    hours, days, weeks, years };

static constexpr string time_unit_names[] = 
{ "s", "ps", "ns", "μs", "ms", "m", "h", "d", "wk", "y" };

static constexpr string time_unit_long_names[] = 
{ "seconds", "picoseconds", "nanoseconds", "microseconds", "milliseconds", 
    "minutes", "hours", "days", "weeks", "years" };

constexpr long double time_value( Time time, time_unit u )
{
    switch( u ) {
    case time_unit::seconds: return seconds( time );
    case time_unit::picoseconds: return 1e12 * seconds( time );
    case time_unit::nanoseconds: return 1e9 * seconds( time );
    case time_unit::microseconds: return 1e6 * seconds( time );
    case time_unit::milliseconds: return 1e3 * seconds( time );
    case time_unit::minutes: return seconds( time ) / seconds_per_minute;
    case time_unit::hours: return seconds( time ) / seconds_per_hour;
    case time_unit::days: return seconds( time ) / seconds_per_day;
    case time_unit::weeks: return seconds( time ) / seconds_per_week;
    case time_unit::years: return seconds( time ) / seconds_per_year;
    }
}

/**
 * represents a mass in kilograms
 */
using Mass = base_unit< mass_unit_id, long double >;

Time::scalar_type kilograms( Mass mass )
{ return mass.get_value(); }

Mass operator""_kg( long double kilograms )
{ return { kilograms }; }
Mass operator""_kg( unsigned long long kilograms )
{ return { (long double)kilograms }; }
Mass operator""_pg( long double picograms )
{ return { 1e-15 * picograms }; }
Mass operator""_pg( unsigned long long picograms )
{ return { 1e-15 * (long double)picograms }; }
Mass operator""_ng( long double nanograms )
{ return { 1e-12 * nanograms }; }
Mass operator""_ng( unsigned long long nanograms )
{ return { 1e-12 * (long double)nanograms }; }
#ifndef NO_UNICODE_COMPILER
Mass operator""_μg( long double micrograms )
{ return { 1e-9 * micrograms }; }
Mass operator""_μg( unsigned long long micrograms )
{ return { 1e-9 * (long double)micrograms }; }
#endif
Mass operator""_mg( long double milligrams )
{ return { 1e-6 * milligrams }; }
Mass operator""_mg( unsigned long long milligrams )
{ return { 1e-6 * (long double)milligrams }; }
Mass operator""_g( long double grams )
{ return { 1e-3 * grams }; }
Mass operator""_g( unsigned long long grams )
{ return { 1e-3 * (long double)grams }; }
Mass operator""_Mg( long double megagrams )
{ return { 1e3 * megagrams }; }
Mass operator""_Mg( unsigned long long megagrams )
{ return { 1e3 * (long double)megagrams }; }
Mass operator""_Gg( long double gigagrams )
{ return { 1e6 * gigagrams }; }
Mass operator""_Gg( unsigned long long gigagrams )
{ return { 1e6 * (long double)gigagrams }; }
Mass operator""_Tg( long double teragrams )
{ return { 1e9 * teragrams }; }
Mass operator""_Tg( unsigned long long teragrams )
{ return { 1e9 * (long double)teragrams }; }
Mass operator""_Pg( long double petagrams )
{ return { 1e12 * petagrams }; }
Mass operator""_Pg( unsigned long long petagrams )
{ return { 1e12 * (long double)petagrams }; }
Mass operator""_Eg( long double exagrams )
{ return { 1e15 * exagrams }; }
Mass operator""_Eg( unsigned long long exagrams )
{ return { 1e15 * (long double)exagrams }; }
Mass operator""_Zg( long double zettagrams )
{ return { 1e18 * zettagrams }; }
Mass operator""_Zg( unsigned long long zettagrams )
{ return { 1e18 * (long double)zettagrams }; }
Mass operator""_Yg( long double yottagrams )
{ return { 1e21 * yottagrams }; }
Mass operator""_Yg( unsigned long long yottagrams )
{ return { 1e21 * (long double)yottagrams }; }
Mass operator""_oz( long double ounces )
{ return { ounces * kilograms_per_ounce }; }
Mass operator""_oz( unsigned long long ounces )
{ return { (long double)ounces * kilograms_per_ounce }; }
Mass operator""_lb( long double pounds )
{ return { pounds * kilograms_per_pound}; }
Mass operator""_lb( unsigned long long pounds )
{ return { (long double)pounds * kilograms_per_pound }; }
Mass operator""_long_ton( long double long_tons )
{ return { long_tons * kilograms_per_long_ton }; }
Mass operator""_long_ton( unsigned long long long_tons )
{ return { (long double)long_tons * kilograms_per_long_ton }; }
Mass operator""_ton( long double tons )
{ return { tons * kilograms_per_ton }; }
Mass operator""_ton( unsigned long long tons )
{ return { (long double)tons * kilograms_per_ton }; }

enum class mass_unit : size_t 
{ kilograms = 0, picograms, nanograms, micrograms, milligrams, grams, megagrams,
    gigagrams, teragrams, petagrams, exagrams, zettagrams, yottagrams, ounces,
    pounds, long_tons, tons };

static constexpr string mass_unit_names[] = 
{ "kg", "pg", "ng", "μg", "mg", "g", "Mg", "Gg", "Tg", "Pg", "Eg", "Zg", "Yg", 
    "oz", "lb", "long-ton", "ton" };

static constexpr string mass_unit_long_names[] =
{ "kilograms", "picograms", "nanograms", "micrograms", "milligrams", "grams", "megagrams",
    "gigagrams", "teragrams", "petagrams", "exagrams", "zettagrams", "yottagrams", "ounces",
    "pounds", "long-tons", "tons" };

constexpr long double mass_value( Mass mass, mass_unit u )
{
    switch( u ) {
    case mass_unit::kilograms: return kilograms( mass );
    case mass_unit::picograms: return 1e15 * kilograms( mass );
    case mass_unit::nanograms: return 1e12 * kilograms( mass );
    case mass_unit::micrograms: return 1e9 * kilograms( mass );
    case mass_unit::milligrams: return 1e6 * kilograms( mass );
    case mass_unit::grams: return 1e3 * kilograms( mass );
    case mass_unit::megagrams: return 1e-3 * kilograms( mass );
    case mass_unit::gigagrams: return 1e-6 * kilograms( mass );
    case mass_unit::teragrams: return 1e-9 * kilograms( mass );
    case mass_unit::petagrams: return 1e-12 * kilograms( mass );
    case mass_unit::exagrams: return 1e-15 * kilograms( mass );
    case mass_unit::zettagrams: return 1e-18 * kilograms( mass );
    case mass_unit::yottagrams: return 1e-21 * kilograms( mass );
    case mass_unit::ounces: return kilograms( mass ) / kilograms_per_ounce;
    case mass_unit::pounds: return kilograms( mass ) / kilograms_per_pound;
    case mass_unit::long_tons: 
        return kilograms( mass ) / kilograms_per_long_ton;
    case mass_unit::tons: return kilograms( mass ) / kilograms_per_ton;
    }
}

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
            units::length_value( meters( s ), format_unit ), ctx );

        if( use_long_name )
            return ranges::copy( units::length_unit_long_names[ (size_t)format_unit ], 
                ctx.out() ).out;

        return ranges::copy( units::length_unit_names[ (size_t)format_unit ], 
            ctx.out() ).out;
    }
};

template<>
struct formatter< units::Time, char > : formatter< long double, char >
{
    units::time_unit format_unit = units::time_unit::seconds;
    bool use_long_name = false;

    constexpr void set_unit_name( string name )
    {
        if( name == "")
            return;

        auto i = find( begin( units::time_unit_names ), 
            end( units::time_unit_names ), name );
        if( i != end(units::time_unit_names) )
        {
            format_unit = (units::time_unit)distance( 
                begin( units::time_unit_names), i );
            use_long_name = false;
        }
        else 
        {
            auto j = find( begin( units::time_unit_long_names ), 
                end( units::time_unit_long_names ), name );
            
            // this is a consteval function so we can't throw exceptions
            // if ( j == end( units::time_unit_long_names ))
            //     throw format_error( "invalid format args for Length " );

            format_unit = (units::time_unit)distance( 
                begin( units::time_unit_long_names), j );
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
    FmtContext::iterator format( units::Time const& s, FmtContext& ctx ) const
    { 
        formatter< long double, char >::format( 
            units::time_value( s, format_unit ), ctx );

        if( use_long_name )
            return ranges::copy( units::time_unit_long_names[ (size_t)format_unit ], 
                ctx.out() ).out;

        return ranges::copy( units::time_unit_names[ (size_t)format_unit ], 
            ctx.out() ).out;
    }
};

template<>
struct formatter< units::Mass, char > : formatter< long double, char >
{
    units::mass_unit format_unit = units::mass_unit::kilograms;
    bool use_long_name = false;

    constexpr void set_unit_name( string name )
    {
        if( name == "")
            return;

        auto i = find( begin( units::mass_unit_names ), 
            end( units::mass_unit_names ), name );
        if( i != end(units::mass_unit_names) )
        {
            format_unit = (units::mass_unit)distance( 
                begin( units::mass_unit_names), i );
            use_long_name = false;
        }
        else 
        {
            auto j = find( begin( units::mass_unit_long_names ), 
                end( units::mass_unit_long_names ), name );
            
            // this is a consteval function so we can't throw exceptions
            // if ( j == end( units::mass_unit_long_names ))
            //     throw format_error( "invalid format args for Length " );

            format_unit = (units::mass_unit)distance( 
                begin( units::mass_unit_long_names), j );
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
    FmtContext::iterator format( units::Mass const& s, FmtContext& ctx ) const
    { 
        formatter< long double, char >::format( 
            units::mass_value( s, format_unit ), ctx );

        if( use_long_name )
            return ranges::copy( units::mass_unit_long_names[ (size_t)format_unit ], 
                ctx.out() ).out;

        return ranges::copy( units::mass_unit_names[ (size_t)format_unit ], 
            ctx.out() ).out;
    }
};


// TODO: format unit products and quotients

} // namespace std


#endif