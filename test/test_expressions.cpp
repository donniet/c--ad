/**
 * test of expressions.hpp
 */

#include "expressions.hpp"

#include <iostream>

int main( int ac, char * av[] ) 
{
    using namespace expressions;
    using namespace expressions::operators;
    using std::cout, std::cerr, std::endl;

    auto zero = constant_zero< double >;
    auto one = constant_one< double >;

    set_variable_value< double, 31415 >( std::numbers::pi );
    auto pi = get_variable_value< double, 31415 >();

    if( pi != (double)std::numbers::pi )
        throw std::logic_error( "get_variable_value returned incorrect value" );

#ifndef NDEBUG
    cout << "retrieved value from <31415, double> is " << pi << endl;
#endif

    set_variable_value< double, 31415 >( 22. / 7. );
    pi = get_variable_value< double, 31415 >();

    if( pi != (double)( 22. / 7. ))
        throw std::logic_error( "get_variable_value returned incorrect value" );

#ifndef NDEBUG
    cout << "retrieved value from <31415, double> after a reset is " << pi << endl;
#endif

    return EXIT_SUCCESS;
}