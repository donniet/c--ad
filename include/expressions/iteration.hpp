#ifndef __EXPRESSIONS_ITERATION_HPP__
#define __EXPRESSIONS_ITERATION_HPP__

#include "expressions/expressions.hpp"

namespace expressions {

template< set_expression... Exprs >
struct Iteration;

template< set_expression... Exprs >
constexpr Iteration< Exprs... >
iteration( Exprs const&... exprs )
{ return { exprs... }; }

template< iteration_expression ExprT, scope ScopeT >
struct Applier< ExprT, ScopeT >
{ 
};

} // namespace expressions


#endif // __EXPRESSIONS_ITERATION_HPP__
