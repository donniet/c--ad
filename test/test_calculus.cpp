// unit tests for include/expressions/calculus.hpp

#include "expressions/comparison.hpp"
#include "expressions/calculus.hpp"
#include "units.hpp"

#include <print>
#include <assert.h>

using std::println, std::print;
using namespace expressions;

static_assert( closed_expression< Func<expressions::Constant<0>, expressions::Var<0, float>>>,
    "empty sub is closed" );

bool test_grad();
bool test_derivatives();

static auto 
d = []( auto f, auto x ) constexpr
{ return derive( f, x ); };


int main( int ac, char* av[] )
{
    println("TESTING EXPRESSION CALCULUS...");

    assert( test_derivatives() );
    assert( test_grad() );

    println( "SUCCESS." );
    return EXIT_SUCCESS;
}

bool test_derivatives()
{
    print( "TESTING DERIVATIVES..." );
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

    println( "SUCCESS." );
    return true;
}

bool test_grad()
{
    print( "TESTING GRADIANT..." );
    Var< 0, Length > x;
    Var< 1, Length > y;

    auto bin1 = pow< 2 >( x - 2_ft ); //, 2_c );
    auto bin2 = pow( y - 3_ft, 2_c );

    static_assert( std::is_same_v< std::remove_cv_t< decltype( bin1 )>,
        PowN< 2, Difference< Var< 0, Length >, StaticValue< Length >>>> );

    Length l0 = 5_ft - 2_ft;
    Length l1 = StaticValue< Length >{ 3_ft } - 2_ft;
    Area c0 = pow< 2 >( 3_ft );
    Area c1 = pow< 2 >( Constant< 3_ft >{} );
    Area c2 = pow< 2 >( StaticValue< Length >{ 3_ft } );
    Area c3 = pow< 2 >( StaticValue< Length >{ 5_ft } - 2_ft )();
    
    assert( c0 == c1 and c1 == c2 and c2 == c3 );

    Area b = bin1( 5_ft );

    auto dbin1_x = derive( bin1, x );
    auto dbin1_y = derive( bin1, y );
    auto dbin2_x = derive( bin2, x );
    auto dbin2_y = derive( bin2, y );

    Length out = dbin1_x( 5_ft );

    println( "d_x((x-2_ft)^2)(5_ft) == {}", out );
    assert( dbin1_x( 5_ft ) == 6_ft );

//    static_assert( std::is_same_v< std::remove_cv_t< decltype( dbin1_x )>,
//        std::remove_cv_t< decltype( 2_c * ( x - 2_ft ))>> );
    println( "SUCCESS." );
    return true;
}


