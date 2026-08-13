#ifndef __EXPRESSIONS_ARITHMETIC_HPP__
#define __EXPRESSIONS_ARITHMETIC_HPP__

#include "expressions.hpp"
#include "units.hpp"

using std::true_type, std::false_type, std::integral_constant;
using std::is_arithmetic_v;

namespace expressions {

/////////////////////////////////////
/// Math Function Implementation ///
///////////////////////////////////
///
namespace impl {

using units::abs;
using units::sin, units::cos, units::tan;
using units::asin, units::acos, units::atan, units::atan2;
using units::sqrt, units::pow, units::exp, units::log;

} // namespace impl

/////////////////
/// Negation ///
///////////////
///
/// @brief negation expression
/// @tparam T the negated type
///
template< typename T >
struct Negation;

template< >
struct IsCompoundOperation< Negation >: true_type { };

template< typename T >
struct Negation: Compound< Negation< T >>
{ 
    static constexpr auto 
    value( T const& arg )
    { return -arg; }

    using Compound< Negation< T >>::Compound;
};

//static_assert(( Negation< Var< 0, int >>{} | simple_scope( 5 )) == -5 );

////////////
/// Sum ///
//////////
///
template< typename... Args >
struct Sum;

template< >
struct IsCompoundOperation< Sum >: true_type { };

template< typename... Args >
struct Sum: Compound< Sum< Args... >> 
{ 
    static constexpr auto 
    value( Args const&... args )
    { return ( args + ... + 0 ); }

    using Compound< Sum< Args... >>::Compound;
};

///////////////////
/// Difference ///
/////////////////
///
template< typename... Ts >
struct Difference;

template< >
struct IsCompoundOperation< Difference >: true_type { };

template< typename... Ts >
struct Difference: Compound< Difference< Ts... >>
{
    static constexpr auto
    value( Ts const&... ts )
    { return ( ts - ... - 0 ); }

    using Compound< Difference< Ts... >>::Compound;
};

////////////////
/// Product ///
//////////////
///
template< typename... >
struct Product;

template< >
struct IsCompoundOperation< Product >: true_type { };

template< typename... Args >
struct Product: Compound< Product< Args... >>
{
    static constexpr auto
    value( Args const&... args )
    { return ( args * ... * 1 ); }

    using Compound< Product< Args... >>::Compound;
};

/////////////////
/// Quotient ///
///////////////
/// 
template< typename... Args >
struct Quotient;

template< >
struct IsCompoundOperation< Quotient >: true_type { };

template< typename... Args >
struct Quotient: Compound< Quotient< Args... >>
{
    static constexpr auto
    value( Args const&... args )
    { return ( args / ... / 1 ); }

    using Compound< Quotient< Args... >>::Compound;
};

///////////////////
/// SquareRoot ///
/////////////////
///
template< typename T >
struct SquareRoot;

template< >
struct IsCompoundOperation< SquareRoot >: true_type { };

template< typename T >
struct SquareRoot: Compound< SquareRoot< T >>
{ 
    static constexpr auto 
    value( T const& arg )
    { return impl::sqrt( arg ); }

    using Compound< SquareRoot< T >>::Compound;
};

////////////
/// Pow ///
//////////
///
template< typename Base, typename Exp >
struct Pow;

template< >
struct IsCompoundOperation< Pow >: true_type { };

// only powers of constants are allowed for unit types
template< typename Base, int N >
struct Pow< Base, Constant< N >>: Compound< Pow< Base, Constant< N >>>
{
    static constexpr auto
    value( Base const& base, Constant< N > )
    { return impl::pow< N >( base ); }

    using Compound< Pow< Base, Constant< N >>>::Compound;
};

// non-unit expressions reduce to the std::pow function
template< typename Base, typename Ex >
requires( not units::unit< result_t< Base >> )
struct Pow< Base, Ex >: Compound< Pow< Base, Ex >>
{
    static constexpr auto
    value( Base const& base, Ex const& ex )
    { return std::pow( base, ex ); }

    using Compound< Pow< Base, Ex >>::Compound;
};

/////////////
/// Sine ///
///////////
/// 
template< typename T >
struct Sine;

template< >
struct IsCompoundOperation< Sine >: true_type { };

template< typename T >
struct Sine: Compound< Sine< T >>
{ 
    static constexpr auto 
    value( T const& arg )
    { return impl::sin( arg ); }

    using Compound< Sine< T >>::Compound;
};

///////////////
/// Cosine ///
/////////////
///
template< typename T >
struct Cosine;

template< >
struct IsCompoundOperation< Cosine >: true_type { };

template< typename T >
struct Cosine: Compound< Cosine< T >>
{ 
    static constexpr auto 
    value( T const& arg )
    { return impl::cos( arg ); }

    using Compound< Cosine< T >>::Compound;
};

////////////////
/// Tangent ///
//////////////
///
template< typename T >
struct Tangent;

template< >
struct IsCompoundOperation< Tangent >: true_type { };

template< typename T >
struct Tangent: Compound< Tangent< T >>
{ 
    static constexpr auto 
    value( T const& arg )
    { return impl::tan( arg ); }

    using Compound< Tangent< T >>::Compound;
};

////////////////
/// Arcsine ///
//////////////
///
template< typename T >
struct Arcsine;

template< >
struct IsCompoundOperation< Arcsine >: true_type { };

template< typename T >
struct Arcsine: Compound< Arcsine< T >>
{ 
    static constexpr auto 
    value( T const& arg )
    { return impl::asin( arg ); }

    using Compound< Arcsine< T >>::Compound;
};

//////////////////
/// Arccosine ///
////////////////
///
template< typename T >
struct Arccosine;

template< >
struct IsCompoundOperation< Arccosine >: true_type { };

template< typename T >
struct Arccosine: Compound< Arccosine< T >>
{ 
    static constexpr auto 
    value( T const& arg )
    { return impl::acos( arg ); }

    using Compound< Arccosine< T >>::Compound;
};

///////////////////
/// Arctangent ///
/////////////////
///
template< typename T >
struct Arctangent;

template< >
struct IsCompoundOperation< Arctangent >: true_type { };

template< typename T >
struct Arctangent: Compound< Arctangent< T >>
{ 
    static constexpr auto 
    value( T const& arg )
    { return impl::atan( arg ); }

    using Compound< Arctangent< T >>::Compound;
};

////////////////////
/// Arctangent2 ///
//////////////////
///
template< typename T, typename U >
struct Arctangent2;

template< >
struct IsCompoundOperation< Arctangent2 >: true_type { };

template< typename T, typename U >
struct Arctangent2: Compound< Arctangent2< T, U >>
{ 
    static constexpr auto 
    value( T const& num, U const& den )
    { return impl::atan2( num, den ); }

    using Compound< Arctangent2< T, U >>::Compound;
};

////////////
/// Log ///
//////////
///
template< typename T >
struct Log;

template< >
struct IsCompoundOperation< Log >: true_type { };

template< typename T >
struct Log: Compound< Log< T >>
{ 
    static constexpr auto 
    value( T const& arg )
    { return impl::log( arg ); }

    using Compound< Log< T >>::Compound;
};

////////////
/// Exp ///
//////////
///
template< typename T >
struct Exp;

template< >
struct IsCompoundOperation< Exp >: true_type { };

template< typename T >
struct Exp: Compound< Exp< T >>
{ 
    static constexpr auto 
    value( T const& arg )
    { return impl::exp( arg ); }

    using Compound< Exp< T >>::Compound;
};

////////////
/// Abs ///
//////////
///
template< typename T >
struct Abs;

template< >
struct IsCompoundOperation< Abs >: true_type { };

template< typename T >
struct Abs: Compound< Abs< T >>
{
    static constexpr auto
    value( T const& arg )
    { return impl::abs( arg ); }

    using Compound< Abs< T >>::Compound;
};

//////////////////
/// Operators ///
////////////////
///
// negation
template< expression T >
constexpr auto operator -( T const& arg )
{ return Negation< T >{ arg }; }

// addition
template< expression T, expression U >
constexpr auto operator +( T const& left, U const& right )
{ return Sum< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator +( T const& left, U const& right )
{ return Sum< T, StaticValue< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator +( T const& left, U const& right )
{ return Sum< StaticValue< T >, U >{ static_expr( left ), right }; }

// subtraction
template< expression T, expression U >
constexpr auto operator -( T const& left, U const& right )
{ return Difference< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator -( T const& left, U const& right )
{ return Difference< T, StaticValue< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator -( T const& left, U const& right )
{ return Difference< StaticValue< T >, U >{ static_expr( left ), right }; }

// multiplication
template< expression T, expression U >
constexpr auto operator *( T const& left, U const& right )
{ return Product< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator *( T const& left, U const& right )
{ return Product< T, StaticValue< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator *( T const& left, U const& right )
{ return Product< StaticValue< T >, U >{ static_expr( left ), right }; }

// division
template< expression T, expression U >
constexpr auto operator /( T const& left, U const& right )
{ return Quotient< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator /( T const& left, U const& right )
{ return Quotient< T, StaticValue< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator /( T const& left, U const& right )
{ return Quotient< StaticValue< T >, U >{ static_expr( left ), right }; }

// absolute value
template< expression T >
constexpr auto 
abs( T const& arg )
{ return Abs< T >{ arg }; }

// trig functions
template< typename T >
requires( not expression< T > )
constexpr auto sin( T const& arg )
{ return impl::sin( arg ); }

template< typename T >
requires( not expression< T > )
constexpr auto cos( T const& arg )
{ return impl::cos( arg ); }

template< typename T >
requires( not expression< T > )
constexpr auto tan( T const& arg )
{ return impl::tan( arg ); }

template< typename T >
requires( not expression< T > )
constexpr auto asin( T const& arg )
{ return impl::asin( arg ); }

template< typename T >
requires( not expression< T > )
constexpr auto acos( T const& arg )
{ return impl::acos( arg ); }

template< typename T >
requires( not expression< T > )
constexpr auto atan( T const& arg )
{ return impl::atan( arg ); }

template< expression T >
constexpr auto sin( T const& arg )
{ return Sine< T >{ arg }; }

template< expression T >
constexpr auto cos( T const& arg )
{ return Cosine< T >{ arg }; }

template< expression T >
constexpr auto tan( T const& arg )
{ return Tangent< T >{ arg }; }

template< expression T >
constexpr auto asin( T const& arg )
{ return Arcsine< T >{ arg }; }

template< expression T >
constexpr auto acos( T const& arg )
{ return Arccosine< T >{ arg }; }

template< expression T >
constexpr auto atan( T const& arg )
{ return Arctangent< T >{ arg }; }

template< expression T, expression U >
constexpr auto atan2( T const& num, U const& den )
{ return Arctangent2< T, U >{ num, den }; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto atan2( T const& num, U const& den )
{ return Arctangent2< StaticValue< T >, U >{ static_expr( num ), den }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto atan2( T const& num, U const& den )
{ return Arctangent2< T, StaticValue< U >>{ num, static_expr( den ) }; }

template< typename T, typename U >
requires( not expression< U > and not expression< T > )
constexpr auto atan2( T const& num, U const& den )
{ return impl::atan2( num, den ); }

// sqrt
template< expression T >
constexpr auto sqrt( T const& arg )
{ return SquareRoot< T >{ arg }; }

// pow
template< typename T, typename U >
requires( not expression< T > and not expression< U > )
constexpr auto pow( T const& base, U const& ex )
{ return impl::pow( base, ex ); }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto pow( T const& base, U const& ex )
{ return Pow< T, StaticValue< U >>{ base, static_expr( ex )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto pow( T const& base, U const& ex )
{ return Pow< StaticValue< T >, U >{ static_expr( base ), ex }; }

template< expression T, expression U >
constexpr auto pow( T const& base, U const& ex )
{ return Pow< T, U >{ base, ex }; }

// log and exp
template< typename T >
requires( not expression< T > )
constexpr auto log( T const& arg )
{ return impl::log( arg ); }

template< typename T >
requires( not expression< T > )
constexpr auto exp( T const& arg )
{ return impl::exp( arg ); }

template< expression T >
constexpr auto log( T const& arg )
{ return Log< T >{ arg }; }

template< expression T >
constexpr auto exp( T const& arg )
{ return Exp< T >{ arg }; }

} // namespace expressions

#endif
