#include "expressions/expression_base.hpp"
#include "expressions/expression_ops.hpp"
// #include "expressions/compound_units.hpp"
#include "expressions/tensor_field.hpp"
#include "expressions/tensor_field_ops.hpp"
// #include "expressions/tensor_contraction.hpp"

#include <print>
#include <cmath>
#include <cassert>

int main( int ac, char * av[] )
{
    using std::println;
    using std::sin, std::cos;
    using namespace std::numbers;
    using namespace expressions;
    // using namespace expressions::units;
    using namespace expressions::operators;


    auto f1 = 1_scalar + 0_scalar + 1_scalar + 0_scalar;

    println("{}", typeid(f1).name() );
    println("{}", invoke( f1 ));

    auto f2 = 1_ft / 2_scalar;

    println( "{}", f2 );

    auto vars = variable_values{};
    vars[1] = 0.3_m;

    auto x1 = Variable< Length, 1 >{};

    auto g = 2_scalar * x1;
    auto g_x1 = invoke( g, vars );

    println("{}", g_x1 );
    // TODO: write formatter for units
    println( "{:in}", g_x1 );

    using mat2 = UniformMatrix< 2, 2, Scalar >;
    using vec2 = UniformVector< 2, Scalar >;
    
    auto m = make_matrix< 2, 2 >(
        1_scalar, 0_scalar,
        0_scalar, 1_scalar );
    auto v = make_vector< 2 >( 2_scalar, 3_scalar );

    

    auto g1 = m * v;

    auto g1_x = invoke( g1 );
    // println( "{}", typeid( g1_x ).name() );

    // println( std::runtime_format( "{}" ), g1_x );
    println( "{}", get_tensor_element< 1 >( g1_x ) );

    assert( (g1_x.template elem< 0 >() == 2_scalar) );
    assert( (g1_x.template elem< 1 >() == 3_scalar) );
    assert( (g1_x.template elem< 2 >() == 0_scalar) );
    assert( (g1_x.template elem< 3 >() == 0_scalar) );
    assert( (g1_x.template elem< 4 >() == 0_scalar) );
    assert( (g1_x.template elem< 5 >() == 0_scalar) );
    assert( (g1_x.template elem< 6 >() == 2_scalar) );
    assert( (g1_x.template elem< 7 >() == 3_scalar) );

    auto g1c = contract< 1, 2 >( m * v );
    // auto g1c_x = invoke( g1c );

    // println("{}", av[0] );

    // using ten22 = UniformTensor< Shape< 2, 2 >, float >;
    // using ten2 = UniformTensor< Shape< 2 >, float >;
    
    // auto m = ten22{ 1.f, 0.f, 
    //                0.f, 1.f };

    // auto n = ten22{ cosf( pi / 3.f ), -sinf( pi / 3.f ), 
    //                 sinf( pi / 3.f ),  cosf( pi / 3.f ) };

    // auto v = ten2{ 0.f, 1.f };
    // auto w = invoke( v );

    // auto x = m * v ;

    // auto x1 = UniformTensor< Shape< 2, 2, 2 >, float >{
    //     0.f, 1.f,   0.f, 0.f,
    //     0.f, 0.f,   0.f, 1.f
    // };

    // if( x != x1 )
    //     throw std::logic_error( "equality test failed" );

    // using mat2 = UniformMatrix< 2, 2, float >;
    // using vec2 = UniformVector< 2, float >;

    // auto g = mat2{ 1.f, 0.f,
    //                0.f, 1.f };
    // auto h = mat2{ 1.f, 2.f,
    //                3.f, 4.f };

    // auto y = g * h;
    // if( y != h )
    //     throw std::logic_error( "identity matrix multiplication failed" );

    // // println( "w = {}", w );

    // static_assert( expression_traits< ten22 >::is_static_expression );

    return 0;
}