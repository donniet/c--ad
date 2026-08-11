// unit tests for include/expressions/calculus.hpp

#include "expressions/calculus.hpp"

#include <print>
#include <assert.h>

using std::println, std::print;
using namespace expressions;

static_assert( closed_expression< Func<expressions::Constant<0>, expressions::Var<0, float>>>, 
    "empty sub is closed" );

int main( int ac, char* av[] )
{
    auto d = [&]( auto f, auto x ) constexpr
    { return derivative( f, x ); };

    print("TESTING EXPRESSION CALCULUS...");

    Var< 0, float > x;
    Var< 1, float > y;
   
    assert(( d( y, x )( 1 )) == 0 );
    assert( d( x, x )( 1 ) == 1 );
    assert( d( sin(x), x )( 0 ) == 1 );
    assert( d( sin(y), x )( 1 ) == 0 );
    assert( d( d( sin(y), x ), x )( 0 ) == 0 );


    println( "SUCCESS." );
    return EXIT_SUCCESS;
}
