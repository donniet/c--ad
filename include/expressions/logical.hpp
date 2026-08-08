#ifndef __EXPRESSIONS_LOGICAL_HPP__
#define __EXPRESSIONS_LOGICAL_HPP__

#include "expressions/expressions.hpp"

namespace expressions {

///////////////////////////////
/// Conditional Expression ///
/////////////////////////////
/// 
/// **WORK IN PROGRESS**
template< typename T >
constexpr T if_( bool cond, T true_value, T false_value = {} )
{ return cond ? true_value : false_value; }

template< typename ConditionT, typename TrueResultT,
    typename FalseResultT >
requires( is_same_v< result_t< ConditionT >, bool > and
    is_same_v< result_t< TrueResultT >, result_t< FalseResultT >> )
struct Conditional;

template< expression ConditionT, typename TrueResultT, typename FalseResultT >
constexpr Conditional< ConditionT, TrueResultT, FalseResultT >
if_( ConditionT condition, TrueResultT true_result, FalseResultT false_result );

template< typename ConditionT, typename TrueResultT,
    typename FalseResultT >
requires( is_same_v< result_t< ConditionT >, bool > and
    is_same_v< result_t< TrueResultT >, result_t< FalseResultT >> )
struct Conditional: Arguments< Conditional, ConditionT, 
    TrueResultT, FalseResultT >
{
    using condition_type = ConditionT;
    using true_result_type = TrueResultT;
    using false_result_type = FalseResultT;

    constexpr condition_type condition() const 
    { return get_argument< 0 >( *this ); }

    constexpr true_result_type true_result() const
    { return get_argument< 1 >( *this ); }

    constexpr false_result_type false_result() const
    { return get_argument< 2 >( *this ); }

    template< typename C, typename T, typename F >
    static constexpr auto value( C cond, T true_case, F false_case )
    { return if_( cond, true_case, false_case ); }

    constexpr Conditional() = default;
    constexpr Conditional( condition_type condition,
        true_result_type true_result, false_result_type false_result ): 
        Arguments< Conditional, ConditionT, TrueResultT, FalseResultT >{ 
            condition, true_result, false_result }
    { }
};

template< expression ConditionT, typename TrueResultT, typename FalseResultT >
constexpr Conditional< ConditionT, TrueResultT, FalseResultT >
if_( ConditionT condition, TrueResultT true_result, FalseResultT false_result )
{ return { condition, true_result, false_result }; }

////////////////////
/// Conjunction ///
//////////////////
///
template< typename... Ts >
struct Conjunction;

template< >
struct IsExpressionOperation< Conjunction >: std::true_type { };

/// @brief logical and expression
/// @tparam Ts... 
template< typename... Ts >
struct Conjunction: Arguments< Conjunction, Ts... >
{
    static constexpr auto
    value( Ts const&... ts )
    { return ( ts and ... and true ); }

    using Arguments< Conjunction, Ts... >::Arguments;
};

////////////////////
/// Disjunction ///
//////////////////
///
template< typename... Ts >
struct Disjunction;

template< >
struct IsExpressionOperation< Disjunction >: std::true_type { };

/// @brief logical or expression
/// @tparam Ts...
template< typename... Ts >
struct Disjunction: Arguments< Disjunction, Ts... >
{
    static constexpr auto
    value( Ts const&... ts )
    { return ( ts or ... or false ); }

    using Arguments< Disjunction, Ts... >::Arguments;
};

///////////////////
/// Compliment ///
/////////////////
///
template< typename T >
struct Compliment;

template< >
struct IsExpressionOperation< Compliment >: std::true_type { };

/// @brief logical not expression
/// @tparam T 
/// @tparam U 
template< typename T >
struct Compliment: Arguments< Compliment, T >
{
    static constexpr auto 
    value( T const& arg )
    { return not arg; }

    using Arguments< Compliment, T >::Arguments;
};

//////////////////////////////////
/// Boolean Expression Traits ///
////////////////////////////////
///
template< typename ExprT >
struct IsBooleanExpression: 
    integral_constant< bool, is_same_v< result_t< ExprT >, bool >> { };

template< typename ExprT >
struct IsConjunction: integral_constant< bool, false > { };

template< typename... Exprs >
struct IsConjunction< Conjunction< Exprs... >>: 
    integral_constant< bool, true > { };

template< typename ExprT >
struct IsDisjunction: integral_constant< bool, false > { };

template< typename... Exprs >
struct IsDisjunction< Disjunction< Exprs... >>: 
    integral_constant< bool, true > { };

template< typename ExprT >
struct IsCompliment: integral_constant< bool, false > { };

template< typename ExprT >
struct IsCompliment< Compliment< ExprT >>: 
    integral_constant< bool, true > { };

template< typename ExprT >
struct IsCanonicalTerminus: integral_constant< bool, 
    not IsConjunction< ExprT >::value and
    not IsDisjunction< ExprT >::value and
    not IsCompliment< ExprT >::value > { };

template< typename ExprT >
struct IsCanonicalComplimentedTerminus: integral_constant< bool, false > { };

template< typename ExprT >
struct IsCanonicalComplimentedTerminus< Compliment< ExprT >>: 
    integral_constant< bool, IsCanonicalTerminus< ExprT >::value > { };

template< typename ExprT >
struct IsCanonicalConjunctiveTerminus: 
    integral_constant< bool, false > { };

template< typename... Exprs >
struct IsCanonicalConjunctiveTerminus< Conjunction< Exprs... >>:
    integral_constant< bool, (
        ( IsCanonicalTerminus< Exprs >::value or 
          IsCanonicalComplimentedTerminus< Exprs >::value ) and ... )> { };

template< typename ExprT >
struct IsCanonicalDisjunctiveTerminus: 
    integral_constant< bool, false > { };

template< typename... Exprs >
struct IsCanonicalDisjunctiveTerminus< Disjunction< Exprs... >>:
    integral_constant< bool, (
        ( IsCanonicalTerminus< Exprs >::value or 
          IsCanonicalComplimentedTerminus< Exprs >::value or 
          IsCanonicalConjunctiveTerminus< Exprs >::value ) and ... )> { };

template< typename ExprT >
struct IsCanonical: integral_constant< bool, 
    IsCanonicalTerminus< ExprT >::value or
    IsCanonicalComplimentedTerminus< ExprT >::value or
    IsCanonicalConjunctiveTerminus< ExprT >::value or
    IsCanonicalDisjunctiveTerminus< ExprT >::value > { };


template< typename ExprT >
constexpr bool is_boolean_expression_v = IsBooleanExpression< ExprT >::value;

template< typename ExprT >
constexpr bool is_conjunction_v = IsBooleanExpression< ExprT >::value;

template< typename ExprT >
constexpr bool is_disjunction_v = IsBooleanExpression< ExprT >::value;

template< typename ExprT >
constexpr bool is_compliment_v = IsBooleanExpression< ExprT >::value;

template< typename ExprT >
constexpr bool is_canonical_v = IsCanonical< ExprT >::value;

// logical operations
template< expression T, expression U >
constexpr auto operator and( T const& left, U const& right )
{ return Conjunction< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator and( T const& left, U const& right )
{ return Conjunction< T, StaticValue< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator and( T const& left, U const& right )
{ return Conjunction< StaticValue< T >, U >{ static_expr( left ), right }; }

template< expression T, expression U >
constexpr auto operator or( T const& left, U const& right )
{ return Disjunction< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator or( T const& left, U const& right )
{ return Disjunction< T, StaticValue< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator or( T const& left, U const& right )
{ return Disjunction< StaticValue< T >, U >{ static_expr( left ), right }; }

template< expression T >
constexpr auto operator not( T const& arg )
{ return Compliment< T >{ arg }; }




} // namespace expressions

#endif
