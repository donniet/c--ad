#ifndef __TENSOR_FIELD_OPS_HPP__
#define __TENSOR_FIELD_OPS_HPP__

#include "expressions/tensor_field.hpp"
#include "expressions/tensor_contraction.hpp"

#include <functional>

namespace expressions {

/**
 * invoking a tensor creates a new tensor of the evaluated elements
 */
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

template< index I, typename V >
struct Subscript;

/**
 * Subscript of a tuple
 */
template< size_t I, typename... Ts >
struct Subscript< Index< I >, tuple< Ts... >> : DependsOn< tuple< Ts... >>
{
    using tuple_type = tuple< Ts... >;
    using type = tuple_element_t< I, tuple_type >;
    using result_type = result_t< type >;

    Subscript( tuple_type const& tup ) : DependsOn< tuple_type >{ tup } { }
};

/**
 * Subscript of a tensor
 */
template< index I, shape S, typename... Es >
struct Subscript< I, Tensor< S, Es... >> : DependsOn< Tensor< S, Es... >>
{ 
    using tensor_type = Tensor< S, Es... >;
    using type = tensor_index_t< I, tensor_type >; 
    using result_type = result_t< type >;

    Subscript( tensor_type const& ten ) : DependsOn< tensor_type >{ ten } { }
};

/**
 * Invoker for the subscript operation
 */
template< index I, typename V >
struct Invoker< Subscript< I, V >>
{
    using subscript_type = Subscript< I, V >;
    using result_type = tensor_index_t< I, result_t< V >>;

    result_type operator()( subscript_type const& sub, 
        variable_values const& values )
    { return get_tensor_index< I >( invoke( sub.tensor(), values )); }
};

/**
 * Element of a tuple
 */
template< size_t I, typename... Ts >
struct Element< I, tuple< Ts... >> : DependsOn< tuple< Ts... >>
{
    using tuple_type = tuple< Ts... >;
    using type = tuple_element_t< I, tuple_type >;
    using result_type = result_t< type >;

    constexpr tuple_type tup() const { return get_dependent< 0 >( *this ); }

    Element( tuple_type const& tup ) : DependsOn< tuple_type >{ tup } { }
};

/**
 * Element of a tensor
 */
template< size_t I, typename T >
requires( tensor_like< T > )
struct Element< I, T > : DependsOn< T >
{ 
    using tensor_type = T;
    using type = tensor_element_t< I, tensor_type >; 
    using result_type = result_t< type >;

    constexpr tensor_type tensor() const { return get_dependent< 0 >( *this ); }

    Element( tensor_type const& tup ) : DependsOn< tensor_type >{ tup } { }
};

/**
 * Invoker for tuple element
 */
template< size_t I, typename... Ts >
struct Invoker< Element< I, tuple< Ts... >>>
{
    using tuple_type = tuple< Ts... >;
    using element_type = Element< I, tuple_type >;
    using result_type = tuple_element_t< I, result_t< tuple_type >>;

    result_type operator()( element_type const& el, 
        variable_values const& values )
    { return get< I >( invoke( el.tup(), values )); }
};

/** 
 * invoker for tensor element 
 */
template< size_t I, typename T >
requires( tensor_like< T > )
struct Invoker< Element< I, T >>
{
    using tensor_type = T;
    using element_type = Element< I, tensor_type >;
    using result_type = tensor_element_t< I, result_t< tensor_type >>;

    result_type operator()( element_type const& el, 
        variable_values const& values )
    { return get_tensor_element< I >( invoke( el.tensor(), values )); }
};

template< typename E, tensor_like V >
struct Scale : Expression
{ 
    using result_type = result_t< V >; 
    using shape_type = shape_of_t< V >;

};

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
    using tensor_type = T;
    using contracted_index = element_to_index_t< E, OutS >;
    // insert Dim into positions I and J of the contracted index
    template< size_t L >
    using uncontracted_index = uncontract_t< contracted_index, I, J, L >;

    template< size_t L >
    static constexpr size_t uncontracted_element_v = 
        index_to_element_v< uncontracted_index< L >, tensor_type >;

    using elements_seq = seq< uncontracted_element_v< Ls >... >;
    using type = Sum< tensor_index_t< uncontracted_index< Ls >, tensor_type >... >;
    using result_type = result_t< type >;

    // this can be invoked by ContractionElement
    constexpr type value() const
    { return { get_tensor_element< uncontracted_element_v< Ls >>( _ten )... }; }

    constexpr contraction_element_selector( tensor_type const& ten ) : 
        _ten{ ten }
    { }

    tensor_type _ten;
};

/**
 * Expression for the Eth element of an I, J contraction of T
 */
template< size_t E, size_t I, size_t J, tensor_like T >
struct ContractionElement : DependsOn< T >
{ 
    using tensor_type = T;
    using input_shape = shape_of_t< tensor_type >;
    using output_shape = contracted_shape_t< I, J, input_shape >;
    static constexpr size_t contraction_size = shape_at_v< I, input_shape >;
    using elements_seq = contraction_element_selector< E, output_shape, I, J, 
        T, make_seq< contraction_size >>::elements_seq;
    using proxy_type = contraction_element_selector< E, output_shape, I, J, 
        T, make_seq< contraction_size >>;
    using type = proxy_type::type;

    // required to make this an expression
    using result_type = result_t< type >;

    constexpr tensor_type tensor() const
    { return get_dependent< 0 >( *this ); }

    constexpr type value() const 
    { return _proxy.value(); }

    ContractionElement( tensor_type const& tup ) : 
        DependsOn< tensor_type >{ tup }, _proxy{ tup }
    { }

    proxy_type _proxy;
};

/**
 * Invoker for ContractionElements
 */
template< size_t E, size_t I, size_t J, tensor_like T >
struct Invoker< ContractionElement< E, I, J, T >>
{
    using tensor_type = T;
    using element_type = ContractionElement< E, I, J, tensor_type >;
    using result_type = element_type::result_type;
    using elements_seq = element_type::elements_seq;

    constexpr result_type operator()( element_type const& el, variable_values const& values )
    { return invoke( el.value(), values ); }
};

template< typename OutSeq, size_t I, size_t J, tensor_like T >
struct contraction_helper;

/**
 * Contraction of V along the Ith and Jth indices
 */
template< size_t I, size_t J, tensor_like V >
requires( isless( I, shape_of_t< V >::size ) and 
    isless( J, shape_of_t< V >::size ) and I != J )
struct Contract : DependsOn< V >
{
    using tensor_type = V;
    using input_shape_type = shape_of_t< tensor_type >;
    using shape_type = contracted_shape_t< I, J, input_shape_type >;
    using helper_type = contraction_helper< 
        make_seq< shape_type::elements_size >, I, J, tensor_type >;

    // required to make this a tensor
    using result_type = helper_type::result_type;

    constexpr tensor_type tensor() const { return get_dependent< 0 >( *this ); }

    Contract( tensor_type const& ten ) : DependsOn< tensor_type >{ ten } { }
};


template< size_t... Es, size_t I, size_t J, tensor_like T >
struct contraction_helper< seq< Es... >, I, J, T >
{
    using tensor_type = T;
    using input_shape = shape_of_t< tensor_type >;
    using shape_type = contracted_shape_t< I, J, input_shape >;
    using type = Tensor< shape_type, ContractionElement< Es, I, J, tensor_type >... >;
    using result_type = result_t< type >;
};

/**
 * tensor element of a contracted tensor
 */
template< size_t E, size_t I, size_t J, typename T >
struct tensor_element< E, Contract< I, J, T >>
{
    using tensor_type = T;
    using contract_type = Contract< I, J, tensor_type >;
    // a tensor of ContractionElements
    using type = contract_type::result_type;

    static constexpr size_t elements_size = 
        shape_of_t< contract_type >::elements_size;
    
    static constexpr type value( contract_type const& x )
    { return helper( x, make_seq< elements_size >{} ); }

    // ContractionElements take the original tensor as a constructor param
    template< size_t... Is >
    static constexpr type helper( contract_type const& x, seq< Is... > )
    { return { get_dependent< Is-Is >( x )... }; } // always returns x.tensor()
};

template< size_t E, size_t I, size_t J, tensor_like T >
constexpr ContractionElement< E, I, J, T > get_tensor_element( Contract< I, J, T > const& x )
{ return { x.tensor() }; }

template< size_t I, size_t J, tensor_like V >
struct Invoker< Contract< I, J, V >>
{
    using tensor_type = V;
    using contract_type = Contract< I, J, tensor_type >;
    using result_type = contract_type::result_type;
    using shape_type = contract_type::shape_type;

    constexpr result_type operator()( contract_type const& con, variable_values const& values )
    { return helper( con, values, make_seq< shape_type::elements_size >{} ); }

    template< size_t... Is >
    constexpr result_type helper( contract_type const& con, 
        variable_values const& values, seq< Is... > )
    { return { invoke( get_tensor_element< Is >( con ), values )... }; }
    
};

// template< size_t I, size_t J, tensor_like T >
// struct contraction;


// template< size_t I, size_t J, tensor_like T >
// requires( not is_same_v< void, contracted_shape_t< I, J, shape_of_t< T >>> )
// struct contraction< I, J, T >
// { 
//     using tensor_type = T;
//     using input_shape = shape_of_t< T >;
//     using shape_type = contracted_shape_t< I, J, input_shape >;
//     static constexpr size_t contraction_size = shape_type::elements_size;

//     using type = contraction_helper< make_seq< shape_type::elements_size >, 
//         I, J, tensor_type >::type;

//     static constexpr type contract( T v )
//     { return contraction_helper< make_seq< contraction_size >, I, J, 
//         tensor_type >::contract( v ); }
// };

// // this contraction results in a singular value
// template< size_t I, size_t J, tensor_like T >
// requires( is_same_v< void, contracted_shape_t< I, J, shape_of_t< T >>> )
// struct contraction< I, J, T >
// { using type = contraction_element< 0, I, J, T >::type; };

// /**
//  * a contraction of tensor T on indices I and J
//  */
// template< size_t I, size_t J, tensor_like T >
// requires( isless( I, J ) and 
//     isless( I, shape_of_t< T >::size ) and 
//     isless( J, shape_of_t< T >::size ) and 
//     shape_at_v< I, shape_of_t< T >> == shape_at_v< J, shape_of_t< T >> )
// using contraction_t = contraction< I, J, T >::type;

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
struct TensorProductElement : DependsOn< LeftT, RightT >
{ 
    using left_type = LeftT;
    using right_type = RightT;
    using left_shape = shape_of_t< LeftT >;
    using right_shape = shape_of_t< RightT >;

    static constexpr size_t left_element = left_prod_element_v< I, 
        left_shape, right_shape >;
    static constexpr size_t right_element = right_prod_element_v< I, 
        left_shape, right_shape >;
    using type = Product< Element< left_element, LeftT >, 
        Element< right_element, RightT >>;

    // required for expressions
    using result_type = result_t< type >;

    constexpr left_type left() const
    { return get_dependent< 0 >( *this ); }
    
    constexpr right_type right() const
    { return get_dependent< 1 >( *this ); }

    // this can be invoked in place of the parent expression
    constexpr type value() const
    { return {{ left() }, { right() }}; }

    TensorProductElement( left_type left, right_type right ) : 
        DependsOn< left_type, right_type >{ left, right }
    { }
};

template< size_t I, tensor_like LeftT, tensor_like RightT >
struct Invoker< TensorProductElement< I, LeftT, RightT >>
{
    using left_type = LeftT;
    using right_type = RightT;
    using product_element_type = TensorProductElement< I, LeftT, RightT >;
    using result_type = product_element_type::result_type;

    constexpr result_type operator()( product_element_type const& el, 
        variable_values const& values )
    { return invoke( el.value(), values ); }
};

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

    using type = Tensor< shape_type, TensorProductElement< Is, left_type, right_type >... >;
    using result_type = result_t< type >;

    template< size_t I >
    static constexpr size_t left_element = left_prod_element_v< I, 
        left_shape, right_shape >;

    template< size_t I >
    static constexpr size_t right_element = right_prod_element_v< I, 
        left_shape, right_shape >;
};

template< shape S, typename... Ts >
struct Product< Tensor< S, Ts... >> : DependsOn< Tensor< S, Ts... >>
{
    using tensor_type = Tensor< S, Ts... >;
    using shape_type = Tensor< S, Ts... >::shape_type;
    using result_type = result_t< Tensor< S, Ts... >>;

    constexpr tensor_type left() const { return get_dependent< 0 >( *this ); }

    constexpr Product( Tensor< S, Ts... > arg ) : DependsOn< Tensor< S, Ts... >>{ arg }
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

    static constexpr size_t elements_size = shape_type::elements_size;

    Product( Tensor< S, Ts... > arg, Es... es ) : 
        DependsOn< Tensor< S, Ts... >, Es... >{ arg, es... }
    { }
};

template< size_t I, shape S, typename... Ts >
struct tensor_element< I, Product< Tensor< S, Ts... >>>
{
    using tensor_type = Tensor< S, Ts... >;
    using product_type = Product< tensor_type >;
    using type = tensor_element< I, tensor_type >::type;
    static constexpr type value( product_type const& x )
    { return tensor_element< I, tensor_type >::value( x.tensor() ); }
};

template< size_t I, tensor_like T, typename... Es >
requires( isgreater( sizeof...( Es ), 0 ))
struct tensor_element< I, Product< T, Es... >>
{
    using left_type = T;
    using right_type = Product< Es... >;
    using product_type = Product< T, Es... >;
    using type = TensorProductElement< I, left_type, right_type >;

    static constexpr type value( product_type const& x )
    { return { x.left(), x.right() }; }
};

template< shape S, typename... Ts, typename... Es >
requires( isgreater( sizeof...( Es ), 0 ))
struct Invoker< Product< Tensor< S, Ts... >, Es... >>
{
    using product_type = Product< Tensor< S, Ts... >, Es... >;
    using result_type = product_type::result_type;
    using helper_type = product_type::helper_type;

    static constexpr size_t elements_size = product_type::elements_size;

    constexpr result_type operator()( product_type const& expr, 
        variable_values const& values )
    { return helper( expr, values, make_seq< elements_size >{} ); }

    template< size_t... Is >
    constexpr result_type helper( product_type const& expr, 
        variable_values const& values, seq< Is... > )
    { return { invoke( get_tensor_element< Is >( expr ), values )... }; }
};


namespace operators {

template< size_t I, size_t J, tensor_like A >
constexpr auto contract( A a )
{ return Contract< I, J, A >{ a }; }

} // namespace operators


} // namespace expressions

#endif