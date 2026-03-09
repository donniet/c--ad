#include "tensor.v1.2.hpp"

#include <iostream>
#include <stdexcept>
#include <cassert>

int main( int ac, char* av[] )
{
    using std::cout, std::cerr, std::endl;
    using namespace tensor;
    using std::is_same_v;

    /**
     * Contraction tests
     */
    auto t1 = make_tensor< Shape< 1, 1, 6 >>( 
        0., 1., 2., 3., 4., 5. );
    auto t2 = make_tensor< Shape< 1, 1, 6 >>(
        5., 4., 3., 2., 1., 0. );
    
    auto t3 = make_tensor< Shape< 1, 1, 6 >>(
        5., 5., 5., 5., 5., 5. );

    assert( t1 + t2 == t3 );

    auto t4 = contract< 0, 1 >( t1 );
    static_assert( is_same_v< tensor_shape_t< decltype( t4 ) >, Shape< 6 >> );

    assert( t4 == make_tensor< Shape< 6 >>( 0., 1., 2., 3., 4., 5. ));

//     static_assert( is_same_v< shape<6>, shape_of< decltype(t2) >> );

    auto t5 = make_tensor< Shape< 3,3 >>( 
        1., 2., 3.,
        4., 5., 6.,
        7., 8., 9. );
//     // determinant(t3) ==
//     // 1*(5*9-6*8) - 2*(4*9-6*7) + 3*(4*8-5*7)
//     // 45 - 48 - 2*(36 - 42) + 3*(32 - 35)
//     // -3 + 12 - 9 == 0

    auto t6 = contract< 0, 1 >( t5 );
    assert( t6 == make_tensor< Shape<>>( 15. ));

    /**
     * Sub Tensor Tests
     */
    auto s1 = subtensor< seq< 1, 1 >>( t5 ); 
    assert(( s1 == make_tensor< Shape< 2, 2 >>( 
        1., 3., 
        7., 9. )));

//     /**
//      * Arithmetic tests
//      */
    auto a1 = make_tensor< Shape< 2, 2 >>( 2., 3., 5., 7. );
    auto a2 = make_tensor< Shape< 2, 2 >>( 7., 5., 3., 2. );
    assert(( a1 + a2 == make_tensor< Shape<2,2>>( 9., 8., 8., 9. )));

//     /**
//      * Multiplication Tests
//      */
    auto m5 = make_tensor<Shape<1,2>>( 5., 6. );
    auto m6 = make_tensor<Shape<2,1>>( 7., 8. );

    auto m5m6 = make_tensor< Shape<1,2,2,1>>(
        5. * 7., 5. * 8.,
        6. * 7., 6. * 8. );

    assert( m5 * m6 == m5m6 );

    auto u5 = make_tensor< Shape<2>>( 5., 6. );
    auto u6 = make_tensor< Shape<2>>( 7., 8. );

    auto u5u6 = make_tensor< Shape< 2,2 >>(
        5. * 7., 5. * 8.,
        6. * 7., 6. * 8. );

    assert( u5 * u6 == u5u6 );

//     /**
//      * Transpose tests
//      */
//     auto u5u6_t = make_tensor< shape< 2,2>>(
//         35., 42.,
//         40., 48. );

//     if( transpose(u7) != u5u6_t )
//         throw std::logic_error("FAIL: transpose test");
    
//     cout << "PASS: transpose test" << endl;

//     /**
//      * Scale tests
//      */
//     auto t8 = make_tensor< shape< 2, 3 >>(
//         4. *  5., 4. *  7.,
//         4. * 11., 4. * 13.,
//         4. * 17., 4. * 19. );

//     auto t9 = t8.scale( 1. / 4. );
//     static_assert( is_same_v< shape< 2, 3 >, shape_of< decltype( t9 )>> );

//     auto t8sby4 = make_tensor< shape< 2, 3 >>(
//         5., 7.,
//         11., 13.,
//         17., 19. );

//     if( t9 != t8sby4 )
//         throw std::logic_error("FAIL: scale test");
    
//     cout << "PASS: scale test" << endl;

//     /**
//      * Determinant tests
//      */
//     double d = determinant( t3 );

//     if( d != 0. )
//         throw std::logic_error("FAIL: determinant test");
    
//     cout << "PASS: determinant test" << endl;
    
//     auto d1 = make_tensor< shape< 1, 1 >>( 5. );

//     if( determinant( d1 ) != 5. )
//         throw std::logic_error("FAIL: determinant test 1x1");
    
//     cout << "PASS: determinant test 1x1" << endl;

//     auto d2 = make_tensor< shape< 2, 2 >>(
//         3., 2.,
//         2., 3. );

//     if( determinant( d2 ) != 5. )
//         throw std::logic_error("FAIL: determinant test 2x2");
    
//     cout << "PASS: determinant test 2x2" << endl;

//     /**
//      * Cofactor tests
//      */
//     auto c2 = cofactor( d2 );
//     static_assert( is_same_v< shape< 2, 2 >, shape_of< decltype( c2 )>> );

//     auto cofactor_c2 = make_tensor< shape< 2, 2 >>(
//         3., -2., 
//         -2., 3. );
    
//     if( c2 != cofactor_c2 )
//         throw std::logic_error("FAIL: cofactor test");

//     cout << "PASS: cofactor test" << endl;

//     /**
//      * Inverse tests
//      */
//     auto i2 = make_tensor< shape< 2, 2 >>(
//         5., 4.,
//         3., 4. );

//     auto i2i = inverse( i2 );
//     static_assert( is_same_v< shape< 2, 2 >, shape_of< decltype( i2i )>> );

//     auto inverse_i2 = make_tensor< shape< 2, 2 >>(
//         4. / 8., -4. / 8.,
//         -3. / 8., 5. / 8. );

// #ifndef NDEBUG
//     std::cout << "i2 ==\n" << i2i << "\ninverse_d2 ==\n" << inverse_i2 << std::endl;
// #endif

//     if( i2i != inverse_i2 )
//         throw std::logic_error("FAIL: inverse test");

//     cout << "PASS: inverse test" << endl;

    

    return EXIT_SUCCESS;
}