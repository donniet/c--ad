#ifndef __EXPRESSIONS_ARITHMETIC_HPP__
#define __EXPRESSIONS_ARITHMETIC_HPP__

#include "expressions.hpp"

namespace expressions {

/////////////////
/// Negation ///
///////////////
///
/// @brief negation expression
/// @tparam T the negated type
///
template< typename T >
struct Negation: Arguments< Negation, T >
{ 
    using result_type = decltype( -result_t< T >{} );
    
    template< typename U >
    static constexpr auto value( U const& arg )
    { return -arg; }

    constexpr T arg() const 
    { return get_argument< 0 >( *this ); }

    // derivative of a negation is the negation of the derivative
    //    template< derivation D >
    //    constexpr auto operator |( D const& d ) const
    //    { return -( arg() | d ); }

    constexpr Negation( T arg ): Arguments< Negation, T >{ arg } { } 
    constexpr Negation() = default;
};

//static_assert(( Negation< Var< 0, int >>{} | simple_scope( 5 )) == -5 );

////////////
/// Sum ///
//////////
///
template< typename... Args >
struct Sum;

template< >
struct IsExpressionOperation< Sum >: std::true_type { };

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
struct IsExpressionOperation< Difference >: std::true_type { };

/// @brief difference expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct Difference< T, U >: Arguments< Difference, T, U >
{ 
    using result_type = decltype( result_t< T >{} - result_t< U >{} );

    constexpr T left_arg() const { return get_argument< 0 >( *this ); }
    constexpr U right_arg() const { return get_argument< 1 >( *this ); }

    template< typename V, typename W >
    static constexpr auto value( V const& left, W const& right )
    { return left - right; }

    // derivative of a sum is the sum of the derivative
    //    template< derivation D >
    //    constexpr auto operator |( D const& d ) const
    //    { return ( left_arg() | d ) - ( right_arg() | d ); } 

    constexpr Difference( T left, U right ): 
        Arguments< Difference, T, U >{ left, right } { } 
    constexpr Difference() = default;
};

template< typename... Ts >
requires( is_greater( sizeof...( Ts ), 2 ))
struct Difference< Ts... >: Arguments< Difference, Ts... >
{
    using result_type = decltype(( result_t< Ts >{} - ... ));

    template< size_t I >
    constexpr Ts...[ I ] arg() const { return get_argument< I >( *this ); }

    template< typename... Us >
    static constexpr auto value( Us const&... us )
    { return ( us - ... ); }

    // derivative of a difference is the difference of the derivative
    //    template< derivation D >
    //    constexpr auto operator |( D const& d ) const
    //    {
    //    auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
    //    { return (( arg< Is >() | d ) - ... ); };
    //
    //    return helper( make_seq< sizeof...( Ts )>{} );
    //}

    constexpr Difference( Ts const&... ts ): 
        Arguments< Difference, Ts... >{ ts... } { }
    constexpr Difference() = default;
};

template< typename... >
struct Product;

template< >
struct IsExpressionOperation< Product >: std::true_type { };

template< typename... Args >
struct Product: Arguments< Product, Args... >
{
    static constexpr auto
    value( Args const&... args )
    { return ( args * ... * 1 ); }

    using Arguments< Product, Args... >::Arguments;
};

//
//template< typename... Ts >
//struct Result< Product< Ts... >>
//{ using type = decltype(( typename Result< Ts >::type{} * ... )); };

/// @brief product expression
/// @tparam T 
/// @tparam U 
//template< typename T, typename U >
//struct Product< T, U >: Arguments< Product, T, U > 
//{ 
//    constexpr T left_arg() const { return get_argument< 0 >( *this ); }
//    constexpr U right_arg() const { return get_argument< 1 >( *this );; }
//
//    static constexpr auto 
//    value( T const& left, U const& right ) -> decltype( left * right )
//    { return ( left * right ); }
//
//    using Arguments< Product, T, U >::operator();
//
//    // product rule
//    //    template< derivation D >
//    //    constexpr auto operator |( D const& d ) const
//    //    { return ( left_arg() | d ) * right_arg() + left_arg() * ( right_arg() | d ); } 
//
//    constexpr Product( T left, U right ): 
//        Arguments< Product, T, U >{ left, right } { }
//    constexpr Product() = default;
//};
//
//template< typename T, typename... Ts >
//requires( is_greater( sizeof...( Ts ), 1 ))
//struct Product< T, Ts... >: Arguments< Product, T, Ts... >
//{
//    using result_type = decltype( result_t< T >{} * ( result_t< Ts >{} * ... ));
//    typedef make_seq< sizeof...( Ts )> for_rest;
//
//    template< size_t I >
//    constexpr Ts...[ I ] arg() const 
//    { return get_argument< I >( *this ); }
//
//    constexpr Ts...[ 0 ] first() const 
//    { return arg< 0 >(); }
//
//    constexpr Product< Ts... > rest() const
//    { 
//        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr ->
//            Product< Ts... >
//        { return { arg< Is + 1 >()... }; };
//
//        return helper( for_rest{} );
//    }
//
//    template< typename... Us >
//    static constexpr auto value( Us const&... us )
//    { return ( us * ... ); }
//
//    using Arguments< Product, T, Ts... >::operator();
//
//    //    template< derivation D >
//    //    constexpr auto operator |( D const& d ) const
//    //    { return ( first() | d ) * rest() + first() * ( rest() | d ); }
//
//    constexpr Product( Ts const&... ts ): 
//        Arguments< Product, Ts... >{ ts... } { }
//    constexpr Product() = default;
//};
//
/////////////////
/// Quotient ///
///////////////
/// 
template< typename T, typename U >
struct Quotient;

template< >
struct IsExpressionOperation< Quotient >: std::true_type { };

/// @brief quotient expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct Quotient: Arguments< Quotient, T, U >
{ 
    using result_type = decltype( result_t< T >{} / result_t< U >{} );

    constexpr T numerator_arg() const { return get_argument< 0 >( *this ); }
    constexpr U denominator_arg() const { return get_argument< 1 >( *this ); }

    template< typename V, typename W >
    static constexpr auto value( V const& left, W const& right )
    { return ( left / right ); }

    // quotient rule
    //    template< derivation D >
    //    constexpr auto operator |( D const& d ) const
    //    { return ( numerator_arg() * ( denominator_arg() | d ) - 
    //    ( numerator_arg() | d ) * denominator_arg() ) / 
    //        ( denominator_arg() * denominator_arg() ); }

    constexpr Quotient( T numerator, U denominator ):
        Arguments< Quotient, T, U >{ numerator, denominator } { }
    constexpr Quotient() = default;
};

template< typename T >
struct SquareRoot;

template< >
struct IsExpressionOperation< SquareRoot >: std::true_type { };

/// @brief square root expression
/// @tparam T 
template< typename T >
struct SquareRoot: Arguments< SquareRoot, T >
{
    using result_type = decltype( std::sqrt( result_t< T >{} ));

    constexpr T arg() const { return get_argument< 0 >( *this ); }

    template< typename U >
    static constexpr auto value( U const& arg )
    { return std::sqrt( arg ); }

    //    template< derivation D >
    //    constexpr auto operator |( D const& d ) const
    //    { return 0.5l / sqrt( arg() ) * ( arg() | d ); }

    constexpr SquareRoot( T arg ):  
        Arguments< SquareRoot, T >{ arg } { }
    constexpr SquareRoot() = default;
};

/// @brief integral power expression
/// @tparam T 
/// @tparam Exp 
//template< int Exp >
//struct Power
//{
//    static constexpr int exponent = Exp;
//
//    template< typename T >
//    struct Of: Arguments< Of, T >
//    {
//        using result_type = decltype( std::pow< Exp >( result_t< T >{} ));
//
//        constexpr T arg() const { return get_argument< 0 >( *this ); }
//
//        template< typename U >
//        static constexpr auto value( U const& arg )
//        { return std::pow< Exp >( arg ); }
//
//        template< derivation D >
//        constexpr auto operator |( D const& d ) const
//        { return exponent * pow< Exp - 1 >( arg() ) * ( arg() | d ); }
//
//        constexpr Of( T arg ): Arguments< Of, T >{ arg } {} 
//        constexpr Of() = default;
//    };
//};
//
//template< int Exp, typename T >
//using power_of = Power< Exp >::template Of< T >;

/////////////
/// Sine ///
///////////
/// 
template< typename T >
struct Sine;

template< >
struct IsExpressionOperation< Sine >: std::true_type { };

/// @brief sine expression
/// @tparam T 
template< typename T >
struct Sine: Arguments< Sine, T >
{
    using result_type = decltype( std::sin( result_t< T >{} ));

    constexpr T arg() const { return get_argument< 0 >( *this ); }

    template< typename U >
    static constexpr auto value( U const& arg )
    { return std::sin( arg ); }

    //    template< derivation D >
    //    constexpr auto operator |( D const& d ) const
    //    { return cos( arg() ) * ( arg() | d ); }

    constexpr Sine( T arg ): Arguments< Sine, T >{ arg } { } 
    constexpr Sine() = default;
};

///////////////
/// Cosine ///
/////////////
///
template< typename T >
struct Cosine;

template< >
struct IsExpressionOperation< Cosine >: std::true_type { };

/// @brief cosine expression
/// @tparam T 
template< typename T >
struct Cosine: Arguments< Cosine, T >
{
    using result_type = decltype( std::cos( result_t< T >{} ));

    constexpr T arg() const { return get_argument< 0 >( *this ); }

    template< typename U >
    static constexpr auto value( U const& arg )
    { return std::cos( arg ); }

    //    template< derivation D >
    //    constexpr auto operator |( D const& d ) const
    //    { return -sin( arg() ) * ( arg() | d ); }

    constexpr Cosine( T arg ): Arguments< Cosine, T >{ arg } { } 
    constexpr Cosine() = default;

    T _arg;
};

////////////////
/// Tangent ///
//////////////
///
template< typename T >
struct Tangent;

template< >
struct IsExpressionOperation< Tangent >: std::true_type { };

/// @brief tangent expression
/// @tparam T 
template< typename T >
struct Tangent: Arguments< Tangent, T >
{
    using result_type = decltype( std::tan( result_t< T >{} ));

    constexpr T arg() const { return get_argument< 0 >( *this ); }

    template< typename U >
    static constexpr auto value( U const& arg )
    { return std::tan( arg ); }

    //    template< derivation D >
    //    constexpr auto operator |( D const& d ) const
    //    { return ( arg() | d ) / ( cos( arg() ) * cos( arg() )); } 

    constexpr Tangent( T arg ): Arguments< Tangent, T >{ arg } { } 
    constexpr Tangent() = default;
};

////////////////
/// Arcsine ///
//////////////
///
template< typename T >
struct Arcsine;

template< >
struct IsExpressionOperation< Arcsine >: std::true_type { };

/// @brief arcsine expression
/// @tparam T 
template< typename T >
struct Arcsine: Arguments< Arcsine, T >
{
    using result_type = decltype( std::asin( result_t< T >{} ));

    constexpr T arg() const { return get_argument< 0 >( *this ); }

    template< typename U >
    static constexpr auto value( U const& arg )
    { return std::asin( arg ); }

    //    template< derivation D >
    //    constexpr auto operator |( D const& d ) const
    //    { return ( arg() | d ) / sqrt( 1l - pow< 2 >( arg() )); }

    constexpr Arcsine( T arg ): Arguments< Arcsine, T >{ arg } { } 
    constexpr Arcsine() = default;
};

//////////////////
/// Arccosine ///
////////////////
///
template< typename T >
struct Arccosine;

template< >
struct IsExpressionOperation< Arccosine >: std::true_type { };

/// @brief arccosine expression
/// @tparam T 
template< typename T >
struct Arccosine: Arguments< Arccosine, T >
{
    using result_type = decltype( std::acos( result_t< T >{} ));

    constexpr T arg() const { return get_argument< 0 >( *this ); }

    template< typename U >
    static constexpr auto value( U const& arg )
    { return std::acos( arg ); }

    //template< derivation D >
    //constexpr auto operator |( D const& d ) const 
    //{ return -( arg() | d ) / sqrt( 1l - pow< 2 >( arg() )); }

    constexpr Arccosine( T arg ): Arguments< Arccosine, T >{ arg } { } 
    constexpr Arccosine() = default;
};

///////////////////
/// Arctangent ///
/////////////////
///
template< typename T >
struct Arctangent;

template< >
struct IsExpressionOperation< Arctangent >: std::true_type { };

/// @brief sine expression
/// @tparam T 
template< typename T >
struct Arctangent: Arguments< Arctangent, T >
{
    using result_type = decltype( std::atan( result_t< T >{} ));

    constexpr T arg() const { return get_argument< 0 >( *this ); }

    template< typename U >
    static constexpr auto value( U const& arg )
    { return std::atan( arg ); }

    //    template< derivation D >
    //    constexpr auto operator |( D const& d ) const
    //    { return ( arg() | d ) / ( 1l + pow< 2 >( arg() )); }

    constexpr Arctangent( T arg ): Arguments< Arctangent, T >{ arg } { } 
    constexpr Arctangent() = default;
};

////////////////////
/// Arctangent2 ///
//////////////////
///
template< typename T, typename U >
struct Arctangent2;

template< >
struct IsExpressionOperation< Arctangent2 >: std::true_type { };

/// @brief arctangent of a slope expression
/// @tparam T rise type
/// @tparam U run type
template< typename T, typename U >
struct Arctangent2: Arguments< Arctangent2, T, U >
{ 
    // we use the result_type of a fraction here to factor units properly
    // this assumes that std::atan2 doesn't change the unit. hopefully it stays
    // true that trig functions operate only on scalars and this won't be an 
    // issue.  
    using result_type = decltype( result_t< T >{} / result_t< U >{} );

    constexpr T numerator_arg() const { return get_argument< 0 >( *this ); }
    constexpr U denominator_arg() const { return get_argument< 1 >( *this ); }

    // TODO: write an eval for std::atan2 that handles units properly
    template< typename V, typename W >
    static constexpr auto value( V const& num, W const& den );

    //template< derivation D >
    //constexpr auto operator |( D const& d ) const;

    constexpr Arctangent2( T numerator, U denominator ):
        Arguments< Arctangent2, T, U >{ numerator, denominator } { }
    constexpr Arctangent2() = default;
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

// sqrt
template< expression T >
constexpr auto sqrt( T const& arg )
{ return SquareRoot< T >{ arg }; }

// pow
//template< int Exp, expression T >
//constexpr auto pow( T const& arg )
//{ return power_of< Exp, T >{ arg }; }

} // namespace expressions

/// TODO: remove this!
namespace std {

template< expressions::expression ExprT >
constexpr auto sqrt( ExprT const& expr )
{ return expressions::sqrt( expr ); }


} // namespace std 


#endif
