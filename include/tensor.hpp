/**
 * Tensors for expressions
 */

#ifndef __TENSOR_HPP__
#define __TENSOR_HPP__

#include "utility.hpp"

#include <utility>
#include <tuple>
#ifndef NO_TENSOR_PRINTING
#include <iostream>
#endif
#include <type_traits>

// tensor namespace
namespace tensor {

using std::size_t;
using std::get;
using std::tuple;

// setup our test namespace
namespace test { using std::is_same_v; } 

/**
 * Subscripts to an element of a tensor
 * 
 * @tparam Is... are the subscripts
 */
template< size_t... Is >
struct index
{ 
    static constexpr size_t size = sizeof...( Is );
    static constexpr bool is_even = (( Is + ... ) % 2 == 0 );
};

/**
 * Shape of a Tensor 
 * 
 * @tparam Is... are the sizes of the respective dimensions
 */
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


// shape_cat details
namespace detail {

template< typename Shape1, typename Shape2 >
struct ShapeCat;

template< size_t... Is, size_t... Js >
struct ShapeCat< shape< Is... >, shape< Js... >>
{ using type = shape< Is..., Js... >; };

} // namepsace detail (shape_cat)

/**
 * Concatentate two tensor shapes.  Needed for outer product of two tensors
 * 
 * @tparam Shape1 first tensor shape 
 * @tparam Shape2 second tensor shape to concatenate to the first
 * @returns concatenated shape
 */
template< typename Shape1, typename Shape2 >
using shape_cat = detail::ShapeCat< Shape1, Shape2 >::type;

// index_cat details
namespace detail {

template< typename Index1, typename Index2 >
struct IndexCat;

template< size_t... Is, size_t... Js >
struct IndexCat< index< Is... >, index< Js... >>
{ using type = index< Is..., Js... >; };

} // namespace detail (index_cat)

/**
 * Concatenate two tensor subscript indices
 * 
 * @tparam Index1 first subscript index
 * @tparam Index2 second subscript index to concatenate to the first
 * @returns the concatenated subscript object
 */
template< typename Index1, typename Index2 >
using index_cat = detail::IndexCat< Index1, Index2 >::type;

// test shape cat
namespace test {
static_assert( is_same_v< shape<1,2,3>, shape_cat<shape<1,2>, shape<3>> > );
static_assert( is_same_v< index<1,2,3>, index_cat<index<1,2>, index<3>> > );
} // namespace test

// insert_at details
namespace details {

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

} // namespace details (insert_at)

/**
 * Insert an element in a shape or index object
 * 
 * @tparam X the shape or index object to have a new element inserted
 * @tparam I the position in X to insert a new element
 * @tparam N the value to insert into X
 */
template< typename X, size_t I, size_t N >
using insert_at = details::InsertAt< X, I, N >::type;

namespace test {
static_assert( is_same_v< shape<1,2,3>, insert_at< shape<1,3>, 1, 2 >> );
static_assert( is_same_v< index<1,2,3>, insert_at< index<1,3>, 1, 2 >> );
} // namespace test

// arity_of details
namespace details {

template< size_t I, typename Shape >
struct ArityOf;

template< size_t J, size_t... Js >
struct ArityOf< 0, shape< J, Js... >>
{ static constexpr size_t value = J; };

template< size_t I, size_t J, size_t... Js >
requires ( I < 1 + sizeof...(Js) )
struct ArityOf< I, shape< J, Js... >>
{ static constexpr size_t value = ArityOf< I-1, shape< Js... >>::value; };

} // namespace details (arity_of)

/**
 * Extracts the size of a dimension of the shape of a tensor
 * 
 * @tparam I the dimension to extract
 * @tparam Shape the shape object to extract from
 * @returns the size of Shape at index I
 */
template< size_t I, typename Shape >
static constexpr size_t arity_of = details::ArityOf< I, Shape >::value;

// dim_index_of details
namespace detail {

template< size_t I, typename Index >
struct DimIndexOf;

template< size_t J, size_t... Js >
struct DimIndexOf< 0, index< J, Js... >>
{ static constexpr size_t value = J; };

template< size_t I, size_t J, size_t... Js >
requires ( I < 1 + sizeof...(Js) )
struct DimIndexOf< I, index< J, Js... >>
{ static constexpr size_t value = DimIndexOf< I-1, index< Js... >>::value; };

} // namespace details (dim_index_of)

/**
 * Extract the value of a dimension of a subscript to a tensor
 * 
 * @tparam I index of subscript to extract
 * @tparam Index the subscript object to extract from
 * @returns the subscript of Index at I
 */
template< size_t I, typename Index >
static constexpr size_t dim_index_of = detail::DimIndexOf< I, Index >::value;

namespace test {
static_assert( 
    arity_of<0,shape1234> == 1 and arity_of<1,shape1234> == 2 and 
    arity_of<2,shape1234> == 3 and arity_of<3,shape1234> == 4 );
static_assert( 
    dim_index_of<0,index012> == 0 and dim_index_of<1,index012> == 1 and
    dim_index_of<2,index012> == 2 );
} // namespace test


// element_from details
namespace details {

// TODO: swich Shape and Index order, right?
template< typename Shape, typename Index >
struct ElementFrom;

template< size_t... Is, size_t... Js >
struct ElementFrom< shape< 1, Is... >, index< 0, Js... >>
{ static constexpr size_t value = 
    ElementFrom< shape< Is... >, index< Js... >>::value; };

template< size_t... Is, size_t... Js >
struct ElementFrom< shape< 1, Is... >, index< Js... >>
{ static constexpr size_t value = 
    ElementFrom< shape< Is... >, index< Js... >>::value; };

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

} // namespace details (element_from)

/**
 * Converts a set of subscripts in to a single size_t element index into the 
 * values tuple of a tensor (opposite of index_from)
 * 
 * @tparam Shape of the tensor
 * @tparam Index subscripts to be translated
 * @returns the element index of the subscripts Index given the tensor Shape
 */
template< typename Shape, typename Index >
static constexpr size_t element_from = 
    details::ElementFrom< Shape, Index >::value;

// index_from details
namespace detail {

template< typename Shape, size_t Element >
struct IndexFrom;

template< size_t I, size_t... Is, size_t Element >
struct IndexFrom< shape< I, Is... >, Element >
{ using type = index_cat< index< Element / ( Is * ... )>, 
    typename IndexFrom< shape< Is... >, Element % ( Is * ... ) >::type >; };

template< size_t I, size_t Element >
struct IndexFrom< shape<I>, Element >
{ using type = index< Element >; };

} // namespace detail (index_from) 

/**
 * Converts an element offset into the Tensor tuple back into a set of 
 * subscripts (opposite of element_from)
 * 
 * @tparam Shape of the tensor
 * @tparam Element index into the values tuple of the tensor
 * @returns an index<...> of subscripts corresponding to the same element of 
 *     the Tensor
 */
template< typename Shape, size_t Element >
using index_from = detail::IndexFrom< Shape, Element >::type;

namespace test {
static_assert( element_from< shape123, index012 > == 1*2*3 - 1 );
static_assert( is_same_v< index_from< shape123, 1*2*3-1 >, index012 > );
} // namespace test

// unconctract details
namespace detail {

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

} // namespace detail (uncontract)

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
using uncontract = detail::Uncontract< X, min_v< I, J >, max_v< I, J >, N >::type;

namespace test {
static_assert( is_same_v<uncontract<shape<9>,1,2,1>, shape<9,1,1>> );
static_assert( is_same_v<uncontract<index<1>,1,2,0>, index<1,0,0>> );
} // namespace test

// valid_index details
namespace detail {

template< typename Index, typename Shape >
struct ValidIndex;

template< typename Index, typename Shape, typename Seq >
struct ValidIndexHelper;

template< typename Index, typename Shape, size_t... Ks >
struct ValidIndexHelper< Index, Shape, seq< Ks... >>
{ static constexpr bool value = 
    ((( dim_index_of< Ks, Index >) < (arity_of< Ks, Shape >) ) and ... ); };

template< size_t... Is, size_t... Js >
struct ValidIndex< index< Is... >, shape< Js... >>
{ static constexpr bool value = ( sizeof...( Is ) == sizeof...( Js ) and
    ValidIndexHelper< index< Is... >, shape< Js... >, make_seq< sizeof...( Is ) >>::value ); };

} // namespace detail

/**
 * ValidIndex ensures that an index<...> is valid for a given shape<...>
 * 
 * @tparam Index the subscripts to be validated
 * @tparam Shape the tensor shape<...> to validate against
 * @returns true if the index dimensions are exclusively less than the shape
 *  dimensions
 */
template< typename Index, typename Shape >
static constexpr bool valid_index = detail::ValidIndex< Index, Shape >::value;

// remove_dimension details
namespace detail {

template< size_t I, typename Shape >
struct RemoveDimension;

template< size_t I, typename Shape, typename Seq >
struct RemoveDimensionHelper;

template< size_t I, typename Shape, size_t... Ks >
struct RemoveDimensionHelper< I, Shape, seq< Ks... >>
{ using type = shape< arity_of<( Ks < I ? Ks : Ks+1 ), Shape >... >; };

template< size_t I, size_t... Js >
struct RemoveDimension< I, shape< Js... >>
{ using type = RemoveDimensionHelper< I, shape< Js... >, make_seq< sizeof...( Js )-1 >>::type; };

} // namespace detail (remove_dimension)

/**
 * RemoveDimension removes dimension I from Shape
 * 
 * @tparam I dimension to be removed
 * @tparam Shape to remove dimension from
 * @returns Shape with dimension I removed 
 */
template< size_t I, typename Shape >
using remove_dimension = detail::RemoveDimension< I, Shape >::type;

// tests of Remove Dimension
namespace test {
static_assert( is_same_v< shape< 0,1,2,4 >, remove_dimension< 3, shape< 0,1,2,3,4 >>> );
} // namespace test

// contracted_shape details
namespace detail {

template< size_t I, size_t J, typename Shape >
struct ContractedShape
{ using type = remove_dimension< I, remove_dimension< J, Shape >>; };

template< size_t I, size_t J, size_t S1, size_t S2 >
struct ContractedShape< I, J, shape< S1, S2 >>
{ using type = shape< 1 >; };

} // namespace details (contracted_shape)

/**
 * ContractedShape computes the shape of a tensor contracted on indices I and J
 * 
 * @tparam I first index to remove
 * @tparam J second index to remove
 */
template< size_t I, size_t J, typename Shape >
using contracted_shape = detail::ContractedShape< I, J, Shape >::type;

/**
 * types is a type holder for the type of each element of a tensor
 * 
 * @tparam Ts are the types of the tensor elements
 */
template< typename... Ts >
struct types { };

// types details
namespace detail {
template< typename T, typename Seq >
struct UniformTypesHelper;

template< typename T, size_t... Is >
struct UniformTypesHelper< T, seq< Is... >>
{ using type = types< noop_t< T, Is >... >; };

template< typename T, size_t Size >
struct UniformTypes 
{ using type = UniformTypesHelper< T, make_seq< Size >>::type; };
} // namespace detail (types)

/**
 * evaluates to a types<...> tuple with the same type Size times
 * 
 * @tparam T the type of each element of our types<...>
 * @tparam Size the size of our values tuple
 * @returns a types<...> of size Size and every element type T
 */
template< typename T, size_t Size >
using uniform_types = detail::UniformTypes< T, Size >::type;

// forward declaration
template< typename Shape, typename Types >
struct Tensor;

/**
 * Helper type which stores tensor values of any type
 * 
 * @tparam Ts are the types of the tensor elements
 */
template< typename... Ts >
using values_of = tuple< Ts... >;

/**
 * Helper function to create tuple of tensor elements
 * 
 * @tparam Ts are the tensor element types
 * @param ts are the tensor element values
 * @returns a tuple-like object of tensor elements
 */
template< typename... Ts >
values_of< Ts... > make_values_of( Ts... ts )
{ return { ts... }; }

// shape_of details
namespace detail {
template< typename Tensor>
struct ShapeOf;

template< typename Shape, typename Types >
struct ShapeOf< Tensor< Shape, Types >>
{ using type = Shape; };
} // namespace detail (shape_of)

/**
 * Extracts the shape<...> of the Tensor parameter
 * 
 * @tparam Tensor the type of the tensor
 * @returns the shape<...> of the tensor type
 */
template< typename Tensor >
using shape_of = detail::ShapeOf< Tensor >::type;

/**
 * types_of extracts the Types of a tensor
 */
namespace detail {

template< typename Tensor>
struct TypesOf;

template< typename Shape, typename Types >
struct TypesOf< Tensor< Shape, Types >>
{ using type = Types; };

} // namespace detail

/**
 * extracts the types<...> of the elements of the tensor template parameter
 * 
 * @tparam Tensor the type of the tensor
 * @returns a pack of types of the elements of the tensor wrapped in types<...>
 */
template< typename Tensor >
using types_of = detail::TypesOf< Tensor >::type;

/**
 * Primary mechanism to create a tensor.  This function should be used to create
 * a new tensor in most situations.  
 * 
 * @tparam Shape is a shape<Is...> of the tensor to be created
 * @tparam Types are the types of the elements of the tensor
 * 
 * EXAMPLE:
 * auto rot = make_tensor< shape< 2, 2 >>( 
 *      cos( theta ), -sin( theta ),
 *      sin( theta ),  cos( theta ));
 * 
 * NOTE: sizeof...(Types) must equal Shape::elements_size
 */
template< typename Shape, typename... Types >
Tensor< Shape, types< Types... >> make_tensor( Types... values )
{ return { values_of( values... ) }; }

template< typename T >
constexpr T identity( T t ) { return t; }

// tensor operation details
namespace detail {

template< typename Shape, typename FirstTypes, typename SecondTypes, size_t... Is >
auto plus_helper( Tensor< Shape, FirstTypes > const& first, 
    Tensor< Shape, SecondTypes > const& second, seq< Is... > )
{ return make_tensor< Shape >(( get< Is >( first.values ) + get< Is >( second.values ))... ); }

template< typename Shape, typename FirstTypes, typename SecondTypes, size_t... Is >
auto minus_helper( Tensor< Shape, FirstTypes > const& first, 
    Tensor< Shape, SecondTypes > const& second, seq< Is... > )
{ return make_tensor< Shape >(( get< Is >( first.values ) - get< Is >( second.values ))... ); }

template< size_t I, size_t J, typename Shape, typename Types >
auto contract( Tensor< Shape, Types > const& );


template< typename ProductShape, size_t LeftSize, typename LeftTensor, typename RightTensor, size_t... Is >
auto multiply_helper( LeftTensor const& left, RightTensor const& right, seq< Is... > )
{ return make_tensor< ProductShape >( 
        (get< Is / LeftSize >( left.values ) * get< Is % LeftSize >( right.values ))... ); }

template< typename Shape1, typename Types1, typename Shape2, typename Types2, size_t... Is >
auto equality_helper( Tensor< Shape1, Types1 > const& first, 
    Tensor< Shape2, Types2 > const& second, seq< Is... > )
{ return (( get< Is >( first.values ) == get< Is >( second.values )) and ... ); }

template< typename Shape1, typename Types1, typename Shape2, typename Types2, size_t... Is >
auto inequality_helper( Tensor< Shape1, Types1 > const& first, 
    Tensor< Shape2, Types2 > const& second, seq< Is... > )
{ return (( get< Is >( first.values ) != get< Is >( second.values )) or ... ); }

template< typename Shape, typename Types, size_t... Is >
auto scale_helper( long double scalar, Tensor< Shape, Types > const& v, seq< Is... > )
{ return make_tensor< Shape >(( get< Is >( v.values ) * scalar )... ); }

template< typename Shape, typename Types, size_t... Is >
auto divide_scale_helper( long double scalar, Tensor< Shape, Types > const& v, seq< Is... > )
{ return make_tensor< Shape >(( get< Is >( v.values ) / scalar )... ); }

} // namespace detail (tensor operations)

/**
 * Tensor class holds an arbitrarily typed tuple of values organized by a 
 * tuple of size_t's or it's shape.
 * 
 * @tparam Is are the size of each dimension of the tensor
 * @tparam Ts are the types of each of the elements of the tensor
 * 
 * This class allows tensor semantics in C++ code on arbirary element types. 
 * The tensor could be constructed of all numeric types in which case tensor 
 * arithmetic would be calculated directly, or the types could be lazily 
 * calculated allowing for tensor calculus and expressions
 *
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

    template< typename Types > 
    auto operator+( Tensor< shape_type, Types > const& other )
    { return detail::plus_helper( *this, other, make_seq< size >{} ); }

    template< typename Shape, typename Types >
    auto operator-( Tensor< Shape, Types > const& other )
    { return detail::minus_helper( *this, other, make_seq< size >{} ); }

    // outer product
    template< typename Shape, typename Types >
    auto operator*( Tensor< Shape, Types > const& other ) const
    { 
        using other_type = Tensor< Shape, Types >;
        using product_shape = shape_cat< shape_type, Shape >;

        return detail::multiply_helper< product_shape, size >( *this, other, 
            make_seq< size * other_type::size >{} ); 
    }

    template< typename Types2 >
    auto operator==( Tensor< shape_type, Types2 > const& other ) const
    { return detail::equality_helper( *this, other, make_seq< size >{} ); }

    template< typename Types2 >
    auto operator!=( Tensor< shape_type, Types2 > const& other ) const
    { return detail::inequality_helper( *this, other, make_seq< size >{} ); }

    auto scale( long double scalar ) const
    { return detail::scale_helper( scalar, *this, make_seq< size >{} ); }

    auto divide_scale( long double scalar ) const
    { return detail::divide_scale_helper( scalar, *this, make_seq< size >{} ); }

    Tensor() : values{} { }
    Tensor( values_of< Ts... > values ) : values{ values } { }

// private:
    values_type values;

};

/**
 * UniformTensor wraps a tuple of (Ds*...) instances of T
 * 
 * @tparam T the type of every element of the tensor
 * @tparam Ds the shape<Ds...> of the tensor
 */
template< typename T, size_t... Ds >
using UniformTensor = Tensor< shape< Ds... >, uniform_types< T, ( Ds * ... ) >>;

namespace detail {

template< typename ValuesType, typename T, size_t... Is >
void fill_uniform_values( ValuesType& values, T in, seq< Is... > )
{ ( get< Is >( values ) = ... = in ); }

} // namespace detail (UniformTensor)

/**
 * Creates a tensor with shape<Ds...> whose elements are all the same type
 * 
 * @tparam Ds the shape<Ds...> of the tensor
 * @param val the value to initialize each element of the tensor
 * @returns a tensor with shape<Ds...> and uniform type with elements 
 *  initialized to val
 */
template< size_t... Ds >
auto make_uniform( auto val )
{ 
    auto u = UniformTensor< decltype(val), Ds... >{};
    detail::fill_uniform_values( u.values, val, make_seq< ( Ds * ... ) >{} );
    return u;
}

/**
 * Matrix alias for a 2D tensor
 */
template< size_t Rows, size_t Cols, typename... Ts >
requires( sizeof...( Ts ) == Rows * Cols )
using Matrix = Tensor< shape< Rows, Cols >, types< Ts... >>;

/**
 * Creates a Rows x Cols matrix with element types Ts...
 * 
 * @tparam Rows the number of rows in the matrix
 * @tparam Cols the number of columns in the matrix
 * @tparam Ts the types of the matrix elements
 * @param ts the values of the matrix elements
 * 
 * NOTE: make_matrix reads the elements in column major order and then 
 * transposes to row major order.  This helps readability in code, ensuring that
 * column values appear to the right and left of each other, and row values
 * appear above and below
 * 
 * constexpr size_t ROWS = 2;
 * constexpr size_t COLS = 3;
 * auto A = make_matrix< ROWS, COLS >(
 *     1., 2., 3.,
 *     4., 5., 6. );
 * ASSERT( at<0, 1>(A) == 2. and at<0, 2>(A) == 3. and
 *         at<1, 0>(A) == 4. and at<1, 2>(A) == 6.);
 * 
 */
template< size_t Rows, size_t Cols, typename... Ts >
requires( sizeof...( Ts ) == Rows * Cols )
auto make_matrix( Ts... ts )
{ return transpose( make_tensor< shape< Cols, Rows >>( ts... )); } 

/**
 * Vector alias for a 1D row-vector
 * 
 * @tparam N the size of the vector
 * @tparam Ts the types of the vector elements
 */
template< size_t N, typename... Ts >
requires( sizeof...( Ts ) == N )
using Vector = Tensor< shape< N >, types< Ts... >>;

/**
 * Creates a row vector of size N and types Ts...
 * 
 * @tparam N the size of the vector
 * @tparam Ts the types of the vector elements
 * @param ts the values of the vector elements
 * @returns a shape<N> tensor with types Ts.. and values ts...
 */
template< size_t N, typename... Ts >
auto make_vector( Ts... ts )
{ return make_tensor< shape< N >>( ts... ); }

/**
 * Contraction represents a contracton on a tensor
 */
template< size_t I, size_t J, typename TensorType >
constexpr auto contract( TensorType const& tensor );

// contraction details
namespace detail {
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
} // namespace detail (contraction)

/**
 * Contracts the tensor along dimensions I and J which must have an equal number
 * of terms.
 * 
 * @tparam I index of the first subscript to contract
 * @tparam J index of the second subscript to contract
 * @tparam Is the size of the corresponding dimension of the tensor shape<...>
 * @tparam Ts the types of the elements of the tensor
 * @param tensor to contract
 * @returns tensor contracted along the Ith and Jth subscript
 */
template< size_t I, size_t J, size_t... Is, typename... Ts >
requires ( I != J and I < sizeof...( Is ) and J < sizeof...( Is ) and 
    arity_of< I, shape< Is... >> == arity_of< J, shape< Is... >> )
constexpr auto contract( Tensor< shape< Is... >, types< Ts... >> const& tensor )
{ 
    using tensor_type = Tensor< shape< Is... >, types< Ts... >>;
    using contraction = detail::Contraction< I, J, tensor_type >;
    return contraction::invoke( tensor );
}

#ifndef NDEBUG
// contraction tests
namespace test {
static_assert( is_same_v< shape<3,4>, detail::Contraction<1,2, UniformTensor<bool, 3,2,2,4>>::output_shape > );
} // namespace test
#endif

// forward declaration
template< typename AboutIndex, typename Shape, typename Types >
auto sub_tensor( Tensor< Shape, Types > const& v );

// sub_tensor details
namespace detail {

template< typename Shape >
struct SubTensorShape;

template< size_t... Is >
requires (( Is > 1 ) and ... ) // cannot subtensor to 0 dimensioned tensors
struct SubTensorShape< shape< Is... >>
{ using type = shape<( Is-1 )...>; };

template< typename Shape >
using sub_tensor_shape = SubTensorShape< Shape >::type;

} // namespace detail

#ifndef NDEBUG
// tests of sub_tensor_shape
namespace test {
static_assert( is_same_v< shape<2>, detail::sub_tensor_shape< shape<3> >> );
static_assert( is_same_v< shape<2,2>, detail::sub_tensor_shape< shape<3,3> >> );
static_assert( is_same_v< shape<2,3,4>, detail::sub_tensor_shape< shape<3,4,5> >> );
static_assert( detail::sub_tensor_shape< shape<3,3> >::elements_size == 4 );
} // namespace test
#endif

// sub_tensor details (continued)
namespace detail {

template< size_t I, typename SuperShape, typename About >
struct SuperTensorElement;

template< size_t I, typename SuperShape, typename About, typename Seq >
struct SuperTensorElementHelper;

template< size_t I, typename SuperShape, typename About, size_t... Ds >
struct SuperTensorElementHelper< I, SuperShape, About, seq< Ds... >>
{
    using super_shape = SuperShape;
    using sub_shape = sub_tensor_shape< super_shape >;
    using sub_index = index_from< sub_shape, I >;
    using about_index = About;
    
    // skip the about_index
    static constexpr size_t value = element_from< super_shape, index<
        (( DimIndexOf< Ds, sub_index >::value < DimIndexOf< Ds, about_index >::value ) ? 
        DimIndexOf< Ds, sub_index >::value : 1 + DimIndexOf< Ds, sub_index >::value )... >>;
};

// calculate the tensor element index based on a sub-element index
template< size_t I, size_t... Ds, size_t... As >
requires ( sizeof...( Ds ) == sizeof...( As ) )
struct SuperTensorElement< I, shape< Ds... >, index< As... >>
{ static constexpr size_t value = SuperTensorElementHelper< I, shape< Ds... >, 
    index< As... >, make_seq< sizeof...( Ds )>>::value; };

template< size_t SubElement, typename SuperShape, typename About >
static constexpr size_t super_tensor_element = 
    SuperTensorElement< SubElement, SuperShape, About >::value;

template< typename AboutIndex, typename Shape, typename Types, size_t... Is >
auto sub_tensor_helper( Tensor< Shape, Types > const& v, seq< Is... > )
{ return make_tensor< sub_tensor_shape< Shape >>(
    get< super_tensor_element< Is, Shape, AboutIndex >>( v.values )... ); }

} // namespace detail (sub_tensor)

/**
 * sub_tensor removes the rows/cols of the given tensor about the given index
 * 
 * @tparam AboutIndex is an index<...> tuple of the subscript to be removed 
 * @tparam Shape is the shape<...> of the input tensor
 * @tparam Types is the types<...> of the input tensor elements
 * @param v is the tensor 
 * @returns a new tensor formed from v by removing any element whose subscript
 *  matches AboutIndex on any dimension
 * 
 * EXAMPLE:
 * auto rot_z = make_tensor< shape< 3, 3 >>(
 *      cos( theta ), -sin( theta ), 0.,
 *      sin( theta ),  cos( theta ), 0.,
 *           0.,            0.,      1. );
 * 
 * auto rot2D = sub_tensor< index< 2, 2 >>( v );
 * static_assert( is_same_v< shape_of< decltype( rot2D )>, shape< 2, 2 >> );
 */
template< typename AboutIndex, typename Shape, typename Types >
auto sub_tensor( Tensor< Shape, Types > const& v )
{ return detail::sub_tensor_helper< AboutIndex >( v, 
    make_seq< detail::sub_tensor_shape< Shape >::elements_size >{} ); }

// forward declaration of transpose
template< size_t N, typename Types >
auto transpose( Tensor< shape< N, N >, Types > const& v );

// transpose details
namespace detail {

template< size_t N, typename Types, size_t... Is >
auto transpose_helper( Tensor< shape< N, N >, Types > const& v, seq< Is... > )
{ return make_tensor< shape< N, N >>( get< ( Is % N ) * N + Is / N >( v.values )... ); }

} // namespace detail (transpose)

/**
 * Calculates the transpose of a square NxN tensor 
 * 
 * @tparam N the size of one side of the square tensor
 * @tparam Types is a types<...> holder of a pack of types of the elements of
 *  the tensor to be transposed
 * @param v the tensor to be transposed
 * @returns the transpose of the square NxN tensor v
 */
template< size_t N, typename Types >
auto transpose( Tensor< shape< N, N >, Types > const& v )
{ return detail::transpose_helper( v, make_seq< N * N >{} ); }

// forward declaration of determinant
template< size_t N, typename Types >
auto determinant( Tensor< shape< N, N >, Types > const& v );

// determinant details
namespace detail {

// recursively calculate the determinant by expanding along row 0
template< size_t N, typename Types, size_t... Ns >
auto det_helper( Tensor< shape< N, N >, Types > const& v, seq< Ns... > )
{
    return ((( Ns % 2 == 0 ? 1. : -1. ) *            // alternating terms
                get< element_from<                   // v[0,Ns]
                    shape< N, N >, index< 0, Ns >>>( v.values ) * 
                determinant(                          // det of submatrix[0,Ns]
                    sub_tensor< index< 0, Ns >>( v ))) + ... );
}

// the determinant of a 1x1 matrix is the value of the only element
template< typename Types >
auto det_helper( Tensor< shape< 1, 1 >, Types > const& v, seq< 0 > )
{ return get< 0 >( v.values ); }

} // namespace detail


/**
 * Calculates the determinant of a square NxN tensor
 * 
 * @tparam N the size of one side of the square tensor
 * @tparam Types is a types<...> holder of a parameter pack of the N*N types of
 *  the tensor elements
 * @param v the tensor
 * @returns the determinant of the square tensor
 */
template< size_t N, typename Types >
auto determinant( Tensor< shape< N, N >, Types > const& v )
{ return detail::det_helper( v, make_seq< N >{} ); }

// forward declaration of cofactor
template< size_t N, typename Types >
auto cofactor( Tensor< shape< N, N >, Types > const& v );

// cofactor details
namespace detail {
template< typename Shape, typename Types, size_t... Is >
auto cofactor_helper( Tensor< Shape, Types > const& v, seq< Is... > )
{
    /**
     * the cofactor of an element of a square matrix is the alternating
     * determinant of the sub matrix formed by removing the row and column
     * of the given element
     */
    return make_tensor< Shape >( 
        (( index_from< Shape, Is >::is_even ? 1. : -1. ) *  // alternating term
        determinant(                                        // det(sub(i,j))
            sub_tensor< index_from< Shape, Is >>( v )))... );
}
} // namespace detail

/**
 * Calculates the cofactor matrix of a square NxN tensor
 * 
 * @tparam N the size of one side of a square tensor
 * @tparam Types the types<...> holder of a parameter pack of the types of the 
 *  square tensor
 * @param v the square tensor to calculate the cofactor matrix
 * @returns the cofactor matrix of the square tensor v
 */
template< size_t N, typename Types >
auto cofactor( Tensor< shape< N, N >, Types > const& v )
{ return detail::cofactor_helper( v, make_seq< shape< N, N >::elements_size >{} ); }

/**
 * Calculates the inverse of an NxN square tensor such that V * inv{V} == I
 * where I is the identity matrix.  The inverse of a square matrix is the 
 * transpose of the cofactor matrix divided by the determinant.
 * 
 * @tparam N the size of one side of the square tensor
 * @tparam Types is the types<...> holder of a pack of types of size N*N of the 
 *  types of the elements of the square tensor
 * @param v the square NxN tensor
 * @returns the inverse matrix of v
 */
template< size_t N, typename Types >
auto inverse( Tensor< shape< N, N >, Types > const& v )
{ return transpose( cofactor( v ).divide_scale( determinant( v ))); }

#ifndef NO_TENSOR_PRINTING
/**
 * print helpers
 */
using std::ostream;

template< typename Shape, typename Types >
ostream& operator<<( ostream& os, Tensor< Shape, Types > const& v );

// printing details
namespace detail {
template< typename Shape, typename Types, size_t... Is >
ostream& print_helper( ostream& os, Tensor< Shape, Types > const& v, seq< Is... > )
{ 
    os << "{ ";
    (( os << get< Is >( v.values ) << " " ), ... );
    os << "}";
    return os;
}
} // namespace detail

template< size_t... Ds, typename Types >
ostream& operator<<( ostream& os, Tensor< shape< Ds... >, Types > const& v )
{ return detail::print_helper( os, v, make_seq< shape< Ds... >::elements_size >{} ); }

#endif

} // namespace tensor

#endif