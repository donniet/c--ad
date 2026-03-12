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

// using namespace std::numbers;

template< typename T >
concept arithmetic = is_arithmetic_v< T >;

constexpr long double pi = std::numbers::pi_v< long double >;
constexpr long double two_pi = pi * 2.l;

// conversion factors for units
constexpr long double meters_per_inch = 0.0254;
constexpr long double meters_per_thou = meters_per_inch * 0.001;
constexpr long double meters_per_foot = meters_per_inch * 12.;
constexpr long double meters_per_mile = meters_per_foot * 5280.;
constexpr long double acres_per_square_mile = 640.;
constexpr long double cubic_meters_per_liter = 1e-3l;
// constexpr long double cups_per_milliliter = 240.;
constexpr long double cubic_inces_per_gallon = 231.;
constexpr long double fluid_ounces_per_cup = 16.;
constexpr long double cups_per_gallon = 16.;
constexpr long double cups_per_milliliter = cups_per_gallon / 
    cubic_inces_per_gallon / meters_per_inch / meters_per_inch / meters_per_inch *
    cubic_meters_per_liter / 1000.;
// static_assert( cups_per_milliliter == 1.l/236.58823l );
constexpr long double light_speed = 299'792'458; // c
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
constexpr long double radians_per_degree = pi / 180.l;
constexpr long double standard_acceleration_of_gravity = 9.80665; /* m/s^2 */
constexpr long double steradians_per_square_degree = 4. * 180. * 180. / pi;
constexpr long double atmospheres_per_pascal = 101325.;
constexpr long double density_of_mercury = 13595.1; /* kg/m^3 */
constexpr long double zero_celsius_in_kelvin = 273.15; /* K */ 
constexpr long double celsius_per_fahrenheit = 5.l / 9.l;
constexpr long double fahrenheit_at_zero_celsius = 32.l;

// start with 7 units including scalars
constexpr size_t total_units = 7;
using ull_t = unsigned long long;

// powers of higher units will be severely limited due to Godel numbering
constexpr std::array< ull_t, total_units > primes = 
/*0  1  2  3  4   5   6         7   8   9  10  11  12  13  14  15  16 */
{ 1, 2, 3, 5, 7, 11, 13 }; //, 17, 19, 23, 31, 37, 41, 47, 53, 61, 67 };
constexpr std::array< string, total_units > base_unit_names =
{ "", "m", "s", "kg", "A", "K", "cd" };
constexpr std::array< string, total_units > base_unit_long_names =
{ "", "meters", "seconds", "kilograms", "amperes", "kelvin", "candelas" };

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

constexpr unit_id_type scalar_unit_id =             { primes[0], 1 };
constexpr unit_id_type length_unit_id =             { primes[1], 1 };
constexpr unit_id_type time_unit_id =               { primes[2], 1 };
constexpr unit_id_type mass_unit_id =               { primes[3], 1 };
constexpr unit_id_type current_unit_id =            { primes[4], 1 };
constexpr unit_id_type temperature_unit_id =        { primes[5], 1 };
constexpr unit_id_type luminous_intensity_unit_id = { primes[6], 1 };

constexpr unit_id_type operator* ( unit_id_type left, unit_id_type right )
{ return { left.first / gcd( left.first, right.second ) * right.first / gcd( right.first, left.second ),
    left.second / gcd( left.second, right.first ) * right.second / gcd( right.second, left.first ) }; }

consteval unit_id_type operator/ ( unit_id_type left, unit_id_type right )
{ return left * unit_id_type{ right.second, right.first }; }

static_assert( unit_id_type{ 2, 1 } * unit_id_type{ 3, 2 } == unit_id_type{ 3, 1 } );
static_assert( unit_id_type{ 2, 1 } * unit_id_type{ 3, 1 } == unit_id_type{ 6, 1 } );
static_assert( ( unit_id_type{ 2, 1 } / unit_id_type{ 3, 1 } ) == unit_id_type{ 2, 3 } );
static_assert( ( unit_id_type{ 2, 1 } / unit_id_type{ 3, 2 } ) == unit_id_type{ 4, 3 } );

// factor the unit id into it's prime factors
constexpr std::array< int, total_units > factor( unit_id_type uid )
{
    std::array< int, total_units > ret;
    for( int i = 0; i < total_units; ++i )
        ret[i] = ( i == 0 ? 1 : 0 );

    // first factor the numerator
    auto [ num, den ] = uid;
    // TODO: there's gotta be a faster way to do this...
    for( int i = 1; i < total_units; ++i )
    {
        while( num % primes[i] == 0 )
        {
            ret[i]++;
            num /= primes[i];
        }
        while( den % primes[i] == 0 )
        {
            ret[i]--;
            num /= primes[i];
        }
    }

    return ret;
}

constexpr unit_id_type reconstitute( std::array< int, total_units > factors )
{
    unsigned long long num = 1;
    unsigned long long den = 1;

    for( int i = 1; i < total_units; ++i )
    {
        int power = factors[i];
        for(; power < 0; ++power )
            den *= primes[i];
        for(; power > 0; --power )
            num *= primes[i];
    }

    return { num, den };
}

static_assert( factor( scalar_unit_id ) == 
    std::array< int, total_units >{ 1, 0, 0, 0, 0, 0, 0 } );
static_assert( factor( length_unit_id ) == 
    std::array< int, total_units >{ 1, 1, 0, 0, 0, 0, 0 } );
static_assert( factor( time_unit_id ) == 
    std::array< int, total_units >{ 1, 0, 1, 0, 0, 0, 0 } );
static_assert( factor( mass_unit_id ) == 
    std::array< int, total_units >{ 1, 0, 0, 1, 0, 0, 0 } );
static_assert( factor( current_unit_id ) == 
    std::array< int, total_units >{ 1, 0, 0, 0, 1, 0, 0 } );
static_assert( factor( temperature_unit_id ) == 
    std::array< int, total_units >{ 1, 0, 0, 0, 0, 1, 0 } );
static_assert( factor( luminous_intensity_unit_id ) == 
    std::array< int, total_units >{ 1, 0, 0, 0, 0, 0, 1 } );
static_assert( factor( luminous_intensity_unit_id * length_unit_id * length_unit_id ) == 
    std::array< int, total_units >{ 1, 2, 0, 0, 0, 0, 1 } );

constexpr bool is_square_unit_id( unit_id_type uid )
{
    auto factors = factor( uid );
    for( int i = 1; i < total_units; i++ )
    {
        if( std::abs( factors[i] ) % 2 != 0 )
            return false;
    }

    return true;
}

constexpr unit_id_type unit_id_square_root( unit_id_type uid )
{
    auto factors = factor( uid );
    for( int i = 1; i < total_units; ++i )
        factors[i] /= 2;
    return reconstitute( factors );
}

template< int Exp >
constexpr unit_id_type unit_id_power( unit_id_type uid )
{ 
    auto factors = factor( uid );
    for( int i = 1; i < total_units; ++i )
        factors[i] *= Exp;
    return reconstitute( factors );
}


// TODO: create a unit_format_type which is parsed by formatter< unit >::parse
struct unit_format_type
{ 
};

constexpr string unit_id_name( unit_id_type uid, unit_format_type = {} )
{
    std::stringstream name;

    auto powers = factor( uid );
    for( int i = 1; i < total_units; ++i )
    {
        if( powers[i] == 0 )
            continue;
        
        name << " " << base_unit_names[i];
        if( powers[i] == 1 )
            continue;
        
        name << "^" << powers[i];
    }

    return name.str();
}

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
    explicit constexpr operator scalar_type() const
    { return value; }
    constexpr base_unit() : value{0.} { }
    constexpr base_unit( base_unit const& other ) : value{ other.value } { }
    explicit constexpr base_unit( scalar_type value ) : value{ value } { }

    constexpr scalar_type get_value() const
    { return value; }

    scalar_type value;
};

template< arithmetic T >
struct base_unit< scalar_unit_id, T >
{
    static constexpr bool is_unit = true;
    using scalar_type = T;
    static constexpr unit_id_type unit_id = scalar_unit_id;

    constexpr base_unit& operator=( base_unit const& other )
    { value = other.value; return *this; }
    constexpr base_unit& operator=( scalar_type const& other )
    { value = other; return *this; }
    // implicit cast operator
    constexpr operator scalar_type() const
    { return value; }
    constexpr base_unit() : value{0.} { }
    constexpr base_unit( base_unit const& other ) : value{ other.value } { }
    // implicit scalar constructor
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

// template< typename T >
// requires is_arithmetic_v< T >
// struct unit_traits< T >
// {
//     static constexpr unit_id_type unit_id = scalar_unit_id;
//     using scalar_type = T;
//     static constexpr bool is_discrete = std::is_integral_v< scalar_type >;
//     static constexpr bool is_continuous = not is_discrete;
// };

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
; //{ static constexpr bool value = is_arithmetic_v< T >; };

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
    static constexpr unit_id_type unit_id = unit_traits< U >::unit_id;
    using scalar_type = unit_traits< U >::scalar_type;

    using type = base_unit< { unit_id.second, unit_id.first }, scalar_type >;
};

template< unit U >
using unit_inverse = UnitInverse< U >::type;

template< unit U >
struct UnitSquareRoot
{
    static constexpr unit_id_type unit_id = 
        unit_id_square_root( unit_traits< U >::unit_id );
    using scalar_type = unit_traits< U >::scalar_type;

    using type = base_unit< unit_id, scalar_type >;
};

template< unit U >
using unit_square_root_t = UnitSquareRoot< U >::type;

template< int Exp, unit U >
struct UnitPower
{
    static constexpr unit_id_type unit_id = 
        unit_id_power< Exp >( unit_traits< U >::unit_id );
    using scalar_type = unit_traits< U >::scalar_type;
    using type = base_unit< unit_id, scalar_type >;
};

template< int Exp, unit U >
using unit_power_t = UnitPower< Exp, U >::type;

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
constexpr Scalar operator ""_scalar( long double x )
{ return { x }; }
constexpr Scalar operator ""_scalar( unsigned long long x )
{ return { (long double)x }; }
Scalar scalar( long double x )
{ return { x }; }
constexpr Scalar operator ""_percent( long double x )
{ return { 0.01 * x }; }
constexpr Scalar operator ""_percent( unsigned long long x )
{ return { 0.01 * (long double)x }; }
Scalar percent( Cardinal n )
{ return { 0.01 * (long double)n.value }; }
Scalar percent( unsigned long long n )
{ return { 0.01 * (long double)n }; }
Scalar percent( long double x )
{ return { 0.01 * x }; }

template< typename T >
constexpr auto mod( T k, T n )
{ return ((k %= n) < 0) ? k + n : k; }

template< >
constexpr auto mod< long double >( long double k, long double n )
{ 
    if( k >= 0 and k < n )
        return k;

    return k - std::floorl( k / n ) * n; 
}

template< >
constexpr auto mod< double >( double k, double n )
{ return mod< long double >( k, n ); }

template< >
constexpr auto mod< float >( float k, float n )
{ return mod< long double >( k, n ); }

constexpr Scalar radians( long double x )
{ 
    return mod( x, two_pi ); 
}

template< typename T >
constexpr Scalar degrees( T x )
{ return radians( (long double)x * radians_per_degree ); }

template< >
constexpr Scalar degrees< int >( int x )
{ return radians((long double)mod< int>( x, 360 ) * radians_per_degree ); }
template< >
constexpr Scalar degrees< unsigned int >( unsigned int x )
{ return radians((long double)mod< long long >( x, 360 ) * radians_per_degree ); }
template< >
constexpr Scalar degrees< long >( long x )
{ return radians((long double)mod< long long >( x, 360 ) * radians_per_degree ); }
template< >
constexpr Scalar degrees< unsigned long >( unsigned long x )
{ return radians((long double)mod< long long >( x, 360 ) * radians_per_degree ); }
template< >
constexpr Scalar degrees< long long >( long long x )
{ return radians((long double)mod< long long >( x, 360 ) * radians_per_degree ); }

template< >
constexpr Scalar degrees< unsigned long long >( unsigned long long x )
{ return degrees< long long >( x ); }

constexpr Scalar operator ""_deg( unsigned long long x )
{ return degrees< long long >( x ); }
constexpr Scalar operator ""_rad( long double x )
{ 
    return radians( x ); 
}
constexpr Scalar operator ""_rad( unsigned long long x )
{ return radians( x ); }

// literals and construction methods for cardinals
constexpr Cardinal operator ""_cardinal( unsigned long long n )
{ return { n }; }
constexpr Cardinal cardinal( Scalar x )
{ return { (unsigned long long)x.value }; }
constexpr Cardinal cardinal( long double x )
{ return { (unsigned long long)x }; }
constexpr Cardinal cardinal( unsigned long long n )
{ return { n }; }

// general unit arithmetic
template< unit LeftU, unit RightU >
constexpr unit_product< LeftU, RightU > operator *( LeftU left, RightU right )
{ return unit_product< LeftU, RightU >{ left.get_value() * right.get_value() }; }

template< typename T, unit RightU >
requires( not unit< T > )
constexpr RightU operator *( T left, RightU right )
{ return RightU{ left * right.get_value() }; }

template< unit LeftU, typename T >
requires( not unit< T > )
constexpr LeftU operator *( LeftU left, T right )
{ return LeftU{ left.get_value() * right }; }

template< unit LeftU, unit RightU >
constexpr unit_quotient< LeftU, RightU > operator /( LeftU left, RightU right )
{ return unit_quotient< LeftU, RightU >{ left.get_value() / right.get_value() }; }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::unit_id == unit_traits< RightU >::unit_id )
constexpr LeftU operator +( LeftU const& left, RightU const& right )
{ return LeftU{ left.get_value() + right.get_value() }; }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::unit_id == unit_traits< RightU >::unit_id )
constexpr LeftU operator -( LeftU const& left, RightU const& right )
{ return LeftU{ left.get_value() - right.get_value() }; }

template< unit U >
requires( unit_traits< U >::is_continuous )
constexpr U operator-( U const& arg )
{ return U{ -arg.get_value() }; }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::is_continuous or unit_traits< RightU >::is_continuous )
constexpr unit_quotient< LeftU, RightU > operator %( LeftU left, RightU right )
{ return unit_quotient< LeftU, RightU >{ mod( left.get_value(), right.get_value() )}; }

// discrete operators
template< unit U >
requires( unit_traits< U >::is_discrete )
constexpr U operator ~( U const& arg )
{ return U{ ~arg.get_value() }; }

constexpr Boolean operator ~( Boolean const& arg )
{ return Boolean{ not arg.get_value() }; }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::is_discrete and unit_traits< RightU >::is_discrete )
constexpr unit_quotient< LeftU, RightU > operator %( LeftU left, RightU right )
{ return unit_quotient< LeftU, RightU >{ left.get_value() % right.get_value() }; }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::is_discrete and unit_traits< RightU >::is_discrete 
    and unit_traits< LeftU >::unit_id == unit_traits< RightU >::unit_id )
constexpr LeftU operator |( LeftU const& left, RightU const& right )
{ return LeftU{ left.get_value() | right.get_value() }; }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::is_discrete and unit_traits< RightU >::is_discrete 
    and unit_traits< LeftU >::unit_id == unit_traits< RightU >::unit_id )
constexpr LeftU operator &( LeftU const& left, RightU const& right )
{ return LeftU{ left.get_value() & right.get_value() }; }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::is_discrete and unit_traits< RightU >::is_discrete 
    and unit_traits< LeftU >::unit_id == unit_traits< RightU >::unit_id )
constexpr LeftU operator ^( LeftU const& left, RightU const& right )
{ return LeftU{ left.get_value() ^ right.get_value() }; }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::is_discrete and unit_traits< RightU >::is_discrete 
    and unit_traits< LeftU >::unit_id == unit_traits< RightU >::unit_id )
constexpr LeftU operator >>( LeftU const& left, RightU const& right )
{ return LeftU{ left.get_value() >> right.get_value() }; }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::is_discrete and unit_traits< RightU >::is_discrete 
    and unit_traits< LeftU >::unit_id == unit_traits< RightU >::unit_id )
constexpr LeftU operator <<( LeftU const& left, RightU const& right )
{ return LeftU{ left.get_value() << right.get_value() }; }

// general unit comparison
template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::unit_id == unit_traits< RightU >::unit_id and
    not is_same_v< typename unit_traits< LeftU >::scalar_type, long double >)
Boolean operator ==( LeftU const& left, RightU const& right )
{ return left.get_value() == right.get_value(); }

// DT: if we are using long doubles, then equality is when the square
// of the distance between them is less than this system's epislon
template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::unit_id == unit_traits< RightU >::unit_id and
    is_same_v< typename unit_traits< LeftU >::scalar_type, long double > )
Boolean operator ==( LeftU const& left, RightU const& right )
{ 
    static constexpr long double eps = std::numeric_limits< double >::min();

    return  std::abs( left.get_value() - right.get_value() ) < eps;
}

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::unit_id == unit_traits< RightU >::unit_id )
Boolean operator !=( LeftU const& left, RightU const& right )
{ return left.get_value() != right.get_value(); }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::unit_id == unit_traits< RightU >::unit_id )
Boolean operator <( LeftU const& left, RightU const& right )
{ return left.get_value() < right.get_value(); }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::unit_id == unit_traits< RightU >::unit_id )
Boolean operator <=( LeftU const& left, RightU const& right )
{ return left.get_value() <= right.get_value(); }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::unit_id == unit_traits< RightU >::unit_id )
Boolean operator >( LeftU const& left, RightU const& right )
{ return left.get_value() > right.get_value(); }

template< unit LeftU, unit RightU >
requires( unit_traits< LeftU >::unit_id == unit_traits< RightU >::unit_id )
Boolean operator >=( LeftU const& left, RightU const& right )
{ return left.get_value() >= right.get_value(); }

// powers (compile time)
template< int Power, unit U >
constexpr power_unit_t< Power, U > pow( U const& u )
{ return power_unit_t< Power, U >{ std::pow( u.get_value(), Power )}; }

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
requires( unit_traits< NumeratorU >::unit_id == unit_traits< DenominatorU >::unit_id )
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
constexpr auto operator""_m(long double meters)
{ return Length{ meters }; }
constexpr auto operator""_m(unsigned long long meters)
{ return Length{ (long double)meters }; }
constexpr auto operator""_pm(long double picometers)
{ return Length{ 1e-12 * picometers }; }
constexpr auto operator""_pm(unsigned long long picometers)
{ return Length{ 1e-12 * (long double)picometers }; }
constexpr auto operator""_nm(long double nanometers)
{ return Length{ 1e-9 * nanometers }; }
constexpr auto operator""_nm(unsigned long long nanometers)
{ return Length{ 1e-9 * (long double)nanometers }; }
#ifndef NO_UNICODE_COMPILER
constexpr auto operator""_μm(long double micrometers)
{ return Length{ 1e-6 * micrometers }; }
constexpr auto operator""_μm(unsigned long long micrometers)
{ return Length{ 1e-6 * (long double)micrometers }; }
#endif
constexpr auto operator""_mm(long double millimeters)
{ return Length{ 1e-3 * millimeters }; }
constexpr auto operator""_mm(unsigned long long millimeters)
{ return Length{ 1e-3 * (long double)millimeters }; }
constexpr auto operator""_km(long double kilometers)
{ return Length{ 1e3 * kilometers }; }
constexpr auto operator""_km(unsigned long long kilometers)
{ return Length{ 1e3 * (long double)kilometers }; } 
constexpr auto operator""_Mm(long double megameters)
{ return Length{ 1e6 * megameters }; }
constexpr auto operator""_Mm(unsigned long long megameters)
{ return Length{ 1e6 * (long double)megameters }; } 
constexpr auto operator""_Gm(long double gigameters)
{ return Length{ 1e9 * gigameters }; }
constexpr auto operator""_Gm(unsigned long long gigameters)
{ return Length{ 1e9 * (long double)gigameters }; } 
constexpr auto operator""_Tm(long double terameters)
{ return Length{ 1e12 * terameters }; }
constexpr auto operator""_Tm(unsigned long long terameters)
{ return Length{ 1e12 * (long double)terameters }; } 
constexpr auto operator""_Pm(long double petameters)
{ return Length{ 1e15 * petameters }; }
constexpr auto operator""_Pm(unsigned long long petameters)
{ return Length{ 1e15 * (long double)petameters }; } 
constexpr auto operator""_Em(long double exameters)
{ return Length{ 1e18 * exameters }; }
constexpr auto operator""_Em(unsigned long long exameters)
{ return Length{ 1e18 * (long double)exameters }; } 
constexpr auto operator""_Zm(long double zettameters)
{ return Length{ 1e21 * zettameters }; }
constexpr auto operator""_Zm(unsigned long long zettameters)
{ return Length{ 1e21 * (long double)zettameters }; } 
constexpr auto operator""_Ym(long double yottameters)
{ return Length{ 1e24 * yottameters }; }
constexpr auto operator""_Ym(unsigned long long yottameters)
{ return Length{ 1e24 * (long double)yottameters }; } 
constexpr auto operator""_ly(long double light_years)
{ return Length{ light_years * seconds_per_day * light_speed }; }
constexpr auto operator""_ly(unsigned long long light_years)
{ return Length{ (long double)light_years * seconds_per_day * light_speed }; } 
constexpr auto operator""_thou(long double thousands_of_an_inch )
{ return Length{ 1e-3 * thousands_of_an_inch * meters_per_inch }; }
constexpr auto operator""_thou(unsigned long long thousands_of_an_inch)
{ return Length{ 1e-3 * (long double)thousands_of_an_inch * meters_per_inch }; } 
constexpr auto operator""_in(long double inches)
{ return Length{ inches * meters_per_inch }; }
constexpr auto operator""_in(unsigned long long inches)
{ return Length{ (long double)inches * meters_per_inch }; }
constexpr auto operator""_ft(long double feet)
{ return Length{ feet * meters_per_foot }; }
constexpr auto operator""_ft(unsigned long long feet)
{ return Length{ (long double)feet * meters_per_foot }; }
constexpr auto operator""_mi(long double miles)
{ return Length{ miles * meters_per_mile }; }
constexpr auto operator""_mi(unsigned long long miles)
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
        return meters( length ) / light_speed / seconds_per_year;
    case length_unit::thou: return 1e-3 * meters( length ) / meters_per_inch;
    case length_unit::inches: return meters( length ) / meters_per_inch;
    case length_unit::feet: return meters( length ) / meters_per_foot;
    case length_unit::miles: return meters( length ) / meters_per_mile;
    }
}

/**
 * represents an area as a length squared  
 */
using Area = base_unit< length_unit_id * length_unit_id, long double >;

// area is stored in square meter units
constexpr Area::scalar_type square_meters( Area area )
{ return area.get_value(); }

static_assert( unit_traits< unit_product< Length, Length >>::unit_id == 
    unit_traits< Area >::unit_id );

constexpr auto operator""_km2( long double sqkm )
{ return Area{ sqkm * 1'000'000. }; }
constexpr auto operator""_km2( unsigned long long sqkm )
{ return Area{ (long double)sqkm * 1'000'000 }; }
constexpr auto operator""_m2( long double sq_meters )
{ return Area{ sq_meters }; }
constexpr auto operator""_m2( unsigned long long sq_meters )
{ return Area{ (long double)sq_meters }; }
constexpr auto operator""_cm2( long double sqcm )
{ return Area{ sqcm * 0.01 * 0.01 }; }
constexpr auto operator""_cm2( unsigned long long sqcm )
{ return Area{ (long double)sqcm * 0.01 * 0.01 }; }
constexpr auto operator""_mm2( long double sqmm )
{ return Area{ sqmm * 0.001 * 0.001 }; }
constexpr auto operator""_mm2( unsigned long long sqmm )
{ return Area{ (long double)sqmm * 0.001 * 0.001 }; }
constexpr auto operator""_sqmi( long double sqmi )
{ return Area{ sqmi * meters_per_mile * meters_per_mile }; }
constexpr auto operator""_sqmi( unsigned long long sqmi )
{ return Area{ (long double)sqmi * meters_per_mile * meters_per_mile }; }
constexpr auto operator""_acre( long double acres )
{ return Area{ acres * meters_per_mile * meters_per_mile / 
    acres_per_square_mile }; }
constexpr auto operator""_acre( unsigned long long acres )
{ return Area{ (long double)acres * meters_per_mile * meters_per_mile / 
    acres_per_square_mile }; }
constexpr auto operator""_sqft( long double sqft )
{ return Area{ sqft * meters_per_foot * meters_per_foot }; }
constexpr auto operator""_sqft( unsigned long long sqft )
{ return Area{ (long double)sqft * meters_per_foot * meters_per_foot }; }
constexpr auto operator""_sqin( long double sqin )
{ return Area{ sqin * meters_per_inch * meters_per_inch }; }
constexpr auto operator""_sqin( unsigned long long sqin )
{ return Area{ (long double)sqin * meters_per_inch * meters_per_inch }; }

enum class area_unit : size_t
{ square_meters = 0, square_millimeters, square_centimeters, 
    square_kilometers, square_feet, square_inches, acres, square_miles };

static constexpr const char* area_unit_names[] =
{ "m2", "mm2", "cm2", "km2", "sqft", "sqin", "acre", "sqmi" };

static constexpr const char* area_unit_long_names[] = 
{ "square meters", "square millimeters", "square centimeters", 
    "square kilometers", "square feet", "square inches", "acres", "square miles" };

constexpr long double area_value( Area area, area_unit u )
{
    switch( u ) {
    case area_unit::square_meters: return square_meters( area );
    case area_unit::square_millimeters: return 1'000'000 * square_meters( area );
    case area_unit::square_centimeters: return 10'000 * square_meters( area );
    case area_unit::square_kilometers: return 0.000'001 * square_meters( area );
    case area_unit::square_feet: return square_meters( area ) / 
        meters_per_foot / meters_per_foot;
    case area_unit::square_inches: return square_meters( area ) / 
        meters_per_inch / meters_per_inch;
    case area_unit::acres: 
        return acres_per_square_mile * 
            area_value( area, area_unit::square_miles );
    case area_unit::square_miles: return square_meters( area ) / 
        meters_per_mile / meters_per_mile;
    }
} 

/**
 * represents a volume or length cubed
 */
using Volume = base_unit< length_unit_id * length_unit_id * length_unit_id,
    long double >;

// volume is stored in cubic meter units
constexpr Volume::scalar_type cubic_meters( Volume volume )
{ return volume.get_value(); }
constexpr Volume::scalar_type liters( Volume volume )
{ return volume.get_value() / cubic_meters_per_liter; }
constexpr Volume::scalar_type milliliters( Volume volume )
{ return volume.get_value() / cubic_meters_per_liter * 1000.; }

static_assert( unit_traits< unit_product< Length, Length, Length >>::unit_id == 
    unit_traits< Volume >::unit_id );

constexpr auto operator""_km3( long double cubic_kilometers )
{ return Volume{ cubic_kilometers * 1e9l }; }
constexpr auto operator""_km3( unsigned long long cubic_kilometers )
{ return Volume{ (long double)cubic_kilometers * 1e9l }; }
constexpr auto operator""_m3( long double cubic_meters )
{ return Volume{ cubic_meters }; }
constexpr auto operator""_m3( unsigned long long cubic_meters )
{ return Volume{ (long double)cubic_meters }; }
constexpr auto operator""_cc( long double cubic_centimeters )
{ return Volume{ cubic_centimeters * 1e-6l }; }
constexpr auto operator""_cc( unsigned long long cubic_centimeters )
{ return Volume{ (long double)cubic_centimeters * 1e-6l }; }
constexpr auto operator""_mL( long double milliliters )
{ return Volume{ milliliters * 1e-6l }; }
constexpr auto operator""_mL( unsigned long long milliliters )
{ return Volume{ (long double)milliliters * 1e-6l }; }
constexpr auto operator""_L( long double liters )
{ return Volume{ liters * 1e-3l }; }
constexpr auto operator""_L( unsigned long long liters )
{ return Volume{ (long double)liters * 1e-3l }; }
constexpr auto operator""_mm3( long double cubic_millimeters )
{ return Volume{ cubic_millimeters * 1e-9l }; }
constexpr auto operator""_mm3( unsigned long long cubic_millimeters )
{ return Volume{ (long double)cubic_millimeters * 1e-9l }; }

constexpr auto operator""_ft3( long double cubic_feet )
{ return Volume{ cubic_feet * meters_per_foot * meters_per_foot * 
    meters_per_foot }; }
constexpr auto operator""_ft3( unsigned long long cubic_feet )
{ return Volume{ (long double)cubic_feet * meters_per_foot * meters_per_foot * 
    meters_per_foot }; }
constexpr auto operator""_mi3( long double cubic_miles )
{ return Volume{ cubic_miles * meters_per_mile * meters_per_mile * 
    meters_per_mile }; }
constexpr auto operator""_mi3( unsigned long long cubic_miles )
{ return Volume{ (long double)cubic_miles * meters_per_mile * meters_per_mile *
    meters_per_mile }; }
constexpr auto operator""_in3( long double cubic_inches )
{ return Volume{ cubic_inches * meters_per_inch * meters_per_inch * 
    meters_per_inch }; }
constexpr auto operator""_in3( unsigned long long cubic_inches )
{ return Volume{ (long double)cubic_inches * meters_per_inch * meters_per_inch * 
    meters_per_inch}; }

constexpr auto operator""_gal( long double gallons )
{ return Volume{ gallons / cubic_inces_per_gallon * meters_per_inch * 
    meters_per_inch * meters_per_inch }; }
constexpr auto operator""_gal( unsigned long long gallons )
{ return Volume{ (long double)gallons / cubic_inces_per_gallon * meters_per_inch * 
    meters_per_inch * meters_per_inch }; }
constexpr auto operator""_cp( long double cups )
{ return Volume{ cups / cups_per_milliliter / 1000.l / 
    cubic_meters_per_liter }; }
constexpr auto operator""_cp( unsigned long long cups )
{ return Volume{ (long double)cups / cups_per_milliliter / 1000.l / 
    cubic_meters_per_liter }; }
constexpr auto operator""_floz( long double fluid_ounces )
{ return Volume{ fluid_ounces / fluid_ounces_per_cup / cups_per_milliliter / 1000.l * 
    cubic_meters_per_liter }; }
constexpr auto operator""_floz( unsigned long long fluid_ounces )
{ return Volume{ (long double)fluid_ounces / fluid_ounces_per_cup / cups_per_milliliter / 1000.l * 
    cubic_meters_per_liter }; }
constexpr auto operator""_Tbsp( long double tablespoons )
{ return Volume{ tablespoons / 2.0l / fluid_ounces_per_cup / cups_per_milliliter / 1000.l * 
    cubic_meters_per_liter }; }
constexpr auto operator""_Tbsp( unsigned long long tablespoons )
{ return Volume{ (long double)tablespoons / 2.0l / fluid_ounces_per_cup / cups_per_milliliter / 1000.l * 
    cubic_meters_per_liter }; }
constexpr auto operator""_tsp( long double tablespoons )
{ return Volume{ tablespoons / 6.0l / fluid_ounces_per_cup / cups_per_milliliter / 1000.l * 
    cubic_meters_per_liter }; }
constexpr auto operator""_tsp( unsigned long long tablespoons )
{ return Volume{ (long double)tablespoons / 6.0l / fluid_ounces_per_cup / cups_per_milliliter / 1000.l * 
    cubic_meters_per_liter }; }

enum class volume_unit : size_t 
{ cubic_meters = 0, cubic_kilometers, cubic_centimeters, liters, cubic_millimeters, 
    cubic_miles, cubic_feet, cubic_inches, gallons, cups, us_fluid_ounces, 
    tablespoons, teaspoons };

static constexpr const char* volume_unit_names[] = 
{ "m3", "km3", "mL", "L", "mm3", "mi3", "ft3", "in3", "gal", "c", "floz", "Tbsp", "tsp" };

static constexpr const char* volume_unit_long_names[] = 
{ "cubic_meters", "cubic_kilometers", "cubic_centimeters", "liters", "cubic_millimeters", 
    "cubic_miles", "cubic_feet", "cubic_inches", "gallons", "cups", "us_fluid_ounces",
    "tablespoons", "teaspoons" };

/**
 * represents an amount of time in seconds
 */
// TODO: interop with std::duration
using Time = base_unit< time_unit_id, long double >;

Time::scalar_type seconds( Time time )
{ return time.get_value(); }

// time measurement units
constexpr auto operator""_s( long double seconds )
{ return Time{ seconds }; }
constexpr auto operator""_s( unsigned long long seconds )
{ return Time{ (long double)seconds }; }
constexpr auto operator""_ps( long double picoseconds )
{ return Time{ 1e-12 * picoseconds }; }
constexpr auto operator""_ps( unsigned long long picoseconds )
{ return Time{ 1e12 * (long double)picoseconds }; }
constexpr auto operator""_ns( long double nanoseconds )
{ return Time{ 1e-9 * nanoseconds }; }
constexpr auto operator""_ns( unsigned long long nanoseconds )
{ return Time{ 1e-9 * (long double)nanoseconds }; }
#ifndef NO_UNICODE_COMPILER
constexpr auto operator""_μs( long double microseconds )
{ return Time{ 1e-6 * microseconds }; }
constexpr auto operator""_μs( unsigned long long microseconds )
{ return Time{ 1e-6 * (long double)microseconds }; }
#endif
constexpr auto operator""_ms( long double milliseconds )
{ return Time{ 1e-3 * milliseconds }; }
constexpr auto operator""_ms( unsigned long long milliseconds )
{ return Time{ 1e-3 * (long double)milliseconds }; }
constexpr auto operator""_min( long double minutes )
{ return Time{ minutes * seconds_per_minute }; }
constexpr auto operator""_min( unsigned long long minutes )
{ return Time{ (long double)minutes * seconds_per_minute }; }
constexpr auto operator""_h( long double hours )
{ return Time{ hours * seconds_per_hour }; }
constexpr auto operator""_h( unsigned long long hours )
{ return Time{ (long double)hours * seconds_per_hour }; }
constexpr auto operator""_d( long double days )
{ return Time{ days * seconds_per_day }; }
constexpr auto operator""_d( unsigned long long days )
{ return Time{ (long double)days * seconds_per_day }; }
constexpr auto operator""_wk( long double weeks )
{ return Time{ weeks * seconds_per_week }; }
constexpr auto operator""_wk( unsigned long long weeks )
{ return Time{ (long double)weeks * seconds_per_week }; }
constexpr auto operator""_y( long double years )
{ return Time{ years * seconds_per_year }; }
constexpr auto operator""_y( unsigned long long years )
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
 * represents a velocity or speed in meters per second
 */
using Velocity = base_unit< length_unit_id / time_unit_id, long double >;

constexpr long double meters_per_second( Velocity const& v )
{ return v.get_value(); }

constexpr auto operator""_mps( long double meters_per_second )
{ return Velocity{ meters_per_second }; }
constexpr auto operator""_mps( unsigned long long meters_per_second )
{ return Velocity{ (long double) meters_per_second }; }
constexpr auto operator""_kmph( long double kilometers_per_hour )
{ return Velocity{ kilometers_per_hour * 1000. /* m per km */ / 3600. /* seconds per hour */ }; }
constexpr auto operator""_kmph( unsigned long long kilometers_per_hour )
{ return Velocity{ (long double)kilometers_per_hour * 1000. /* m per km */ / 3600. /* seconds per hour */ }; }
constexpr auto operator""_miph( long double miles_per_hour )
{ return Velocity{ miles_per_hour * meters_per_mile / 3600. /* seconds per hour */ }; }
constexpr auto operator""_miph( unsigned long long miles_per_hour )
{ return Velocity{ (long double)miles_per_hour * meters_per_mile / 3600. /* seconds per hour */ }; }
constexpr auto operator""_ftps( long double feet_per_second )
{ return Velocity{ feet_per_second * meters_per_foot }; }
constexpr auto operator""_ftps( unsigned long long feet_per_second )
{ return Velocity{ (long double)feet_per_second * meters_per_foot }; }

enum class velocity_unit_type: size_t 
{ meters_per_second = 0, kilometers_per_hour, miles_per_hour, feet_per_second };
constexpr string velocity_unit_names[] = 
{ "m/s", "km/s", "mi/s", "ft/s" };
constexpr const char* velocity_unit_long_names[] = 
{ "meters per second", "kilometers per hour", "miles per hour", "feet per second" };
constexpr long double velocity_value( Velocity v, velocity_unit_type u )
{
    switch( u ) {
    case velocity_unit_type::meters_per_second: 
    default:
        return meters_per_second( v );
    case velocity_unit_type::kilometers_per_hour:
        return meters_per_second( v ) * 1e-3 * 3600.;
    case velocity_unit_type::miles_per_hour:
        return meters_per_second( v ) / meters_per_mile * 1e-3 * 3600.;
    case velocity_unit_type::feet_per_second:
        return meters_per_second( v ) / meters_per_foot;
    }
}


/**
 * represents an electric current in Amperes
 */
using Current = base_unit< current_unit_id, long double >;

constexpr auto operator""_A ( long double amperes )
{ return Current{ amperes }; }
constexpr auto operator""_A ( unsigned long long amperes )
{ return Current{ (long double)amperes }; }
constexpr auto operator""_mA ( long double milliamperes )
{ return Current{ milliamperes * 1e-3}; }
constexpr auto operator""_mA ( unsigned long long milliamperes )
{ return Current{ (long double)milliamperes * 1e-3 }; }
constexpr auto operator""_kA ( long double kiloamperes )
{ return Current{ kiloamperes * 1e3 }; }
constexpr auto operator""_kA ( unsigned long long kiloamperes )
{ return Current{ (long double)kiloamperes * 1e3 }; }

constexpr long double amperes( Current c )
{ return c.get_value(); }
enum class current_unit_type: size_t 
{ amperes = 0, milliamperes, kiloamperes };
constexpr string current_unit_names[] = 
{ "A", "mA", "kA" };
constexpr string current_unit_long_names[] =
{ "amperes", "milliamperes", "kiloamperes" };
constexpr long double current_value( Current c, current_unit_type u )
{
    switch( u ) {
    case current_unit_type::amperes:
    default:
        return amperes( c );
    case current_unit_type::milliamperes:
        return amperes( c ) * 1e3;
    case current_unit_type::kiloamperes:
        return amperes( c ) * 1e-3;
    }
}

/**
 * represents a unit of electric charge in coulombs ( amp seconds )
 */
using Charge = base_unit< current_unit_id * time_unit_id, long double >;

constexpr auto operator""_C ( long double coulombs )
{ return Charge{ coulombs }; }
constexpr auto operator""_C ( unsigned long long coulombs )
{ return Charge{ (long double)coulombs }; }

constexpr long double coulombs( Charge c )
{ return c.get_value(); }

enum class charge_unit_type: size_t 
{ coulombs = 0 };
constexpr string charge_unit_names[] = 
{ "C" };
constexpr string charge_unit_long_names[] =
{ "coulombs" };
constexpr long double charge_value( Charge c, charge_unit_type u )
{
    switch( u ) {
    case charge_unit_type::coulombs:
    default:
        return coulombs( c );
    }
}

/**
 * represents a mass in kilograms
 */
using Mass = base_unit< mass_unit_id, long double >;

Mass::scalar_type kilograms( Mass mass )
{ return mass.get_value(); }

constexpr auto operator""_kg( long double kilograms )
{ return Mass{ kilograms }; }
constexpr auto operator""_kg( unsigned long long kilograms )
{ return Mass{ (long double)kilograms }; }
constexpr auto operator""_pg( long double picograms )
{ return Mass{ 1e-15 * picograms }; }
constexpr auto operator""_pg( unsigned long long picograms )
{ return Mass{ 1e-15 * (long double)picograms }; }
constexpr auto operator""_ng( long double nanograms )
{ return Mass{ 1e-12 * nanograms }; }
constexpr auto operator""_ng( unsigned long long nanograms )
{ return Mass{ 1e-12 * (long double)nanograms }; }
#ifndef NO_UNICODE_COMPILER
constexpr auto operator""_μg( long double micrograms )
{ return Mass{ 1e-9 * micrograms }; }
constexpr auto operator""_μg( unsigned long long micrograms )
{ return Mass{ 1e-9 * (long double)micrograms }; }
#endif
constexpr auto operator""_mg( long double milligrams )
{ return Mass{ 1e-6 * milligrams }; }
constexpr auto operator""_mg( unsigned long long milligrams )
{ return Mass{ 1e-6 * (long double)milligrams }; }
constexpr auto operator""_g( long double grams )
{ return Mass{ 1e-3 * grams }; }
constexpr auto operator""_g( unsigned long long grams )
{ return Mass{ 1e-3 * (long double)grams }; }
constexpr auto operator""_Mg( long double megagrams )
{ return Mass{ 1e3 * megagrams }; }
constexpr auto operator""_Mg( unsigned long long megagrams )
{ return Mass{ 1e3 * (long double)megagrams }; }
constexpr auto operator""_Gg( long double gigagrams )
{ return Mass{ 1e6 * gigagrams }; }
constexpr auto operator""_Gg( unsigned long long gigagrams )
{ return Mass{ 1e6 * (long double)gigagrams }; }
constexpr auto operator""_Tg( long double teragrams )
{ return Mass{ 1e9 * teragrams }; }
constexpr auto operator""_Tg( unsigned long long teragrams )
{ return Mass{ 1e9 * (long double)teragrams }; }
constexpr auto operator""_Pg( long double petagrams )
{ return Mass{ 1e12 * petagrams }; }
constexpr auto operator""_Pg( unsigned long long petagrams )
{ return Mass{ 1e12 * (long double)petagrams }; }
constexpr auto operator""_Eg( long double exagrams )
{ return Mass{ 1e15 * exagrams }; }
constexpr auto operator""_Eg( unsigned long long exagrams )
{ return Mass{ 1e15 * (long double)exagrams }; }
constexpr auto operator""_Zg( long double zettagrams )
{ return Mass{ 1e18 * zettagrams }; }
constexpr auto operator""_Zg( unsigned long long zettagrams )
{ return Mass{ 1e18 * (long double)zettagrams }; }
constexpr auto operator""_Yg( long double yottagrams )
{ return Mass{ 1e21 * yottagrams }; }
constexpr auto operator""_Yg( unsigned long long yottagrams )
{ return Mass{ 1e21 * (long double)yottagrams }; }
constexpr auto operator""_oz( long double ounces )
{ return Mass{ ounces * kilograms_per_ounce }; }
constexpr auto operator""_oz( unsigned long long ounces )
{ return Mass{ (long double)ounces * kilograms_per_ounce }; }
constexpr auto operator""_lb( long double pounds )
{ return Mass{ pounds * kilograms_per_pound}; }
constexpr auto operator""_lb( unsigned long long pounds )
{ return Mass{ (long double)pounds * kilograms_per_pound }; }
constexpr auto operator""_long_ton( long double long_tons )
{ return Mass{ long_tons * kilograms_per_long_ton }; }
constexpr auto operator""_long_ton( unsigned long long long_tons )
{ return Mass{ (long double)long_tons * kilograms_per_long_ton }; }
constexpr auto operator""_ton( long double tons )
{ return Mass{ tons * kilograms_per_ton }; }
constexpr auto operator""_ton( unsigned long long tons )
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

/**
 * Acceleration in meters per second squared
 */
using Acceleration = base_unit< length_unit_id / time_unit_id / time_unit_id, long double >;

constexpr auto operator""_mps2( long double meters_per_second_squared )
{ return Acceleration{ meters_per_second_squared }; }
constexpr auto operator""_mps2( unsigned long long meters_per_second_squared )
{ return Acceleration{ (long double)meters_per_second_squared }; }

static constexpr Acceleration standard_gravity = Acceleration
{ standard_acceleration_of_gravity /* m/s^2 */ };

constexpr long double meters_per_second_squared( Acceleration a )
{ return a.get_value(); }
enum class acceleration_unit_type: size_t
{ meters_per_second_squared };
constexpr string acceleration_unit_names[] = 
{ "m/s^2" };
constexpr const char* acceleration_unit_long_names[] = 
{ "meters per second-squared" };
constexpr long double acceleration_value( Acceleration a, acceleration_unit_type u )
{ 
    switch( u ) {
    case acceleration_unit_type::meters_per_second_squared:
    default:
        return meters_per_second_squared( a );
    }
}


/**
 * a unit of force in kilogram meters per second squared
 */
using Force = base_unit< mass_unit_id * Acceleration::unit_id, long double >;

constexpr auto operator""_N( long double newtons )
{ return Force{ newtons }; }
constexpr auto operator""_N( unsigned long long newtons )
{ return Force{ (long double)newtons }; }
constexpr auto operator""_lbf( long double pounds_force )
{ return Force{ pounds_force * kilograms_per_pound * standard_acceleration_of_gravity }; }
constexpr auto operator""_lbf( unsigned long long pounds_force )
{ return Force{ (long double)pounds_force * kilograms_per_pound * standard_acceleration_of_gravity }; }

constexpr long double newtons( Force f )
{ return f.get_value(); }
enum class force_unit_type : size_t 
{ newtons = 0, pounds_force };
constexpr string force_unit_names[] = 
{ "N", "lbf" };
constexpr string force_unit_long_names[] =
{ "newtons", "pounds-force" };
constexpr long double force_value( Force f, force_unit_type u )
{
    switch( u ) {
    case force_unit_type::newtons:
    default:
        return newtons( f );
    case force_unit_type::pounds_force:
        return newtons( f ) / kilograms_per_pound / standard_acceleration_of_gravity;
    }
}


/**
 *  a unit of energy in newton meters
 */
using Energy = base_unit< Force::unit_id * Length::unit_id, long double >;

constexpr auto operator""_J( long double joules )
{ return Energy{ joules }; }
constexpr auto operator""_J( unsigned long long joules )
{ return Energy{ (long double)joules }; }

constexpr long double joules( Energy e )
{ return e.get_value(); }
enum class energy_unit_type : size_t 
{ joules = 0 };
constexpr string energy_unit_names[] = 
{ "J" };
constexpr string energy_unit_long_names[] = 
{ "joules" };
constexpr long double energy_value( Energy e, energy_unit_type u )
{
    switch( u ) {
    case energy_unit_type::joules:
    default:
        return joules( e );
    }
}

/** 
 * a unit of power in watts
 */
using Power = base_unit< Energy::unit_id / Time::unit_id, long double >;

constexpr auto operator""_W( long double watts )
{ return Power{ watts }; }
constexpr auto operator""_W( unsigned long long watts )
{ return Power{ (long double)watts }; }

constexpr long double watts( Power p )
{ return p.get_value(); }
enum class power_unit_type : size_t 
{ watts = 0 };
constexpr string power_unit_names[] = 
{ "W" };
constexpr string power_unit_long_names[] =
{ "watts" };
constexpr long double power_value( Power p, power_unit_type u )
{
    switch( u ) {
    case power_unit_type::watts:
    default:
        return watts( p );
    }
}

/**
 * electric potential in volts
 */
using ElectricPotential = base_unit< Power::unit_id / Current::unit_id, long double >;

constexpr auto operator""_V( long double volts )
{ return ElectricPotential{ volts }; }
constexpr auto operator""_V( unsigned long long volts )
{ return ElectricPotential{ (long double)volts }; }

constexpr long double volts( ElectricPotential v )
{ return v.get_value(); }
enum class electric_potential_unit_type : size_t 
{ volts = 0 };
constexpr string electric_potential_unit_names[] = 
{ "V" };
constexpr string electric_potential_unit_long_names[] = 
{ "volts" };
constexpr long double electric_potential_value( ElectricPotential v, 
    electric_potential_unit_type u )
{
    switch( u ) {
    case electric_potential_unit_type::volts:
    default:
        return volts( v );
    }
}

/**
 * electrical resistance ohms
 */
using Resistance = base_unit< ElectricPotential::unit_id / Current::unit_id, long double >;

constexpr auto operator""_Ω( long double ohms )
{ return Resistance{ ohms }; }
constexpr auto operator""_Ω( unsigned long long ohms )
{ return Resistance{ (long double)ohms }; }

constexpr long double ohms( Resistance r )
{ return r.get_value(); }
enum class resistance_unit_type : size_t 
{ ohms = 0 };
constexpr string resistance_unit_names[] =
{ "Ω" };
constexpr string resistance_unit_long_names[] = 
{ "ohms" };
constexpr long double resistance_value( Resistance r, resistance_unit_type u )
{
    switch( u ) {
    case resistance_unit_type::ohms: 
    default:
        return ohms( r );
    }
}

/**
 * electrical inductance henries
 */
using Inductance = base_unit< Resistance::unit_id * Time::unit_id, long double >;

constexpr auto operator""_H( long double henries )
{ return Inductance{ henries }; }
constexpr auto operator""_H( unsigned long long henries )
{ return Inductance{ (long double)henries }; }
constexpr auto operator""_mH( long double millihenries )
{ return Inductance{ millihenries * 1e-3 }; }
constexpr auto operator""_mH( unsigned long long millihenries )
{ return Inductance{ (long double)millihenries * 1e-3 }; }

constexpr long double henries( Inductance c )
{ return c.get_value(); }
enum class inductance_unit_type : size_t 
{ henries = 0, millihenries };
constexpr string inductance_unit_names[] = 
{ "H", "mH" };
constexpr string inductance_unit_long_names[] = 
{ "henries", "millihenries" };

constexpr long double inductance_value( Inductance c, inductance_unit_type u )
{
    switch( u ) {
    case inductance_unit_type::henries: return henries( c );
    case inductance_unit_type::millihenries: return henries( c ) * 1e3;
    }
} 

/**
 * electrical capacitance in farads
 */
using Capacitance = base_unit< Charge::unit_id / ElectricPotential::unit_id, long double >;

constexpr auto operator""_F( long double farads )
{ return Capacitance{ farads }; }
constexpr auto operator""_F( unsigned long long farads )
{ return Capacitance{ (long double)farads }; }
constexpr auto operator""_pF( long double picofarads )
{ return Capacitance{ picofarads * 1e-12 }; }
constexpr auto operator""_pF( unsigned long long picofarads )
{ return Capacitance{ (long double)picofarads * 1e-12 }; }
constexpr auto operator""_nF( long double nanofarads )
{ return Capacitance{ nanofarads * 1e-9 }; }
constexpr auto operator""_nF( unsigned long long nanofarads )
{ return Capacitance{ (long double)nanofarads * 1e-9 }; }
constexpr auto operator""_µF( long double microfarads )
{ return Capacitance{ microfarads * 1e-6 }; }
constexpr auto operator""_µF( unsigned long long microfarads )
{ return Capacitance{ (long double)microfarads * 1e-6 }; }
constexpr auto operator""_mF( long double millifarads )
{ return Capacitance{ millifarads * 1e-3 }; }
constexpr auto operator""_mF( unsigned long long millifarads )
{ return Capacitance{ (long double)millifarads * 1e-3 }; }

constexpr long double farads( Capacitance c )
{ return c.get_value(); }
enum class capacitance_unit_type : size_t 
{ farads = 0, picofarads, nanofarads, microfarads, millifarads };
constexpr string capacitance_unit_names[] = 
{ "F", "pF", "nF", "µF", "mF" };
constexpr string capacitance_unit_long_names[] = 
{ "farads", "picofarads", "nanofarads", "microfarads", "millifarads" };

constexpr long double capacitance_value( Capacitance c, capacitance_unit_type u )
{
    switch( u ) {
    case capacitance_unit_type::farads: return farads( c );
    case capacitance_unit_type::picofarads: return farads( c ) * 1e12;
    case capacitance_unit_type::nanofarads: return farads( c ) * 1e9;
    case capacitance_unit_type::microfarads: return farads( c ) * 1e6;
    case capacitance_unit_type::millifarads: return farads( c ) * 1e3;
    }
} 

/**
 * magnetic flux in webers
 */
using MagneticFlux = base_unit< ElectricPotential::unit_id * Time::unit_id, long double >;

constexpr auto operator""_Wb( long double webers )
{ return MagneticFlux{ webers }; }
constexpr auto operator""_Wb( unsigned long long webers )
{ return MagneticFlux{ (long double)webers }; }

constexpr long double webers( MagneticFlux m )
{ return m.get_value(); }
enum class magnetic_flux_unit_type { webers = 0 };
constexpr string magnetic_flux_unit_names[] = { "wb" };
constexpr string magnetic_flux_unit_long_names[] = { "webers" };
constexpr long double magnetic_flux_value( MagneticFlux m, 
    magnetic_flux_unit_type u )
{
    switch( u ) {
    case magnetic_flux_unit_type::webers:
    default:
        return webers( m );
    }
}

/**
 * magnetic flux density in teslas
 */
using MagneticFluxDensity = base_unit< MagneticFlux::unit_id / Area::unit_id, long double >;

constexpr auto operator""_T( long double teslas )
{ return MagneticFluxDensity{ teslas }; }
constexpr auto operator""_T( unsigned long long teslas )
{ return MagneticFluxDensity{ (long double)teslas }; }

constexpr long double teslas( MagneticFluxDensity m )
{ return m.get_value(); }

enum class magnetic_flux_density_unit_type : size_t
{ teslas = 0 };

constexpr string magnetic_flux_density_unit_names[] = { "T" };
constexpr string magnetic_flux_density_unit_long_names[] = { "teslas" };

constexpr long double magnetic_flux_density_value( MagneticFluxDensity m, 
    magnetic_flux_density_unit_type u )
{
    switch( u ) {
    case magnetic_flux_density_unit_type::teslas:
    default:
        return teslas( m );
    }
}

/**
 * frequency in hertz
 */
using Frequency = base_unit< Scalar::unit_id / Time::unit_id, long double >;

constexpr auto operator""_Hz( long double hertz )
{ return Frequency{ hertz }; }
constexpr auto operator""_Hz( unsigned long long hertz )
{ return Frequency{ (long double)hertz }; }
constexpr auto operator""_THz( long double terahertz )
{ return Frequency{ terahertz * 1e-12 }; }
constexpr auto operator""_THz( unsigned long long terahertz )
{ return Frequency{ (long double)terahertz * 1e-12 }; }
constexpr auto operator""_GHz( long double gigahertz )
{ return Frequency{ gigahertz * 1e-9 }; }
constexpr auto operator""_GHz( unsigned long long gigahertz )
{ return Frequency{ (long double)gigahertz * 1e-9 }; }
constexpr auto operator""_MHz( long double megahertz )
{ return Frequency{ megahertz * 1e-6 }; }
constexpr auto operator""_MHz( unsigned long long megahertz )
{ return Frequency{ (long double)megahertz * 1e-6 }; }
constexpr auto operator""_kHz( long double kilohertz )
{ return Frequency{ kilohertz * 1e-3 }; }
constexpr auto operator""_kHz( unsigned long long kilohertz )
{ return Frequency{ (long double)kilohertz * 1e-3 }; }
constexpr auto operator""_mHz( long double millihertz )
{ return Frequency{ millihertz * 1e3 }; }
constexpr auto operator""_mHz( unsigned long long millihertz )
{ return Frequency{ (long double)millihertz * 1e3 }; }
constexpr auto operator""_µHz( long double microhertz )
{ return Frequency{ microhertz * 1e6 }; }
constexpr auto operator""_µHz( unsigned long long microhertz )
{ return Frequency{ (long double)microhertz * 1e6 }; }
constexpr auto operator""_nHz( long double nanohertz )
{ return Frequency{ nanohertz * 1e9 }; }
constexpr auto operator""_nHz( unsigned long long nanohertz )
{ return Frequency{ (long double)nanohertz * 1e9 }; }
constexpr auto operator""_pHz( long double picohertz )
{ return Frequency{ picohertz * 1e12 }; }
constexpr auto operator""_pHz( unsigned long long picohertz )
{ return Frequency{ (long double)picohertz * 1e12 }; }


/**
 * Luminous intensity in candelas
 */
using LuminousIntensity = base_unit< luminous_intensity_unit_id, long double >;

constexpr auto operator""_cd( long double candelas )
{ return LuminousIntensity{ candelas }; }
constexpr auto operator""_cd( unsigned long long candelas )
{ return LuminousIntensity{ (long double)candelas }; }

constexpr long double candelas( LuminousIntensity lum )
{ return lum.get_value(); }

enum class luminous_intensity_unit_type : size_t 
{ candelas = 0 };

static constexpr string luminous_intensity_unit_names[] = 
{ "cd" };

static constexpr string luminous_intensity_unit_long_names[] = 
{ "candelas" };

constexpr long double luminous_intensity_value( LuminousIntensity lums, 
    luminous_intensity_unit_type u )
{
    switch( u ) {
    case luminous_intensity_unit_type::candelas: 
    default:
        return candelas( lums );
    }
}

/** 
 * pressure in pascals, ie: newtons per meter squared 
 */
using Pressure = base_unit< Force::unit_id / Area::unit_id, long double >;

constexpr auto operator""_Pa( long double pascals )
{ return Pressure{ pascals }; }
constexpr auto operator""_Pa( unsigned long long pascals )
{ return Pressure{ (long double)pascals }; }
constexpr auto operator""_atm( long double atmospheres )
{ return Pressure{ atmospheres / atmospheres_per_pascal }; }
constexpr auto operator""_atm( unsigned long long atmospheres )
{ return Pressure{ (long double)atmospheres / atmospheres_per_pascal }; }
constexpr auto operator""_bar( long double bars )
{ return Pressure{ bars * 1e5 }; }
constexpr auto operator""_bar( unsigned long long bars )
{ return Pressure{ (long double)bars * 1e5 }; }
constexpr auto operator""_mbar( long double millibars )
{ return Pressure{ millibars * 1e2 }; }
constexpr auto operator""_mbar( unsigned long long millibars )
{ return Pressure{ (long double)millibars * 1e2 }; }
constexpr auto operator""_psi( long double pounds_per_square_inch )
{ return Pressure{ pounds_per_square_inch * kilograms_per_pound * 
    standard_acceleration_of_gravity / meters_per_inch / meters_per_inch }; }
constexpr auto operator""_psi( unsigned long long pounds_per_square_inch )
{ return Pressure{ (long double)pounds_per_square_inch * kilograms_per_pound * 
    standard_acceleration_of_gravity / meters_per_inch / meters_per_inch }; }
constexpr auto operator""_mmHg( long double millimeters_of_mercury )
{ return Pressure{ millimeters_of_mercury / density_of_mercury / 
    standard_acceleration_of_gravity * 1e3 }; }
constexpr auto operator""_mmHg( unsigned long long millimeters_of_mercury )
{ return Pressure{ (long double)millimeters_of_mercury / density_of_mercury / 
    standard_acceleration_of_gravity * 1e3 }; }

constexpr long double pascals( Pressure p )
{ return p.get_value(); }

enum class pressure_unit_type : size_t
{ pascals = 0, atmospheres, bars, millibars, pounds_per_square_inch, millimeters_mercury };

static constexpr string pressure_unit_names[] =
{ "Pa", "atm", "bar", "mbar", "psi", "mmHg" };

static constexpr const char* pressure_unit_long_names[] = 
{ "pascals", "atmospheres", "bars", "millibars", "pounds_per_square_inch", "millimeters_mercury" };

constexpr long double pressure_value( Pressure p, pressure_unit_type u )
{
    switch( u ) {
    case pressure_unit_type::pascals: return pascals( p );
    case pressure_unit_type::atmospheres: return pascals( p ) * 
        atmospheres_per_pascal;;
    case pressure_unit_type::bars: return pascals( p ) * 1e-5;
    case pressure_unit_type::millibars: return pascals( p ) * 1e-2;
    case pressure_unit_type::pounds_per_square_inch: return pascals( p ) / 
        kilograms_per_pound / standard_acceleration_of_gravity * 
        meters_per_inch * meters_per_inch;
    case pressure_unit_type::millimeters_mercury: return pascals( p ) *
        density_of_mercury / standard_acceleration_of_gravity * 1e-3;
    }
}

/**
 * Thermodynamic temperature in Kelvin
 */
using Temperature = base_unit< temperature_unit_id, long double >;

constexpr auto operator""_K( long double kelvin )
{ return Temperature{ kelvin }; }
constexpr auto operator""_K( unsigned long long kelvin )
{ return Temperature{ (long double)kelvin }; }
constexpr auto operator""_degC( long double celcius )
{ return Temperature{ celcius + zero_celsius_in_kelvin }; }
constexpr auto operator""_degC( unsigned long long celcius )
{ return Temperature{ (long double)celcius + zero_celsius_in_kelvin }; }
constexpr auto operator""_degF( long double fahrenheit )
{ return Temperature{ (fahrenheit - fahrenheit_at_zero_celsius ) * celsius_per_fahrenheit + zero_celsius_in_kelvin }; }
constexpr auto operator""_degF( unsigned long long fahrenheit )
{ return Temperature{ ((long double)fahrenheit - fahrenheit_at_zero_celsius ) * celsius_per_fahrenheit + zero_celsius_in_kelvin }; }


enum class scalar_unit : size_t
{ none = 0, percent, radians, degrees, arcseconds, steradian, square_degrees, square_arcseconds };

static constexpr string scalar_unit_names[] =
{ "", "%", "rad", "deg", "asec", "sr", "deg2", "asec2" };

static constexpr string scalar_unit_long_names[] =
{ "", "percent", "radians", "degrees", "arcseconds", "steradian", "square degrees", "arcseconds2" };

constexpr long double scalar_value( Scalar scalar, scalar_unit u )
{
    switch( u ) {
    case scalar_unit::none: 
    case scalar_unit::radians:
    case scalar_unit::steradian:
        return scalar.get_value();
    case scalar_unit::percent: 
        return 100. * scalar.get_value();
    case scalar_unit::degrees:
        return scalar.get_value() / radians_per_degree;
    case scalar_unit::arcseconds:
        return scalar.get_value() / radians_per_degree * 3600.;
    case scalar_unit::square_degrees:
        return scalar.get_value() / steradians_per_square_degree;
    case scalar_unit::square_arcseconds:
        return scalar.get_value() / steradians_per_square_degree * 3600. * 3600.;
    }
}

} // namespace units

// formating of units
namespace std {

template< units::unit U >
requires( units::is_square_unit_id( units::unit_traits< U >::unit_id ))
constexpr units::unit_square_root_t< U > sqrt( U u )
{ return units::unit_square_root_t< U >{ std::sqrt( u.get_value()) }; }

/**
 * pow function with templated exponent
 */
template< int Exp, typename T >
constexpr T pow( T arg );

template< int Exp, typename T >
requires( Exp == 0 )
constexpr T pow( T arg )
{ return 1.; }

template< int Exp, typename T >
requires( Exp > 0 )
constexpr T pow( T arg )
{ return ( Exp % 2 ? arg : 1. ) * pow< Exp/2 >( arg * arg ); }

template< int Exp, typename T >
requires( Exp < 0 )
constexpr T pow( T arg )
{ return 1. / pow< -Exp >( arg ); }

template< int Exp, units::unit U >
constexpr units::unit_power_t< Exp, U > pow( U u )
{ return units::unit_power_t< Exp, U >{ pow< Exp >( u.get_value() ) }; }


// overrides for std function objects
template< units::unit_id_type Id, typename T >
struct multiplies< units::base_unit< Id, T >>
{
    template< typename U >
    constexpr auto operator()( units::base_unit< Id, T > const& left, 
        U const& right ) const
    { return left * right; }
};

template< units::unit_id_type uid, units::arithmetic T >
struct formatter< units::base_unit< uid, T >, char >:
    formatter< T >
{
    template< class FormatContext >
    FormatContext::iterator format( units::base_unit< uid, T > u, FormatContext& ctx ) const
    { 
        auto out = formatter< T >::format( u.get_value(), ctx );
        return ranges::copy( units::unit_id_name( uid ), out ).out; 
    }
};
    
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
    units::scalar_unit format_unit = units::scalar_unit::none;
    bool use_long_name = false;

    constexpr void set_unit_name( string name )
    {
        if( name == "")
            return;

        auto i = find( begin( units::scalar_unit_names ), 
            end( units::scalar_unit_names ), name );
        if( i != end(units::scalar_unit_names) )
        {
            format_unit = (units::scalar_unit)distance( 
                begin( units::scalar_unit_names), i );
            use_long_name = false;
        }
        else 
        {
            auto j = find( begin( units::scalar_unit_long_names ), 
                end( units::scalar_unit_long_names ), name );
            
            // this is a consteval function so we can't throw exceptions
            // if ( j == end( units::scalar_unit_long_names ))
            //     throw format_error( "invalid format args for Length " );

            format_unit = (units::scalar_unit)distance( 
                begin( units::scalar_unit_long_names), j );
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
    
    template< class FmtContext >
    FmtContext::iterator format( units::Scalar s, FmtContext& ctx ) const
    { 
        formatter< long double, char >::format( 
            units::scalar_value( s, format_unit ), ctx );

        if( use_long_name )
            return ranges::copy( units::scalar_unit_long_names[ (size_t)format_unit ], 
                ctx.out() ).out;

        return ranges::copy( units::scalar_unit_names[ (size_t)format_unit ], 
            ctx.out() ).out;
    }
};

template<>
struct formatter< units::Cardinal, char > : formatter< unsigned long long, char >
{
    template< class ParseContext >
    ParseContext::iterator parse( ParseContext& ctx )
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