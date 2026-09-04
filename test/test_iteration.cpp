// iteration tests

#include "expressions/iteration.hpp"
#include "expressions/arithmetic.hpp"
#include "expressions/comparison.hpp"
#include "expressions/logical.hpp"
#include "expressions/calculus.hpp"

#include "testing.hpp"

#include <print>


using std::print, std::println;
using namespace expressions;
using namespace units;

bool test_iteration();

constexpr auto lt = LessThan<StaticValue<int>, StaticValue<int>>{};

//using scope_type = Scope<Var<0, int>, Var<1, int>, Var<2, Length>, Var<3, Length>, Var<4, Length>, Var<5, Length>, Var<6, Length>>;

//using applier_type = typename Applier< LessThan< Var< 0, int >, StaticValue< int >>, scope_type >::type;

//static_assert( closed_expression< LessThan<Var<0, int>, StaticValue<int>>> );

//static_assert( requires { typename std::invoke_result< Scope< Var< 0, int >>, 
//    tuple< Var< 0, int >>>::type; });
//
//static_assert( requires { typename std::invoke_result<Scope<>, 
//    tensors::Tensor<tensors::Shape<2>, Sum<Sum<Product<Product<Constant<2L>, PowN<1L, Difference<Var<4, Length>, StaticValue<Length>>>>, Difference<Constant<result_t<Var<4, Length>>{1.000000e+00}>, Constant<0>>>, Constant<0>>, Constant<0>>, Sum<Sum<Constant<0>, Product<Product<Constant<2L>, PowN<1L, Difference<Var<5, Length>, StaticValue<Length>>>>, Difference<Constant<result_t<Var<4, Length>>{1.000000e+00}>, Constant<0>>>>, Constant<0>>>>::type; });
//
//
//static_assert( requires { typename std::invoke_result<Scope<Var<0, int>, Var<1, int>, Var<2, Length>, Var<3, Length>, Var<4, Length>, Var<5, Length>, Var<6, Length>>, tensors::Tensor<tensors::Shape<2>, Sum<Sum<Product<Product<Constant<2L>, PowN<1L, Difference<Var<4, Length>, StaticValue<Length>>>>, Difference<Constant<result_t<Var<4, Length>>{1.000000e+00}>, Constant<0>>>, Constant<0>>, Constant<0>>, Sum<Sum<Constant<0>, Product<Product<Constant<2L>, PowN<1L, Difference<Var<5, Length>, StaticValue<Length>>>>, Difference<Constant<result_t<Var<4, Length>>{1.000000e+00}>, Constant<0>>>>, Constant<0>>>>::type; });
//

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
    auto para2 = func( 
        pow( x - 2_ft, 2_c ) + pow( y - 3_ft, 2_c ) + 3_ft * 1_ft, x, y );

    auto grad_p = grad( para2 );


    scope( n = 0, x = 0_ft, y = 0_ft );

    println( "grad_p | scope = {}", get_element< 0 >( grad_p ) | scope );

    ( x - rate * para2 * get< 0 >( grad_p( x, y )),
      y - rate * para2 * get< 1 >( grad_p( x, y )),
      n = n + 1 ) | 
        do_while( n < 1000 and norm( grad_p( x, y )) < 0.001_sqft, scope );

    println( "minimum of para2 is ( {}, {} )", scope( x ), scope( y ));


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

