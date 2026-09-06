// select expression tests

#include "expressions/expressions.hpp"
#include "expressions/conditional.hpp"

#include "expressions/arithmetic.hpp"
#include "expressions/comparison.hpp"
#include "expressions/logical.hpp"

#include <print>

#include "testing.hpp"

using namespace expressions;

using std::print, std::println;

//static_assert( compound_expression< Select< Var< 0, int >, int, long >> );
//static_assert( is_same_v< result_t< Select< int, int, long >>, int >,
//    "result of a select with no expression arguments is the first option" );
//static_assert( is_same_v< result_t< Select< Var< 0, int >,
//    Constant< (int)0 >, Constant< (long)1 >>>, int >,
//        "result of a select is the result of it's first option" );

bool test_select();
bool test_chain();

int main( int ac, char * av[] )
{
    println( "SELECT EXPRESSION TETS..." );

    test::ensure( test_select, "Select Basic Testing" );
    test::ensure( test_chain, "Culmination test" );

    print( "SUCCESS." );
    return EXIT_SUCCESS;
}

bool test_chain()
{
    auto scope = declare_variables(
        var< int >( "n" ),
        var< int >( "m" ));

    auto [ n, m ] = scope.variables();
    scope( n = 0, m = 0 );

    ( n = 1 ) | scope;

    println( "n == {}", scope(n) );
    if( scope( n ) != 1 )
        return false;
    
    ( n = n + 1 ) | scope;

    println( "n == {}", scope(n) );
    if( scope( n ) != 2 )
        return false;

    ( m = 1 ) | scope;
    if( scope( m ) != 1 )
        return false;

    println( "m == {}", scope(m) );

    // chain expression test
    ( n = n + 1, m = m + n ) | scope;

    // GCC fails here, perhaps second expression is executed first?
    println( "n == {}, m == {}", scope( n ), scope( m ));
    if( scope( n ) != 3 or scope( m ) != 4 )
        return false;

    ( n = n + 1, m = m + n ) | scope;

    println( "n == {}, m == {}", scope( n ), scope( m ));
    if( scope( n ) != 4 or scope( m ) != 8 )
        return false;

    return true;
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
