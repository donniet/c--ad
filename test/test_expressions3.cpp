#include "expressions.v1.2.hpp"
#include "units.hpp"

#include <print>
#include <format>


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

    variable< 0, long double > x{{ "x{}" }};
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
    println( "depends on g: {}", g.dependents_size );
    println( "{}", eval( g, values ));
    // println( std::runtime_format( "g == {}" ), g );

    // auto dg = d< 0 >( g );
    // println( "depends on dg: {}", dg.dependents_size );
    // println( "g() == {}", eval( dg ));

    // variable< 1, Length > l;
    // values[ 1 ] = 12_in;

    // auto h = ( 1_sqft + l * l ) / 254_mm;
    // println( "h(x) == {:ft}", eval( h, values ));

    // auto dh = d< 1 >( h );
    // println( "dh(x) == {:ft}", eval( dh, values ));

    // auto a = array_of( zero );
    // auto b = array_of( zero , 1 );

    // println( "{}", eval( element_of< 1 >(b) ));
    // // println( "{}")


    return EXIT_SUCCESS;
}