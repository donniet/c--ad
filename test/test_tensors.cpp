#include "tensors.hpp"

#include <print>
#include <stdexcept>
#include <cassert>

/*
TODO: test something like this:
tensors::contract<0UL, 1UL, 
    tensors::Tensor<tensors::Shape<2, 2, 3>, 
        long double, long double, long double, 
        long double, long double, long double, 

        Length, Length, Length, 
        Length, Length, Length>>
 */

using std::is_same_v;
using std::println;

using namespace tensors;

// forward decl.
void test_contraction();
void test_subtensor();
void test_multiplication();
void test_transpose();
void test_determinant();
void test_cofactor();
void test_inverse();

int main( int ac, char* av[] )
{  
    test_contraction();
    test_subtensor();
    test_multiplication();
    test_transpose();
    test_determinant();
    test_cofactor();
    test_inverse();

    return EXIT_SUCCESS;
}

void test_contraction()
{
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
}

void test_subtensor()
{
    auto t5 = make_tensor< Shape< 3,3 >>( 
        1., 2., 3.,
        4., 5., 6.,
        7., 8., 9. );

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
}

void test_multiplication()
{
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
}

void test_transpose()
{
    auto u5u6 = make_tensor< Shape< 2,2 >>(
        5. * 7., 5. * 8.,
        6. * 7., 6. * 8. );

    auto u5u6_t = make_tensor< Shape< 2,2>>(
        35., 42.,
        40., 48. );

    assert(( transpose< 0, 1 >( u5u6 ) == u5u6_t ));

    auto t5_t = make_tensor< Shape< 3,3 >>(
        1., 4., 7.,
        2., 5., 8.,
        3., 6., 9. );

    auto t5 = make_tensor< Shape< 3,3 >>( 
        1., 2., 3.,
        4., 5., 6.,
        7., 8., 9. );

    assert(( transpose< 0, 1 >( t5 ) == t5_t ));
}

void test_determinant()
{
    auto t5 = make_tensor< Shape< 3,3 >>( 
        1., 2., 3.,
        4., 5., 6.,
        7., 8., 9. );

    assert( det( t5 ) == 1*5*9 - 1*8*6 - 4*2*9 + 4*8*3 + 7*2*6 - 7*5*3 );

    auto d2 = make_tensor< Shape< 2, 2 >>(
        3., 2.,
        2., 3. );

    assert( det( d2 ) == 3*3 - 2*2 );
}

void test_cofactor()
{
    auto d2 = make_tensor< Shape< 2, 2 >>(
        3., 2.,
        2., 3. );

    auto cofactor_d2 = make_tensor< Shape< 2, 2 >>(
        3., -2., 
        -2., 3. );

    auto d2_co = cofactor( d2 );

    // println( "d2_co:\n{} {}\n{} {}", get< 0 >( d2_co ), get< 1 >( d2_co ), 
    //     get< 2 >( d2_co ), get< 3 >( d2_co ));
    assert( cofactor( d2 ) == cofactor_d2 );
}

void test_inverse()
{

    auto i2 = make_tensor< Shape< 2, 2 >>(
        5., 4.,
        3., 4. );

    assert(( inverse( i2 ) == make_tensor< Shape< 2, 2 >>( 
        4. / 8., -4. / 8.,
        -3. / 8., 5. / 8. )));

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

    assert(( matmul( i2, inverse( i2 )) == make_tensor< Shape< 2, 2 >>( 
        1., 0.,
        0., 1. )));
}
