#ifndef __EXPRESSIONS_ARITHMETIC_HPP__
#define __EXPRESSIONS_ARITHMETIC_HPP__

#include "expressions.hpp"
#include "units.hpp"

using std::true_type, std::false_type, std::integral_constant;
using std::is_arithmetic_v, std::is_integral_v;

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

using units::abs;
using units::sin, units::cos, units::tan;
using units::asin, units::acos, units::atan, units::atan2;
using units::sqrt, units::pow, units::exp, units::log;

template< typename T >
concept integral = is_integral_v< T >;

template< typename T >
concept arithmetic = is_arithmetic_v< T >;

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
    { return ( args + ... ); }

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
    { return ( ts - ... ); }

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
    { return ( args * ... ); }

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
    { return ( args / ... ); }

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

template< integral auto N, typename Base >
struct PowN;

template< >
struct IsCompoundOperation< Pow >: true_type { };

template< >
struct IsDiscriminatedOperation< PowN >: true_type { };

/// @brief Pow expression selector based on exponent
template< typename BaseT, typename ExpT >
struct PowerExpr
{
    using type = Pow< BaseT, ExpT >;
    static constexpr type
    value( BaseT const& base, ExpT const& ex )
    { return { base, ex }; }
};

template< typename BaseT, integral auto N >
struct PowerExpr< BaseT, Constant< N >>
{
    using type = PowN< N, BaseT >;
    static constexpr type
    value( BaseT const& base, Constant< N > )
    { return { base }; }
};

template< typename BaseT, typename ExpT >
using power_expr_t = PowerExpr< BaseT, ExpT >::type;

template< typename BaseT, typename ExpT >
constexpr power_expr_t< BaseT, ExpT >
power_expr( BaseT const& base, ExpT const& ex )
{ return PowerExpr< BaseT, ExpT >::value( base, ex ); }

template< integral auto N, typename Base >
struct PowN: Compound< PowN< N, Base >>
{
    static constexpr auto
    value( Base const& base )
    { return pow< N >( base ); }

    using Compound< PowN< N, Base >>::Compound;
};

// only powers of constants are allowed for unit types
//template< typename Base, auto N >
//requires( integral< decltype( N )> )
//struct Pow< Base, Constant< N >>: 
//    PowN< N, Base >
//{
//    // overriding PowN's statics and typedefs
//    using expression_type = Pow< Base, Constant< N >>;
//    using arguments_tuple = tuple< Base, Constant< N >>;
//    static constexpr size_t arguments_size = 2;
//
//    constexpr arguments_tuple
//    args() const
//    { return { PowN< N, Base >::template arg< 0 >(), Constant< N >{} }; }
//
//    template< size_t I >
//    requires( I == 0 )
//    constexpr Base const&
//    arg() const
//    { return PowN< N, Base >::template arg< 0 >(); }
//
//    template< size_t I >
//    requires( I == 1 )
//    constexpr Constant< N >
//    arg() const
//    { return Constant< N >{}; }
//
//    static constexpr auto
//    value( Base const& base, Constant< N > )
//    { return PowN< N, Base >::value( base ); }
//
//    Pow( Base const& base, Constant< N > ):
//        PowN< N, Base >{ base } { };
//    Pow( Pow const& ) = default;
//    Pow( ) = default;
//};

// non-unit expressions reduce to the std::pow function
template< typename Base, typename Ex >
requires( not units::unit< result_t< Base >> and 
    not is_constant_v< Ex > )
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

/////////////////////////
/// Identically Zero ///
///////////////////////
/// 
/// Concept to determine if an expression will always evaluate to zero
///
/// NOTE: this will always likely have gaps since it would depend on things like
/// is_identically_equal
template< typename ExprT >
struct IsIdenticallyZero: false_type { };

template< >
struct IsIdenticallyZero< exact_zero >: true_type { };

template< arithmetic auto N >
struct IsIdenticallyZero< Constant< N >>: integral_constant< bool, N == 0 > { };

// use the bool conversion operator
template< units::unit auto M >
struct IsIdenticallyZero< Constant< M >>: integral_constant< bool, M > { };

template< typename T, typename U >
struct IsIdenticallyZero< Sum< T, U >>: integral_constant< bool,
    IsIdenticallyZero< T >::value and IsIdenticallyZero< U >::value > { };

template< typename T, typename U >
struct IsIdenticallyZero< Difference< T, U >>: integral_constant< bool,
    IsIdenticallyZero< T >::value and IsIdenticallyZero< U >::value > { };

template< typename T, typename U >
struct IsIdenticallyZero< Product< T, U >>: integral_constant< bool,
    IsIdenticallyZero< T >::value or IsIdenticallyZero< U >::value > { };

template< typename T >
struct IsIdenticallyZero< Negation< T >>: IsIdenticallyZero< T > { };

template< typename T, typename U >
struct IsIdenticallyZero< Quotient< T, U >>: IsIdenticallyZero< T > { };

template< typename T >
struct IsIdenticallyZero< SquareRoot< T >>: IsIdenticallyZero< T > { };

template< long N, typename T >
struct IsIdenticallyZero< PowN< N, T >>: IsIdenticallyZero< T > { };

template< typename T, typename U >
struct IsIdenticallyZero< Pow< T, U >>: IsIdenticallyZero< T > { };

template< typename T >
struct IsIdenticallyZero< Sine< T >>: IsIdenticallyZero< T > { };

template< typename T >
struct IsIdenticallyZero< Tangent< T >>: IsIdenticallyZero< T > { };

template< typename T >
struct IsIdenticallyZero< Arcsine< T >>: IsIdenticallyZero< T > { };

template< typename T >
struct IsIdenticallyZero< Arctangent< T >>: IsIdenticallyZero< T > { };

template< typename T >
struct IsIdenticallyZero< Abs< T >>: IsIdenticallyZero< T > { };

template< typename ExprT >
constexpr bool is_identically_zero_v = IsIdenticallyZero< ExprT >::value;

template< typename T >
concept identically_zero = is_identically_zero_v< T >;

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
requires( not expression< T > and not is_constant_v< U > )
constexpr auto pow( T const& base, U const& ex )
{ return Pow< StaticValue< T >, U >{ static_expr( base ), ex }; }

template< expression T, expression U >
requires( not is_constant_v< U > )
constexpr auto 
pow( T const& base, U const& ex )
{ return Pow< T, U >{ base, ex }; }

template< expression T, integral auto N >
constexpr auto 
pow( T const& base, Constant< N > )
{ return PowN< N, T >{ base }; }

template< integral auto N, expression T >
constexpr auto 
pow( T const& base )
{ return PowN< N, T >{ base }; }

// log and exp
template< expression T >
constexpr auto log( T const& arg )
{ return Log< T >{ arg }; }

template< expression T >
constexpr auto exp( T const& arg )
{ return Exp< T >{ arg }; }

} // namespace expressions

#endif
