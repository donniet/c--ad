#ifndef __EXPRESSIONS_ARITHMETIC_HPP__
#define __EXPRESSIONS_ARITHMETIC_HPP__

#include "expressions.hpp"

using std::true_type;

namespace expressions {

namespace impl {

using std::sqrt;
using std::sin, std::cos, std::tan;
using std::asin, std::acos, std::atan, std::atan2;
using std::exp, std::log, std::pow;
using std::abs;

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
struct IsExpressionOperation< Negation >: true_type { };

template< typename T >
struct Negation: Arguments< Negation, T >
{ 
    static constexpr auto 
    value( T const& arg )
    { return -arg; }

    using Arguments< Negation, T >::Arguments;
};

//static_assert(( Negation< Var< 0, int >>{} | simple_scope( 5 )) == -5 );

////////////
/// Sum ///
//////////
///
template< typename... Args >
struct Sum;

template< >
struct IsExpressionOperation< Sum >: true_type { };

template< typename... Args >
struct Sum: Arguments< Sum, Args... >
{ 
    static constexpr auto 
    value( Args const&... args )
    { return ( args + ... + 0 ); }

    using Arguments< Sum, Args... >::Arguments;
};

///////////////////
/// Difference ///
/////////////////
///
template< typename... Ts >
struct Difference;

template< >
struct IsExpressionOperation< Difference >: true_type { };

template< typename... Ts >
struct Difference: Arguments< Difference, Ts... >
{
    static constexpr auto
    value( Ts const&... ts )
    { return ( ts - ... - 0 ); }

    using Arguments< Difference, Ts... >::Arguments;
};

////////////////
/// Product ///
//////////////
///
template< typename... >
struct Product;

template< >
struct IsExpressionOperation< Product >: true_type { };

template< typename... Args >
struct Product: Arguments< Product, Args... >
{
    static constexpr auto
    value( Args const&... args )
    { return ( args * ... * 1 ); }

    using Arguments< Product, Args... >::Arguments;
};

/////////////////
/// Quotient ///
///////////////
/// 
template< typename... Args >
struct Quotient;

template< >
struct IsExpressionOperation< Quotient >: true_type { };

template< typename... Args >
struct Quotient: Arguments< Quotient, Args... >
{
    static constexpr auto
    value( Args const&... args )
    { return ( args / ... / 1 ); }

    using Arguments< Quotient, Args... >::Arguments;
};

///////////////////
/// SquareRoot ///
/////////////////
///
template< typename T >
struct SquareRoot;

template< >
struct IsExpressionOperation< SquareRoot >: true_type { };

template< typename T >
struct SquareRoot: Arguments< SquareRoot, T >
{ 
    static constexpr auto 
    value( T const& arg )
    { return impl::sqrt( arg ); }

    using Arguments< SquareRoot, T >::Arguments;
};

////////////
/// Pow ///
//////////
///
template< typename Base, typename Exp >
struct Pow;

template< >
struct IsExpressionOperation< Pow >: true_type { };

template< typename Base, typename Exp >
struct Pow: Arguments< Pow, Base, Exp >
{
    static constexpr auto
    value( Base const& base, Exp const& exp )
    { return impl::pow( base, exp ); }

    using Arguments< Pow, Base, Exp >::Arguments;
};

/////////////
/// Sine ///
///////////
/// 
template< typename T >
struct Sine;

template< >
struct IsExpressionOperation< Sine >: true_type { };

template< typename T >
struct Sine: Arguments< Sine, T >
{ 
    static constexpr auto 
    value( T const& arg )
    { return impl::sin( arg ); }

    using Arguments< Sine, T >::Arguments;
};

///////////////
/// Cosine ///
/////////////
///
template< typename T >
struct Cosine;

template< >
struct IsExpressionOperation< Cosine >: true_type { };

template< typename T >
struct Cosine: Arguments< Cosine, T >
{ 
    static constexpr auto 
    value( T const& arg )
    { return impl::cos( arg ); }

    using Arguments< Cosine, T >::Arguments;
};

////////////////
/// Tangent ///
//////////////
///
template< typename T >
struct Tangent;

template< >
struct IsExpressionOperation< Tangent >: true_type { };

template< typename T >
struct Tangent: Arguments< Tangent, T >
{ 
    static constexpr auto 
    value( T const& arg )
    { return impl::tan( arg ); }

    using Arguments< Tangent, T >::Arguments;
};

////////////////
/// Arcsine ///
//////////////
///
template< typename T >
struct Arcsine;

template< >
struct IsExpressionOperation< Arcsine >: true_type { };

template< typename T >
struct Arcsine: Arguments< Arcsine, T >
{ 
    static constexpr auto 
    value( T const& arg )
    { return impl::asin( arg ); }

    using Arguments< Arcsine, T >::Arguments;
};

//////////////////
/// Arccosine ///
////////////////
///
template< typename T >
struct Arccosine;

template< >
struct IsExpressionOperation< Arccosine >: true_type { };

template< typename T >
struct Arccosine: Arguments< Arccosine, T >
{ 
    static constexpr auto 
    value( T const& arg )
    { return impl::acos( arg ); }

    using Arguments< Arccosine, T >::Arguments;
};

///////////////////
/// Arctangent ///
/////////////////
///
template< typename T >
struct Arctangent;

template< >
struct IsExpressionOperation< Arctangent >: true_type { };

template< typename T >
struct Arctangent: Arguments< Arctangent, T >
{ 
    static constexpr auto 
    value( T const& arg )
    { return impl::atan( arg ); }

    using Arguments< Arctangent, T >::Arguments;
};

////////////////////
/// Arctangent2 ///
//////////////////
///
template< typename T, typename U >
struct Arctangent2;

template< >
struct IsExpressionOperation< Arctangent2 >: true_type { };

template< typename T, typename U >
struct Arctangent2: Arguments< Arctangent2, T, U >
{ 
    static constexpr auto 
    value( T const& num, U const& den )
    { return impl::atan2( num, den ); }

    using Arguments< Arctangent2, T, U >::Arguments;
};

////////////
/// Log ///
//////////
///
template< typename T >
struct Log;

template< >
struct IsExpressionOperation< Log >: true_type { };

template< typename T >
struct Log: Arguments< Log, T >
{ 
    static constexpr auto 
    value( T const& arg )
    { return impl::log( arg ); }

    using Arguments< Log, T >::Arguments;
};

////////////
/// Exp ///
//////////
///
template< typename T >
struct Exp;

template< >
struct IsExpressionOperation< Exp >: true_type { };

template< typename T >
struct Exp: Arguments< Exp, T >
{ 
    static constexpr auto 
    value( T const& arg )
    { return impl::exp( arg ); }

    using Arguments< Exp, T >::Arguments;
};

////////////
/// Abs ///
//////////
///
template< typename T >
struct Abs;

template< >
struct IsExpressionOperation< Abs >: true_type { };

template< typename T >
struct Abs: Arguments< Abs, T >
{
    static constexpr auto
    value( T const& arg )
    { return impl::abs( arg ); }

    using Arguments< Abs, T >::Arguments;
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
