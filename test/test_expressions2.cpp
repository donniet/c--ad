#include "expressions/expression_base.hpp"
#include "expressions/expression_ops.hpp"
#include "expressions/units.hpp"
// #include "expressions/tensor_field.hpp"
// #include "expressions/tensor_field_ops.hpp"
// #include "expressions/tensor_contraction.hpp"

#include <print>
#include <cmath>

int main( int ac, char * av[] )
{
    using std::println;
    using std::sin, std::cos;
    using namespace std::numbers;
    using namespace expressions;
    using namespace expressions::units;
    using namespace expressions::operators;


    auto f = 1_scalar + 0_scalar + 1_scalar + 0_scalar;

    println("{}", typeid(f).name() );
    println("{}", invoke( f ));

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