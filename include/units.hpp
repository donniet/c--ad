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

constexpr unit_id_type operator* ( unit_id_type left, unit_id_type right )
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
constexpr Boolean true_bool = Boolean{ true };
constexpr Boolean false_bool = Boolean{ false };

// literals and construction methods for scalars
auto operator ""_scalar( long double x )
{ return Scalar{ x }; }
auto operator ""_scalar( unsigned long long x )
{ return Scalar{ (long double)x }; }
auto scalar( long double x )
{ return Scalar{ x }; }
auto operator ""_percent( long double x )
{ return Scalar{ 0.01 * x }; }
auto operator ""_percent( unsigned long long x )
{ return Scalar{ 0.01 * (long double)x }; }
auto percent( Cardinal n )
{ return Scalar{ 0.01 * (long double)n.value }; }
auto percent( unsigned long long n )
{ return Scalar{ 0.01 * (long double)n }; }
auto percent( long double x )
{ return Scalar{ 0.01 * x }; }

// literals and construction methods for cardinals
auto operator ""_cardinal( unsigned long long n )
{ return Cardinal{ n }; }
auto cardinal( Scalar x )
{ return Cardinal{ (unsigned long long)x.value }; }
auto cardinal( long double x )
{ return Cardinal{ (unsigned long long)x }; }
auto cardinal( unsigned long long n )
{ return Cardinal{ n }; }

// logical operations
auto operator and( Boolean const& left, Boolean const& right )
{ return Boolean{ left.value and right.value }; }
auto operator or( Boolean const& left, Boolean const& right )
{ return Boolean{ left.value or right.value }; }
auto operator not( Boolean const& arg )
{ return Boolean{ not arg.value }; }

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
{ return Boolean{ not arg.get_value() }; }

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
auto operator ==( Cardinal const& left, Scalar const& right )
{ return Boolean{ (long double)left.value == right.value }; }
auto operator !=( Cardinal const& left, Scalar const& right )
{ return Boolean{ (long double)left.value != right.value }; }
auto operator <( Cardinal const& left, Scalar const& right )
{ return Boolean{ (long double)left.value < right.value }; }
auto operator <=( Cardinal const& left, Scalar const& right )
{ return Boolean{ (long double)left.value <= right.value }; }
auto operator >( Cardinal const& left, Scalar const& right )
{ return Boolean{ (long double)left.value > right.value }; }
auto operator >=( Cardinal const& left, Scalar const& right )
{ return Boolean{ (long double)left.value >= right.value }; }
auto operator ==( Scalar const& left, Cardinal const& right )
{ return Boolean{ left.value == (long double)right.value }; }
auto operator !=( Scalar const& left, Cardinal const& right )
{ return Boolean{ left.value != (long double)right.value }; }
auto operator <( Scalar const& left, Cardinal const& right )
{ return Boolean{ left.value < (long double)right.value }; }
auto operator <=( Scalar const& left, Cardinal const& right )
{ return Boolean{ left.value <= (long double)right.value }; }
auto operator >( Scalar const& left, Cardinal const& right )
{ return Boolean{ left.value > (long double)right.value }; }
auto operator >=( Scalar const& left, Cardinal const& right )
{ return Boolean{ left.value >= (long double)right.value }; }

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

// powers (compile time)
template< int Power, unit U >
constexpr power_unit_t< Power, U > pow( U const& u )
{ return { std::pow( u.get_value(), Power )}; }

constexpr auto sin( Scalar const& u )
{ return Scalar{ std::sin( u.get_value() )}; }
constexpr auto cos( Scalar const& u )
{ return Scalar{ std::cos( u.get_value() )}; }
constexpr auto tan( Scalar const& u )
{ return Scalar{ std::tan( u.get_value() )}; }

constexpr auto asin( Scalar const& arg )
{ return Scalar{ std::asin( arg.get_value() ) }; }
constexpr auto acos( Scalar const& arg )
{ return Scalar{ std::acos( arg.get_value() ) }; }
constexpr auto atan( Scalar const& arg )
{ return Scalar{ std::atan( arg.get_value() ) }; }

template< unit NumeratorU, unit DenominatorU >
requires( unit_traits< NumeratorU >::id == unit_traits< DenominatorU >::id )
constexpr auto atan2( NumeratorU const& num, DenominatorU const& den )
{ return Scalar{ std::atan2( num.get_value(), den.get_value()) }; }

/**
 * represents a length in meters
 */
using Length = base_unit< length_unit_id, long double >;

constexpr Length::scalar_type meters( Length length ) 
{ return length.get_value(); }

static_assert( unit_traits< unit_product< Length, Length >>::unit_id == unit_id_type{ 4, 1 } );
static_assert( unit_traits< unit_quotient< Length, Length >>::unit_id == unit_traits< Scalar >::unit_id );

// length measurement units
auto operator""_m(long double meters)
{ return Length{ meters }; }
auto operator""_m(unsigned long long meters)
{ return Length{ (long double)meters }; }
auto operator""_pm(long double picometers)
{ return Length{ 1e-12 * picometers }; }
auto operator""_pm(unsigned long long picometers)
{ return Length{ 1e-12 * (long double)picometers }; }
auto operator""_nm(long double nanometers)
{ return Length{ 1e-9 * nanometers }; }
auto operator""_nm(unsigned long long nanometers)
{ return Length{ 1e-9 * (long double)nanometers }; }
#ifndef NO_UNICODE_COMPILER
auto operator""_μm(long double micrometers)
{ return Length{ 1e-6 * micrometers }; }
auto operator""_μm(unsigned long long micrometers)
{ return Length{ 1e-6 * (long double)micrometers }; }
#endif
auto operator""_mm(long double millimeters)
{ return Length{ 1e-3 * millimeters }; }
auto operator""_mm(unsigned long long millimeters)
{ return Length{ 1e-3 * (long double)millimeters }; }
auto operator""_km(long double kilometers)
{ return Length{ 1e3 * kilometers }; }
auto operator""_km(unsigned long long kilometers)
{ return Length{ 1e3 * (long double)kilometers }; } 
auto operator""_Mm(long double megameters)
{ return Length{ 1e6 * megameters }; }
auto operator""_Mm(unsigned long long megameters)
{ return Length{ 1e6 * (long double)megameters }; } 
auto operator""_Gm(long double gigameters)
{ return Length{ 1e9 * gigameters }; }
auto operator""_Gm(unsigned long long gigameters)
{ return Length{ 1e9 * (long double)gigameters }; } 
auto operator""_Tm(long double terameters)
{ return Length{ 1e12 * terameters }; }
auto operator""_Tm(unsigned long long terameters)
{ return Length{ 1e12 * (long double)terameters }; } 
auto operator""_Pm(long double petameters)
{ return Length{ 1e15 * petameters }; }
auto operator""_Pm(unsigned long long petameters)
{ return Length{ 1e15 * (long double)petameters }; } 
auto operator""_Em(long double exameters)
{ return Length{ 1e18 * exameters }; }
auto operator""_Em(unsigned long long exameters)
{ return Length{ 1e18 * (long double)exameters }; } 
auto operator""_Zm(long double zettameters)
{ return Length{ 1e21 * zettameters }; }
auto operator""_Zm(unsigned long long zettameters)
{ return Length{ 1e21 * (long double)zettameters }; } 
auto operator""_Ym(long double yottameters)
{ return Length{ 1e24 * yottameters }; }
auto operator""_Ym(unsigned long long yottameters)
{ return Length{ 1e24 * (long double)yottameters }; } 
auto operator""_ly(long double light_years)
{ return Length{ light_years * seconds_per_day * meters_per_second }; }
auto operator""_ly(unsigned long long light_years)
{ return Length{ (long double)light_years * seconds_per_day * meters_per_second }; } 
auto operator""_thou(long double thousands_of_an_inch )
{ return Length{ 1e-3 * thousands_of_an_inch * meters_per_inch }; }
auto operator""_thou(unsigned long long thousands_of_an_inch)
{ return Length{ 1e-3 * (long double)thousands_of_an_inch * meters_per_inch }; } 
auto operator""_in(long double inches)
{ return Length{ inches * meters_per_inch }; }
auto operator""_in(unsigned long long inches)
{ return Length{ (long double)inches * meters_per_inch }; }
auto operator""_ft(long double feet)
{ return Length{ feet * meters_per_foot }; }
auto operator""_ft(unsigned long long feet)
{ return Length{ (long double)feet * meters_per_foot }; }
auto operator""_mi(long double miles)
{ return Length{ miles * meters_per_mile }; }
auto operator""_mi(unsigned long long miles)
{ return Length{ (long double)miles * meters_per_mile }; }

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
auto operator""_s( long double seconds )
{ return Time{ seconds }; }
auto operator""_s( unsigned long long seconds )
{ return Time{ (long double)seconds }; }
auto operator""_ps( long double picoseconds )
{ return Time{ 1e-12 * picoseconds }; }
auto operator""_ps( unsigned long long picoseconds )
{ return Time{ 1e12 * (long double)picoseconds }; }
auto operator""_ns( long double nanoseconds )
{ return Time{ 1e-9 * nanoseconds }; }
auto operator""_ns( unsigned long long nanoseconds )
{ return Time{ 1e-9 * (long double)nanoseconds }; }
#ifndef NO_UNICODE_COMPILER
auto operator""_μs( long double microseconds )
{ return Time{ 1e-6 * microseconds }; }
auto operator""_μs( unsigned long long microseconds )
{ return Time{ 1e-6 * (long double)microseconds }; }
#endif
auto operator""_ms( long double milliseconds )
{ return Time{ 1e-3 * milliseconds }; }
auto operator""_ms( unsigned long long milliseconds )
{ return Time{ 1e-3 * (long double)milliseconds }; }
auto operator""_min( long double minutes )
{ return Time{ minutes * seconds_per_minute }; }
auto operator""_min( unsigned long long minutes )
{ return Time{ (long double)minutes * seconds_per_minute }; }
auto operator""_h( long double hours )
{ return Time{ hours * seconds_per_hour }; }
auto operator""_h( unsigned long long hours )
{ return Time{ (long double)hours * seconds_per_hour }; }
auto operator""_d( long double days )
{ return Time{ days * seconds_per_day }; }
auto operator""_d( unsigned long long days )
{ return Time{ (long double)days * seconds_per_day }; }
auto operator""_wk( long double weeks )
{ return Time{ weeks * seconds_per_week }; }
auto operator""_wk( unsigned long long weeks )
{ return Time{ (long double)weeks * seconds_per_week }; }
auto operator""_y( long double years )
{ return Time{ years * seconds_per_year }; }
auto operator""_y( unsigned long long years )
{ return Time{ (long double)years * seconds_per_year }; }

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

auto operator""_kg( long double kilograms )
{ return Mass{ kilograms }; }
auto operator""_kg( unsigned long long kilograms )
{ return Mass{ (long double)kilograms }; }
auto operator""_pg( long double picograms )
{ return Mass{ 1e-15 * picograms }; }
auto operator""_pg( unsigned long long picograms )
{ return Mass{ 1e-15 * (long double)picograms }; }
auto operator""_ng( long double nanograms )
{ return Mass{ 1e-12 * nanograms }; }
auto operator""_ng( unsigned long long nanograms )
{ return Mass{ 1e-12 * (long double)nanograms }; }
#ifndef NO_UNICODE_COMPILER
auto operator""_μg( long double micrograms )
{ return Mass{ 1e-9 * micrograms }; }
auto operator""_μg( unsigned long long micrograms )
{ return Mass{ 1e-9 * (long double)micrograms }; }
#endif
auto operator""_mg( long double milligrams )
{ return Mass{ 1e-6 * milligrams }; }
auto operator""_mg( unsigned long long milligrams )
{ return Mass{ 1e-6 * (long double)milligrams }; }
auto operator""_g( long double grams )
{ return Mass{ 1e-3 * grams }; }
auto operator""_g( unsigned long long grams )
{ return Mass{ 1e-3 * (long double)grams }; }
auto operator""_Mg( long double megagrams )
{ return Mass{ 1e3 * megagrams }; }
auto operator""_Mg( unsigned long long megagrams )
{ return Mass{ 1e3 * (long double)megagrams }; }
auto operator""_Gg( long double gigagrams )
{ return Mass{ 1e6 * gigagrams }; }
auto operator""_Gg( unsigned long long gigagrams )
{ return Mass{ 1e6 * (long double)gigagrams }; }
auto operator""_Tg( long double teragrams )
{ return Mass{ 1e9 * teragrams }; }
auto operator""_Tg( unsigned long long teragrams )
{ return Mass{ 1e9 * (long double)teragrams }; }
auto operator""_Pg( long double petagrams )
{ return Mass{ 1e12 * petagrams }; }
auto operator""_Pg( unsigned long long petagrams )
{ return Mass{ 1e12 * (long double)petagrams }; }
auto operator""_Eg( long double exagrams )
{ return Mass{ 1e15 * exagrams }; }
auto operator""_Eg( unsigned long long exagrams )
{ return Mass{ 1e15 * (long double)exagrams }; }
auto operator""_Zg( long double zettagrams )
{ return Mass{ 1e18 * zettagrams }; }
auto operator""_Zg( unsigned long long zettagrams )
{ return Mass{ 1e18 * (long double)zettagrams }; }
auto operator""_Yg( long double yottagrams )
{ return Mass{ 1e21 * yottagrams }; }
auto operator""_Yg( unsigned long long yottagrams )
{ return Mass{ 1e21 * (long double)yottagrams }; }
auto operator""_oz( long double ounces )
{ return Mass{ ounces * kilograms_per_ounce }; }
auto operator""_oz( unsigned long long ounces )
{ return Mass{ (long double)ounces * kilograms_per_ounce }; }
auto operator""_lb( long double pounds )
{ return Mass{ pounds * kilograms_per_pound}; }
auto operator""_lb( unsigned long long pounds )
{ return Mass{ (long double)pounds * kilograms_per_pound }; }
auto operator""_long_ton( long double long_tons )
{ return Mass{ long_tons * kilograms_per_long_ton }; }
auto operator""_long_ton( unsigned long long long_tons )
{ return Mass{ (long double)long_tons * kilograms_per_long_ton }; }
auto operator""_ton( long double tons )
{ return Mass{ tons * kilograms_per_ton }; }
auto operator""_ton( unsigned long long tons )
{ return Mass{ (long double)tons * kilograms_per_ton }; }

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
            units::length_value( s, format_unit ), ctx );

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