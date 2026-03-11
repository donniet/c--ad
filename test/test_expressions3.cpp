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

    println( "{}", zero );

    auto f = ( 5 + zero + one );

    println( "{}", eval( f ));

    variable< 0, long double > x{ "x_{}" };
    println( "{}", x );
    println( "{}", x * one );
    println( "{}", x + one );
    println( "{}", x - one );
    println( "{}", -x );
    println( "{}", ( x == one ) );
    println( "{}", ( x == one and x == zero ) );
    

    variable_values values;
    values[ 0 ] = 8.l;

    auto g = ( 5 + 3*x - f );
    println( "depends on g: {}", depends( g ).size() );
    println( "{}", eval( g, values ));
    println( std::runtime_format( "g == {}" ), g );

    auto dg = d< 0 >( g );
    println( "{}", dg );
    println( "depends on dg: {}", depends( dg ).size() );
    println( "g() == {}", eval( dg ));


    variable< 1, Length > l{ "l_{}" };
    values[ 1 ] = 12_in;

    auto h = ( 1_sqft + l * l ) / 254_mm;
    println( "h({}) == {}", l, h );
    println( "h({}) == {:ft}", 12_in, eval( h, values ));

    auto dh = d< 1 >( h );
    println( "{}", dh );
    println( "dh({}) == {:ft}", 12_in, eval( dh, values ));

    variable< 2, Velocity > v2( "v_{}" );

    auto t1 = make_tuple( 3_mm / 1_s, 5_mm / 1_s, 2_mm / 1_s );
    auto t2 = make_tuple( 3_mm / 1_s, 5_mm / 1_s, v2 );
    auto s1 = make_tensor< Shape< 3 >>( 3_mm / 1_s, 5_mm / 1_s, 2_mm / 1_s );
    auto s2 = make_tensor< Shape< 3 >>( 3_mm / 1_s, 5_mm / 1_s, v2 );

    static_assert( dependent_variables_t< 
        tuple< Velocity, Velocity, variable< 2, Velocity >>>::size() == 1 );
    static_assert( dependent_variables_t< Tensor< Shape< 3 >, 
        Velocity, Velocity, variable< 2, Velocity >>>::size() == 1 );

    static_assert( dependent_variables_t< decltype( s1 )>::size() == 0 );
    static_assert( dependent_variables_t< decltype( s2 )>::size() == 1 );
    static_assert( depends( t1 ).size() == 0 );
    static_assert( depends( t2 ).size() == 1 );
    static_assert( depends( s1 ).size() == 0 );
    static_assert( depends( s2 ).size() == 1 );

    auto eq = ( t1 == t2 );
    values[2] = 2_mm / 1_s;
    assert( eval( eq, values ));
    values[2] = 2_ft / 1_s;
    assert( not eval( eq, values ));

    auto a3 = variable< 3, Scalar >{ "a_{}" };

    auto m1 = make_tensor< Shape< 2, 2 >>( 
        a3, -a3,
        a3, a3 );

    // det(m1) == 2 * a3 * a3
    // det'(m1) == 4 * a3
    values[3] = 1_scalar;
    println( "det(1,-1,1,1) == {}", eval( det( m1 ), values ));
    assert( eval( det( m1 ), values ) == 2_scalar );
    assert( eval( d< 3 >( det( m1 )), values ) == 4_scalar );

    // auto a = array_of( zero );
    // auto b = array_of( zero , 1 );

    // println( "{}", eval( element_of< 1 >(b) ));
    // // println( "{}")


    return EXIT_SUCCESS;
}