// iteration tests

#include "expressions/iteration.hpp"
#include "expressions/arithmetic.hpp"
#include "expressions/comparison.hpp"
#include "expressions/logical.hpp"

#include "testing.hpp"

#include <print>


using std::print, std::println;
using namespace expressions;
using namespace units;

bool test_iteration();

int main( int ac, char* av[] )
{
    println("ITERATION TESTS");

    test::ensure( test_iteration, "Iteration" );

    println("SUCCESS.");
    return EXIT_SUCCESS;
}

bool test_iteration() 
{
    using std::println;

    auto scope = declare_variables(
        var< int >( "n" ),
        var< int >( "m" ),
        var< Length >( "w" ),
        var< Length >( "z" ),
        var< Length >( "x" ),
        var< Length >( "y" ),
        var< Length >( "f" ));

    auto [ n, m, w, z, x, y, f ] = scope.variables();
    
    scope( m = 0, n = 0 );

    ( n = n + 1, m = m + n ) | do_while( n < 100, scope );
    println( "m == {}", scope( m ) );
    
    if( scope( m ) != 5050 )
        return false;

    auto rate = 1.0 / 100.0_sqft;

    // parabloid with vertex at (2_ft, 3_ft) and minimum 3_sqft
    auto para2 = pow( x - 2_ft, 2_c ) + pow( y - 3_ft, 2_c ) + 3_ft * 1_ft;

    //auto p = func( para2, x, y );
    //
    //auto grad_p = grad( p );
    //
    //auto [ min_value, steps2 ] = 
    //    iteration( x, y, n ).
    //    initial_values( 0_ft, 0_ft, 0 ).
    //    update( 
    //        x - rate * para2 * get_element< 0 >( grad_p( x, y )), 
    //        y - rate * para2 * get_element< 1 >( grad_p( x, y )),
    //        n + 1 ).
    //    until( n == 1000 or norm( grad_p( x, y )) < 0.001_sqft ) | eval();

    return true;
}

