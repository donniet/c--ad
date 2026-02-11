#include "jacobian.hpp"
#include "expressions.hpp"
#include "tensor.hpp"

#include <print>

int main( int ac, char * av[] )
{
    using std::println;

    println( "test_jacobian ");

    using namespace expressions;
    using namespace expressions::operators;
    using namespace tensor;

    auto x0 = Variable< float, 0 >{};
    auto y0 = Variable< float, 1 >{};

    x0.set( 1.f );
    y0.set( 2.f );

    auto f = equals_zero( x0 );
    auto g = equals_zero( y0 );

    static_assert( is_same_v< decltype( f ), EqualsZero< Variable< float, 0 >>> );
    static_assert( is_same_v< decltype( g ), EqualsZero< Variable< float, 1 >>> );

    solver::solve( f and g );

    println( "x0 == {}\ny0 == {}", x0.get(), y0.get() );

}