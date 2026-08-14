// unit tests for include/expressions/calculus.hpp

#include "expressions/comparison.hpp"
#include "expressions/calculus.hpp"

#include <print>
#include <assert.h>

using std::println, std::print;
using namespace expressions;

static_assert( closed_expression< Func<expressions::Constant<0>, expressions::Var<0, float>>>,
    "empty sub is closed" );

int main( int ac, char* av[] )
{
    auto d = [&]( auto f, auto x ) constexpr
    { return derive( f, x ); };

    print("TESTING EXPRESSION CALCULUS...");

    Var< 0, float > x;
    Var< 1, float > y;

    // base cases: constant rule
    assert( d( 5, x )( 1 ) == 0 );                // derivative of a raw (non-expression) value is 0
    assert(( d( y, x )( 1 )) == 0 );               // derivative of an unrelated variable is 0

    // variable rule
    assert( d( x, x )( 1 ) == 1 );                 // d(x)/dx == 1
    assert( d( x, x ) == 1 );

    // Negation
    assert( d( -x, x )( 2 ) == -1 );               // d(-x)/dx == -1
    assert( d( -x, x ) == -1 ); // should auto-eval
    assert( d( -x, x )() == -1 );

    // Sum
    assert( d( x + 3.f, x )( 5 ) == 1 );           // constant term drops out
    assert( d( x + y, x )( 1 ) == 1 );             // unrelated term (y) drops out

    // Difference
    assert( d( x - 3.f, x )( 5 ) == 1 );           // d(x-3)/dx == 1
    assert( d( 3.f - x, x )( 5 ) == -1 );          // d(3-x)/dx == -1

    // Product (product rule)
    assert( d( x * 3.f, x )( 2 ) == 3 );           // d(3x)/dx == 3
    assert( d( x * x, x )( 3 ) == 6 );             // d(x^2)/dx == 2x, at x=3 -> 6

    // Quotient (quotient rule)
    assert( d( x / 4.f, x )( 2 ) == 0.25f );       // d(x/4)/dx == 1/4

    // SquareRoot
    // d(sqrt(x))/dx == 1/(2*sqrt(x)); at x=4 -> 0.25
    assert( d( sqrt(x), x )( 4 ) == 0.25f );

    // Pow (only constant-exponent Pow<T,Constant<N>> has a derivative rule)
    assert( d( pow( x, Constant< 3 >{}), x )( 2 ) == 12 );  // d(x^3)/dx == 3x^2, at x=2 -> 12

    // Sine / Cosine
    assert( d( sin(x), x )( 0 ) == 1 );            // d(sin(x))/dx == cos(x), at x=0 -> 1
    assert( d( sin(y), x )( 1 ) == 0 );             // unrelated variable drops out
    assert( d( cos(x), x )( 0 ) == 0 );            // d(cos(x))/dx == -sin(x), at x=0 -> 0

    // Tangent
    assert( d( tan(x), x )( 0 ) == 1 );            // d(tan(x))/dx == 1/cos(x)^2, at x=0 -> 1

    // Arcsine / Arccosine
    assert( d( asin(x), x )( 0 ) == 1 );           // d(asin(x))/dx == 1/sqrt(1-x^2), at x=0 -> 1
    assert( d( acos(x), x )( 0 ) == -1 );          // d(acos(x))/dx == -1/sqrt(1-x^2), at x=0 -> -1

    // Arctangent / Arctangent2 (atan2)
    // d(atan(x))/dx == 1/(1+x^2); at x=0 -> 1
        assert( d( atan(x), x )( 0 ) == 1 );
    assert( d( atan2(x, 2.f), x )( 0 ) == 0.5f );  // atan2(x,2) == atan(x/2); shares the bug above

    // Log / Exp
    assert( d( log(x), x )( 2 ) == 0.5f );         // d(log(x))/dx == 1/|x|, at x=2 -> 0.5
    assert( d( exp(x), x )( 0 ) == 1 );            // d(exp(x))/dx == exp(x), at x=0 -> 1

    // second derivative (nested application of derivative())
    assert( d( d( sin(y), x ), x )( 0 ) == 0 );    // unrelated variable stays 0
    assert( d( d( x * x, x ), x )( 3 ) == 2 );     // d^2(x^2)/dx^2 == 2

    // NOTE: Abs<T>'s derivative is explicitly unimplemented in calculus.hpp
    // (static_assert(false, "piece-wise functions are not implemented yet")), and
    // differentiating a Var with respect to a *different* variable it structurally
    // depends on (second-order/higher-order variables) is likewise unimplemented
    // (static_assert(false, "second-order variable derivative not implemented")).
    // Both are hard compile errors on instantiation, so neither is tested here.
    //
    // NOTE: tuple<Ts...> and Tensor<S,Ts...> have element-wise derivative rules,
    // but aren't separately exercised here -- their correctness follows directly
    // from the scalar rules already tested above, applied element-by-element.

    println( "SUCCESS." );
    return EXIT_SUCCESS;
}
