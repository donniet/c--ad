// iteration tests

#include "expressions/iteration.hpp"
#include "expressions/arithmetic.hpp"
#include "expressions/comparison.hpp"
#include "expressions/logical.hpp"
#include "expressions/calculus.hpp"

#include "expressions/format.hpp"

#include "testing.hpp"

#include <print>


using std::print, std::println;
using namespace expressions;
using namespace units;

bool test_iteration();
bool test_minimize_parabola();
bool test_gradient_descent();


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
    test::ensure( test_minimize_parabola, "Minimize Parabola" );
    test::ensure( test_gradient_descent, "Gradient Descent" );
    
    println("SUCCESS.");
    return EXIT_SUCCESS;
}

bool test_iteration() 
{
    using std::println;

    auto scope = declare_variables(
        var< int >( "n" ),
        var< int >( "m" ));

    auto [ n, m ] = scope.variables();
    
    scope( m = 0, n = 0 );

    ( n = n + 1, 
      m = m + n ) | 
        do_while( n < 100, scope );

    println( "m == {}", scope( m ) );
    
    if( scope( m ) != 5050 )
        return false;

    return true;
}

bool test_minimize_parabola()
{
    using std::println;

    auto scope = declare_variables( 
        var< int >( "n" ), var< float >( "x" ));

    auto [ n, x ] = scope.variables();

    scope( n = 0, x = 0 );

    auto p = ( x - 2 ) * ( x - 2 ) + 5;
    auto dp_x = derive< x.id >( p );

    auto rate = 0.03_c;

    ( x = x - rate * p(x) * dp_x(x), n = n + 1 ) | 
        do_while( n < 100 and dp_x( x ) * dp_x( x ) > 0.00000001, scope );

    println( "minimum of (x-2)^2+5 is {} at x={}; steps {}", 
        p( x ) | scope, x | scope, n | scope );

    return true;
}

bool test_gradient_descent() 
{
    using std::println;

    auto scope = declare_variables(
        var< int >( "n" ),
        var< float >( "x" ), var< float >( "y" ),
        var< float >( "f" ), var< float >( "dx" ),
        var< float >( "dy" ));

    auto [ n, x, y, f, dx, dy ] = scope.variables();

    auto rate = 0.015_c;

    // parabloid with vertex at (2_ft, 3_ft) and minimum 3_sqft
    auto para2 = func( 
        ( x - 2 ) * ( x - 2 ) + ( y - 3 ) * ( y - 3 ) + 3, x, y );

    auto grad_p = grad( para2 );

    println("grad_p[0] = {}", get<0>( grad_p ));
    println("grad_p[1] = {}", get<1>( grad_p ));

    //static_assert( is_same_v< void, decltype( grad_p )> );

    scope( n = 0, x = 0, y = 0, dx = 0, dy = 0, f = 0 );

    println( "grad_p | scope = ( {}, {} )", 
        get< 0 >( grad_p ) | scope, 
            get< 1 >( grad_p ) | scope );

    // DT: this works, so executing each step one-by-one works
    for( int i = 0; i < 30; ++i )
    {
        auto vf = para2 | scope;
        auto vdfx = get<0>(grad_p) | scope;
        auto vdfy = get<1>(grad_p) | scope;

        auto vx = scope(x) - (float)rate * vf * vdfx;
        auto vy = scope(y) - (float)rate * vf * vdfy;

        scope( x = vx, y = vy, n = i );
        
    }
    println( "[MANUAL ITERATION]: para2 is {} at ( {}, {} ) step {}",
        para2( x, y ) | scope, scope( x ), scope( y ), scope( n ));

    scope( n = 0, x = 0, y = 0, dx = 0, dy = 0, f = 0 );

    // DT: this version does not work perhaps because x is set then
    //     y is recalculated with the new x.
    //
    //     We could set tensors each time maybe
    ( f = para2( x, y ), 
      dx = get< 0 >( grad_p( x, y )),
      dy = get< 1 >( grad_p( x, y )),
      x = x - rate * f * dx,
      y = y - rate * f * dy,
      n = n + 1 ) |
        do_while( n < 30 and norm( grad_p( x, y )) > 0.001, scope );

    println( "[AUTO ITERATION]:   para2 is {} at ( {}, {} ) step {}", 
        para2( x, y ) | scope, scope( x ), scope( y ), scope( n ));

    return true;
}

