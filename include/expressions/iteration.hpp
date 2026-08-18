#ifndef __EXPRESSIONS_ITERATION_HPP__
#define __EXPRESSIONS_ITERATION_HPP__

#include "expressions/expressions.hpp"

namespace expressions {

// inspiration:
//
// scope( n = 0, m = 0 );
// do_while( n < 100, m += n, n++ ) | on( scope );
//
// while_( n < 100 ).do_( m += n, n++ ) | iterate_on( scope );
//
// assert( scope( n ) == 100 and scope( m ) = 5500 );

/// @brief body set expressions and acts as a builder for iteration expressions
template< typename... SetExprs >
struct IterationBody;

template< typename WhileExpr, typename... SetExprs >
struct Iteration;

template< >
struct IsCompoundOperation< Iteration >: true_type { };

template< typename WhileExpr, typename... SetExprs >
struct Iteration: Compound< Iteration< WhileExpr, SetExprs... >>
{
    using compound_type = Compound< Iteration< WhileExpr, SetExprs... >>;

    static constexpr auto
    value( WhileExpr const& cond, SetExprs const&... exprs )
    { 
        // if our condition is met, 
    }

    using compound_type::args;
    using compound_type::arg;

    static constexpr size_t 
    set_expressions_size = sizeof...( SetExprs );

    constexpr WhileExpr const& 
    condition() const
    { return arg< 0 >(); }

    constexpr tuple< SetExprs... > 
    set_expressions() const
    { 
        auto [ cond, ...set_exprs ] = args();
        return { set_exprs... };
    }

    template< size_t I >
    constexpr SetExprs...[ I ] const&
    { return std::get< I + 1 >( args() ); }

    using Compound< Iteration< SetExprs... >>::Compound;
};

template< typename... SetExprs >
struct IterationBody
{
    template< typename WhileExpr >
    Iteration< WhileExpr, SetExprs... >
    do_while( WhileExpr const& cond )
    { 
        auto [ ...exprs ] = _exprs;
        return { cond, exprs... };
    }

    constexpr IterationBody( SetExprs const&... exprs ): _exprs{ exprs... }
    { }
    constexpr IterationBody( IterationBody const& ) = default;
    constexpr IterationBody( ) = default;

private:
    tuple< SetExprs... > _exprs;
};

template< typename T >
struct IsIterationExpression: false_type { };

template< typename WhileExpr, typename... SetExprs >
struct IsIterationExpression< Iteration< WhileExpr, SetExprs... >>: true_type
{ };

template< typename T >
constexpr bool is_iteration_expression_v = IsIterationExpression< T >::value;

template< typename T >
concept iteration_expression = is_iteration_expression_v< T >;

template< set_expression... Exprs >
constexpr IterationBody< Exprs... >
iteration( Exprs const&... exprs )
{ return { exprs... }; }

} // namespace expressions


#endif // __EXPRESSIONS_ITERATION_HPP__
