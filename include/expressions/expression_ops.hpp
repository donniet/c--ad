/**
 * These are operations on expressions.  They are merely the type holders of the 
 * parsed expressions.  The invoker is what executes the operations, so we can
 * remove all the executing methods from these types
 */

#ifndef __EXPRESSION_OPS_HPP__
#define __EXPRESSION_OPS_HPP__

#include "expressions/expression_base.hpp"

namespace expressions {

// template< typename Seq, template< size_t > class Index, template< typename... > class Op >
// struct gather_op;

// template< size_t... Is, template< size_t > class Index, template< typename... > class Op >
// struct gather_op< seq< Is... >, Index, Op >
// { using type = Op< Index< Is >::type... >; };

// template< typename Seq, template< size_t > class Index, template< typename... > classs Op >
// using gather_op_t = gather_op< Seq, Index, Op >::type;

template< expression... Ts >
struct Conjunction : DependsOn< Ts... >
{ 
    using result_type = bool; 
};

template< expression... Ts >
struct Disjunction : DependsOn< Ts... >
{ 
    using result_type = bool; 
};

template< expression T >
struct Compliment : DependsOn< T >
{ 
    using result_type = bool; 
};

template< expression Left, expression Right >
struct Equals : DependsOn< Left, Right >
{ 
    using result_type = bool; 
};

template< expression Left, expression Right >
struct NotEquals : DependsOn< Left, Right >
{ 
    using result_type = bool; 
};

template< expression Left, expression Right >
struct Less : DependsOn< Left, Right >
{ 
    using result_type = bool;
};

template< expression Left, expression Right >
struct LessOrEqual : DependsOn< Left, Right >
{ 
    using result_type = bool; 
};

template< expression Left, expression Right >
struct Greater : DependsOn< Left, Right >
{ 
    using result_type = bool; 
};

template< expression Left, expression Right >
struct GreaterOrEqual : DependsOn< Left, Right >
{ 
    using result_type = bool; 
};

// TODO: should the parameters be required to be an expression?
template< expression E, expression... Es >
struct Sum : DependsOn< E, Es... >
{ 
    using dependent_types = tuple< E, Es... >;
    using result_type = result_t< E >; 

    Sum() = default;
    Sum( E e, Es... es ) : DependsOn< E, Es... >{ e, es... } { }
};

template< expression E, expression... Es >
struct Difference : DependsOn< E, Es... >
{ 
    using result_type = result_t< E >; 
};

template< expression E >
struct Negation : DependsOn< E >
{ 
    using result_type = result_t< E >;
};

template< expression E, expression... Es >
struct Product : DependsOn< E, Es... >
{ 
    using result_type = result_t< E >;
};

template< expression E, expression... Es >
struct Quotient : DependsOn< E, Es... >
{ 
    using result_type = result_t< E >; 
};

template< expression E >
struct Inverse : DependsOn< E >
{ 
    using result_type = result_t< E >;
};

/**
 * Helper methods
 */
template< typename E, typename... Es >
Sum< E, Es... > sum( E e, Es... es )
{ return { e, es... }; }

template< typename E, typename... Es >
Product< E, Es... > product( E e, Es... es )
{ return { e, es... }; }

/**
 * Invokers
 */
template< typename E, typename... Es >
struct Invoker< Sum< E, Es... >>
{
    template< size_t... Is >
    auto helper( Sum< E, Es... > expr, variable_values const& values, seq< Is... > )
    { return ( ... + invoke( get_dependent< Is >( expr ), values )); }

    auto operator()( Sum< E, Es... > expr, variable_values const& values )
    { return helper( expr, values, make_seq< 1 + sizeof...( Es ) >{} ); }
};

namespace operators {

template< typename LeftT, typename RightT >
constexpr auto operator and( LeftT left, RightT right )
{ return Conjunction< LeftT, RightT >{ left, right }; }

template< typename LeftT, typename RightT >
constexpr auto operator or( LeftT left, RightT right )
{ return Disjunction< LeftT, RightT >{ left, right }; }

template< typename ArgT >
constexpr auto operator not( ArgT arg )
{ return Compliment< ArgT >{ arg  }; }

template< typename LeftT, typename RightT >
constexpr auto operator ==( LeftT left, RightT right )
{ return Equals< LeftT, RightT >{ left, right }; }

template< typename LeftT, typename RightT >
constexpr auto operator !=( LeftT left, RightT right )
{ return NotEquals< LeftT, RightT >{ left, right }; }

template< typename LeftT, typename RightT >
constexpr auto operator <( LeftT left, RightT right )
{ return Less< LeftT, RightT >{ left, right }; }

template< typename LeftT, typename RightT >
constexpr auto operator <=( LeftT left, RightT right )
{ return LessOrEqual< LeftT, RightT >{ left, right }; }

template< typename LeftT, typename RightT >
constexpr auto operator >( LeftT left, RightT right )
{ return Greater< LeftT, RightT >{ left, right }; }

template< typename LeftT, typename RightT >
constexpr auto operator >=( LeftT left, RightT right )
{ return GreaterOrEqual< LeftT, RightT >{ left, right }; }

template< typename LeftT, typename RightT >
constexpr auto operator +( LeftT left, RightT right )
{ return Sum< LeftT, RightT >{ left, right }; }

template< typename LeftT, typename RightT >
constexpr auto operator -( LeftT left, RightT right )
{ return Difference< LeftT, RightT >{ left, right }; }

template< typename ArgT >
constexpr auto operator -( ArgT arg )
{ return Negation< ArgT >{ arg  }; }

template< typename LeftT, typename RightT >
constexpr auto operator *( LeftT left, RightT right )
{ return Product< LeftT, RightT >{ left, right }; }

template< typename LeftT, typename RightT >
constexpr auto operator /( LeftT left, RightT right )
{ return Quotient< LeftT, RightT >{ left, right }; }

template< typename ArgT >
constexpr auto inverse( ArgT arg )
{ return Quotient< result_t< ArgT >, ArgT >{ result_t< ArgT >{ 1. }, arg }; }

} // namespace operators

} // namespace expressions

#endif