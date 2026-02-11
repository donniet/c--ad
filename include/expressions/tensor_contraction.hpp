#ifndef __TENSOR_CONTRACTION_HPP__
#define __TENSOR_CONTRACTION_HPP__

#include "expressions/expression_ops.hpp"
#include "expressions/tensor_field.hpp"

namespace expressions {

template< typename X, size_t I, size_t J, size_t N >
struct uncontract;

// NOTE: DT: is there a better way to do this?!?
template< size_t... Ks, size_t I, size_t J, size_t N >
requires ( I < J and I < sizeof...( Ks ) + 2 and J < sizeof...( Ks ) + 2 )
struct uncontract< Shape< Ks... >, I, J, N >
{ using type = insert_at_t< insert_at_t< Shape< Ks... >, I, N >, J, N >; };

template< size_t... Ks, size_t I, size_t J, size_t N >
requires ( I < J and I < sizeof...( Ks ) + 2 and J < sizeof...( Ks ) + 2 )
struct uncontract< Index< Ks... >, I, J, N >
{ using type = insert_at_t< insert_at_t< Index< Ks... >, I, N >, J, N >; };

/**
 * Inserts a value of N into two positions of a shape<...> or index<...> which
 * represents the uncontracted shape (or index) of a tensor
 * 
 * @tparam X the shape<...> or index<...> object to be uncontracted
 * @tparam I first index to uncontract
 * @tparam J second index to uncontract
 * @tparam N the size (or subscript value) to insert at indices I and J
 */
template< typename X, size_t I, size_t J, size_t N >
using uncontract_t = uncontract< X, min_v< I, J >, max_v< I, J >, N >::type;

#ifndef NDEBUG
static_assert( is_same_v< uncontract_t< Shape< 0,2 >, 1, 2, 1 >, Shape< 0,1,1,2 >> );
static_assert( is_same_v< uncontract_t< Shape<9>,1,2,1 >, Shape<9,1,1>> );
static_assert( is_same_v< uncontract_t< Index<1>,1,2,0 >, Index<1,0,0>> );
#endif

template< size_t I, shape S >
struct remove_dimension;

template< size_t I, shape S, typename Seq >
struct remove_dimension_helper;

template< size_t I, shape S, size_t... Ks >
struct remove_dimension_helper< I, S, seq< Ks... >>
{ using type = Shape< shape_at_v<( Ks < I ? Ks : Ks+1 ), S >... >; };

template< size_t I, size_t... Js >
struct remove_dimension< I, Shape< Js... >>
{ using type = remove_dimension_helper< I, Shape< Js... >, make_seq< sizeof...( Js )-1 >>::type; };

/**
 * remove_dimension removes dimension I from Shape
 * 
 * @tparam I dimension to be removed
 * @tparam Shape to remove dimension from
 * @returns Shape with dimension I removed 
 */
template< size_t I, typename S >
using remove_dimension_t = remove_dimension< I, S >::type;

// tests of Remove Dimension
namespace test {
static_assert( is_same_v< Shape< 0,1,2,4 >, remove_dimension_t< 3, Shape< 0,1,2,3,4 >>> );
} // namespace test

template< size_t I, size_t J, shape S >
struct contracted_shape
{ using type = remove_dimension_t< I, remove_dimension_t< J, S >>; };

/**
 * contracted_shape computes the shape of a tensor contracted on indices I and J
 * 
 * @tparam I first index to remove
 * @tparam J second index to remove
 * @tparam S shape of the original tensor
 */
template< size_t I, size_t J, shape S >
using contracted_shape_t = contracted_shape< I, J, S >::type;

template< size_t E, typename OutS, size_t I, size_t J, tensor_like T, typename DimSeq >
struct contraction_element_selector;

template< size_t E, shape OutS, size_t I, size_t J, tensor_like T, size_t... Ls >
struct contraction_element_selector< E, OutS, I, J, T, seq< Ls... >>
{
    // get the subscripts of the contracted element
    using contracted_index = element_to_index_t< E, OutS >;
    // insert Dim into positions I and J of the contracted index
    template< size_t L >
    using uncontracted_index = uncontract_t< contracted_index, I, J, L >;

    using elements_seq = seq< index_to_element_v< uncontracted_index< Ls >, T >... >;
};

template< typename Seq, tensor_like T >
struct sum_elements_helper;

template< size_t... Is, tensor_like T >
struct sum_elements_helper< seq< Is... >, T >
{ 
    using input_shape = shape_of_t< T >;
    using type = Sum< tensor_element_t< Is, T >... >; 

    template< typename... Us >
    static constexpr type sum( tensor_t< input_shape , Us... > v )
    { return { v.template elem< Is >()... }; }
};

template< size_t E, size_t I, size_t J, tensor_like T >
struct contraction_element
{ 
    using input_shape = shape_of_t< T >;
    using output_shape = contracted_shape_t< I, J, input_shape >;
    static constexpr size_t contraction_size = shape_at_v< I, input_shape >;
    using elements_seq = contraction_element_selector< E, output_shape, I, J, 
        T, make_seq< contraction_size >>::elements_seq;
    using type = sum_elements_helper< elements_seq, T >::type;

    template< typename... Us >
    static constexpr type sum_elements( tensor_t< input_shape, Us... > v )
    { return sum_elements_helper< elements_seq, T >::sum( v ); }
};


template< typename OutSeq, size_t I, size_t J, tensor_like T >
struct contraction_helper;

template< size_t... Es, size_t I, size_t J, tensor_like T >
struct contraction_helper< seq< Es... >, I, J, T >
{
    using input_shape = shape_of_t< T >;
    using output_shape = contracted_shape_t< I, J, input_shape >;

    using type = tensor_t< output_shape, 
        typename contraction_element< Es, I, J, T >::type... >;

    template< typename... Us >
    static constexpr type contract_helper( 
        tensor_t< input_shape, Us... > v )
    { return { contraction_element< Es, I, J, T >::sum_elements( v )... }; }

};

template< size_t I, size_t J, tensor_like T >
struct contraction;

template< size_t I, size_t J, tensor_like T >
requires( not is_same_v< void, contracted_shape_t< I, J, shape_of_t< T >>> )
struct contraction< I, J, T >
{ 
    using input_shape = shape_of_t< T >;
    using output_shape = contracted_shape_t< I, J, input_shape >;
    static constexpr size_t contraction_size = output_shape::elements_size;

    using type = contraction_helper< make_seq< output_shape::elements_size >, 
        I, J, T >::type;

    template< typename... Us >
    static constexpr type contract( tensor_t< input_shape, Us... > v )
    { return contraction_helper< make_seq< contraction_size >, I, J, T >::
        contract_helper( v ); }
};

// this contraction results in a singular value
template< size_t I, size_t J, tensor_like T >
requires( is_same_v< void, contracted_shape_t< I, J, shape_of_t< T >>> )
struct contraction< I, J, T >
{ using type = contraction_element< 0, I, J, T >::type; };

/**
 * a contraction of tensor T on indices I and J
 */
template< size_t I, size_t J, tensor_like T >
requires( isless( I, J ) and 
    isless( I, shape_of_t< T >::size ) and 
    isless( J, shape_of_t< T >::size ) and 
    shape_at_v< I, shape_of_t< T >> == shape_at_v< J, shape_of_t< T >> )
using contraction_t = contraction< I, J, T >::type;

#ifndef NDEBUG
using t22 = tensor_t< Shape< 2, 2 >, float, float, float, float >;
using t221 = tensor_t< Shape< 2, 2, 1 >, float, float, float, float >;

static_assert( is_same_v< contraction_t< 0, 1, t221 >, 
    tensor_t< Shape< 1 >, Sum< float, float >>> );

static_assert( is_same_v< contraction_t< 0, 1, t22 >, Sum< float, float >> );

// static_assert( is_same_v< contraction_t< 1, 2,
//     Tensor< Shape< 1, 2, 2 >, float, float, float, float >>, 
//     Tensor< Shape< 1 >, sum_t< float, float, float, float >>> );

// static_assert( is_same_v< contraction_t< 1, 2,
//     Tensor< Shape< 1, 2, 2 >, float, float, float, float >>,
//     Tensor< Shape< 1, 1 >, sum_t< float, float, float, float >>> );

#endif // DEBUG


} // namespace expressions

#endif