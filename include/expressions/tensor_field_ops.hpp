#ifndef __TENSOR_FIELD_OPS_HPP__
#define __TENSOR_FIELD_OPS_HPP__

#include "expressions/tensor_field.hpp"
#include "expressions/tensor_contraction.hpp"

#include <functional>

namespace expressions {

template< shape S, typename... Ts, typename... Us >
constexpr auto operator==( Tensor< S, Ts... > const& left, Tensor< S, Us... > const& right )
{ return equals_helper( left, right, make_seq< S::elements_size >{} ); }

template< shape S, typename... Ts, typename... Us >
constexpr auto operator!=( Tensor< S, Ts... > const& left, Tensor< S, Us... > const& right )
{ return not equals_helper( left, right, make_seq< S::elements_size >{} ); }

template< shape S, typename... Ts, typename... Us >
constexpr auto operator+( Tensor< S, Ts... > const& left, Tensor< S, Us... > const& right )
{ return plus_helper( left, right, make_seq< S::elements_size >{} ); }

template< shape S, typename... Ts, typename... Us >
constexpr auto operator-( Tensor< S, Ts... > const& left, Tensor< S, Us... > const& right )
{ return minus_helper( left, right, make_seq< S::elements_size >{} ); }


template< index I, typename V >
struct Subscript;

/**
 * Subscript of a tuple
 */
template< size_t I, typename... Ts >
struct Subscript< Index< I >, tuple< Ts... >> : Expression
{
    using tuple_type = tuple< Ts... >;
    using dependent_types = tuple< tuple_type >;
    using type = tuple_element_t< I, tuple_type >;
    using result_type = result_t< type >;

    template< size_t J >
    requires( J == 0 )
    constexpr tuple_type get_dependent() const
    { return tup; }
    
    constexpr result_type eval( variable_values const& values )
    { return eval( get< I >( tup ), values ); }

    constexpr Subscript( tuple_type const& tup ) : 
        tup{ tup } 
    { }

    tuple_type tup;
};

/**
 * Subscript of a tensor
 */
template< index I, typename ExprT >
requires( tensor_like< ExprT > )
struct Subscript< I, ExprT > : Expression
{ 
    using tensor_type = ExprT;
    using shape_type = shape_of_t< tensor_type >;
    using dependents_type = tuple< tensor_type >;
    using type = tuple_element_t< index_to_element_v< I, tensor_type >, tensor_type >; 
    using result_type = result_t< type >;

    template< size_t J >
    requires( J == 0 )
    constexpr tensor_type get_dependent() const
    { return ten; }

    constexpr result_type eval( variable_values const& values )
    { return eval( get< I >( ten ), values ); }

    constexpr Subscript( tensor_type const& ten ) : ten{ ten } { }

    tensor_type ten;
};

/**
 * Element of a tuple
 */
template< size_t I, typename TupleT >
struct Element : Expression 
{
    using tuple_type = TupleT;
    using type = tuple_element_t< I, tuple_type >;
    using result_type = result_t< type >;
    using dependents_type = tuple< tuple_type >;

    template< size_t J >
    requires( J == 0 )
    constexpr tuple_type get_dependent() const
    { return tup; }

    constexpr result_type eval( variable_values const& values )
    { return eval( get< I >( tup ), values ); }

    constexpr Element( tuple_type const& tup ) : tup{ tup } { }

    tuple_type tup;
};

// forward declaration of uncontract for shapes and indices
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

template< size_t E, size_t I, size_t J, tensor_like T, typename DimSeq >
struct ContractedElementHelper;

/**
 * a contracted element is the sum of the I-J diagonal of the uncontracted 
 * tensor
 * 
 * @tparam E is the element of the contracted tensor
 * @tparam I is the first contraction index
 * @tparam J is the second contraction index
 * @tparam Ls is an integer sequence of size equal to the diagonal I-J
 * 
 */
template< size_t E, size_t I, size_t J, tensor_like T, size_t... Ls >
struct ContractedElementHelper< E, I, J, T, seq< Ls... >> : 
    Sum< Subscript< uncontract_t< 
        element_to_index_t< E, contracted_shape_t< I, J, shape_of_t< T >>>, 
            I, J, Ls >, T >... >
{
    using base_type = Sum< Subscript< uncontract_t< 
        element_to_index_t< E, contracted_shape_t< I, J, shape_of_t< T >>>, 
            I, J, Ls >, T >... >;

    constexpr ContractedElementHelper( T const& ten ) : 
        base_type{ ( Ls, ten )... }
    { }
};

/**
 * Expression for the Eth element of an I, J contraction of T
 */
template< size_t E, size_t I, size_t J, tensor_like T >
requires( shape_at_v< I, shape_of_t< T >> == shape_at_v< J, shape_of_t< T >> )
struct ContractedElement : 
    ContractedElementHelper< E, I, J, T, make_seq< 
        shape_at_v< I, shape_of_t< T >>>>
{ 
    using base_type = ContractedElementHelper< E, I, J, T, make_seq< 
        shape_at_v< I, shape_of_t< T >>>>;

    constexpr ContractedElement( T const& tup ) : 
        base_type{ tup }
    { }
};

/**
 * Contraction helper
 */
template< size_t I, size_t J, tensor_like T, typename Seq >
struct ContractionHelper;


template< size_t I, size_t J, tensor_like T, size_t... Is >
struct ContractionHelper< I, J, T, seq< Is... >> : 
    Tensor< contracted_shape_t< I, J, shape_of_t< T >>, 
        ContractedElement< Is, I, J, T >... >
{
    using base_type = Tensor< contracted_shape_t< I, J, shape_of_t< T >>, 
        ContractedElement< Is, I, J, T >... >;
    
    constexpr ContractionHelper( T const& tup ) :
        base_type{ tup }
    { }
};

/**
 * Contraction of V along the Ith and Jth indices
 */
template< size_t I, size_t J, tensor_like T >
requires( isless( I, shape_of_t< T >::size ) and 
    isless( J, shape_of_t< T >::size ) and I != J )
struct Contract : ContractionHelper< I, J, T, 
    make_seq< contracted_shape_t< I, J, shape_of_t< T >>::elements_size >>
{
    using base_type = ContractionHelper< I, J, T, 
        make_seq< contracted_shape_t< I, J, shape_of_t< T >>::elements_size >>;

    constexpr Contract( T const& ten ) : base_type{ ten } { }
};

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
struct TensorProductElement : Product< 
    Element< left_prod_element_v< I, shape_of_t< LeftT >, shape_of_t< RightT >>, LeftT >,
    Element< right_prod_element_v< I, shape_of_t< LeftT >, shape_of_t< RightT >>, RightT >>
{ 
    using base_type = Product< 
        Element< left_prod_element_v< I, shape_of_t< LeftT >, shape_of_t< RightT >>, LeftT >,
        Element< right_prod_element_v< I, shape_of_t< LeftT >, shape_of_t< RightT >>, RightT >>;

    constexpr TensorProductElement( LeftT const& left, RightT const& right ) : 
        base_type{ left, right }
    { }
};

template< tensor_like LeftT, tensor_like RightT, typename Seq >
struct TensorProductHelper;

template< tensor_like LeftT, tensor_like RightT, size_t... Is >
struct TensorProductHelper< LeftT, RightT, seq< Is... >> : 
    Tensor< shape_cat_t< shape_of_t< LeftT >, shape_of_t< RightT >>, 
        TensorProductElement< Is, LeftT, RightT >... >
{
    using shape_type = shape_cat_t< 
        shape_of_t< LeftT >, shape_of_t< RightT >>;
    using base_type = Tensor< shape_type, 
        TensorProductElement< Is, LeftT, RightT >... >;
    
    constexpr TensorProductHelper( LeftT const& left, RightT const& right ) :
        base_type{ left, right }
    { }
};

template< typename ScalarT, tensor_like TensorT, typename Seq >
struct TensorScaleHelper;

template< typename ScalarT, tensor_like TensorT, size_t... Is >
requires( not tensor_like< ScalarT > )
struct TensorScaleHelper< ScalarT, TensorT, seq< Is... >> :
    Tensor< shape_of_t< TensorT >, Product< ScalarT, Element< Is, TensorT >>... >
{
    using base_type = Tensor< shape_of_t< TensorT >, 
        Product< ScalarT, Element< Is, TensorT >>... >;

    constexpr TensorScaleHelper( ScalarT const& scalar, TensorT const& ten ):
        base_type{ { scalar, Element< Is, TensorT >{ ten }}... }
    { }
};

struct tensor_product
{   
    template< tensor_like LeftT, tensor_like RightT >
    constexpr auto operator()( LeftT const& left, RightT const& right ) const
    { 
        using shape_type = shape_cat_t< 
            shape_of_t< LeftT >, shape_of_t< RightT >>;
        return TensorProductHelper< LeftT, RightT, 
            make_seq< shape_type::elements_size >>{ left, right };
    }

    template< tensor_like LeftT, typename RightT >
    requires( not tensor_like< RightT >)
    constexpr auto operator()( LeftT const& left, RightT const& right ) const
    { return TensorScaleHelper< RightT, LeftT, 
        make_seq< shape_of_t< LeftT >::element_size >>{ right, left }; }
    
    template< typename LeftT, tensor_like RightT >
    requires( not tensor_like< LeftT >)
    constexpr auto operator()( LeftT const& left, RightT const& right ) const
    { return TensorScaleHelper< LeftT, RightT, 
        make_seq< shape_of_t< LeftT >::element_size >>{ left, right }; }
};

template< typename T, typename... Ts >
requires( tensor_like< T > and (tensor_like< Ts > and ... ) and 
    expression< T > and ( expression< Ts > and ... ))
struct Product< T, Ts... > : Associative< tensor_product, T, Ts... >
{
    constexpr Product( T const& t, Ts const&... ts ) : 
        Associative< tensor_product, T, Ts... >{ t, ts... }
    { }
};

// static_assert( tensor_like< Scale< double, Vector< 1, double >>> );

template< tensor_like V, typename Seq >
struct FlattenedHelper;

template< tensor_like V, size_t... Is >
struct FlattenedHelper< V, seq< Is... >>
{ using type = tuple< tensor_element_t< Is, V >... >; };

template< tensor_like V >
struct Flattened
{ using type = FlattenedHelper< V, make_seq< shape_of_t< V >::elements_size >>::type; };

template< tensor_like V >
using flattened_t = Flattened< V >::type;

template< size_t I, tensor_like V, typename Seq >
struct DimRemovedBase;

template< size_t I, tensor_like V, size_t... Is >
struct DimRemovedBase< I, V, seq< Is... >>
{ 
    using input_shape = shape_of_t< V >;
    using output_shape = remove_dimension_t< I, input_shape >;
    using type = Tensor< output_shape, tensor_element_t< Is, V >... >;
    
    static constexpr type construct( V const& ten )
    { return { get_tensor_element< Is >( ten )... }; }
};

template< size_t I, tensor_like V >
requires( shape_at_v< I, shape_of_t< V >> == 1 )
struct DimRemoved : 
    DimRemovedBase< I, V, make_seq< shape_of_t< V >::elements_size >>
{ };

template< size_t I, tensor_like V >
using dim_removed_t = DimRemoved< I, V >::type;

template< tensor_like V, size_t... Is >
constexpr tuple< tensor_element_t< Is, V >... > flatten_helper( V ten, 
    seq< Is... > )
{ return { get_tensor_element< Is >( ten )... }; }

template< typename TupleT, shape S, size_t... Is >
constexpr Tensor< S, tuple_element_t< Is, TupleT >... > shapen_helper(
    TupleT const& tup, seq< Is... > )
{ return { get< Is >( tup )... }; }

namespace operators {

template< tensor_like A, tensor_like B >
constexpr Product< A, B > outer_product( A const& a, B const& b )
{ return { a, b }; }

template< size_t I, size_t J, tensor_like A >
constexpr Contract< I, J, A > contract( A a )
{ return { a }; }

template< typename ScalarT, tensor_like V >
constexpr Product< ScalarT, V > scale( ScalarT s, V ten )
{ return { s, ten }; }

template< tensor_like A, tensor_like B >
requires( shape_at_v< shape_of_t< A >::size - 1, shape_of_t< A >> ==
    shape_at_v< 0, shape_of_t< B >> )
constexpr auto matmul( A const& a, B const& b )
{
    static constexpr size_t I = shape_of_t< A >::size;
    return contract< I-1, I >( outer_product( a, b ));
}

template< tensor_like V >
constexpr flattened_t< V > flatten( V const& ten )
{ return flatten_helper( ten, make_seq< shape_of_t< V >::elements_size >{} ); } 

template< shape S, typename... Ts >
requires( S::elements_size == sizeof...( Ts ) )
constexpr Tensor< S, Ts... > shapen( tuple< Ts... > const& tup )
{ return shapen_helper( tup, make_seq< sizeof...( Ts )>{} ); }

// remove a unitary dimension from a tensor
template< size_t I, tensor_like V >
requires( shape_at_v< I, shape_of_t< V >> == 1 )
constexpr DimRemoved< I, V >::type remove_dim( V ten )
{ return DimRemoved< I, V >::construct( ten ); }

} // namespace operators


} // namespace expressions

#endif