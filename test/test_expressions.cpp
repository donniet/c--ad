
#include "units.hpp"
#include "expressions/expressions.hpp"
#include "expressions/solvers.hpp"
#include "expressions/format.hpp"

#include <print>
#include <format>
#include <cassert>

using namespace expressions;
using namespace units;


void test_iteration();
void test_minimization();

int main( int ac, char* av[] )
{
    using std::println;

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

    auto [ x, y, l, v, a, z, w ] = vars.variables();

    auto d_x = differential( x );
    auto d_y = differential( y );
    auto d_l = differential( l );
    auto d_v = differential( v );
    auto d_a = differential( a );
    auto d_z = differential( z );
    auto d_w = differential( w );

    println( "{}", zero );

    auto f = ( 5 + zero + one );

    println( "{}", f | vars );
    println( "{}", x );
    println( "{}", x * one );
    println( "{}", x + one );
    println( "{}", x - one );
    println( "{}", -x );
    println( "{}", ( x == one ) );
    println( "{}", ( x == one and x == zero ) );
    
    x = 8.l;

    auto g = ( 5 + 3*x - f );
    println( "{}", g | vars );
    println( std::runtime_format( "g == {}" ), g );

    auto dg = d_x( g );
    println( "{}", dg );
    println( "g() == {}", dg | vars );

    l = 12_in;

    auto h = ( 1_sqft - l * l ) / 254_mm;
    println( std::runtime_format( "h({}) == {}" ), l, h );
    println( "h({}) == {:ft}", 12_in, h | vars );

    auto dh = d_l( h );
    println( "{}", dh );
    println( "dh({}) == {}", 12_in, dh | vars );

    auto t1 = make_tuple( 3_mm / 1_s, 5_mm / 1_s, 2_mm / 1_s );
    auto t2 = make_tuple( 3_mm / 1_s, 5_mm / 1_s, v );
    auto s1 = make_tensor< Shape< 3 >>( 3_mm / 1_s, 5_mm / 1_s, 2_mm / 1_s );
    auto s2 = make_tensor< Shape< 3 >>( 3_mm / 1_s, 5_mm / 1_s, v );

    auto eq = ( t1 == t2 );

    v = 2_mm / 1_s;

    assert( eq | vars );
    println( "{} == {} => {}", t1, t2, eq | vars );
    assert(( s1 == s2 ) | vars );

    v = 2_ft / 1_s;

    assert( not ( eq | vars ));
    assert(( s1 != s2 ) | vars );

    auto m1 = make_tensor< Shape< 2, 2 >>( 
        a, -a,
        a, a );

    a = 1_scalar;

    println( "det(1,-1,1,1) == {}", det( m1 ) | vars );
    assert(( det( m1 ) | vars ) == 2_scalar );
    assert(( d_a( det( m1 )) | vars ) == 4_scalar );

    //auto grad = gradient( a );

    // eval( vec( w, z ), minimize( para2, iterations( n ), iteration_delta( d )) and n < 1000 and 
    // 
    // auto rate = 0.01_sqft;
    // auto gradient = grad( para2 );
    // auto p = vec< Length, Length >( 0_ft, 0_ft );
    //
    //
    // DT: I like this format because it's flexible and can be implimented with the current
    //	   scoping of variables.
    //
    // auto min = eval( 
    //	    iteratation( p, n ).initial_condition( n == 0 ).
    //	    iterate_until( n == 1000 or norm( gradient ) < 0.001_sqft ).
    //	    update( p - rate * para2( p ) * gradient( p ), n + 1 ).
    //	    value( p ));

//    auto solver2 = gradient_descent( w, z );
//    solver2[ maximum_iterations ] = 1000;
//    solver2[ learning_rate ] = 1e-2;
//    solver2( para2 );
//    println( std::runtime_format( "solved: para2({:ft}, {:ft}) == {}" ), eval( w ), eval( z ), eval( para2 ));

    test_iteration();

    return EXIT_SUCCESS;
}

void test_iteration() 
{
    using std::println;

    auto vars = declare_variables(
        var< uniform_vector_t< 2, Length >>( "p" ),
        var< Cardinal    >( "n" ),
        var< Cardinal    >( "m" ),
        var< Length      >( "w" ),
        var< Length      >( "z" ));

    auto [ p, n, m, z, w ] = vars.variables();

//    auto para2 = ( pow< 2 >( w - 2_ft ) + pow< 2 >( z - 3_ft ) + 3_ft * 1_ft );
    
    // sum of the first n integers
    auto first_n = iteration( m, n ).
        initial_values( 0, 1 ).
        update( m + n, n + 1 ).
        until( n > 100 );

    auto [ s, steps ] = first_n | vars;

    println( std::runtime_format( "sum of first {} integers: {}" ), steps-1, s );

//    auto gradw = gradient( w );
//    auto gradz = gradient( z );
//    auto rate = 1.0 / 100.0_sqft;
//
//    auto [ min_value, steps ] = eval( iteration( w, z, n ).
//        initial_values( 0_ft, 0_ft, 0 ).
//        update( w - rate * para2( w, z ) * gradw( para2 ), n + 1 ).
//        until( n == 1000 or norm( grad( p )) < 0.001_sqft ));
}

void test_minimization()
{
    //auto solver = gradient_descent( x, y );
    //solver[ maximum_iterations ] = 1000;
    //solver[ learning_rate ] = 1e-3;
    //solver( para );
    //println( "solved para({}, {}) == {}", eval( x ), eval( y ), eval( para ));

    auto vars = declare_variables(
        var< Cardinal    >( "n" ),
        var< double      >( "x" ),
        var< Length      >( "w" ),
        var< Length      >( "z" ));

    auto [ n, x, w, z ] = vars.variables();

    // parabola
    auto para = ( pow< 2 >( x - 2 ) + 3 );

    auto x0 = para | min_arg( x );

    // paraboloid
    auto para2 = ( pow< 2 >( w - 2_ft ) + pow< 2 >( z - 3_ft ) + 3_ft * 1_ft );

    // verify the dependent_variables_t trait works
//    static_assert( 
//        ( z.index < w.index and is_same_v< 
//            tuple< decltype( z ), decltype( w )>,
//            dependent_variables_t< decltype( para2 )>> ) or 
//        ( w.index < z.index and is_same_v< 
//            tuple< decltype( w ), decltype( z )>,
//            dependent_variables_t< decltype( para2 )>> ));
//

}
