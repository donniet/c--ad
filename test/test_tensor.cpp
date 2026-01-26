#include "tensor.hpp"

#include <iostream>
#include <stdexcept>

int main( int ac, char* av[] )
{
    using std::cout, std::cerr, std::endl;
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
    // 1*(5*9-6*8) - 2*(4*9-6*7) + 3*(4*8-5*7)
    // 45 - 48 - 2*(36 - 42) + 3*(32 - 35)
    // -3 + 12 - 9 == 0

    auto t4 = contract< 0, 1 >( t3 );

    static_assert( is_same_v< shape<1>, shape_of< decltype(t4) >> );

    if( get< 0 >( t4.values ) != 1. + 5. + 9. )
        throw std::logic_error("FAIL: contraction test");

    cout << "PASS: contraction test" << endl;

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

    cout << "PASS: sub_matrix test" << endl;

    /**
     * Arithmetic tests
     */
    auto a1 = make_tensor< shape< 2, 2 >>( 2., 3., 5., 7. );
    auto a2 = make_tensor< shape< 2, 2 >>( 7., 5., 3., 2. );

    auto a12 = a1 + a2;
    static_assert( is_same_v< shape< 2, 2 >, shape_of< decltype( a12 )>> );
    
    auto a1plusa2 = make_tensor< shape< 2, 2 >>( 9., 8., 8., 9. );
    if( a12 != a1plusa2 )
        throw std::logic_error("FAIL: plus test");

    cout << "PASS: plus test" << endl;

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
    
    cout << "PASS: multiplication test" << endl;

    auto u5 = make_tensor< shape<2>>( 5., 6. );
    auto u6 = make_tensor< shape<2>>( 7., 8. );

    auto u7 = u5 * u6;

    auto u5u6 = make_tensor< shape< 2,2 >>(
        5. * 7., 5. * 8.,
        6. * 7., 6. * 8. );

    if( u7 != u5u6 )
        throw std::logic_error("FAIL: multiplication test");

    cout << "PASS: multiplication test 2" << endl;

    /**
     * Transpose tests
     */
    auto u5u6_t = make_tensor< shape< 2,2>>(
        35., 42.,
        40., 48. );

    if( transpose(u7) != u5u6_t )
        throw std::logic_error("FAIL: transpose test");
    
    cout << "PASS: transpose test" << endl;

    /**
     * Scale tests
     */
    auto t8 = make_tensor< shape< 2, 3 >>(
        4. *  5., 4. *  7.,
        4. * 11., 4. * 13.,
        4. * 17., 4. * 19. );

    auto t9 = t8.scale( 1. / 4. );
    static_assert( is_same_v< shape< 2, 3 >, shape_of< decltype( t9 )>> );

    auto t8sby4 = make_tensor< shape< 2, 3 >>(
        5., 7.,
        11., 13.,
        17., 19. );

    if( t9 != t8sby4 )
        throw std::logic_error("FAIL: scale test");
    
    cout << "PASS: scale test" << endl;

    /**
     * Determinant tests
     */
    double d = determinant( t3 );

    if( d != 0. )
        throw std::logic_error("FAIL: determinant test");
    
    cout << "PASS: determinant test" << endl;
    
    auto d1 = make_tensor< shape< 1, 1 >>( 5. );

    if( determinant( d1 ) != 5. )
        throw std::logic_error("FAIL: determinant test 1x1");
    
    cout << "PASS: determinant test 1x1" << endl;

    auto d2 = make_tensor< shape< 2, 2 >>(
        3., 2.,
        2., 3. );

    if( determinant( d2 ) != 5. )
        throw std::logic_error("FAIL: determinant test 2x2");
    
    cout << "PASS: determinant test 2x2" << endl;

    /**
     * Cofactor tests
     */
    auto c2 = cofactor( d2 );
    static_assert( is_same_v< shape< 2, 2 >, shape_of< decltype( c2 )>> );

    auto cofactor_c2 = make_tensor< shape< 2, 2 >>(
        3., -2., 
        -2., 3. );
    
    if( c2 != cofactor_c2 )
        throw std::logic_error("FAIL: cofactor test");

    cout << "PASS: cofactor test" << endl;

    /**
     * Inverse tests
     */
    auto i2 = make_tensor< shape< 2, 2 >>(
        5., 4.,
        3., 4. );

    auto i2i = inverse( i2 );
    static_assert( is_same_v< shape< 2, 2 >, shape_of< decltype( i2i )>> );

    auto inverse_i2 = make_tensor< shape< 2, 2 >>(
        4. / 8., -4. / 8.,
        -3. / 8., 5. / 8. );

#ifndef NDEBUG
    std::cout << "i2 ==\n" << i2i << "\ninverse_d2 ==\n" << inverse_i2 << std::endl;
#endif

    if( i2i != inverse_i2 )
        throw std::logic_error("FAIL: inverse test");

    cout << "PASS: inverse test" << endl;

    

    return EXIT_SUCCESS;
}