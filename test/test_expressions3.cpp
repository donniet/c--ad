#include "expressions.v1.2.hpp"
#include "units.hpp"

#include <print>

int main( int ac, char* av[] )
{
    using std::println;

    using namespace expressions;

    auto zero = constant_zero;
    auto one = constant_one;

    auto f = ( 5 + zero + one );

    println( "{}", eval( f ));

    variable< 0, long double > x;
    variable_values values;
    values[ 0 ] = 8.l;

    auto g = ( 5 + x - f );
    println( "{}", eval( g, values ));

    auto a = array_of( zero );
    auto b = array_of( zero , 1 );

    println( "{}", eval( element_of< 1 >(b) ));


    return EXIT_SUCCESS;
}