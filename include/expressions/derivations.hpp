#ifndef __DERIVATIONS_HPP__
#define __DERIVATIONS_HPP__

#include "expressions/expression_ops.hpp"

namespace expressions {

template< template< typename > class D, typename ExprT >
struct Derivation : DependsOn< ExprT >
{ 
    using expression_type = ExprT;
    using type = D< ExprT >::type; 

    constexpr type value() const
    { return construct< type >( expr ); }

    expression_type expr;
};

template< typename VarT >
struct Partial;

template< typename T, size_t I >
struct Partial< Variable< T, I >>
{
    template< typename ExprT >
    struct Of
    { using type = Derivation< Of, ExprT >::type; };
};

template< typename T, size_t I >
template< typename U, size_t J >
requires( I == J )
struct Partial< Variable< T, I >>::Of< Variable< U, J >>
{ using type = one_expr< T >; };

template< typename T, size_t I >
template< typename U, size_t J >
requires( I != J )
struct Partial< Variable< T, I >>::Of< Variable< U, J >>
{ using type = zero_expr< T >; };

template< template< typename > class D, typename... Ts >
struct Derivation< D, Sum< Ts... >> : DependsOn< Sum< Ts... >>
{ 
    using type = Sum< typename Derivation< D, Ts >::type... >; 
    using type::result_type;

    Derivation( Sum< Ts... > const& expr ) : DependsOn< Sum< Ts... >>{ expr }
    { }

    
};

template< template< typename > class D, typename T, typename... Ts >
struct Derivation< D, Product< T, Ts... >> : DependsOn< Product< T, Ts... >>
{
    using left_type = T;
    using right_type = Product< Ts... >;

    using type = Sum< Product< left_type, typename Derivation< D, right_type >::type >,
        Product< typename Derivation< D, left_type >::type, right_type >>;
};

template< template< typename > class D, typename ExprT >
struct Invoker< Derivation< D, ExprT >>
{
    using derivation_type = Derivation< D, ExprT >;
    using type = derivation_type::type;
    using result_type = result_t< type >;

    constexpr result_type operator()( derivation_type const& expr, 
        variable_values const& values )
    {  }
};

namespace operators {

template< template< typename > class D, typename T >
D< T >::type d( T expr )
{ return { expr }; }

template< template< typename > class D, typename T, typename... Ts >
auto d( Sum< T, Ts... > const& sum )
{

}

} // namespace operators
} // namespace expressions

#endif