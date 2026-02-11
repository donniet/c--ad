#include "expressions/expression_base.hpp"
#include "expressions/expression_ops.hpp"
#include "expressions/tensor_field.hpp"
#include "expressions/tensor_field_ops.hpp"
#include "expressions/tensor_contraction.hpp"

#include <print>
#include <cmath>

int main( int ac, char * av[] )
{
    using std::println;
    using std::sin, std::cos;
    using namespace std::numbers;
    using namespace expressions;
    using namespace expressions::operators;

    println("{}", av[0] );

    using mat2 = Tensor< Shape< 2, 2 >, float, float, float, float >;
    using vec2 = Tensor< Shape< 2 >, float, float >;
    
    auto m = mat2{ 1.f, 0.f, 
                   0.f, 1.f };

    auto n = mat2{ cosf( pi / 3.f ), -sinf( pi / 3.f ), 
                   sinf( pi / 3.f ),  cosf( pi / 3.f ) };

    auto v = vec2{ 0.f, 1.f };
    auto w = invoke( v );

    auto x = m * v ;

    if( x.template subscript< 0, 0, 0 >() != 0.f or 
        x.template subscript< 0, 0, 1 >() != 1.f or 
        x.template subscript< 0, 1, 0 >() != 0.f or 
        x.template subscript< 0, 1, 1 >() != 0.f or 
        x.template subscript< 1, 0, 0 >() != 0.f or 
        x.template subscript< 1, 0, 1 >() != 0.f or 
        x.template subscript< 1, 1, 0 >() != 0.f or 
        x.template subscript< 1, 1, 1 >() != 1.f )
        throw std::logic_error( "tensor multiplication failed." );

    // println( "w = {}", w );

    static_assert( expression_traits< mat2 >::is_static_expression );

    return 0;
}