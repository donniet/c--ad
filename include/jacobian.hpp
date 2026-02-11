#ifndef __JACOBIAN_HPP__
#define __JACOBIAN_HPP__

#include "tensor.hpp"

namespace tensor {
namespace detail {

using namespace expressions;
using namespace expressions::operators;

template< size_t Rows, size_t Cols, typename Vars, typename ExprVector, size_t... Is >
auto jacobian_helper( ExprVector expressions, seq< Is... > )
{
    return make_matrix< Rows, Cols >( 
        partial< tensor_element_t< index< Is / Rows >, ExprVector >, 
            tuple_element_t< Is % Rows, Vars >>{}... );
}
} // namespace detail

template< typename Vars, size_t Rows, typename... Exprs >
auto jacobian( Vector< Rows, Exprs... > expressions )
{
    using namespace expressions;
    using namespace expressions::operators;

    static constexpr size_t Cols = tuple_size_v< Vars >;
    static_assert( Cols == Rows );

    // we use partial derivatives to construct a Rows x Cols matrix where 
    // M[i,j] == d/dx_j f_i where d/dx represents a partial derivative
    return detail::jacobian_helper< Rows, Cols, Vars >( 
        expressions, make_seq< Rows * Cols >{} );
}

} // namespace tensor

namespace solver {

using namespace expressions;
using namespace tensor;

template< typename Expr >
auto solve( Expr expr );

namespace detail {

template< typename ConjEqualsZeroExpr, size_t... Is >
auto solve_conjunction_equalszero_helper( ConjEqualsZeroExpr expr, 
    seq< Is... > )
{
    // TODO: get this or an epsilon to be configurable... in a nice way
    static constexpr size_t maximum_steps = 100;

    // create a vector of the conjunction of operands to the equals zero
    // expressions

    auto vec = make_vector< sizeof...( Is ) >( 
        conjunct< Is >( expr ).quantity()... );

    static_assert( is_same_v< typename decltype( vec )::shape_type, shape< 2 >>);
    static_assert( ConjEqualsZeroExpr::size == 2 );

    // we use sum_t to combine the expressions in the vector into a single
    // expression and collect the dependent variables
    // TODO: use vectors to extract the variables from expressions
    // perhaps it's operations.hpp <-- tensor.hpp <-- expressions.hpp
    using Vars = dependent_variable_tuple< ConjEqualsZeroExpr >;
    auto vars = Vars{};
    auto X = make_vector< tuple_size_v< Vars >>( get< Is >( vars )... );

    // calculate the jaobian of the vector of expressions
    auto jac = jacobian< Vars >( vec );

    static_assert( is_same_v< typename decltype( jac )::shape_type, shape< 2, 2 >> );
    auto inv_jac = inverse( jac );

    for( size_t step = 0; step < maximum_steps; ++step )
    {
        // X = X - inv( J ) * F( X )
        X = difference_of( X(), product_of( inv_jac(), vec() ));
    }


    return X;
}

} // namespace detail

template< typename... Exprs >
auto solve( Conjunction< EqualsZero< Exprs >... > expr )
{ return detail::solve_conjunction_equalszero_helper( expr, 
    make_seq< sizeof...( Exprs )>{} ); }

} // namespace solver


#endif