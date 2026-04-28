
#include "units.hpp"
#include "expressions.hpp"

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
        var< long double >( "y" ),
        var< Length      >( "l" ),
        var< Velocity    >( "v" ),
        var< Scalar      >( "a" ),
        var< Length      >( "w" ),
        var< Length      >( "z" ));

    auto [ x, y, l, v, a, z, w ] = vars.all();

    auto d_x = differential( x );
    auto d_y = differential( y );
    auto d_l = differential( l );
    auto d_v = differential( v );
    auto d_a = differential( a );
    auto d_z = differential( z );
    auto d_w = differential( w );

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
    assert( eval( s1 == s2 ));

    v = 2_ft / 1_s;

    assert( not eval( eq ));
    assert( eval( s1 != s2 ));

    auto m1 = make_tensor< Shape< 2, 2 >>( 
        a, -a,
        a, a );

    a = 1_scalar;

    println( "det(1,-1,1,1) == {}", eval( det( m1 ) ));
    assert( eval( det( m1 ) ) == 2_scalar );
    assert( eval( d_a( det( m1 )) ) == 4_scalar );

    auto grad = gradient( a );

    // paraboloid 
    auto para = ( pow< 2 >( x - 2 ) + pow< 2 >( y - 3 ) + 3 );

    auto solver = gradient_descent( x, y );
    solver[ maximum_iterations ] = 1000;
    solver[ learning_rate ] = 1e-3;

    solver( para );
    println( "solved: para({}, {}) == {}", eval( x ), eval( y ), eval( para ));

    auto para2 = ( pow< 2 >( w - 2_ft ) + pow< 2 >( z - 3_ft ) + 3_ft * 1_ft );

    // verify the dependent_variables_t trait works
    static_assert( 
        ( z.index < w.index and is_same_v< 
            tuple< decltype( z ), decltype( w )>,
            dependent_variables_t< decltype( para2 )>> ) or 
        ( w.index < z.index and is_same_v< 
            tuple< decltype( w ), decltype( z )>,
            dependent_variables_t< decltype( para2 )>> ));

    auto solver2 = gradient_descent( w, z );
    solver2[ maximum_iterations ] = 1000;
    solver2[ learning_rate ] = 1e-2;
    solver2( para2 );
    println( std::runtime_format( "solved: para2({:ft}, {:ft}) == {}" ), eval( w ), eval( z ), eval( para2 ));



    return EXIT_SUCCESS;
}