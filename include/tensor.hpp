/**
 * Tensors for expressions
 */

#ifndef __TENSOR_HPP__
#define __TENSOR_HPP__

#include "utility.hpp"

#include <utility>
#include <tuple>
#include <type_traits>

using std::size_t;

namespace tensor {

// setup our test namespace
namespace test { using std::is_same_v; } 

using std::get;
using std::tuple;

template< typename... Ts >
using values_of = tuple< Ts... >;

template< typename... Ts >
values_of< Ts... > make_values_of( Ts... ts )
{ return { ts... }; }

/**
 * index is a compile-time reference to the indices of a tensor
 */
template< size_t... Is >
struct index
{ 
    static constexpr size_t size = sizeof...( Is );
};

template< size_t... Is >
struct shape 
{ 
    static constexpr size_t size = sizeof...( Is );
    static constexpr size_t elements_size = ( Is * ... );
};

// test shapes and indices
namespace test {
using shape123 = shape<1,2,3>;
using shape1234 = shape<1,2,3,4>;
using shape3 = shape<3>;
using shape4 = shape<4>;
using shape911 = shape<9,1,1>;
using shape9 = shape<9>;
using shape124 = shape<1,2,4>;
using index012 = index<0,1,2>;
using index123 = index<1,2,3>;
} // namespace test

/**
 * ShapeCat concatenates two shape objects together
 */
template< typename Shape1, typename Shape2 >
struct ShapeCat;

template< size_t... Is, size_t... Js >
struct ShapeCat< shape< Is... >, shape< Js... >>
{ using type = shape< Is..., Js... >; };

template< typename Shape1, typename Shape2 >
using shape_cat = ShapeCat< Shape1, Shape2 >::type;

/**
 * IndexCat concatenates two index objects together
 */
template< typename Index1, typename Index2 >
struct IndexCat;

template< size_t... Is, size_t... Js >
struct IndexCat< index< Is... >, index< Js... >>
{ using type = index< Is..., Js... >; };

template< typename Index1, typename Index2 >
using index_cat = IndexCat< Index1, Index2 >::type;

// test shape cat
namespace test {
static_assert( is_same_v< shape<1,2,3>, shape_cat<shape<1,2>, shape<3>> > );
static_assert( is_same_v< index<1,2,3>, index_cat<index<1,2>, index<3>> > );
} // namespace test

/**
 * index and shape inserters
 */
template< typename X, size_t I, size_t N >
struct InsertAt;

template< size_t... Js, size_t N >
struct InsertAt< shape< Js... >, 0, N >
{ using type = shape< N, Js... >; };

template< size_t J, size_t... Js, size_t I, size_t N >
requires ( I > 0 and I <= 1+sizeof...( Js ))
struct InsertAt< shape< J, Js... >, I, N >
{ using type = shape_cat< shape< J >, typename InsertAt< shape< Js... >, I-1, N >::type >; };

template< size_t... Js, size_t N >
struct InsertAt< index< Js... >, 0, N >
{ using type = index< N, Js... >; };

template< size_t J, size_t... Js, size_t I, size_t N >
requires ( I > 0 and I <= 1+sizeof...( Js ))
struct InsertAt< index< J, Js... >, I, N >
{ using type = index_cat< index< J >, typename InsertAt< index< Js... >, I-1, N >::type >; };

template< typename X, size_t I, size_t N >
using insert_at = InsertAt< X, I, N >::type;

namespace test {
static_assert( is_same_v< shape<1,2,3>, insert_at< shape<1,3>, 1, 2 >> );
static_assert( is_same_v< index<1,2,3>, insert_at< index<1,3>, 1, 2 >> );
} // namespace test


/**
 * ArityOf extracts the number of dimensions of a given tensor shape at index I
 */
template< size_t I, typename Shape >
struct ArityOf;

template< size_t J, size_t... Js >
struct ArityOf< 0, shape< J, Js... >>
{ static constexpr size_t value = J; };

template< size_t I, size_t J, size_t... Js >
requires ( I < 1 + sizeof...(Js) )
struct ArityOf< I, shape< J, Js... >>
{ static constexpr size_t value = ArityOf< I-1, shape< Js... >>::value; };

template< size_t I, typename Shape >
static constexpr size_t arity_of = ArityOf< I, Shape >::value;

/**
 * ArityOf extracts the number of dimensions of a given tensor shape at index I
 */
template< size_t I, typename Index >
struct DimIndexOf;

template< size_t J, size_t... Js >
struct DimIndexOf< 0, index< J, Js... >>
{ static constexpr size_t value = J; };

template< size_t I, size_t J, size_t... Js >
requires ( I < 1 + sizeof...(Js) )
struct DimIndexOf< I, index< J, Js... >>
{ static constexpr size_t value = DimIndexOf< I-1, index< Js... >>::value; };

template< size_t I, typename Index >
using dim_index_of = DimIndexOf< I, Index >::value;

namespace test {
static_assert( 
    ArityOf<0,shape1234>::value == 1 and ArityOf<1,shape1234>::value == 2 and 
    ArityOf<2,shape1234>::value == 3 and ArityOf<3,shape1234>::value == 4 );
static_assert( 
    DimIndexOf<0,index012>::value == 0 and DimIndexOf<1,index012>::value == 1 and
    DimIndexOf<2,index012>::value == 2 );
} // namespace test

/**
 * ElementFrom calculates the element index from a
 */
template< typename Shape, typename Index >
struct ElementFrom;

template< size_t... Is, size_t... Js >
struct ElementFrom< shape< 1, Is... >, index< 0, Js... >>
{ static constexpr size_t value = ElementFrom< shape< Is... >, index< Js... >>::value; };

template< size_t... Is, size_t... Js >
struct ElementFrom< shape< 1, Is... >, index< Js... >>
{ static constexpr size_t value = ElementFrom< shape< Is... >, index< Js... >>::value; };

template< size_t I, size_t J >
struct ElementFrom< shape<I>, index<J> >
{ static constexpr size_t value = J; };

// allow for an extra 0 index
template< size_t I, size_t J >
struct ElementFrom< shape<I>, index<J, 0>>
{ static constexpr size_t value = J; };

template< >
struct ElementFrom< shape<>, index<> >
{ static constexpr size_t value = 0; };

template< size_t I, size_t... Is, size_t J, size_t... Js >
struct ElementFrom< shape< I, Is... >, index< J, Js... >>
{ static constexpr size_t value = 
    J * ( Is * ... ) + ElementFrom< shape< Is... >, index< Js... >>::value; };

template< typename Shape, typename Index >
static constexpr size_t element_from = ElementFrom< Shape, Index >::value;

/**
 * IndexFrom is the reverse of ElementFrom and calculates the index into the 
 * tensor of a given element id
 */
template< typename Shape, size_t Element >
struct IndexFrom;

template< size_t I, size_t... Is, size_t Element >
struct IndexFrom< shape< I, Is... >, Element >
{ using type = index_cat< index< Element / ( Is * ... )>, 
    typename IndexFrom< shape< Is... >, Element % ( Is * ... ) >::type >; };

template< size_t I, size_t Element >
struct IndexFrom< shape<I>, Element >
{ using type = index< Element >; };

template< typename Shape, size_t Element >
using index_from = IndexFrom< Shape, Element >::type;

namespace test {
static_assert( element_from< shape123, index012 > == 1*2*3 - 1 );
static_assert( is_same_v< index_from< shape123, 1*2*3-1 >, index012 > );
} // namespace test

/**
 * uncontract will insert N in the Ith and Jth position of X
 */
template< typename X, size_t I, size_t J, size_t N >
struct Uncontract;

// NOTE: DT: is there a better way to do this?!?
template< size_t... Ks, size_t I, size_t J, size_t N >
requires ( I < J and I < sizeof...( Ks ) + 2 and J < sizeof...( Ks ) + 2 )
struct Uncontract< shape< Ks... >, I, J, N >
{ using type = insert_at< insert_at< shape< Ks... >, I, N >, J, N >; };

template< size_t... Ks, size_t I, size_t J, size_t N >
requires ( I < J and I < sizeof...( Ks ) + 2 and J < sizeof...( Ks ) + 2 )
struct Uncontract< index< Ks... >, I, J, N >
{ using type = insert_at< insert_at< index< Ks... >, I, N >, J, N >; };

// ensure I < J when selecting Uncontract template
template< typename X, size_t I, size_t J, size_t N >
using uncontract = Uncontract< X, ( I < J ? I : J ), ( I < J ? J : I ), N >::type;

namespace test {
static_assert( is_same_v<uncontract<shape<9>,1,2,1>, shape<9,1,1>> );
static_assert( is_same_v<uncontract<index<1>,1,2,0>, index<1,0,0>> );
} // namespace test

/**
 * ValidIndex ensures that an index<...> is valid for a given shape<...>
 */
template< typename Index, typename Shape >
struct ValidIndex;

template< typename Index, typename Shape, typename Seq >
struct ValidIndexHelper;

template< typename Index, typename Shape, size_t... Ks >
struct ValidIndexHelper< Index, Shape, seq< Ks... >>
{ static constexpr bool value = 
    (( DimIndexOf<Ks, Index>::value < ArityOf<Ks, Shape>::value ) and ... ); };

template< size_t... Is, size_t... Js >
struct ValidIndex< index< Is... >, shape< Js... >>
{ static constexpr bool value = ( sizeof...( Is ) == sizeof...( Js ) and
    ValidIndexHelper< index< Is... >, shape< Js... >, make_seq< sizeof...( Is ) >>::value ); };

/**
 * RemoveDimension removes dimension I from Shape
 */
template< size_t I, typename Shape >
struct RemoveDimension;

template< size_t I, typename Shape, typename Seq >
struct RemoveDimensionHelper;

template< size_t I, typename Shape, size_t... Ks >
struct RemoveDimensionHelper< I, Shape, seq< Ks... >>
{ using type = shape< ArityOf<( Ks < I ? Ks : Ks+1 ), Shape >::value... >; };

template< size_t I, size_t... Js >
struct RemoveDimension< I, shape< Js... >>
{ using type = RemoveDimensionHelper< I, shape< Js... >, make_seq< sizeof...( Js )-1 >>::type; };

template< size_t I, typename Shape >
using remove_dimension = RemoveDimension< I, Shape >::type;

// tests of Remove Dimension
namespace test {
using std::is_same_v;

static_assert( is_same_v< shape< 0,1,2,4 >, remove_dimension< 3, shape< 0,1,2,3,4 >>> );

} // namespace test

/**
 * ContractedShape computes the shape of a tensor contracted on indices I and J
 */
template< size_t I, size_t J, typename Shape >
struct ContractedShape
{ using type = remove_dimension< I, remove_dimension< J, Shape >>; };

template< size_t I, size_t J, size_t S1, size_t S2 >
struct ContractedShape< I, J, shape< S1, S2 >>
{ using type = shape< 1 >; };

template< size_t I, size_t J, typename Shape >
using contracted_shape = ContractedShape< I, J, Shape >::type;

/**
 * types is a type holder for the type of each element of a tensor
 */
template< typename... Ts >
struct types { };

// forward declaration
template< typename Shape, typename Types >
struct Tensor;

/**
 * shape_of extracts the shape of a tensor
 */
template< typename Tensor>
struct ShapeOf;

template< typename Shape, typename Types >
struct ShapeOf< Tensor< Shape, Types >>
{ using type = Shape; };

template< typename Tensor >
using shape_of = ShapeOf< Tensor >::type;

/**
 * types_of extracts the Types of a tensor
 */
template< typename Tensor>
struct TypesOf;

template< typename Shape, typename Types >
struct TypesOf< Tensor< Shape, Types >>
{ using type = Types; };

template< typename Tensor >
using types_of = TypesOf< Tensor >::type;


template< typename Shape, typename... Types >
Tensor< Shape, types< Types... >> make_tensor( Types... values )
{ return { values_of( values... ) }; }

template< typename T >
constexpr T identity( T t ) { return t; }

/**
 * op_helper constructs the tuple-like values_type of a tensor from two operands 
 * and an operation. The tuple-like values_type is indexed by the index sequence
 * Seq, and the relative index of the elements of the operand tuples are
 * calculated with left_at and right_at:
 * 
 * out[I] = op(left[left_at(I)], right[right_at(I)])
 */
template< typename Op, typename LeftValuesType, typename RightValuesType, 
    typename Seq, typename LeftIndex, typename RightIndex >
auto op_helper( Op op, LeftValuesType const&, RightValuesType const&, 
    Seq, LeftIndex = identity< size_t >, RightIndex = identity< size_t > );

template< typename Op, typename LeftValuesType, typename RightValuesType, 
    typename LeftIndex, typename RightIndex, size_t... Is >
auto op_helper( Op op, LeftValuesType const& left_values, RightValuesType const& right_values, 
    seq< Is... >, LeftIndex left_at, RightIndex right_at )
{ return make_values_of( op( 
    get< left_at( Is )>( left_values ), get< right_at( Is )>( right_values ))
    ... ); }

template< size_t I, size_t J, typename Shape, typename Types >
auto contract( Tensor< Shape, Types > const& );

/**
 * Tensor class holds an arbitrarily typed tuple of values organized by a 
 * tuple of size_t's or it's shape.
 */
template< size_t... Is, typename... Ts >
requires (( Is * ... ) == sizeof...( Ts ))
struct Tensor< shape< Is... >, types< Ts... >>
{
    using this_type = Tensor< shape< Is... >, types< Ts... >>;
    using values_type = values_of< Ts... >;
    using shape_type = shape< Is... >;
    static constexpr size_t size = sizeof...( Ts ); // == ( Is * ... )
    static constexpr size_t arity = sizeof...( Is ); // how many dimensions

    template< typename Shape, typename Types > 
    auto operator+( Tensor< Shape, Types > const& other )
    { return make_tensor< Shape >( op_helper( std::plus< >{}, 
        values, other.values, make_seq< size >{} )); }

    template< typename Shape, typename Types >
    auto operator-( Tensor< Shape, Types > const& other )
    { return make_tensor< Shape >( op_helper( std::minus< >{}, 
        values, other.values, make_seq< size >{} )); }

    // outer product
    template< typename Shape, typename Types >
    auto operator*( Tensor< Shape, Types > const& other )
    { 
        using other_type = Tensor< Shape, Types >;
        using other_shape = Shape;
        using product_shape = shape_cat< shape_type, other_shape >;

        return make_tensor< product_shape >( op_helper( std::multiplies< >{},
            values, other.values, make_seq< size * other_type::size >{},
            div_by< size >, mod_by< size > ));
    }

    Tensor() : values{} { }
    Tensor( values_of< Ts... > values ) : values{ values } { }

// private:
    values_type values;

};

/**
 * Contraction represents a contracton on a tensor
 */
template< size_t I, size_t J, typename Tensor>
struct Contraction;

// a contracted element is a sum of values from the contracted tensor
// the indices of the elements to sum will be passed into this template
template< typename Seq, typename ElementsType >
struct SumElements;

template< size_t... Is, typename ElementsType >
struct SumElements< seq< Is... >, ElementsType >
{
    static constexpr auto value( ElementsType const& values )
    { return ( get< Is >( values ) + ... ); }
};

template< typename Seq, typename ElementsType >
auto sum_elements( ElementsType const& values )
{ return SumElements< Seq, ElementsType >::value( values ); }

template< size_t ContractedElement, typename Shape, size_t I, size_t J, typename InputShape, typename DimSeq >
struct ElementSelector;

template< size_t ContractedElement, typename Shape, size_t I, size_t J, typename InputShape, size_t... Ls >
struct ElementSelector< ContractedElement, Shape, I, J, InputShape, seq< Ls... >>
{
    // get the subscripts of the contracted element
    using contracted_index = index_from< Shape, ContractedElement >;
    // insert Dim into positions I and J of the contracted index
    template< size_t L >
    using uncontracted_index = uncontract< contracted_index, I, J, L >;

    using elements_seq = seq< element_from< InputShape, uncontracted_index< Ls >>... >;

    template< typename Values >
    static constexpr auto value( Values const& values )
    { return sum_elements< elements_seq >( values ); }
};

template< typename OutputShape, template< size_t > class Element, 
    typename Values, size_t... Is >
constexpr auto do_contraction( Values const& values, seq< Is... > elements )
{ return make_tensor< OutputShape >( Element< Is >::value( values )... ); }

template< size_t I, size_t J, size_t... Ks, typename... Ts >
requires ( I < J and I < sizeof...( Ks ) and J < sizeof...( Ks ) and 
    arity_of< I, shape< Ks... >> == arity_of< J, shape< Ks... >> )
struct Contraction< I, J, Tensor< shape< Ks... >, types< Ts... >>>
{
    using shape_type = shape< Ks... >;
    using types_type = types< Ts... >;
    using tensor_type = Tensor< shape_type, types_type >;
    static constexpr tuple< size_t, size_t > indices = { I, J };
    static constexpr size_t arity = arity_of< I, shape_type >;
    static constexpr size_t size = sizeof...( Ts );
    static constexpr size_t shape_size = sizeof...( Ks );

    using output_shape = contracted_shape< I, J, shape_type >;
    static constexpr size_t output_size = output_shape::elements_size;
    using elements_seq = make_seq< output_size >;
    using arity_seq = make_seq< arity >;

    template< size_t N >
    using element = ElementSelector< N, output_shape, I, J, shape_type, arity_seq >;
    
    static constexpr auto invoke( tensor_type const& v )
    { return do_contraction< output_shape, element >( v.values, elements_seq{} ); }
};

template< size_t I, size_t J, size_t... Is, typename... Ts >
requires ( I != J and I < sizeof...( Is ) and J < sizeof...( Is ) and 
    arity_of< I, shape< Is... >> == arity_of< J, shape< Is... >> )
constexpr auto contract( Tensor< shape< Is... >, types< Ts... >> const& tensor )
{ 
    using tensor_type = Tensor< shape< Is... >, types< Ts... >>;
    using contraction = Contraction< I, J, tensor_type >;
    return contraction::invoke( tensor );
}

// contraction tests
namespace test {

} // namespace test


} // namespace tensor

#endif