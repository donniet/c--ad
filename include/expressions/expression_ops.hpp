/**
 * These are operations on expressions.  They are merely the type holders of the 
 * parsed expressions.  The invoker is what executes the operations, so we can
 * remove all the executing methods from these types
 */

#ifndef __EXPRESSION_OPS_HPP__
#define __EXPRESSION_OPS_HPP__

#include "expressions/expression_base.hpp"
#include "units.hpp"

namespace expressions {
namespace detail {
// custom functors
template< typename T = void>
struct inverses
{ static constexpr T operator()( T const& value ) { return T{ 1 } / value; }};

template<>
struct inverses<void>
{ static constexpr auto operator()( auto const& value ) { return 1 / value; }};

template< typename T = void, typename Bits = void >
struct bitshift_left
{ static constexpr T operator()( T const& value, Bits const& bits ) 
    { return value << bits; } };

template< typename T >
struct bitshift_left< T, void >
{ static constexpr T operator()( T const& value, auto const& bits ) 
    { return value << bits; } };

template<>
struct bitshift_left< void, void >
{ static constexpr auto operator()( auto const& value, auto const& bits ) 
    { return value << bits; } };

template< typename T = void, typename Bits = void >
struct bitshift_right
{ static constexpr T operator()( T const& value, Bits const& bits ) 
    { return value >> bits; } };

template< typename T >
struct bitshift_right< T, void >
{ static constexpr T operator()( T const& value, auto const& bits ) 
    { return value >> bits; } };

template<>
struct bitshift_right< void, void >
{ static constexpr auto operator()( auto const& value, auto const& bits ) 
    { return value >> bits; } };

template< typename T = void >
struct sin
{ static constexpr auto operator()( T const& value ) 
    { return std::sin( value ); }};

// default to long double
template<>
struct sin< void >
{ static constexpr auto operator()( long double const& value ) 
    { return std::sinl( value ); }};

template< typename T = void >
struct cos
{ static constexpr auto operator()( T const& value ) 
    { return std::cos( value ); }};

// default to long double
template<>
struct cos< void >
{ static constexpr auto operator()( long double const& value ) 
    { return std::cosl( value ); }};

template< typename T = void >
struct tan
{ static constexpr auto operator()( T const& value ) 
    { return std::tan( value ); }};

// default to long double
template<>
struct tan< void >
{ static constexpr auto operator()( long double const& value ) 
    { return std::tanl( value ); }};

template< typename T = void >
struct asin
{ static constexpr auto operator()( T const& value ) 
    { return std::asin( value ); }};

// default to long double
template<>
struct asin< void >
{ static constexpr auto operator()( long double const& value ) 
    { return std::asinl( value ); }};

template< typename T = void >
struct acos
{ static constexpr auto operator()( T const& value ) 
    { return std::acos( value ); }};

// default to long double
template<>
struct acos< void >
{ static constexpr auto operator()( long double const& value ) 
    { return std::acosl( value ); }};

template< typename T = void >
struct atan
{ static constexpr auto operator()( T const& value ) 
    { return std::atan( value ); }};

// default to long double
template<>
struct atan< void >
{ static constexpr auto operator()( long double const& value ) 
    { return std::atanl( value ); }};

template< typename Y = void, typename X = void >
struct atan2
{ 
    using result_type = decltype( std::atan( Y{} / X{} ));
    static constexpr result_type operator()( Y y, X x ) 
    { 
        if( x == 0 ) return 0; 
        return std::atan( y / x );
    }
};

template< typename Y >
struct atan2< Y, void >
{ 
    using result_type = decltype( std::atan( Y{} ));
    static constexpr result_type operator()( Y y, auto x ) 
    { 
        if( x == 0 ) return 0; 
        return std::atan( y / x );
    }
};

template< >
struct atan2< void, void >
{ 
    using result_type = decltype( std::atanl( 0. ));
    static constexpr result_type operator()( auto y, auto x ) 
    { 
        if( x == 0 ) return 0; 
        return std::atanl( y / x );
    }
};

} // namespace detail

// boolean operators
template< typename... Ts >
struct Conjunction : 
    Associative< std::logical_and<>, Ts... >
{ constexpr Conjunction( Ts const&... ts ) : 
    Associative< std::logical_and< bool >, Ts... >{ ts... } {}};

template< typename... Ts >
struct Disjunction : 
    Associative< std::logical_or<>, Ts... >
{ constexpr Disjunction( Ts const&... ts ) :
    Associative< std::logical_or< bool >, Ts... >{ ts... } {}};

template< typename T >
struct Compliment : 
    Unary< std::logical_not<>, T >
{ constexpr Compliment( T const& expr ) : 
    Unary< std::logical_not< bool >, T >{ expr } {}};

// comparison
template< typename LeftT, typename RightT >
struct Equals : Associative< std::equal_to<>, LeftT, RightT >
{ constexpr Equals( LeftT left, RightT right ) : 
    Associative< std::equal_to< result_t< LeftT >>, LeftT, RightT >{ left, right } {}};

template< typename LeftT, typename RightT >
struct NotEquals : Associative< std::not_equal_to<>, LeftT, RightT >
{ constexpr NotEquals( LeftT left, RightT right ) : 
    Associative< std::not_equal_to< result_t< LeftT >>, LeftT, RightT >{ left, right } {}};

template< typename LeftT, typename RightT >
struct Less : Associative< std::less<>, LeftT, RightT >
{ constexpr Less( LeftT left, RightT right ) : 
    Associative< std::less< result_t< LeftT >>, LeftT, RightT >{ left, right } {}};

template< typename LeftT, typename RightT >
struct LessOrEqual : Associative< std::less_equal<>, LeftT, RightT >
{ constexpr LessOrEqual( LeftT left, RightT right ) : 
    Associative< std::less_equal< result_t< LeftT >>, LeftT, RightT >{ left, right } {}};

template< typename LeftT, typename RightT >
struct Greater : Associative< std::greater<>, LeftT, RightT >
{ constexpr Greater( LeftT left, RightT right ) : 
    Associative< std::greater< result_t< LeftT >>, LeftT, RightT >{ left, right } {}};

template< typename LeftT, typename RightT >
struct GreaterOrEqual : Associative< std::greater_equal<>, LeftT, RightT >
{ constexpr GreaterOrEqual( LeftT left, RightT right ) : 
    Associative< std::greater_equal< result_t< LeftT >>, LeftT, RightT >{ left, right } {}};

// accessor
template< size_t I, typename E >
struct Element;

template< size_t I, typename E >
requires tuple_like< E >
struct Element< I, E > : Expression
{
    using result_type = result_t< tuple_element_t< I, E >>;
    using dependent_types = tuple< E >;

    template< size_t J >
    requires( J == 0 )
    constexpr E get_dependent() 
    { return expr; }

    constexpr result_type eval( variable_values const& values ) const
    { return get< I >( eval( expr, values )); }

    constexpr Element() = default;
    constexpr Element( E const& expr ) : expr{ expr } { }

    E expr;
};

template< size_t I, typename E >
requires( not tuple_like< E > and tuple_like< result_t< E >> )
struct Element< I, E > : Expression
{
    using result_type = tuple_element_t< I, result_t< E >>;
    using dependent_types = tuple< E >;

    template< size_t J >
    requires( J == 0 )
    constexpr E get_dependent() 
    { return expr; }

    constexpr result_type eval( variable_values const& values ) const
    { return get< I >( eval( expr, values )); }

    constexpr Element() = default;
    constexpr Element( E const& expr ) : expr{ expr } { }

    E expr;
};

template< size_t I, typename E >
requires( not tuple_like< E > and not tuple_like< result_t< E >> and 
    indexable< result_t< E >> )
struct Element< I, E > : Expression
{
    using result_type = decltype( result_t< E >{}[I] );
    using dependent_types = tuple< E >;

    template< size_t J >
    requires( J == 0 )
    constexpr E get_dependent() 
    { return expr; }

    constexpr result_type eval( variable_values const& values ) const
    { return eval( expr, values )[ I ]; }

    constexpr Element() = default;
    constexpr Element( E const& expr ) : expr{ expr } { }

    E expr;
};

// real arithmetic
template< typename... Ts >
struct Sum : Associative< std::plus< result_t< Ts... >>, Ts... >
{ constexpr Sum( Ts const&... ts ) : 
    Associative< std::plus< result_t< Ts... >>, Ts... >{ ts... } {}};

template< typename... Ts >
struct Difference : Associative< bool, std::minus< result_t< Ts... >>, Ts... >
{ constexpr Difference( Ts const&... ts ) : 
    Associative< bool, std::minus< result_t< Ts... >>, Ts... >{ ts... } {} };

template< typename T >
struct Negation : Unary< std::negate< result_t< T>>, T >
{ constexpr Negation( T const& expr ) : Unary< std::negate< result_t< T>>, T >{ expr } { }};

template< typename T, typename... Ts >
struct Product : Associative< std::multiplies< result_t< T >>, T, Ts... >
{ constexpr Product( T const& t, Ts const&... ts ) : 
    Associative< std::multiplies< result_t< T >>, T, Ts... >{ t, ts... } { }};

template< typename... Ts >
struct Quotient : Associative< std::divides< result_t< Ts... >>, Ts... >
{ constexpr Quotient( Ts const&... ts ) : 
    Associative< std::divides< result_t< Ts... >>, Ts... >{ ts... } { }};

template< typename T >
struct Inverse : Unary< detail::inverses< T >, T >
{ constexpr Inverse( T const& expr ) : 
    Unary< detail::inverses< T >, T >{ expr } { }};

// integer arithmetic
template< typename T >
struct BitwiseNot : Unary< std::bit_not< result_t< T >>, T >
{ constexpr BitwiseNot( T const& expr ): Unary< std::bit_not< result_t< T >>, T >{ expr } { }};

template< typename... Ts >
struct BitwiseAnd : Associative< std::bit_and< result_t< Ts... >>, Ts... >
{ constexpr BitwiseAnd( Ts const&... ts ) : 
    Associative< std::bit_and< result_t< Ts... >>, Ts... >{ ts... } { }};

template< typename... Ts >
struct BitwiseOr : Associative< std::bit_or< result_t< Ts... >>, Ts... >
{ constexpr BitwiseOr( Ts const&... ts ) : 
    Associative< std::bit_or< result_t< Ts... >>, Ts... >{ ts... } { }};

template< typename... Ts >
struct BitwiseXor : Associative< std::bit_xor< result_t< Ts... >>, Ts... >
{ constexpr BitwiseXor( Ts const&... ts ) : 
    Associative< std::bit_xor< result_t< Ts... >>, Ts... >{ ts... } { }};

template< typename BitsT, typename ShiftT >
struct BitshiftLeft : Associative< detail::bitshift_left< result_t< BitsT >, result_t< ShiftT >>, BitsT, ShiftT >
{ constexpr BitshiftLeft( BitsT const& bits, ShiftT const& shift ) : 
    Associative< detail::bitshift_left< result_t< BitsT >, result_t< ShiftT >>, BitsT, ShiftT >{ bits, shift } { }};

template< typename BitsT, typename ShiftT >
struct BitshiftRight : Associative< detail::bitshift_right< result_t< BitsT >, result_t< ShiftT >>, BitsT, ShiftT >
{ constexpr BitshiftRight( BitsT const& bits, ShiftT const& shift ) : 
    Associative< detail::bitshift_right< result_t< BitsT >, result_t< ShiftT >>, BitsT, ShiftT >{ bits, shift } { }};

// trig functions
template< typename T >
struct Sine : Unary< detail::sin< result_t< T >>, T >
{ constexpr Sine( T const& expr ) : Unary< detail::sin< result_t< T >>, T >{ expr } { }};

template< typename T >
struct Cosine : Unary< detail::cos< result_t< T >>, T >
{ constexpr Cosine( T const& expr ) : Unary< detail::cos< result_t< T >>, T >{ expr } { }};

template< typename T >
struct Tangent : Unary< detail::tan< result_t< T >>, T >
{ constexpr Tangent( T const& expr ) : Unary< detail::tan< result_t< T >>, T >{ expr } { }};

template< typename T >
struct Arcsine : Unary< detail::asin< result_t< T >>, T >
{ constexpr Arcsine( T const& expr ) : Unary< detail::asin< result_t< T >>, T >{ expr } { }};

template< typename T >
struct Arccosine : Unary< detail::acos< result_t< T >>, T >
{ constexpr Arccosine( T const& expr ) : Unary< detail::acos< result_t< T >>, T >{ expr } { }};

template< typename T >
struct Arctangent : Unary< detail::atan< result_t< T >>, T >
{ constexpr Arctangent( T const& expr ) : Unary< detail::atan< result_t< T >>, T >{ expr } { }};

template< typename Y, typename X >
struct Arctangent2 : Associative< detail::atan2< result_t< Y >, result_t< X >>, Y, X >
{ constexpr Arctangent2( Y const& y, X const& x ) : 
     Associative< detail::atan2<>, Y, X >{ x, y } { } };

/**
 * Helper methods
 */

// boolean operators
template< typename... Ts >
constexpr Conjunction< Ts... > conjunction( Ts... ts )
{ return { ts... }; }

template< typename... Ts >
constexpr Disjunction< Ts... > disjunction( Ts... ts )
{ return { ts... }; }

template< typename E >
constexpr Compliment< E > compliment( E e )
{ return { e }; }

// comparison
template< typename LeftT, typename RightT >
constexpr Equals< LeftT, RightT > equals( LeftT left, RightT right )
{ return { left, right }; }

template< typename LeftT, typename RightT >
constexpr NotEquals< LeftT, RightT > not_equals( LeftT left, RightT right )
{ return { left, right }; }

template< typename LeftT, typename RightT >
constexpr Less< LeftT, RightT > less( LeftT left, RightT right )
{ return { left, right }; }

template< typename LeftT, typename RightT >
constexpr LessOrEqual< LeftT, RightT > less_or_equal( LeftT left, RightT right )
{ return { left, right }; }

template< typename LeftT, typename RightT >
constexpr Greater< LeftT, RightT > greater( LeftT left, RightT right )
{ return { left, right }; }

template< typename LeftT, typename RightT >
constexpr GreaterOrEqual< LeftT, RightT > greater_or_equal( LeftT left, RightT right )
{ return { left, right }; }

// real arithmetic
template< typename... Ts >
constexpr Sum< Ts... > sum( Ts... ts )
{ return { ts... }; }

template< typename... Ts >
constexpr Difference< Ts... > difference( Ts... ts )
{ return { ts... }; }

template< typename E >
constexpr Negation< E > negation( E e )
{ return { e }; }

template< typename... Ts >
constexpr Product< Ts... > product( Ts... ts )
{ return { ts... }; }

template< typename... Ts >
constexpr Quotient< Ts... > quotient( Ts... ts )
{ return { ts... }; }

template< typename E >
constexpr Inverse< E > inverse( E e )
{ return { e }; }

// integer arithmetic
template< typename E >
constexpr BitwiseNot< E > bitwise_not( E e )
{ return { e }; }

template< typename... Ts >
constexpr BitwiseAnd< Ts... > bitwise_and( Ts... ts )
{ return { ts... }; }

template< typename... Ts >
constexpr BitwiseOr< Ts... > bitwise_or( Ts... ts )
{ return { ts... }; }

template< typename... Ts >
constexpr BitwiseXor< Ts... > bitwise_xor( Ts... ts )
{ return { ts... }; }

template< typename Bits, typename Shift >
constexpr BitshiftLeft< Bits, Shift > bitshift_left( Bits bits, Shift shift )
{ return { bits, shift }; }

template< typename Bits, typename Shift >
constexpr BitshiftRight< Bits, Shift > bitshift_right( Bits bits, Shift shift )
{ return { bits, shift }; }

// trig functions
template< typename E >
constexpr Sine< E > sine( E arg )
{ return { arg }; }

template< typename E >
constexpr Cosine< E > cosine( E arg )
{ return { arg }; }

template< typename E >
constexpr Tangent< E > tangent( E arg )
{ return { arg }; }

template< typename E >
constexpr Arcsine< E > arcsine( E arg )
{ return { arg }; }

template< typename E >
constexpr Arccosine< E > arccosine( E arg )
{ return { arg }; }

template< typename E >
constexpr Arctangent< E > arctangent( E arg )
{ return { arg }; }

template< typename Num, typename Den >
constexpr Arctangent2< Num, Den > arctangent2( Num num, Den den )
{ return { num, den }; }

// operators
namespace operators {

template< expression LeftT, expression RightT >
constexpr auto operator and( LeftT left, RightT right )
{ return conjunction( left, right ); }

template< unit LeftU, expression RightT >
constexpr auto operator and( LeftU left, RightT right )
{ return conjunction( static_expr( left ), right ); }

template< expression LeftT, unit RightU >
constexpr auto operator and( LeftT left, RightU right )
{ return conjunction( left, static_expr( right )); }

template< expression LeftT, expression RightT >
constexpr auto operator or( LeftT left, RightT right )
{ return disjunction( left, right ); }

template< unit LeftU, expression RightT >
constexpr auto operator or( LeftU left, RightT right )
{ return disjunction( static_expr( left ), right ); }

template< expression LeftT, unit RightU >
constexpr auto operator or( LeftT left, RightU right )
{ return disjunction( left, static_expr( right )); }

template< expression ArgT >
constexpr auto operator not( ArgT arg )
{ return compliment( arg ); }

template< expression LeftT, expression RightT >
constexpr auto operator ==( LeftT left, RightT right )
{ return equals( left, right ); }

template< unit LeftU, expression RightT >
constexpr auto operator ==( LeftU left, RightT right )
{ return equals( static_expr( left ), right ); }

template< expression LeftT, unit RightU >
constexpr auto operator ==( LeftT left, RightU right )
{ return equals( left, static_expr( right )); }

template< expression LeftT, expression RightT >
constexpr auto operator !=( LeftT left, RightT right )
{ return not_equals( left, right ); }

template< unit LeftU, expression RightT >
constexpr auto operator !=( LeftU left, RightT right )
{ return not_equals( static_expr( left ), right ); }

template< expression LeftT, unit RightU >
constexpr auto operator !=( LeftT left, RightU right )
{ return not_equals( left, static_expr( right )); }

template< expression LeftT, expression RightT >
constexpr auto operator <( LeftT left, RightT right )
{ return less( left, right ); }

template< unit LeftU, expression RightT >
constexpr auto operator <( LeftU left, RightT right )
{ return less( static_expr( left ), right ); }

template< expression LeftT, unit RightU >
constexpr auto operator <( LeftT left, RightU right )
{ return less( left, static_expr( right )); }

template< expression LeftT, expression RightT >
constexpr auto operator <=( LeftT left, RightT right )
{ return less_or_equal( left, right ); }

template< unit LeftU, expression RightT >
constexpr auto operator <=( LeftU left, RightT right )
{ return less_or_equal( static_expr( left ), right ); }

template< expression LeftT, unit RightU >
constexpr auto operator <=( LeftT left, RightU right )
{ return less_or_equal( left, static_expr( right )); }

template< expression LeftT, expression RightT >
constexpr auto operator >( LeftT left, RightT right )
{ return greater( left, right ); }

template< unit LeftU, expression RightT >
constexpr auto operator >( LeftU left, RightT right )
{ return greater( static_expr( left ), right ); }

template< expression LeftT, unit RightU >
constexpr auto operator >( LeftT left, RightU right )
{ return greater( left, static_expr( right )); }

template< expression LeftT, expression RightT >
constexpr auto operator >=( LeftT left, RightT right )
{ return greater_or_equal( left, right ); }

template< unit LeftU, expression RightT >
constexpr auto operator >=( LeftU left, RightT right )
{ return greater_or_equal( static_expr( left ), right ); }

template< expression LeftT, unit RightU >
constexpr auto operator >=( LeftT left, RightU right )
{ return greater_or_equal( left, static_expr( right )); }

template< expression LeftT, expression RightT >
constexpr auto operator +( LeftT left, RightT right )
{ return sum( left, right ); }

template< unit LeftU, expression RightT >
constexpr auto operator +( LeftU left, RightT right )
{ return sum( static_expr( left ), right ); }

template< expression LeftT, unit RightU >
constexpr auto operator +( LeftT left, RightU right )
{ return sum( left, static_expr( right )); }

template< expression LeftT, expression RightT >
constexpr auto operator -( LeftT left, RightT right )
{ return differenece( left, right ); }

template< unit LeftU, expression RightT >
constexpr auto operator -( LeftU left, RightT right )
{ return difference( static_expr( left ), right ); }

template< expression LeftT, unit RightU >
constexpr auto operator -( LeftT left, RightU right )
{ return difference( left, static_expr( right )); }

template< expression ArgT >
constexpr auto operator -( ArgT arg )
{ return negation( arg  ); }

template< expression LeftT, expression RightT >
constexpr auto operator *( LeftT left, RightT right )
{ return product( left, right ); }

template< unit LeftU, expression RightT >
constexpr auto operator *( LeftU left, RightT right )
{ return product( static_expr( left ), right ); }

template< expression LeftT, unit RightU >
constexpr auto operator *( LeftT left, RightU right )
{ return product( left, static_expr( right )); }

template< expression LeftT, expression RightT >
constexpr auto operator /( LeftT left, RightT right )
{ return quotient( left, right ); }

template< unit LeftU, expression RightT >
constexpr auto operator /( LeftU left, RightT right )
{ return quotient( static_expr( left ), right ); }

template< expression LeftT, unit RightU >
constexpr auto operator /( LeftT left, RightU right )
{ return quotient( left, static_expr( right )); }

template< expression E >
constexpr auto operator ~( E arg )
{ return bitwise_not( arg ); }

template< expression LeftT, expression RightT >
constexpr auto operator &( LeftT left, RightT right )
{ return bitwise_and( left, right ); }

template< expression LeftT, unit RightT >
constexpr auto operator &( LeftT left, RightT right )
{ return bitwise_and( left, static_expr( right )); }

template< unit LeftT, expression RightT >
constexpr auto operator &( LeftT left, RightT right )
{ return bitwise_and( static_expr( left ), right ); }

template< expression LeftT, expression RightT >
constexpr auto operator |( LeftT left, RightT right )
{ return bitwise_or( left, right ); }

template< expression LeftT, unit RightT >
constexpr auto operator |( LeftT left, RightT right )
{ return bitwise_or( left, static_expr( right )); }

template< unit LeftT, expression RightT >
constexpr auto operator |( LeftT left, RightT right )
{ return bitwise_or( static_expr( left ), right ); }

template< expression LeftT, expression RightT >
constexpr auto operator ^( LeftT left, RightT right )
{ return bitwise_xor( left, right ); }

template< expression LeftT, unit RightT >
constexpr auto operator ^( LeftT left, RightT right )
{ return bitwise_xor( left, static_expr( right )); }

template< unit LeftT, expression RightT >
constexpr auto operator ^( LeftT left, RightT right )
{ return bitwise_xor( static_expr( left ), right ); }

template< expression LeftT, expression RightT >
constexpr auto operator <<( LeftT left, RightT right )
{ return bitshift_left( left, right ); }

template< expression LeftT, unit RightT >
constexpr auto operator <<( LeftT left, RightT right )
{ return bitshift_left( left, static_expr( right )); }

template< unit LeftT, expression RightT >
constexpr auto operator <<( LeftT left, RightT right )
{ return bitshift_left( static_expr( left ), right ); }

template< expression LeftT, expression RightT >
constexpr auto operator >>( LeftT left, RightT right )
{ return bitshift_right( left, right ); }

template< expression LeftT, unit RightT >
constexpr auto operator >>( LeftT left, RightT right )
{ return bitshift_right( left, static_expr( right )); }

template< unit LeftT, expression RightT >
constexpr auto operator >>( LeftT left, RightT right )
{ return bitshift_right( static_expr( left ), right ); }

template< expression E >
constexpr auto sin( E arg )
{ return sine( arg ); }

template< expression E >
constexpr auto cos( E arg )
{ return cosinee( arg ); }

template< expression E >
constexpr auto tan( E arg )
{ return tangent( arg ); }

template< expression E >
constexpr auto asin( E arg )
{ return arcsine( arg ); }

template< expression E >
constexpr auto acos( E arg )
{ return arccosine( arg ); }

template< expression E >
constexpr auto atan( E arg )
{ return arctangent( arg ); }

template< expression Num, expression Den >
constexpr auto atan2( Num num, Den den )
{ return arctangent2( num, den ); }

template< expression Num, unit Den >
constexpr auto atan2( Num num, Den den )
{ return arctangent2( num, static_expr( den )); }

template< unit Num, expression Den >
constexpr auto atan2( Num num, Den den )
{ return arctangent2( static_expr( num ), den ); }

} // namespace operators
} // namespace expressions

#endif