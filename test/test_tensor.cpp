#include "tensor.hpp"

#include <iostream>
#include <stdexcept>

int main( int ac, char* av[] )
{
    using namespace tensor;
    using std::is_same_v;

    /**
     * Contraction tests
     */
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

    /**
     * Sub Tensor Tests
     */
    auto s4 = sub_tensor< index< 1, 1 >>( t3 );
    static_assert( is_same_v< shape< 2, 2 >, shape_of< decltype( s4 )>> );

    auto t3_s11 = make_tensor< shape< 2, 2 >>(
        1., 3.,
        7., 9. );

    if( s4 != t3_s11 )
        throw std::logic_error("FAIL: sub_matrix test");


    /**
     * Multiplication Tests
     */
    auto t5 = make_tensor<shape<1,2>>( 5., 6. );
    auto t6 = make_tensor<shape<2,1>>( 7., 8. );

    auto t7 = t5 * t6;

    auto t5t6 = make_tensor< shape<1,2,2,1>>(
        5. * 7., 5. * 8.,
        6. * 7., 6. * 8. );

    if( t7 != t5t6 )
        throw std::logic_error("FAIL: multiplication test");

    auto u5 = make_tensor< shape<2>>( 5., 6. );
    auto u6 = make_tensor< shape<2>>( 7., 8. );

    auto u7 = u5 * u6;

    auto u5u6 = make_tensor< shape< 2,2 >>(
        5. * 7., 5. * 8.,
        6. * 7., 6. * 8. );

    if( u7 != u5u6 )
        throw std::logic_error("FAIL: multiplication test");

    /**
     * Transpose tests
     */
    auto u5u6_t = make_tensor< shape< 2,2>>(
        35., 42.,
        40., 48. );

    if( transpose(u7) != u5u6_t )
        throw std::logic_error("FAIL: transpose test");

    return EXIT_SUCCESS;
}