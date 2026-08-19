// select expression tests

#include "expressions/conditional.hpp"

#include "expressions/arithmetic.hpp"
#include "expressions/comparison.hpp"
#include "expressions/logical.hpp"

#include <print>

#include "testing.hpp"

using namespace expressions;

using std::print, std::println;

bool test_select();

int main( int ac, char * av[] )
{
    println( "SELECT EXPRESSION TETS..." );

    test::ensure( test_select, "Select Basic Testing" );

    print( "SUCCESS." );
    return EXIT_SUCCESS;
}

bool test_select() 
{
    Var< 1, int > x;
    Var< 2, int > y;

    if( not select( 2_c, 0_c, 1_c, 2_c ) == 2_c )
        return false;

    if( not select( 2_c - 1_c, 0_c, 1_c, 2_c ) == 1_c )
        return false;

    if( not select( x + 1_c, 0_c, 1_c, 2_c )( 1_c ) == 2_c )
        return false;

    if( not select( x + 1_c, x + 0_c, x + 1_c, x + 2_c )( 1_c ) == 3_c )
        return false;

    return true;
}
