#ifndef __TENSOR_FIELD_OPS_HPP__
#define __TENSOR_FIELD_OPS_HPP__

#include "expressions/tensor_field.hpp"
#include "expressions/tensor_contraction.hpp"

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

template< size_t I, size_t J, tensor_like V >
requires( isless( I, shape_of_t< V >::size ) and 
    isless( J, shape_of_t< V >::size ) and I != J )
struct Contract : DependsOn< V >
{ 
    static constexpr size_t First = min_v< I, J >;
    static constexpr size_t Second = max_v< I, J >;
    using result_type = contraction_t< First, Second, V >;

    result_type operator()( result_t< V > v )
    { return contraction< First, Second, V >::contract( v ); } 
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
    using type = product_of_t< tensor_element_t< left_prod_element_v< I, left_shape, right_shape >, LeftT >,
        tensor_element_t< right_prod_element_v< I, left_shape, right_shape >, RightT >>;
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
    using result_shape = shape_cat_t< shape_of_t< left_type >, 
        shape_of_t< right_type >>;

    using type = Tensor< result_shape, tensor_product_element_t< Is, left_type, right_type >... >;
    using result_type = Tensor< result_shape, result_t< tensor_product_element_t< Is, left_type, right_type >>... >;

    static constexpr result_type product( left_type left, right_type right )
    { 
        return {( left.template elem< left_prod_element_v< Is, left_shape, 
            right_shape >>() * right.template elem< right_prod_element_v< Is,
            left_shape, right_shape>>() )... };
    }
};

template< tensor_like E, tensor_like F >
struct TensorProduct;

template< shape TShape, typename... Ts, shape UShape, typename... Us >
struct TensorProduct< Tensor< TShape, Ts... >, Tensor< UShape, Us... >> :
    DependsOn< Tensor< TShape, Ts... >, Tensor< UShape, Us... >>
{
    using left_type = tensor_t< TShape, Ts... >;
    using right_type = tensor_t< UShape, Us... >;
    using left_shape = shape_of_t< left_type >;
    using right_shape = shape_of_t< right_type >;
    using helper_type = TensorProductHelper< left_type, right_type, 
        make_seq< left_shape::elements_size * right_shape::elements_size >>;

    using result_type = helper_type::result_type;
    using result_shape = helper_type::result_shape;

    constexpr result_type operator()( result_t< left_type > left, 
        result_t< right_type > right )
    { return helper_type::product( left, right ); }
};

template< tensor_like E >
struct Product< E > : DependsOn< E >
{
    using result_type = result_t< E >;
    constexpr result_type operator()( result_t< E > x )
    { return x; }
};

// TODO: this uses right-side precedence.  Convert to left precedence
// DT: does it matter for tenser outer products?
template< tensor_like E, tensor_like... Es >
struct Product< E, Es... > : DependsOn< E, Es... >
{
    using right_type = Product< Es... >;
    using helper_type = TensorProduct< E, typename right_type::result_type >;
    using result_type = helper_type::result_type;
    constexpr result_type operator()( result_t< E > x, result_t< Es >... xs )
    { return helper_type{}( x, Product< Es... >{}( xs... )); }
};

namespace operators {

template< size_t I, size_t J, tensor_like A >
constexpr auto contract( A a )
{ return Contract< I, J, A >{ a }; }

template< tensor_like LeftT, tensor_like RightT >
requires( static_expr< LeftT > and static_expr< RightT > )
constexpr auto operator*( LeftT left, RightT right )
{ return Product< LeftT, RightT >{}( left, right ); }

} // namespace operators

} // namespace expressions

#endif