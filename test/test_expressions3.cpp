#include "expressions.v1.2.hpp"
#include "units.hpp"

#include <print>
#include <format>
#include <cassert>


int main( int ac, char* av[] )
{
    using std::println;

    using namespace expressions;
    using namespace units;

    auto zero = constant_zero;
    auto one = constant_one;

    auto vars = declare_variables(
        var< long double >( "x" ), 
        var< Length      >( "l" ),
        var< Velocity    >( "v" ),
        var< Scalar      >( "a" ));

    auto [ x, l, v, a ] = vars.all();

    auto d_x = differential( x );
    auto d_l = differential( l );
    auto d_v = differential( v );
    auto d_a = differential( a );

    println( "{}", zero );

    auto f = ( 5 + zero + one );

    println( "{}", eval( f ));
    println( "{}", x );
    println( "{}", x * one );
    println( "{}", x + one );
    println( "{}", x - one );
    println( "{}", -x );
    println( "{}", ( x == one ) );
    println( "{}", ( x == one and x == zero ) );
    
    x = 8.l;

    auto g = ( 5 + 3*x - f );
    println( "{}", eval( g ));
    println( std::runtime_format( "g == {}" ), g );

    auto dg = d_x( g );
    println( "{}", dg );
    println( "g() == {}", eval( dg ));

    l = 12_in;

    auto h = ( 1_sqft - l * l ) / 254_mm;
    println( "h({}) == {}", l, h );
    println( "h({}) == {:ft}", 12_in, eval( h ));

    auto dh = d_l( h );
    println( "{}", dh );
    println( "dh({}) == {}", 12_in, eval( dh ));

    auto t1 = make_tuple( 3_mm / 1_s, 5_mm / 1_s, 2_mm / 1_s );
    auto t2 = make_tuple( 3_mm / 1_s, 5_mm / 1_s, v );
    auto s1 = make_tensor< Shape< 3 >>( 3_mm / 1_s, 5_mm / 1_s, 2_mm / 1_s );
    auto s2 = make_tensor< Shape< 3 >>( 3_mm / 1_s, 5_mm / 1_s, v );

    auto eq = ( t1 == t2 );

    v = 2_mm / 1_s;

    assert( eval( eq ));

    v = 2_ft / 1_s;

    assert( not eval( eq ));

    auto m1 = make_tensor< Shape< 2, 2 >>( 
        a, -a,
        a, a );

    a = 1_scalar;

    println( "det(1,-1,1,1) == {}", eval( det( m1 ) ));
    assert( eval( det( m1 ) ) == 2_scalar );
    assert( eval( d_a( det( m1 )) ) == 4_scalar );

    auto grad = gradient( a );

    auto solver = gradient_descent( l );
    solver( h );
    println( "solved: h({}) == {}", eval( l ), eval( h ));


    // auto sol = solve< default_gradient_descent_solver >( h );
    // println( "solved: {}", eval( l, sol ));

    

    // auto a = array_of( zero );
    // auto b = array_of( zero , 1 );

    // println( "{}", eval( element_of< 1 >(b) ));
    // // println( "{}")


    return EXIT_SUCCESS;
}