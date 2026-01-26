#include "tensor.hpp"

#include <iostream>
#include <stdexcept>

int main( int ac, char* av[] )
{
    using namespace tensor;
    using std::is_same_v;

    auto t1 = make_tensor<shape<1,1,6>>( 
        0., 1., 2., 3., 4., 5. );

    auto t2 = contract< 0, 1 >( t1 );

    static_assert( is_same_v< shape<6>, shape_of< decltype(t2) >> );

    auto t3 = make_tensor<shape<3,3>>( 
        1., 2., 3.,
        4., 5., 6.,
        7., 8., 9. );

    auto t4 = contract< 0, 1 >( t3 );

    static_assert( is_same_v< shape<1>, shape_of< decltype(t4) >> );

    if( get< 0 >( t4.values ) != 1. + 5. + 9. )
        throw std::logic_error("FAIL: contraction test");

    auto t5 = make_tensor<shape<1,2>>( 5., 6. );
    auto t6 = make_tensor<shape<2,1>>( 7., 8. );

    auto t7 = t5 * t6;

    auto t5t6 = make_tensor< shape<1,2,2,1>>(
        5. * 7., 5. * 8.,
        6. * 7., 6. * 8.
    );

    if( t7 != t5t6 )
        throw std::logic_error("FAIL: multiplication test");

    return EXIT_SUCCESS;
}