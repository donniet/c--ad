
#include "testing.hpp"

#include "units.hpp"
#include "expressions/expressions.hpp"
#include "expressions/arithmetic.hpp"
#include "expressions/logical.hpp"
#include "expressions/comparison.hpp"
#include "expressions/format.hpp"

#include <print>
#include <format>
#include <cassert>
#include <string>
#include <stdexcept>
#include <stdlib.h>

using namespace expressions;

namespace test {

static_assert( expression< Var< 1, int >>, "variables are expressions" );
static_assert( expression< StaticValue< int >>, "statics are expressions" );
static_assert( expression< Constant< 1 >>, "constants are expressions" );

static_assert( expression< Product< StaticValue< int >, Var< 1, int >>>,
    "product of a static and a variable is an expression" );

static_assert( not expression< int >, "an int is not an expression" );

static_assert( open_expression< Var< 1, int >>, "variables are open" );
static_assert( closed_expression< StaticValue< int >>, "statics are closed" );
static_assert( closed_expression< Constant< 1 >>, "constants are closed" );

static_assert( not open_expression< int > and not closed_expression< int >,
    "an int is neither open nor closed expression" );

static_assert( open_expression< Product< StaticValue< int >, Var< 1, int >>>,
    "product of a static and a variable is an open expression" );


static_assert( is_same_v< unique_variables< Var< 0, int >>, 
    free_variables_t< Var< 0, int >>> );
static_assert( is_same_v< unique_variables< Var< 0, int >, Var< 1, int >>, 
    free_variables_t< tuple< Var< 0, int >, Var< 1, int >>>> );
static_assert( is_same_v< unique_variables< Var< 0, int >, Var< 1, int >>, 
    free_variables_t< tuple< Var< 1, int >, Var< 0, int >>>> );
static_assert( is_same_v< unique_variables< Var< 0, int >>, 
    free_variables_t< tuple< Var< 0, int >, Var< 0, int >>>> );
static_assert( is_same_v< unique_variables< Var< 1, int >>, 
    free_variables_t< tuple< Var< 1, int >, Var< 1, int >>>> );

// tests for higher-order variables
//
// 1 free second order variable of type int
//static_assert( is_same_v< unique_variables< Var< 0, Var< 0, int >>>,
//    free_variables_t< Var< 0, Var< 0, int >>>> );
////
//// 1 free second order variable from a tuple of variables
//static_assert( is_same_v< unique_variables< Var< 0, Var< 0, int >>>,
//    free_variables_t< tuple< Var< 0, Var< 0, int >>>>> );
//
// A constant will be substituted into an expression substituted into variable 0
static_assert( next_var_id_v< Var< 0, int >> == 1 );
static_assert( next_var_id_v< Var< 1, int >> == 2 );
static_assert( next_var_id_v< Var< 2, Var< 2, int >>> == 3 );

static_assert( next_var_id_v< tuple< Var< 0, int >, Var< 1, int >>> == 2 );

} // namespace test

using test::ensure;
using namespace expressions;
using namespace units;

bool test_simple_expressions();
constexpr bool test_dependent_vars();

int main( int ac, char* av[] )
{
    ensure( test_dependent_vars, "Dependent Variables" );
    ensure( test_simple_expressions, "Simple Expressions" );
    return EXIT_SUCCESS;
}

bool test_simple_expressions() 
{
    using std::println, std::print;

    auto vars = declare_variables(
        var< long double >( "x" ), 
        var< long double >( "y" ),
        var< Length      >( "l" ),
        var< Velocity    >( "v" ),
        var< Scalar      >( "a" ),
        var< Length      >( "w" ),
        var< Length      >( "z" ));

    auto [ x, y, l, v, a, z, w ] = vars.variables();

//    auto d_x = differential( x );
//    auto d_y = differential( y );
//    auto d_l = differential( l );
//    auto d_v = differential( v );
//    auto d_a = differential( a );
//    auto d_z = differential( z );
//    auto d_w = differential( w );

    println( "{}", 0_c );

    auto f = ( 5 + 0_c + 1_c );

    static_assert( is_same_v< remove_cv_t< decltype( f )>, 
        Sum< Sum< StaticValue< int >, Constant< 0L >>, Constant< 1L >>> );

    println( "( 5 + 0c + 1c ) == {}", f() );
    assert( f() == 6 );

    println( "{}", f | eval( vars ) );
    assert(( f | eval( vars )) == 6 );

    println( "{}", x | eval( vars ) );
    println( "{}", ( x * 1_c )| eval( vars ) );
    println( "{}", ( x + 1_c )| eval( vars ) );
    println( "{}", ( x - 1_c )| eval( vars ) );
    println( "{}", ( -x )| eval( vars ) );
    println( "{}", ( x == 1_c )| eval( vars ) );
    println( "{}", (( x == 1_c ) and ( x == 0_c )) | eval( vars ) );
    
    vars( x = 8.l, l = 12_in );

    auto g = ( 5 + 3*x - f );
    println( "{}", g | eval( vars ) );
    println( std::runtime_format( "g == {}" ), g );

//    auto dg = d_x( g );
//    println( "{}", dg );
//    println( "g() == {}", dg | eval( vars ) );

    auto h = ( 1_sqft - l * l ) / 254_mm;
    println( std::runtime_format( "h({}) == {}" ), l, h );
    println( std::runtime_format( "h({}) == {:ft}" ), 12_in, h | eval( vars ) );

//    auto dh = d_l( h );
//    println( "{}", dh );
//    println( "dh({}) == {}", 12_in, dh | vars );

    auto t1 = make_tuple( 3_mm / 1_s, 5_mm / 1_s, 2_mm / 1_s );
    auto t2 = make_tuple( 3_mm / 1_s, 5_mm / 1_s, v );
    auto s1 = make_tensor< Shape< 3 >>( 3_mm / 1_s, 5_mm / 1_s, 2_mm / 1_s );
    auto s2 = make_tensor< Shape< 3 >>( 3_mm / 1_s, 5_mm / 1_s, v );

    auto eq = ( t1 == t2 );

    vars( v = 2_mm / 1_s );

    assert( eq | eval( vars ) );
    println( "{} == {} => {}", t1, t2, eq | eval( vars ) );
    assert(( s1 == s2 ) | eval( vars ) );

    vars( v = 2_ft / 1_s );

    assert( not ( eq | eval( vars ) ));
    assert(( s1 != s2 ) | eval( vars ) );

    auto m1 = make_tensor< Shape< 2, 2 >>( 
        a, -a,
        a, a );

    vars( a = 1_scalar );

//    assert(( static_expr( 1 ) + static_expr( 2 )   | 
//        manipulate( [&]( auto n ){ return n + 1; }) | 
//        eval()) == 5 );
 
//    println( "det(1,-1,1,1) == {}", det( m1 ) | vars );
//    assert(( det( m1 ) | vars ) == 2_scalar );
//    assert(( d_a( det( m1 )) | vars ) == 4_scalar );
//
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

    //test_iteration();

    return true;
}

constexpr bool test_dependent_vars()
{
    Var< 0, int > x;
    Var< 1, int > y;
    Var< 2, int > z;

    return true; 
}

/// testing visitors
template< size_t I, expression ExprT >
struct CountSubExpressions
{ 
    using type = Constant< I + 1 >;
    static constexpr type value( ExprT const& )
    { return {}; }
};

template< expression ExprT >
constexpr auto count_expressions( ExprT const& expr )
{ return visit< DepthFirst, 0, CountSubExpressions >( expr ); }

struct PreOrderVisitTests 
{
    static constexpr Var< 0, int > v0;
    static constexpr Var< 1, int > v1;
    static constexpr Var< 2, int > v2;

    static_assert(( count_expressions( v0 + 0_c ) | eval( )) == 3ul );
    static_assert(( count_expressions( v0 ) | eval( )) == 1ul );
    static_assert(( count_expressions( v0 + v0 ) | eval( )) == 3ul );
    static_assert(( count_expressions( v0 + v1 ) | eval( )) == 3ul );
    static_assert(( count_expressions( v0 + ( v1 * v2 )) | eval( )) == 5ul );
    static_assert(( count_expressions( 0_c + 0_c * 0_c / 0_c + 0_c ) | eval( )) == 9ul );
    static_assert(( count_expressions( v0 + 0_c * v1 / v2 + 0_c ) | eval( )) == 9ul );
};

