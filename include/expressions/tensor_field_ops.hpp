#ifndef __TENSOR_FIELD_OPS_HPP__
#define __TENSOR_FIELD_OPS_HPP__

#include "expressions/tensor_field.hpp"
#include "expressions/tensor_contraction.hpp"

#include <functional>

namespace expressions {

template< index I, typename V >
struct Subscript;

template< size_t I, typename... Ts >
struct Subscript< Index< I >, tuple< Ts... >> : DependsOn< tuple< Ts... >>
{
    using type = tuple_element_t< I, tuple< Ts... >>;
    using result_type = result_t< type >;
    // constexpr type operator()( tuple< Ts... > x ) const
    // { return get< I >( x ); }
};

template< index I, shape S, typename... Es >
struct Subscript< I, Tensor< S, Es... >> : DependsOn< Tensor< S, Es... >>
{ 
    using type = tensor_index_t< I, Tensor< S, Es... >>; 
    using result_type = result_t< type >;
    // constexpr type operator()( Tensor< S, Es... > x )
    // { return x.template elem< index_to_element_v< I >>(); }
};

template< size_t I, typename V >
struct Element;

template< size_t I, typename... Ts >
struct Element< I, tuple< Ts... >> : DependsOn< tuple< Ts... >>
{
    using type = tuple_element_t< I, tuple< Ts... >>;
    using result_type = result_t< type >;
    // constexpr type operator()( tuple< Ts... > x ) const
    // { return get< I >( x ); }
};

template< size_t I, shape S, typename... Es >
struct Element< I, Tensor< S, Es... >> : DependsOn< Tensor< S, Es... >>
{ 
    using type = tensor_element_t< I, Tensor< S, Es... >>; 
    using result_type = result_t< type >;
    // constexpr type operator()( Tensor< S, Es... > x )
    // { return x.template elem< index_to_element_v< I >>(); }
};

template< typename E, tensor_like V >
struct Scale : Expression
{ using result_type = result_t< V >; };


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

template< size_t I, size_t... Is, tensor_like T >
struct sum_elements_helper< seq< I, Is... >, T >
{ static constexpr auto sum( T v )
    { return ::expressions::sum( get_tensor_element< I >( v ), 
        get_tensor_element< Is >( v )... ); } };

template< size_t E, size_t I, size_t J, tensor_like T >
struct contraction_element
{ 
    using input_shape = shape_of_t< T >;
    using output_shape = contracted_shape_t< I, J, input_shape >;
    static constexpr size_t contraction_size = shape_at_v< I, input_shape >;
    using elements_seq = contraction_element_selector< E, output_shape, I, J, 
        T, make_seq< contraction_size >>::elements_seq;

    static constexpr auto value( T v )
    { return sum_elements_helper< elements_seq, T >::sum( v ); }

    // static constexpr result_type value( result_t< T > v )
    // { return sum_elements_helper< elements_seq, T >::sum( v ); }
};


template< size_t I, size_t J, tensor_like V >
requires( isless( I, shape_of_t< V >::size ) and 
    isless( J, shape_of_t< V >::size ) and I != J )
struct Contract;

template< typename OutSeq, size_t I, size_t J, tensor_like T >
struct contraction_helper;

template< size_t... Es, size_t I, size_t J, tensor_like T >
struct contraction_helper< seq< Es... >, I, J, T >
{
    using input_shape = shape_of_t< T >;
    using output_shape = contracted_shape_t< I, J, input_shape >;

    using type = Contract< I, J, T >;

    static constexpr type contract( T v )
    { return { contraction_element< Es, I, J, T >::value( v )... }; }
};

template< size_t I, size_t J, tensor_like T >
struct contraction;


template< size_t I, size_t J, tensor_like T >
requires( not is_same_v< void, contracted_shape_t< I, J, shape_of_t< T >>> )
struct contraction< I, J, T >
{ 
    using tensor_type = T;
    using input_shape = shape_of_t< T >;
    using output_shape = contracted_shape_t< I, J, input_shape >;
    static constexpr size_t contraction_size = output_shape::elements_size;

    using type = contraction_helper< make_seq< output_shape::elements_size >, 
        I, J, tensor_type >::type;

    static constexpr type contract( T v )
    { return contraction_helper< make_seq< contraction_size >, I, J, 
        tensor_type >::contract( v ); }
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

// static_assert( is_same_v< contraction_t< 0, 1, t221 >, 
//     tensor_t< Shape< 1 >, Sum< float, float >>> );

// static_assert( is_same_v< contraction_t< 0, 1, t22 >, Sum< float, float >> );

// static_assert( is_same_v< contraction_t< 1, 2,
//     Tensor< Shape< 1, 2, 2 >, float, float, float, float >>, 
//     Tensor< Shape< 1 >, sum_t< float, float, float, float >>> );

// static_assert( is_same_v< contraction_t< 1, 2,
//     Tensor< Shape< 1, 2, 2 >, float, float, float, float >>,
//     Tensor< Shape< 1, 1 >, sum_t< float, float, float, float >>> );

#endif // DEBUG


template< size_t I, size_t J, tensor_like V >
requires( isless( I, shape_of_t< V >::size ) and 
    isless( J, shape_of_t< V >::size ) and I != J )
struct Contract : DependsOn< V >
{ 
    static constexpr size_t First = min_v< I, J >;
    static constexpr size_t Second = max_v< I, J >;

    static constexpr auto contract( V v )
    { return contraction< First, Second, V >::contract( v ); }

    constexpr auto operator()( V v )
    { return contract( v ); } 
};

template< size_t K, size_t I, size_t J, tensor_like E >
struct tensor_element< K, Contract< I, J, E >>
{ 
    using type = contraction_element< K, I, J, E >::type; 
    static constexpr type value( Contract< I, J, E > x )
    { return contraction_element< K, I, J, E >::value( x ); }
};

template< index D, shape LeftS, shape RightS >
struct left_prod_index
{
    using left_shape = LeftS;
    using right_shape = RightS;
    using result_shape = shape_cat_t< left_shape, right_shape >;
    using type = subindex_helper< D, 0, make_seq< left_shape::size >>::type;
};

template< index D, shape LeftS, shape RightS >
struct right_prod_index
{
    using left_shape = LeftS;
    using right_shape = RightS;
    using result_shape = shape_cat_t< left_shape, right_shape >;
    using type = subindex_helper< D, left_shape::size, make_seq< right_shape::size >>::type;
};

template< index D, shape LeftS, shape RightS >
using left_prod_index_t = left_prod_index< D, LeftS, RightS >::type;

template< index D, shape LeftS, shape RightS >
using right_prod_index_t = right_prod_index< D, LeftS, RightS >::type;

#ifndef NDEBUG
static_assert( is_same_v< left_prod_index_t< Index<2,3>, Shape<3>, Shape<4> >, Index<2>> );
static_assert( is_same_v< right_prod_index_t< Index<2,3>, Shape<3>, Shape<4> >, Index<3>> );
static_assert( is_same_v< left_prod_index_t< Index<2,3,4,5>, Shape<3,4,5>, Shape<6> >, Index<2,3,4>> );
static_assert( is_same_v< right_prod_index_t< Index<2,3,4,5>, Shape<3,4,5>, Shape<6> >, Index<5>> );
#endif

template< size_t I, shape LeftS, shape RightS >
struct left_prod_element
{ 
    using left_shape = LeftS;
    using right_shape = RightS;
    using result_shape = shape_cat_t< left_shape, right_shape >;
    using result_index_type = element_to_index_t< I, result_shape >;
    static constexpr size_t value = index_to_element_helper< 
        left_prod_index_t< result_index_type, left_shape, right_shape >, 
        left_shape >::value;
};

template< size_t I, shape LeftS, shape RightS >
struct right_prod_element
{ 
    using left_shape = LeftS;
    using right_shape = RightS;
    using result_shape = shape_cat_t< left_shape, right_shape >;
    using result_index_type = element_to_index_t< I, result_shape >;
    static constexpr size_t value = index_to_element_helper< 
        right_prod_index_t< result_index_type, left_shape, right_shape >, 
        right_shape >::value;
};

template< size_t I, shape LeftS, shape RightS >
constexpr size_t left_prod_element_v = 
    left_prod_element< I, LeftS, RightS >::value;

template< size_t I, shape LeftS, shape RightS >
constexpr size_t right_prod_element_v = 
    right_prod_element< I, LeftS, RightS >::value;

template< size_t I, tensor_like LeftT, tensor_like RightT >
struct tensor_product_element
{ 
    using left_shape = shape_of_t< LeftT >;
    using right_shape = shape_of_t< RightT >;
    static constexpr size_t left_element = left_prod_element_v< I, 
        left_shape, right_shape >;
    static constexpr size_t right_element = right_prod_element_v< I, 
        left_shape, right_shape >;
    using type = Product< tensor_element_t< left_element, LeftT >, 
        tensor_element_t< right_element, RightT >>;
};

template< size_t I, tensor_like LeftT, tensor_like RightT >
using tensor_product_element_t = tensor_product_element< I, LeftT, RightT >::type;

template< tensor_like LeftT, tensor_like RightT, typename Seq >
struct TensorProductHelper;

template< tensor_like LeftT, tensor_like RightT, size_t... Is >
struct TensorProductHelper< LeftT, RightT, seq< Is... >>
{
    using left_type = LeftT;
    using right_type = RightT;
    using left_shape = shape_of_t< left_type >;
    using right_shape = shape_of_t< right_type >;
    using shape_type = shape_cat_t< shape_of_t< left_type >, 
        shape_of_t< right_type >>;

    using type = Tensor< shape_type, tensor_product_element_t< Is, left_type, right_type >... >;
    using result_type = result_t< type >;

    template< size_t I >
    static constexpr size_t left_element = left_prod_element_v< I, 
        left_shape, right_shape >;

    template< size_t I >
    static constexpr size_t right_element = right_prod_element_v< I, 
        left_shape, right_shape >;

    static constexpr result_type product( result_t< left_type > left, 
        result_t< right_type > right )
    { return {( get_tensor_element< left_element< Is >>( left ) * 
            get_tensor_element< right_element< Is >>( right ))... }; }
};

template< shape S, typename... Ts >
struct Product< Tensor< S, Ts... >> : DependsOn< Tensor< S, Ts... >>
{
    using shape_type = Tensor< S, Ts... >::shape_type;
    using result_type = result_t< Tensor< S, Ts... >>;

    Product( Tensor< S, Ts... > arg ) : DependsOn< Tensor< S, Ts... >>{ arg }
    { }
};

template< shape S, typename... Ts, typename... Es >
struct Product< Tensor< S, Ts... >, Es... > : 
    DependsOn< Tensor< S, Ts... >, Es... >
{
    using left_type = Tensor< S, Ts... >;
    using left_shape = shape_of_t< left_type >;
    using right_type = Product< Es... >;
    using right_shape = shape_of_t< right_type >;
    // not sure this should be result_type...
    using helper_type = TensorProductHelper< left_type, right_type, 
        make_seq< left_shape::elements_size * right_shape::elements_size >>;

    constexpr left_type left() const { return get_dependent< 0 >( *this ); }
    constexpr right_type right() const 
    { return right_helper( make_seq< sizeof...( Es )>{} ); }

    template< size_t... Is >
    constexpr right_type right_helper( seq< Is... > ) const
    { return { get_dependent< 1 + Is >( *this )... }; }
    
    // required for tensor_like objects
    using shape_type = helper_type::shape_type;
    using result_type = helper_type::result_type;

    Product( Tensor< S, Ts... > arg, Es... es ) : 
        DependsOn< Tensor< S, Ts... >, Es... >{ arg, es... }
    { }
};

template< size_t I, tensor_like E >
struct tensor_element< I, Product< E >>
{ 
    using type = tensor_element< I, E >::type; 
    static constexpr type value( Product< E > x )
    { return get_tensor_element< I >( get< 0 >( x.exprs )); }
};

template< size_t I, tensor_like E, tensor_like... Es >
struct tensor_element< I, Product< E, Es... >>
{ 
    using left_type = E;
    using right_type = Product< Es... >;
    using type = tensor_product_element< I, left_type, right_type >::type; 
    static constexpr type value( Product< E, Es... > x )
    { return get_tensor_element< I >( get< 0 >( x.exprs )); }
};

template< shape S, typename... Ts >
struct Invoker< Tensor< S, Ts... >>
{
    Tensor< S, result_t< Ts >... > operator()( Tensor< S, Ts... > ten, 
        variable_values const& values )
    { return helper( ten, values, make_seq< sizeof...( Ts )>{} ); }

    template< size_t... Is >
    Tensor< S, result_t< Ts >... > helper( Tensor< S, Ts... > ten, 
        variable_values const& values, seq< Is... > )
    { return { invoke( get_tensor_element< Is >( ten ), values )... }; }
};

template< shape S, typename... Ts, typename... Es >
requires( isgreater( sizeof...( Es ), 0 ))
struct Invoker< Product< Tensor< S, Ts... >, Es... >>
{
    using product_type = Product< Tensor< S, Ts... >, Es... >;
    using result_type = product_type::result_type;
    using helper_type = product_type::helper_type;

    result_type operator()( product_type expr, variable_values const& values )
    { 
        auto left = invoke( expr.left(), values );
        auto right = invoke( expr.right(), values );

        return helper_type::product( left, right );
    }
};


namespace operators {

template< size_t I, size_t J, tensor_like A >
constexpr auto contract( A a )
{ return Contract< I, J, A >{ a }; }

} // namespace operators


} // namespace expressions

#endif