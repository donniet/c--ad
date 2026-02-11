/**
 * test of expressions.hpp
 */

#include "expressions.hpp"

#include <print>

int main( int ac, char * av[] ) 
{
    using namespace expressions;
    using namespace expressions::operators;
    using std::println;

    auto zero = constant_zero< double >;
    auto one = constant_one< double >;

    set_variable_value< double, 31415 >( std::numbers::pi );
    auto pi = get_variable_value< double, 31415 >();

    if( pi != (double)std::numbers::pi )
        throw std::logic_error( "get_variable_value returned incorrect value" );

#ifndef NDEBUG
    println( "retrieved value from <31415, double> is {}", pi );
#endif

    set_variable_value< double, 31415 >( 22. / 7. );
    pi = get_variable_value< double, 31415 >();

    if( pi != (double)( 22. / 7. ))
        throw std::logic_error( "get_variable_value returned incorrect value" );

#ifndef NDEBUG
    println( "retrieved value from <31415, double> after a reset is {}", pi );
#endif

    auto x0 = Variable< double, 0 >{};
    auto fx = sin( x0 );
    auto gx = cos( x0 );

    x0.set( pi / 2. );

    println( "sin( pi / 2 ) == {}", fx() );
    println( "cos( pi / 2 ) == {}", gx() );


    return EXIT_SUCCESS;
}