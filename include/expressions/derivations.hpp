#ifndef __DERIVATIONS_HPP__
#define __DERIVATIONS_HPP__

#include "expressions/expression_ops.hpp"

template< template< typename > class D, expression E >
struct Derivation
{ using type = D< E >::type; };

template< template< typename > class D, unit U >
struct Derivation< D, StaticExpression< U >>
{ using type = StaticExpression< U >; };

template< template< typename > class D, unit U >
struct Derivation< D, StaticExpression< U >>
{ using type = StaticExpression< U >; };

template< template< typename > class D, expression... Es >
struct Derivation< D, Sum< Es... >>
{ using type = Sum< Derivation< D, Es >::type... >; };

template< template< typename > class D, expression... Es >
struct Derivation< D, Difference< Es... >>
{ using type = Difference< Derivation< D, Es >::type... >; };

template< template< typename > class D, expression E >
struct Derivation< D, Negation< E >>
{ using type = Negation< Derivation< D, E >::type >; };

// product rule
template< template< typename > class D, expression E, expression... Es >
struct Derivation< D, Product< E, Es... >>
{ using type = Sum< Product< Derivation< D, E >::type, Es... >, 
    Product< E, Derivation< D, Product< Es... >>::type >>; };

// quotient rule
template< template< typename > class D, expression E, expression... Es >
struct Derivation< D, Quotient< E, Es... >>
{ 
    using type = Quotient< Difference< 
        Product< Quotient< Es... >, Derivation< D, E >::type >,
        Product< E, Derivation< D, Quotient< Es... >>::type >>, 
            Power< 2, Quotient< Es... >>>;
};

// power rule
template< template< typename > class D, int Exponent, expression E >
struct Derivation< D, Power< Exponent, E >>
{
};

template< template< typename > class D, expression E >
struct Derivation< D, Inverse< E >>
{
    using type = Negation< Quotient< Deriviation< D, E >::type, Power< 2, E >>>;
};

template< template< typename > class D, expression E >
struct Derivation< D, Sine< E >>
{
    using type = Product< Cosine< E >, Derivation< D, E >::type >;
};

template< template< typename > class D, expression E >
struct Derivation< D, Cosine< E >>
{
    using type = Negation< Product< Sine< E >, Derivation< D, E >::type >>;
};

template< template< typename > class D, expression E >
struct Derivation< D, Tangent< E >>
{
    using type = Quotient< Derivation< D, E >::type, Power< 2, Cosine< E >>>;
};

// TODO: 


#endif // __DERIVATIONS_HPP__