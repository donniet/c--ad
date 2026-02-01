/**
 * test of expressions.hpp
 */

#include "expressions.hpp"

#include <iostream>

int main( int ac, char * av[] ) 
{
    using namespace expressions;
    using namespace expressions::operators;

    auto zero = constant_zero< double >;
    auto one = constant_one< double >;

    return EXIT_SUCCESS;
}