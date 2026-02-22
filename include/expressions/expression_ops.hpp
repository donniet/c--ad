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

// boolean operators
template< expression T, expression... Ts >
struct Conjunction : DependsOn< T, Ts... >
{ 
    using dependent_types = tuple< T, Ts... >;
    using result_type = bool; 

    constexpr Conjunction() = default;
    constexpr Conjunction( T t, Ts... ts ) : DependsOn< T, Ts... >{ t, ts... } { }
};

template< expression T, expression... Ts >
struct Disjunction : DependsOn< T, Ts... >
{ 
    using dependent_types = tuple< T, Ts... >;
    using result_type = bool; 

    constexpr Disjunction() = default;
    constexpr Disjunction( T t, Ts... ts ) : DependsOn< T, Ts... >{ t, ts... } { }
};

template< expression T >
struct Compliment : DependsOn< T >
{ 
    using dependent_types = tuple< T >;
    using result_type = bool; 

    constexpr Compliment() = default;
    constexpr Compliment( T t ) : DependsOn< T >{ t } { }
};

// comparison
template< expression Left, expression Right >
struct Equals : DependsOn< Left, Right >
{ 
    using dependent_types = tuple< Left, Right >;
    using result_type = bool; 

    constexpr Equals() = default;
    constexpr Equals( Left left, Right right ) : DependsOn< Left, Right >{ left, right } { }
};

template< expression Left, expression Right >
struct NotEquals : DependsOn< Left, Right >
{ 
    using dependent_types = tuple< Left, Right >;
    using result_type = bool; 

    constexpr NotEquals() = default;
    constexpr NotEquals( Left left, Right right ) : DependsOn< Left, Right >{ left, right } { }
};

template< expression Left, expression Right >
struct Less : DependsOn< Left, Right >
{ 
    using dependent_types = tuple< Left, Right >;
    using result_type = bool; 

    constexpr Less() = default;
    constexpr Less( Left left, Right right ) : DependsOn< Left, Right >{ left, right } { }
};

template< expression Left, expression Right >
struct LessOrEqual : DependsOn< Left, Right >
{ 
    using dependent_types = tuple< Left, Right >;
    using result_type = bool; 

    constexpr LessOrEqual() = default;
    constexpr LessOrEqual( Left left, Right right ) : DependsOn< Left, Right >{ left, right } { }
};

template< expression Left, expression Right >
struct Greater : DependsOn< Left, Right >
{ 
    using dependent_types = tuple< Left, Right >;
    using result_type = bool; 

    constexpr Greater() = default;
    constexpr Greater( Left left, Right right ) : DependsOn< Left, Right >{ left, right } { }
};

template< expression Left, expression Right >
struct GreaterOrEqual : DependsOn< Left, Right >
{ 
    using dependent_types = tuple< Left, Right >;
    using result_type = bool; 

    constexpr GreaterOrEqual() = default;
    constexpr GreaterOrEqual( Left left, Right right ) : DependsOn< Left, Right >{ left, right } { }
};

// real arithmetic
template< expression E, expression... Es >
struct Sum : DependsOn< E, Es... >
{ 
    using dependent_types = tuple< E, Es... >;
    using result_type = result_t< E >; 

    constexpr Sum() = default;
    constexpr Sum( E e, Es... es ) : DependsOn< E, Es... >{ e, es... } { }
};

template< expression E, expression... Es >
struct Difference : DependsOn< E, Es... >
{ 
    using dependent_types = tuple< E, Es... >;
    using result_type = result_t< E >; 

    constexpr Difference() = default;
    constexpr Difference( E e, Es... es ) : DependsOn< E, Es... >{ e, es... } { }
};

template< expression E >
struct Negation : DependsOn< E >
{ 
    using dependent_types = tuple< E >;
    using result_type = result_t< E >;

    constexpr Negation() = default;
    constexpr Negation( E e ) : DependsOn< E >{ e } { }
};

template< expression E, expression... Es >
struct Product : DependsOn< E, Es... >
{ 
    using dependent_types = tuple< E, Es... >;
    using result_type = unit_product< result_t< E >, result_t< Es >... >;

    constexpr Product() = default;
    constexpr Product( E e, Es... es ) : DependsOn< E, Es... >{ e, es... } { }
};

template< expression E, expression... Es >
struct Quotient : DependsOn< E, Es... >
{ 
    using dependent_types = tuple< E, Es... >;
    using result_type = unit_quotient< result_t< E >, result_t< Es >... >;

    constexpr Quotient() = default;
    constexpr Quotient( E e, Es... es ) : DependsOn< E, Es... >{ e, es... } { }
};

template< expression E >
struct Inverse : DependsOn< E >
{ 
    using dependent_types = tuple< E >;
    using result_type = unit_inverse< result_t< E >>;

    constexpr Inverse() = default;
    constexpr Inverse( E e ) : DependsOn< E >{ e } { }
};

// integer arithmetic
template< expression E >
struct BitwiseNot : DependsOn< E >
{ 
    using dependent_types = tuple< E >;
    using result_type = result_t< E >;

    constexpr BitwiseNot() = default;
    constexpr BitwiseNot( E e ) : DependsOn< E >{ e } { }
};

template< expression E, expression... Es >
struct BitwiseAnd : DependsOn< E, Es... >
{ 
    using dependent_types = tuple< E, Es... >;
    using result_type = result_t< E >;

    constexpr BitwiseAnd() = default;
    constexpr BitwiseAnd( E e, Es... es ) : DependsOn< E, Es... >{ e, es... } { }
};

template< expression E, expression... Es >
struct BitwiseOr : DependsOn< E, Es... >
{ 
    using dependent_types = tuple< E, Es... >;
    using result_type = result_t< E >;

    constexpr BitwiseOr() = default;
    constexpr BitwiseOr( E e, Es... es ) : DependsOn< E, Es... >{ e, es... } { }
};

template< expression E, expression... Es >
struct BitwiseXor : DependsOn< E, Es... >
{ 
    using dependent_types = tuple< E, Es... >;
    using result_type = result_t< E >;

    constexpr BitwiseXor() = default;
    constexpr BitwiseXor( E e, Es... es ) : DependsOn< E, Es... >{ e, es... } { }
};

template< expression Bits, expression Shift >
struct BitshiftLeft : DependsOn< Bits, Shift >
{ 
    using dependent_types = tuple< Bits, Shift >;
    using result_type = result_t< Bits >;

    constexpr Bits bits() { return get_dependent< 0 >( *this ); }
    constexpr Shift shift() { return get_dependent< 1 >( *this ); }

    constexpr BitshiftLeft() = default;
    constexpr BitshiftLeft( Bits bits, Shift shift ) : 
        DependsOn< Bits, Shift >{ bits, shift } { }
};

template< expression Bits, expression Shift >
struct BitshiftRight : DependsOn< Bits, Shift >
{ 
    using dependent_types = tuple< Bits, Shift >;
    using result_type = result_t< Bits >;

    constexpr Bits bits() { return get_dependent< 0 >( *this); }
    constexpr Shift shift() { return get_dependent< 1 >( *this ); }

    constexpr BitshiftRight() = default;
    constexpr BitshiftRight( Bits bits, Shift shift ) : 
        DependsOn< Bits, Shift >{ bits, shift } { }
};

/**
 * Helper methods
 */

// boolean operators
template< typename E, typename... Es >
constexpr Conjunction< E, Es... > conjunction( E e, Es... es )
{ return { e, es... }; }

template< typename E, typename... Es >
constexpr Disjunction< E, Es... > disjunction( E e, Es... es )
{ return { e, es... }; }

template< typename E >
constexpr Compliment< E > compliment( E e )
{ return { e }; }

// comparison
template< typename Left, typename Right >
constexpr Equals< Left, Right > equals( Left left, Right right )
{ return { left, right }; }

template< typename Left, typename Right >
constexpr NotEquals< Left, Right > not_equals( Left left, Right right )
{ return { left, right }; }

template< typename Left, typename Right >
constexpr Less< Left, Right > less( Left left, Right right )
{ return { left, right }; }

template< typename Left, typename Right >
constexpr LessOrEqual< Left, Right > less_or_equal( Left left, Right right )
{ return { left, right }; }

template< typename Left, typename Right >
constexpr Greater< Left, Right > greater( Left left, Right right )
{ return { left, right }; }

template< typename Left, typename Right >
constexpr GreaterOrEqual< Left, Right > greater_or_equal( Left left, Right right )
{ return { left, right }; }

// real arithmetic
template< typename E, typename... Es >
constexpr Sum< E, Es... > sum( E e, Es... es )
{ return { e, es... }; }

template< typename E, typename... Es >
constexpr Difference< E, Es... > difference( E e, Es... es )
{ return { e, es... }; }

template< typename E >
constexpr Negation< E > negation( E e )
{ return { e }; }

template< typename E, typename... Es >
constexpr Product< E, Es... > product( E e, Es... es )
{ return { e, es... }; }

template< typename E, typename... Es >
constexpr Quotient< E, Es... > quotient( E e, Es... es )
{ return { e, es... }; }

template< typename E >
constexpr Inverse< E > inverse( E e )
{ return { e }; }

// integer arithmetic
template< typename E >
constexpr BitwiseNot< E > bitwise_not( E e )
{ return { e }; }

template< typename E, typename... Es >
constexpr BitwiseAnd< E, Es... > bitwise_and( E e, Es... es )
{ return { e, es... }; }

template< typename E, typename... Es >
constexpr BitwiseOr< E, Es... > bitwise_or( E e, Es... es )
{ return { e, es... }; }

template< typename E, typename... Es >
constexpr BitwiseXor< E, Es... > bitwise_xor( E e, Es... es )
{ return { e, es... }; }

template< typename Bits, typename Shift >
constexpr BitshiftLeft< Bits, Shift > bitshift_left( Bits bits, Shift shift )
{ return { bits, shift }; }

template< typename Bits, typename Shift >
constexpr BitshiftRight< Bits, Shift > bitshift_right( Bits bits, Shift shift )
{ return { bits, shift }; }

/**
 * Invokers
 */
template< typename E, typename... Es >
struct Invoker< Conjunction< E, Es... >>
{
    template< size_t... Is >
    auto helper( Conjunction< E, Es... > expr, variable_values const& values, 
        seq< Is... > )
    { return ( ... and invoke( get_dependent< Is >( expr ), values )); }

    auto operator()( Conjunction< E, Es... > expr, variable_values const& values )
    { return helper( expr, values, make_seq< 1 + sizeof...( Es ) >{} ); }
};

template< typename E, typename... Es >
struct Invoker< Disjunction< E, Es... >>
{
    template< size_t... Is >
    auto helper( Disjunction< E, Es... > expr, variable_values const& values, 
        seq< Is... > )
    { return ( ... or invoke( get_dependent< Is >( expr ), values )); }

    auto operator()( Disjunction< E, Es... > expr, variable_values const& values )
    { return helper( expr, values, make_seq< 1 + sizeof...( Es ) >{} ); }
};

template< typename E >
struct Invoker< Compliment< E >>
{
    auto operator()( Compliment< E > expr, variable_values const& values )
    { return not invoke( expr, values ); }
};

template< typename Left, typename Right >
struct Invoker< Equals< Left, Right >>
{
    auto operator()( Equals< Left, Right > expr, variable_values const& values )
    { return invoke( get_dependent< 0 >( expr )) == invoke( get_dependent< 1 >( expr )); }
};

template< typename Left, typename Right >
struct Invoker< NotEquals< Left, Right >>
{
    auto operator()( NotEquals< Left, Right > expr, variable_values const& values )
    { return invoke( get_dependent< 0 >( expr )) != invoke( get_dependent< 1 >( expr )); }
};

template< typename Left, typename Right >
struct Invoker< Less< Left, Right >>
{
    auto operator()( Less< Left, Right > expr, variable_values const& values )
    { return invoke( get_dependent< 0 >( expr )) < invoke( get_dependent< 1 >( expr )); }
};

template< typename Left, typename Right >
struct Invoker< LessOrEqual< Left, Right >>
{
    auto operator()( LessOrEqual< Left, Right > expr, variable_values const& values )
    { return invoke( get_dependent< 0 >( expr )) <= invoke( get_dependent< 1 >( expr )); }
};

template< typename Left, typename Right >
struct Invoker< Greater< Left, Right >>
{
    auto operator()( Greater< Left, Right > expr, variable_values const& values )
    { return invoke( get_dependent< 0 >( expr )) > invoke( get_dependent< 1 >( expr )); }
};

template< typename Left, typename Right >
struct Invoker< GreaterOrEqual< Left, Right >>
{
    auto operator()( GreaterOrEqual< Left, Right > expr, variable_values const& values )
    { return invoke( get_dependent< 0 >( expr )) >= invoke( get_dependent< 1 >( expr )); }
};

template< typename E, typename... Es >
struct Invoker< Sum< E, Es... >>
{
    template< size_t... Is >
    auto helper( Sum< E, Es... > expr, variable_values const& values, seq< Is... > )
    { return ( ... + invoke( get_dependent< Is >( expr ), values )); }

    auto operator()( Sum< E, Es... > expr, variable_values const& values )
    { return helper( expr, values, make_seq< 1 + sizeof...( Es ) >{} ); }
};

template< typename E, typename... Es >
struct Invoker< Difference< E, Es... >>
{
    template< size_t... Is >
    auto helper( Difference< E, Es... > expr, variable_values const& values, seq< Is... > )
    { return ( ... - invoke( get_dependent< Is >( expr ), values )); }

    auto operator()( Difference< E, Es... > expr, variable_values const& values )
    { return helper( expr, values, make_seq< 1 + sizeof...( Es ) >{} ); }
};

template< typename E >
struct Invoker< Negation< E >>
{
    auto operator()( Negation< E > expr, variable_values const& values )
    { return -invoke( expr, values ); }
};

template< typename E, typename... Es >
struct Invoker< Product< E, Es... >>
{
    template< size_t... Is >
    auto helper( Product< E, Es... > expr, variable_values const& values, seq< Is... > )
    { return ( ... * invoke( get_dependent< Is >( expr ), values )); }

    auto operator()( Product< E, Es... > expr, variable_values const& values )
    { return helper( expr, values, make_seq< 1 + sizeof...( Es ) >{} ); }
};

template< typename E, typename... Es >
struct Invoker< Quotient< E, Es... >>
{
    template< size_t... Is >
    auto helper( Quotient< E, Es... > expr, variable_values const& values, seq< Is... > )
    { return ( ... / invoke( get_dependent< Is >( expr ), values )); }

    auto operator()( Quotient< E, Es... > expr, variable_values const& values )
    { return helper( expr, values, make_seq< 1 + sizeof...( Es )>{} ); }
};

template< typename E >
struct Invoker< Inverse< E >>
{
    auto operator()( Inverse< E > expr, variable_values const& values )
    { return result_t< E >{ 1. } / invoke( expr, values ); }
};

// bitwise operations
template< typename E >
struct Invoker< BitwiseNot< E >>
{
    auto operator()( BitwiseNot< E > expr, variable_values const& values )
    { return ~invoke( expr, values ); }
};

template< typename E, typename... Es >
struct Invoker< BitwiseAnd< E, Es... >>
{
    template< size_t... Is >
    auto helper( BitwiseAnd< E, Es... > expr, variable_values const& values )
    { return ( invoke( get_dependent< Is >( expr ), values ) & ... ); }

    auto operator()( BitwiseAnd< E, Es... > expr, variable_values const& values )
    { return helper( expr, values, make_seq< 1 + sizeof...( Es )>{} ); }
};

template< typename E, typename... Es >
struct Invoker< BitwiseOr< E, Es... >>
{
    template< size_t... Is >
    auto helper( BitwiseOr< E, Es... > expr, variable_values const& values )
    { return ( invoke( get_dependent< Is >( expr ), values ) | ... ); }

    auto operator()( BitwiseOr< E, Es... > expr, variable_values const& values )
    { return helper( expr, values, make_seq< 1 + sizeof...( Es )>{} ); }
};

template< typename E, typename... Es >
struct Invoker< BitwiseXor< E, Es... >>
{
    template< size_t... Is >
    auto helper( BitwiseXor< E, Es... > expr, variable_values const& values )
    { return ( invoke( get_dependent< Is >( expr ), values ) ^ ... ); }

    auto operator()( BitwiseXor< E, Es... > expr, variable_values const& values )
    { return helper( expr, values, make_seq< 1 + sizeof...( Es )>{} ); }
};

template< typename Bits, typename Shift >
struct Invoker< BitshiftLeft< Bits, Shift >>
{
    auto operator()( BitshiftLeft< Bits, Shift > expr, variable_values const& values )
    { return invoke( expr.bits() ) << invoke( expr.shift() ); }
};

template< typename Bits, typename Shift >
struct Invoker< BitshiftRight< Bits, Shift >>
{
    auto operator()( BitshiftRight< Bits, Shift > expr, variable_values const& values )
    { return invoke( expr.bits() ) >> invoke( expr.shift() ); }
};

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

} // namespace operators
} // namespace expressions

#endif