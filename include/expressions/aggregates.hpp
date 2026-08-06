/// work in progress ///

#ifndef __EXPRESSIONS_AGGREGATES_HPP__
#define __EXPRESSIONS_AGGREGATES_HPP__

#include "expressions/expression.hpp"
#include "expressions/comparison.hpp"

namespace expressions {

////////////////
/// Aggregates
///
// 
 template< typename ExprT >
 struct Minimum: Iterative 
 {
     using expression_type = ExprT;
     using arguments_tuple = tuple< expression_type >;
     using result_type = result_t< ExprT >;
 
     constexpr expression_type expr() const { return _expr; }
 
     constexpr Minimum( expression_type expr ): _expr{ expr } { }
     constexpr Minimum() = default;
     
     expression_type _expr;
 };
 
 template< typename ExprT >
 struct ArgumentMinimum: Iterative
 {
     using expression_type = ExprT;
     using arguments_tuple = tuple< expression_type >;
     using variable_types = free_variables_t< ExprT >;
 
     template< typename TupleT >
     struct ResultHelper;
 
     template< typename... Vars >
     struct ResultHelper< tuple< Vars... >>
     { using type = tuple< result_t< Vars >... >; };
 
     using result_type = ResultHelper< variable_types >::type;
 
     constexpr expression_type expr() const { return _expr; }
 
     constexpr ArgumentMinimum( expression_type expr ): _expr{ expr } { }
     constexpr ArgumentMinimum() = default;
 
     expression_type _expr;
 };
 
 #ifndef NDEBUG
 
 static_assert( ForExpression< Sum< Var< 0, int >, Var< 1, int >>>::
     Is< Sum< int, int >>::value );
 
 #endif // DEBUG
 

 template< typename ExprT >
 constexpr auto min( ExprT const& expr )
 { return Minimum< ExprT >{ expr }; }
 
 template< typename ExprT >
 constexpr auto argmin( ExprT const& expr )
 { return ArgumentMinimum< ExprT >{ expr }; }
 




} // namespace expressions

#endif

