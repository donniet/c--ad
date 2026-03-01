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

template< template< typename > class D, typename... Ts >
struct Derivation< D, Sum< Ts... >> : DependsOn< Sum< Ts... >>
{ 
    using type = Sum< Derivation< D, Ts >::type... >; 
};

template< template< typename > class D, typename T, typename... Ts >
struct Derivation< D, Product< T, Ts... >> : DependsOn< Product< T, Ts... >>
{
    using left_type = T;
    using right_type = Product< Ts... >;

    using type = Sum< Product< left_type, Derivation< right_type >::type >,
        Product< Derivation< left_type >::type, right_type >>;
};

template< template< typename > class D, typename ExprT >
struct Invoke< Derivation< D, ExprT >>
{
    using derivation_type = Derivation< D, ExprT >;
    using type = derivation_type::type;
    using result_type = result_t< type >;

    constexpr result_type operator()( derivation_type const& expr, 
        variable_values const& values )
    {  }
};

namespace operators {

template< template< typename > clsss D, typename T >
D< T >::type d( T expr )
{ return { expr }; }

template< template< typename > clsss D, typename T, typename... Ts >
auto d( Sum< T, Ts... > const& sum )
{

}

} // namespace operators
} // namespace expressions

#endif