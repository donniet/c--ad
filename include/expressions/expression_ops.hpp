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

template< typename... Ts >
struct Conjunction : DependsOn< Ts... >
{ 
    using result_type = bool; 

    result_type operator()( result_t< Ts >... ts )
    { return ( ... and ts ); }
};

template< typename... Ts >
struct Disjunction : DependsOn< Ts... >
{ 
    using result_type = bool; 
    result_type operator()( result_t< Ts >... ts )
    { return ( ... or ts ); }
};

template< typename T >
struct Compliment : DependsOn< T >
{ 
    using result_type = bool; 
    result_type operator()( result_t< T > t )
    { return !t; }
};

template< typename Left, typename Right >
struct Equals : DependsOn< Left, Right >
{ 
    using result_type = bool; 
    result_type operator()( result_t< Left > left, result_t< Right > right )
    { return left == right; }
};

template< typename Left, typename Right >
struct NotEquals : DependsOn< Left, Right >
{ 
    using result_type = bool; 
    result_type operator()( result_t< Left > left, result_t< Right > right )
    { return left != right; }
};

template< typename Left, typename Right >
struct Less : DependsOn< Left, Right >
{ 
    using result_type = bool;
    result_type operator()( result_t< Left > left, result_t< Right > right )
    { return left < right; } 
};

template< typename Left, typename Right >
struct LessOrEqual : DependsOn< Left, Right >
{ 
    using result_type = bool; 
    result_type operator()( result_t< Left > left, result_t< Right > right )
    { return left <= right; }
};

template< typename Left, typename Right >
struct Greater : DependsOn< Left, Right >
{ 
    using result_type = bool; 
    result_type operator()( result_t< Left > left, result_t< Right > right )
    { return left > right; }
};

template< typename Left, typename Right >
struct GreaterOrEqual : DependsOn< Left, Right >
{ 
    using result_type = bool; 
    result_type operator()( result_t< Left > left, result_t< Right > right )
    { return left >= right; }
};

// TODO: should the parameters be required to be an expression?
template< typename E, typename... Es >
struct Sum : DependsOn< E, Es... >
{ 
    using result_type = result_t< E >; 
    result_type operator()( result_t< E > first, result_t< Es >... rest )
    { return ( first + ( rest + ... )); }
};

template< typename E, typename... Es >
struct Difference : DependsOn< E, Es... >
{ 
    using result_type = result_t< E >; 
    result_type operator()( result_t< E > first, result_t< Es >... rest )
    { return ( first - ( rest - ... )); }
};

template< typename E >
struct Negation : DependsOn< E >
{ 
    using result_type = result_t< E >;
    result_type operator()( result_t< E > arg )
    { return -arg; } 
};

template< typename E, typename... Es >
struct Product : DependsOn< E, Es... >
{ 
    using result_type = result_t< E >;
    result_type operator()( result_t< E > first, result_t< Es >... rest )
    { return first * ( rest * ... ); } 
};

template< typename E, typename... Es >
struct product_of
{ using type = Product< E, Es... >; };

template< typename E, typename... Es >
requires( static_expr< E > and ( ... and static_expr< Es > ))
struct product_of< E, Es... >
{ using type = decltype( E{} * ( ... * Es{} )); };

template< typename E, typename... Es >
using product_of_t = product_of< E, Es... >::type;

template< typename E, typename... Es >
struct Quotient : DependsOn< E, Es... >
{ 
    using result_type = result_t< E >; 
    result_type operator()( result_t< E > first, result_t< Es >... rest )
    { return ( first / ( rest / ... )); }
};

template< typename E >
struct Inverse : DependsOn< E >
{ 
    using result_type = result_t< E >;
    result_type operator()( result_t< E > arg )
    { return result_type{ 1 } / arg; } 
};

namespace operators {

template< typename LeftT, typename RightT >
requires( not expression_traits< LeftT >::is_static_expression and
    not expression_traits< RightT >::is_static_expression  )
constexpr auto operator *( LeftT left, RightT right )
{ return Product< LeftT, RightT >{ left, right }; }


} // namespace operators

} // namespace expressions

#endif