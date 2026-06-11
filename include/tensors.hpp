#ifndef __TENSOR_HPP__
#define __TENSOR_HPP__

//////////////////////// 
/// Tensors Library ///
//////////////////////
///
/// A tensor is a shaped array of arbitrary arithmetic values.  They themselves
/// can be combined with arithmetic operations. 
///
/// Tensor is the primary template in this library. It takes a Shape<...> as
/// the first template parameter.  THe remaining template parameters are the
/// types of the elements in row-major order.
///
/// Example: 
/// 
/// auto rotz = make_tensor< Shape< 3, 3 >>( 
///     cos( theta ), -sin( theta ), 0f,
///     sin( theta ),  cos( theta ), 0f,
///               0f,            0f, 1f    );
/// 
/// auto v = make_vector< 3 >( 
///     x, y, z );
///
/// auto rotated_vector = matmul( rotz, v );
///

#include "utility.hpp"

namespace tensors {

/// @brief default scalar type
using scalar_type = long double;

///////////////
/// Tensor ///
/////////////
///
/// A tensor is a tuple-like object imbued with a tensor shape defined above

// forward declaration
template< typename ShapeT, typename... Ts >
struct Tensor;

/////////////////////
/// Tensor Shape ///
///////////////////
///
/// @brief forward declaration of a tensor shape
template< size_t... >
struct Shape;

////////////////////////////////////////
/// Tensor Shape / Empty Base Class ///
//////////////////////////////////////
///
/// @brief an empty shape corresponds to a tensor with a single element.  It 
/// is also the base class of all Shapes, and so contains the size_t data 
/// member.
template< >
struct Shape< >
{  
    // empty shapes have size == 1
    static constexpr size_t size() { return 1; }
    // the number of dimensions in this shape
    static constexpr size_t degree() { return 0; }

    // instances of shapes are always equivalent 
    constexpr bool operator ==( Shape const& other ) const
    { return element() == element(); }
    constexpr bool operator !=( Shape const& other ) const
    { return element() != element(); }

    // the element and first index of an instance of an empty shape are always
    // zero since they describe a single-valued object
    constexpr size_t element() const { return _value % size(); }

protected:
    constexpr void set_value( size_t value )
    { _value = value; }

    constexpr size_t get_value() const
    { return _value; }

public:
    constexpr operator size_t() const { return element(); }
    constexpr size_t first() const { return element(); }

    constexpr size_t operator[]( size_t dim ) const
    { return element(); }

    constexpr Shape< 1 > insert( Shape here ) const;

    static consteval Shape from_element( size_t el )
    { return { el }; }

    constexpr Shape(): _value{ 0 } { }

protected:    
    constexpr Shape( size_t value ): _value{ value } { }

private:
    size_t _value;
};

////////////////////////////////////////////
/// Tensor Shape / Recursive Definition ///
//////////////////////////////////////////
///
/// @brief a non-empty shape of an array. An instance of this shape acts as an 
/// index into an array:
/// 
/// auto index_2by2 = Shape< 2, 2 >;
/// int arr[] = { 0b00, 0b01, 
///               0b10, 0b11 };
/// assert( arr[ index_2by2{ 0, 0 }] == 0b00 );
/// assert( arr[ index_2by2{ 0, 1 }] == 0b01 );
/// assert( arr[ index_2by2{ 1, 0 }] == 0b10 );
/// assert( arr[ index_2by2{ 1, 1 }] == 0b11 );
///
/// @tparam First is the size of the first dimension
/// @tparam Rest... are the remaining dimension sizes
template< size_t First, size_t... Rest >
struct Shape< First, Rest... >: Shape< Rest... >
{ 
    // the size of the first dimension is First
    static constexpr size_t first_size() 
    { return First; }

    // the overall size is the product of each size dimension
    static constexpr size_t size() 
    { return first_size() * Shape< Rest... >::size(); }

    // the number of dimensions in this shape
    static constexpr size_t degree() 
    { return 1 + sizeof...( Rest ); }

    // concatenate this shape with another shape
    template< size_t... Others >
    constexpr Shape< First, Rest..., Others... > cat( Shape< Others... > other )
    { return Shape< First, Rest..., Others... >::from_element( 
        element() * other.size() + other.element() ); }

    constexpr bool operator ==( Shape const& other ) const
    { return Shape< Rest... >::operator ==( other ); }
    constexpr bool operator !=( Shape const& other ) const
    { return Shape< Rest... >::operator !=( other ); }

    static constexpr Shape from_element( size_t elem )
    { return Shape{ elem / Shape< Rest... >::size(), 
        Shape< Rest... >::from_element( elem % Shape< Rest... >::size() )}; }

    constexpr size_t element() const { return Shape<>::get_value() % size(); }
    constexpr operator size_t() const { return element(); }

    constexpr Shape< First+1, ( Rest+1 )... > 
    insert( Shape< First+1, ( Rest+1 )... > here ) const
    { return Shape< First+1, ( Rest+1 )... >{ first() < here ? first() : 1 + first(), 
            Shape< Rest... >::insert( here )}; }

    constexpr size_t first() const 
    { return element() / Shape< Rest... >::size(); }

    constexpr Shape< Rest... > rest() const
    { return *this; }

    constexpr size_t operator[]( size_t dim ) const
    {
        if( dim == 0 ) 
            return first();
        return Shape< Rest... >::operator[]( dim - 1 );
    }

    /// @brief constructor of an instance of this shape from the indices
    /// of each degree
    template< typename... Indices >
    requires(( is_convertible_v< Indices, size_t > and ... ))
    constexpr Shape( size_t first, Indices... rest ):
        Shape{ first, Shape< Rest... >{ static_cast< size_t >( rest )... }}
    { }

    explicit constexpr Shape( size_t first, Shape< Rest... > rest ): 
        Shape< Rest... >{ rest } 
    { Shape<>::set_value( rest.element() + first * Shape< Rest... >::size()); }
};

/// @brief Empty shape insert method implementation.  Since this must return
/// a concrete instance to the recursively defined Shape template we must 
/// implement it outside of the Shape<> class definition
constexpr inline Shape< 1 > Shape<>::insert( Shape ) const
{ return { get_value() }; }

#ifndef NDEBUG
/// ensure that the Shape class is always just the size of a size_t
static_assert( sizeof( Shape<> ) == sizeof( Shape<2,2,2> ) and
    sizeof( Shape<> ) == sizeof( size_t ));
static_assert( Shape< 2, 2 >{ 1, 1 } == Shape< 2, 2 >{ 1, 1 } );
static_assert( Shape< 2, 2 >{ 1, 1 }.element() == 
    Shape< 2, 2, 2 >{ 0, 1, 1 }.element() );
static_assert( Shape< 2, 2 >{ 1, 1 } == Shape< 2 >{ 1 }.cat( Shape< 2 >{ 1 }));
static_assert( Shape< 2, 2 >{ 1, 1 }.element() == 3 );
static_assert( Shape< 2, 2 >::from_element( 3 ) == Shape< 2, 2 >{ 1, 1 });
// static_assert( Shape< 2, 2 >{ 1, 1 }.of( make_tuple( 0, 1, 2, 3 )) == 3 );

static_assert( 0 == Shape< 1, 1 >{ 0, 0 }.element() );
static_assert( 0 == Shape< 2, 2 >{ 0, 0 }.element() );
static_assert( 1 == Shape< 2, 2 >{ 0, 1 }.element() );
static_assert( 2 == Shape< 2, 2 >{ 1, 0 }.element() );
static_assert( 3 == Shape< 2, 2 >{ 1, 1 }.element() );

static_assert( 0 == Shape< 2, 3 >{ 0, 0 }.element() );
static_assert( 1 == Shape< 2, 3 >{ 0, 1 }.element() );
static_assert( 2 == Shape< 2, 3 >{ 0, 2 }.element() );
static_assert( 3 == Shape< 2, 3 >{ 1, 0 }.element() );
static_assert( 4 == Shape< 2, 3 >{ 1, 1 }.element() );
static_assert( 5 == Shape< 2, 3 >{ 1, 2 }.element() );

static_assert( 0 == Shape< 3, 2 >{ 0, 0 }.element() );
static_assert( 1 == Shape< 3, 2 >{ 0, 1 }.element() );
static_assert( 2 == Shape< 3, 2 >{ 1, 0 }.element() );
static_assert( 3 == Shape< 3, 2 >{ 1, 1 }.element() );
static_assert( 4 == Shape< 3, 2 >{ 2, 0 }.element() );
static_assert( 5 == Shape< 3, 2 >{ 2, 1 }.element() );

static_assert( 0 == Shape< 2, 2, 2 >{ 0, 0, 0 }.element() );
static_assert( 1 == Shape< 2, 2, 2 >{ 0, 0, 1 }.element() );
static_assert( 2 == Shape< 2, 2, 2 >{ 0, 1, 0 }.element() );
static_assert( 3 == Shape< 2, 2, 2 >{ 0, 1, 1 }.element() );
static_assert( 4 == Shape< 2, 2, 2 >{ 1, 0, 0 }.element() );
static_assert( 5 == Shape< 2, 2, 2 >{ 1, 0, 1 }.element() );
static_assert( 6 == Shape< 2, 2, 2 >{ 1, 1, 0 }.element() );
static_assert( 7 == Shape< 2, 2, 2 >{ 1, 1, 1 }.element() );

static_assert( 0 == Shape< 2, 2, 1 >{ 0, 0, 0 }.element() );
static_assert( 3 == Shape< 2, 2, 1 >{ 1, 1, 0 }.element() );

static_assert( Shape< 2, 2 >{ 1, 1 }.cat( 
    Shape< 1 >{ 0 }) == Shape< 2, 2, 1 >{ 1, 1, 0 });
static_assert( 
    std::array< int, 4 >{ 0b00, 0b01, 0b10, 0b11 }[ Shape< 2, 2 >{ 1, 0 }] == 
        0b10 );
static_assert( 
    std::array< int, 16 >{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }
        [ Shape< 4, 4 >{ 1, 0 }] == 4 );
#endif // DEBUG

//////////////////////////////////////////////
/// Tensor Shape / Implementation Details ///
////////////////////////////////////////////
///

namespace detail {

// forward decl
template< size_t I, typename S >
struct ShapeGet;

/// @brief helper to get the zeroeth element of a shape
/// @tparam First is the element of the shape
/// @tparam ...Rest are the remaining shape elements
///
template< size_t First, size_t... Rest >
struct ShapeGet< 0, Shape< First, Rest... >>
{ 
    static constexpr size_t value( Shape< First, Rest... > shp ) 
    { return shp.first(); } 
};

/// @brief helper to get the Ith element of a shape
/// @tparam I the element of the shape to extract
/// @tparam First the first element of the shape
/// @tparam ...Rest the remaining elements of a shape
template< size_t I, size_t First, size_t... Rest >
requires( isgreater( I, 0 ))
struct ShapeGet< I, Shape< First, Rest... >>
{ 
    static constexpr size_t value( Shape< First, Rest... > shp ) 
    { return ShapeGet< I-1, Shape< Rest... >>::value( shp ); } 
};

// forward decl
template< typename T, typename U >
struct ShapeCat;

/// @brief helper to concatenate two shapes
/// @tparam ...Ts the indices of the left shape
/// @tparam ...Us the indices of the right shape
template< size_t... Ts, size_t... Us >
struct ShapeCat< Shape< Ts... >, Shape< Us... >>
{ using type = Shape< Ts..., Us... >; };

/// @brief trait to determine if a type is a Shape<...>
/// @tparam T the type to test
template< typename T >
struct IsShape : std::integral_constant< bool, false > { };

/// @brief specialization to identify a shape type
/// @tparam ...Is the indices of the shape
template< size_t... Is >
struct IsShape< Shape< Is... >> : std::integral_constant< bool, true > { };

/// @brief determines if element I is an odd element of a tensor with shape S
/// @tparam I the element to test
/// @tparam S the shape of the tensor
template< size_t I, typename S >
struct IsElementEven;

/// @brief the zero-th element of a shape is always even
template<>
struct IsElementEven< 0, Shape<>>: integral_constant< bool, true > { };

/// @brief determines if element I is an odd element of a tensor with shape S
/// @tparam I the element to test
/// @tparam First is the first size of the shape
/// @tparam ...Rest are the remaining sizes
template< size_t I, size_t First, size_t... Rest >
struct IsElementEven< I, Shape< First, Rest... >>
{
    // this is the first index of element I in a tensor with shape S
    static constexpr size_t first = I / Shape< Rest... >::size();
    // this is the element of a Shape< Rest... > tensor
    static constexpr size_t rest = I % Shape< Rest... >::size();

    static constexpr bool is_first_even = ( first % 2 == 0 );
    static constexpr bool is_rest_even = 
        IsElementEven< rest, Shape< Rest... >>::value;

    // I is considered even if the first xor the rest is even
    static constexpr bool value = ( is_first_even and is_rest_even ) or
        ( not is_first_even and not is_rest_even );
};

// forward decl
template< typename S, typename Seq >
struct ShapeInsertAt;

// inserting an empty sequence into an empty shape 
template< >
struct ShapeInsertAt< Shape< >, seq< >>
{ static consteval Shape< > insert( Shape< > ) { return { }; } };

/// @brief inserts an element into each dimension of the shape identified by a
/// sequence.  Effectively the opposite of sub-tensors
/// @tparam First the first size of the shape
/// @tparam ...Rest the remaining sizes
/// @tparam I the position in the first dimension of this shape to insert
/// @tparam ...Is the remaining positions
template< size_t First, size_t... Rest, size_t I, size_t... Is >
struct ShapeInsertAt< Shape< First, Rest... >, seq< I, Is... >>
{
    using type = Shape< 1 + First, ( 1 + Rest )... >;
    static constexpr type insert( Shape< First, Rest... > from )
    { return type{ from.first() < I ? from.first() : 1 + from.first(),  
        ShapeInsertAt< Shape< Rest... >, seq< Is... >>::insert( from ) }; }
};  

// forard declaration
template< typename S, size_t I >
struct ShapeInsertAtElement;

/// @brief inserting into an empty shape is still empty and only supported if
/// the index is 0
template< >
struct ShapeInsertAtElement< Shape< >, 0 >
{ static consteval Shape< > insert( Shape< > ) { return {}; } };

/// @brief inserting a row at each dimension of a tensor at element I
/// @tparam First the first size of the tensor
/// @tparam ...Rest the reamining sizes of the tensor
/// @tparam I the position to insert into the original tensor
template< size_t First, size_t... Rest, size_t I  >
struct ShapeInsertAtElement< Shape< First, Rest... >, I >
{
    using type = Shape< 1 + First, ( 1 + Rest )... >;
    using rest_type = Shape<( 1 + Rest )... >;
    static constexpr size_t first = I / rest_type::size();
    static constexpr size_t rem = I % rest_type::size();

    static constexpr type insert( Shape< First, Rest... > from )
    { return type{ from.first() < first ? from.first() : 1 + from.first(),  
        ShapeInsertAtElement< Shape< Rest... >, rem >::insert( from ) }; }
};  

// forward declaration
template< size_t I, typename S >
struct ShapeElement;

template<>
struct ShapeElement< 0, Shape<> >: integral_constant< size_t, 0 > { };

/// @brief extracts the first size from a shape
/// @tparam First the first size of the shape
/// @tparam ...Rest the remaining sizes
template< size_t First, size_t... Rest >
struct ShapeElement< 0, Shape< First, Rest... >>:
    std::integral_constant< size_t, First > { };

/// @brief extract the Ith size of a shape
/// @tparam I the dimension to extract the size
/// @tparam First the first size of this shape
/// @tparam ...Rest the remaining sizes of this shape
template< size_t I, size_t First, size_t... Rest >
struct ShapeElement< I, Shape< First, Rest... >>:
    std::integral_constant< size_t, 
        ShapeElement< I-1, Shape< Rest... >>::value > 
{ };

// forward decl
template< size_t I, typename S >
struct RemoveShapeElement;

/// @brief helper to remove the first size from a shape
/// @tparam First the first size of the shape
/// @tparam ...Rest the reamining sizes
template< size_t First, size_t... Rest >
struct RemoveShapeElement< 0, Shape< First, Rest... >>
{ 
    using type = Shape< Rest... >; 
    static constexpr type value( Shape< First, Rest... > shp )
    { return shp; }
};

/// @brief helper to remove the Ith size from a shape
/// @tparam I the size to remove
/// @tparam First the first size of a shape
/// @tparam ...Rest the remaining sizes
template< size_t I, size_t First, size_t... Rest >
requires( isgreater( I, 0 ))
struct RemoveShapeElement< I, Shape< First, Rest... >>
{ 
    using type = ShapeCat< Shape< First >, 
        typename RemoveShapeElement< I - 1, Shape< Rest... >>::type >::type; 

    static constexpr type value( Shape< First, Rest... > shp )
    { return Shape< First >{ shp.first() }.cat(
        RemoveShapeElement< I-1, Shape< Rest... >>::value( shp )); }
};

/// @brief a trait to calculate the shape of a tensor contracted on two of its
/// dimensions
/// @tparam I the first dimension to contract
/// @tparam J the second dimension to contract
/// @tparam S the shape of the tensor
template< size_t I, size_t J, typename S >
requires( isless( I, J ) and 
    ShapeElement< I, S >::value == ShapeElement< J, S >::value )
struct ContractShape
{ using type = RemoveShapeElement< I, 
    typename RemoveShapeElement< J, S >::type >::type; };

// forward decl
template< size_t K, size_t I, typename S >
struct InsertShapeElement;

/// @brief insert K at the first dimension of a shape
/// @tparam K the size to insert
/// @tparam ...Is the sizes of the shape dimensions
template< size_t K, size_t... Is >
struct InsertShapeElement< K, 0, Shape< Is... >>
{ 
    using type = Shape< K, Is... >; 

    static constexpr type value( size_t k, Shape< Is... > shp )
    { return type{ k, shp }; }
};

/// @brief insert K at dimension I of a shape
/// @tparam K the size to insert
/// @tparam I the dimension to insert at
/// @tparam First the size of the first dimension of the shape
/// @tparam ...Rest the sizes of the remaining dimensions
template< size_t K, size_t I, size_t First, size_t... Rest >
requires( isgreater( I, 0 ))
struct InsertShapeElement< K, I, Shape< First, Rest... >>
{ 
    using type = ShapeCat< Shape< First >, 
        typename InsertShapeElement< K, I-1, Shape< Rest... >>::type >::type; 

    static constexpr type value( size_t k, Shape< First, Rest... > shp )
    { return Shape< First >{ shp.first() }.cat( 
        InsertShapeElement< K, I-1, Shape< Rest... >>::value( k, shp )); }
};

// forward decl
template< typename S >
struct SubTensorShape;

/// @brief reduce each dimension of a tensor's shape by 1
/// @tparam ...Sizes are the sizes of each dimension of the shape
template< size_t... Sizes >
requires(( isgreater( Sizes, 1 ) and ... ))
struct SubTensorShape< Shape< Sizes... >>
{ using type = Shape< (Sizes - 1)... >; };

/// @brief reverses a contraction operation on dimensions I and J
/// @tparam K the size of the contracted dimensions
/// @tparam I the first contraction dimension
/// @tparam J the second contraction dimension
/// @tparam S the shape to uncontract
template< size_t K, size_t I, size_t J, typename S >
requires( isless( I, J ))
struct UncontractShape
{ 
    using type = InsertShapeElement< K, J, 
        typename InsertShapeElement< K, I, S >::type >::type; 

    static constexpr type value( size_t k, S shp )
    { return insert_shape_element< K, J >( k, 
        insert_shape_element< K, I >( k, shp )); }
};

/// @brief helper class to transpose indices I and J of a shape S
/// @tparam I the first dimension to transpose
/// @tparam J the second dimension to transpose
/// @tparam S the shape to transpose the I and J dimensions
template< size_t I, size_t J, typename S >
struct TransposeShape
{ 
    static constexpr size_t ith = ShapeElement< I, S >::value;
    static constexpr size_t jth = ShapeElement< J, S >::value;

    using base_type = detail::RemoveShapeElement< I, 
        typename detail::RemoveShapeElement< J, S >::type >::type;
    using type = InsertShapeElement< ith, J, 
        typename InsertShapeElement< jth, I, base_type >::type >::type;

    /// @brief calculates the value of an index of shape S with the Ith and Jth
    /// dimensions transposed
    /// @param shp the shape to transpose
    /// @return the transposed shape
    static constexpr type value( S shp )
    {
        size_t i = shape_get< I >( shp );
        size_t j = shape_get< J >( shp );

        auto base = remove_shape_element< I >( 
            remove_shape_element< J >( shp ));

        return insert_shape_element< ith, J >( i,
            insert_shape_element< jth, I >( j, base ));
    }
};

/// @brief helper class for creation of uniformly typed tensors
template< typename S, typename T, typename Seq >
struct UniformTensorHelper;

template< typename S, typename T, size_t... Is >
struct UniformTensorHelper< S, T, seq< Is... >>
{ using type = Tensor< S, std::conditional_t< Is == Is, T, T >... >; };

// forward decl
template< size_t I, typename TensorT >
struct TensorElement;

/// @brief the type of the Ith element of a tensor
/// @tparam I the element
/// @tparam S the shape of the tensor
/// @tparam ...Ts the types of the tensor
template< size_t I, typename S, typename... Ts >
struct TensorElement< I, Tensor< S, Ts... >>
{ using type = tuple_element_t< I, tuple< Ts... >>; };

/// @brief trait to identify tensor types
/// @tparam T type type to test
template< typename T >
struct IsTensor: integral_constant< bool, false > { };

/// @brief trait to identify tensor types
/// @tparam S the shape of the tensor
/// @tparam ...Ts are the types of the tensor elements
template< typename S, typename... Ts >
struct IsTensor< Tensor< S, Ts... >>: integral_constant< bool, true > { };

template< typename TensorT, size_t... Is >
TensorT zero_tensor_helper( seq< Is... > )
{ return { static_cast< typename TensorElement< Is, TensorT >::type >( 
    Is - Is )... }; }

template< typename, typename... >
struct ExtendVector;

template< size_t N, typename... Ts, typename... Us >
struct ExtendVector< Tensor< Shape< N >, Ts... >, Us... >
{
    using vector_type = Tensor< Shape< N >, Ts... >;
    using type = Tensor< Shape< N + sizeof...( Us ) >, Ts..., Us... >;

    template< size_t... Is >
    static constexpr type make_helper( vector_type const& vec, Us const&... values, seq< Is... > )
    { return { tensor_get< Is >( vec )..., values... }; }

    static constexpr type make( vector_type const& vec, Us const&... values )
    { return make_helper( vec, values..., make_seq< sizeof...( Ts )>{} ); }
};

/// @brief trait to identify if the given tensor has shape S
/// @tparam T the type of the tensor to test
/// @tparam S the shape to compare
template< typename T, typename S >
struct IsTensorShape: 
    integral_constant< bool, std::is_same_v< S, typename T::shape_type >> { };

} // namespace detail

/////////////////////////////////
/// Tensor Shape / Utilities ///
///////////////////////////////
///
/// These are traits and methods to use and manipulate tensor shapes
///

/// @brief method to get the Ith element of a shape
/// @tparam S the shape to extract from
/// @tparam I the element to extract
/// @param shp the shape to extract from
/// @return the size of element I in shape S
template< size_t I, typename S >
constexpr size_t shape_get( S shp )
{ return detail::ShapeGet< I, S >::value( shp ); }

/// @brief concatenate two shapes
/// @tparam T the left shape
/// @tparam U the right shape
template< typename T, typename U >
using shape_cat_t = detail::ShapeCat< T, U >::type;

/// @brief create a shape from a constant tuple
/// @tparam ...Ts remaining index types (must be size_t)
/// @param tup the const tuple to turn into a Shape<...>
/// @return a Shape<...>
template< typename... Ts >
consteval auto shape_from_tuple( tuple< size_t, Ts... > tup )
{ return Shape< get< 0 >( tup ) >::cat( 
    shape_from_tuple< Ts... >( remove_first( tup ))); }

/// @brief an empty shape from an empty tuple
/// @param tup an empty tuple
/// @return an empty shape
consteval Shape< > shape_from_tuple( tuple< > tup )
{ return { }; }

/// @brief trait to determine if a type is a shape
/// @tparam T the type to test
template< typename T >
constexpr bool is_shape_v = detail::IsShape< T >::value;

/// @brief the concept of a tensor shape
/// @tparam T is the type to test
template< typename T >
concept shape = is_shape_v< T >;

/// @brief determines if I is an even element of a tensor with shape S
/// @tparam I the element to test
/// @tparam S the shape of the tensor
template< size_t I, shape S >
static constexpr bool is_element_even_v = detail::IsElementEven< I, S >::value;

/// @brief create a shape with one larger size at each dimension and adjust the
/// indices of the shape instance relative to the supplied sequence
/// @tparam Seq a sequence representing the position to insert
/// @tparam S the shape of the original tensor
/// @param shp the indices to adjust
/// @return an adjusted shape to the supertensor
template< typename Seq, shape S >
consteval auto shape_insert_at( S shp )
{ return detail::ShapeInsertAt< S, Seq >::insert( shp ); }

/// @brief the shape and index of a supertensor with rows added at `here`
/// @tparam First the first size of the shape
/// @tparam ...Rest the remaining sizes of the shape
/// @param here the position to insert
/// @param shp the index to adjust
/// @return the index into a supertensor
template< size_t First, size_t... Rest >
consteval auto shape_insert_at( Shape< 1 + First, ( 1 + Rest )... > here, 
    Shape< First, Rest... > shp )
{ return shp.insert( here ); }

/// @brief the shape and index of a supeprtensor with rows added at element I
/// @tparam I the element to adjust the index
/// @tparam S the original shape
/// @param shp the index to adjust
/// @return the adjusted index of the supertensor
template< size_t I, shape S >
consteval auto shape_insert_at_element( S shp )
{ return detail::ShapeInsertAtElement< S, I >::insert( shp ); }

/// @brief alias of the Ith element of shape S
/// @tparam I the element of shape S to extract
/// @tparam S the shape to extract from
template< size_t I, shape S >
constexpr size_t shape_element_v = detail::ShapeElement< I, S >::value;

/// @brief remove the Ith size from a shape
/// @tparam I the size to remove from the shape
/// @tparam S the type of the shape
/// @param shp an instance of this shape to adjust
/// @return the adjusted, shrunken shape 
template< size_t I, shape S >
constexpr auto remove_shape_element( S shp )
{ return detail::RemoveShapeElement< I, S >::value( shp ); }

/// @brief alias to the contracted shape of a tensor
/// @tparam I the first contraction dimension
/// @tparam J the second contraction dimension
/// @tparam S the shape to contract
template< size_t I, size_t J, shape S >
using contract_shape_t = detail::ContractShape< I, J, S >::type;

#ifndef NDEBUG
// verify shape contraction
static_assert( is_same_v< Shape< 1, 2, 4 >, 
    contract_shape_t< 2, 3, Shape< 1, 2, 3, 3, 4 >>> );
#endif // DEBUG

/// @brief insert a dimension of size K at dimension I of shape S
/// @tparam K the size of the dimension to insert
/// @tparam I the dimension to insert at
/// @tparam S the shape to insert into
/// @param k the value of the index at the inserted dimension ( k < K )
/// @param shp the indices of the remaining dimensions
/// @return the new index with a new dimension of size K inserted at dimension I
/// with value k
template< size_t K, size_t I, shape S >
constexpr detail::InsertShapeElement< K, I, S >::type 
insert_shape_element( size_t k, S shp )
{ return detail::InsertShapeElement< K, I, S >::value( k, shp ); }

//// @brief alias that reduces shape S by one in each dimension
template< shape S >
using sub_tensor_shape_t = detail::SubTensorShape< S >::type;

#ifndef NDEBUG
// tests of shape manipulation
static_assert( Shape< 2 >{ 1 }.element() == insert_shape_element< 2, 0 >( 1, Shape< >{} ).element() );
static_assert( Shape< 2, 2 >{ 1, 0 }.element() == insert_shape_element< 2, 0 >( 1, Shape< 2 >{ 0 } ).element() );
static_assert( Shape< 2, 2 >{ 1, 1 }.element() == insert_shape_element< 2, 0 >( 1, Shape< 2 >{ 1 } ).element() );
static_assert( Shape< 2, 3 >{ 1, 2 }.element() == insert_shape_element< 2, 0 >( 1, Shape< 3 >{ 2 } ).element() );
static_assert( Shape< 2, 3 >{ 1, 2 }.element() == insert_shape_element< 3, 1 >( 2, Shape< 2 >{ 1 } ).element() );
static_assert( Shape< 2, 2 >{ 0, 1 } == insert_shape_element< 2, 0 >( 0, Shape< 2 >{ 1 }) );
static_assert( Shape< 3, 2 >{ 2, 1 } == insert_shape_element< 3, 0 >( 2, Shape< 2 >{ 1 }) );
static_assert( Shape< 2, 3 >{ 1, 2 }.element() == insert_shape_element< 3, 1 >( 2, Shape< 2 >{ 1 }).element() );
static_assert( Shape< 3, 3 >{ 2, 2 } == insert_shape_element< 3, 0 >( 2, Shape< 3 >{ 2 }) );
#endif // DEBUG

/// @brief reverses the contraction operation on dimensions I and J of shape S
/// @tparam K the size of the contracted dimensions
/// @tparam I the first contraction dimension
/// @tparam J the second contraction dimension
/// @tparam S the shape to uncontract
template< size_t K, size_t I, size_t J, shape S >
using uncontract_shape_t = detail::UncontractShape< K, I, J, S >::type;

#ifndef NDEBUG
// tests of the uncontract operation
static_assert( is_same_v< Shape< 3, 3 >, uncontract_shape_t< 3, 0, 1, Shape<>>> );
static_assert( is_same_v< Shape< 1, 2, 3, 3, 4 >, uncontract_shape_t< 3, 2, 3, Shape< 1, 2, 4 >>> );
#endif // DEBUG

/// @brief reverses the contraction operation on dimensions I and J of shape S
/// and calculate the index into the uncontracted tensor
/// @tparam K the size of the contracted dimension
/// @tparam I the first contraction index
/// @tparam J the second contraction index
/// @tparam S the shape of the contracted tensor
/// @param k the value of each of the contraction indices
/// @param shp the contracted index of shape S
/// @return shp is grown by inserting size K at final indices I and J and
/// returning an index with the uncontracted shape with value k at the I and J
/// dimensions
template< size_t K, size_t I, size_t J, shape S >
constexpr uncontract_shape_t< K, I, J, S >
uncontract_index( size_t k, S shp )
{ return detail::UncontractShape< K, I, J, S >::value( k, shp ); }

#ifndef NDEBUG
// tests of uncontraction
static_assert( uncontract_index< 1, 0, 1 >( 0, Shape< >{} ).element() == Shape< 1, 1 >{ 0, 0 }.element() );
static_assert( uncontract_index< 2, 0, 1 >( 0, Shape< >{} ).element() == Shape< 2, 2 >{ 0, 0 }.element() );
static_assert( uncontract_index< 2, 0, 1 >( 0, Shape< 1 >{ 0 } ).element() == Shape< 2, 2, 1 >{ 0, 0, 0 }.element() );
static_assert( uncontract_index< 2, 0, 1 >( 1, Shape< 1 >{ 0 } ).element() == Shape< 2, 2, 1 >{ 1, 1, 0 }.element() );
static_assert( uncontract_index< 2, 0, 1 >( 1, Shape< >{} ).element() == Shape< 2, 2 >{ 1, 1 }.element() );
static_assert( uncontract_index< 3, 0, 1 >( 1, Shape< >{} ).element() == 1 + 3*1 );
static_assert( uncontract_index< 3, 2, 3 >( 2, Shape< 1, 2, 4 >{ 0, 1, 2 }).element() == 2 + 4*( 2 + 3*( 2 + 3*( 1 + 1 * 0 ))) );
// static_assert( Shape< 1, 2, 3, 3, 4 >{ 0, 1, 2, 2, 3 } == uncontract< 3, 2, 3 >( 2, Shape< 1, 2, 4 >{ 0, 1, 3 }) );
#endif // DEBUG

/// @brief the shape of a tensor transposed on the Ith and Jth indices
/// @tparam I the first dimension to transpose
/// @tparam J the second dimension to transpose
/// @tparam S the shape of the tensor
template< size_t I, size_t J, shape S >
using transpose_shape_t = detail::TransposeShape< I, J, S >::type;

/// @brief transposes dimensions I and J of the index shp into a tensor with 
/// shape S
/// @tparam I the first dimension to transpose 
/// @tparam J the second dimension to transpose
/// @tparam S the shape of the tensor
/// @param shp an index into a tensor of shape S
/// @return the transposed index 
template< size_t I, size_t J, shape S >
constexpr transpose_shape_t< I, J, S > transpose_shape( S shp )
{ return detail::TransposeShape< I, J, S >::value( shp ); }

/////////////////////////////
/// Tensor / Null Tensor ///
///////////////////////////
///
/// @brief a null tensor implements the bare minimum class methods required
/// for any tensor class
template< >
struct Tensor< Shape< 0 >>
{
    using shape_type = Shape<>;
    static constexpr size_t size() { return 0; }
    static constexpr Tensor zero() { return {}; }
};

/// helper type for a null tensor
using null_tensor_t = Tensor< Shape< 0 >>;

/// @brief a uniformly typed tensor with run-time shape
/// TODO: implement using something like std::any
template< typename T >
struct DynamicTensor
{
/* ... */
};

///////////////////////////////////
/// Tensor / Arbitrarily Typed ///
/////////////////////////////////
///
/// @brief a shaped array of arbitrary arithmetic types
/// @tparam T the type of the first element of the tensor
/// @tparam ...Ts the remaining types
/// @tparam S the shape of the tensor
template< shape S, typename T, typename... Ts >
requires( S::size() == 1 + sizeof...( Ts ) and 
    (( not is_same_v< T, Ts > ) or ... or false ))
struct Tensor< S, T, Ts... > : tuple< T, Ts... >
{
    using shape_type = S;
    static consteval size_t size() { return 1 + sizeof...( Ts ); }
    // static consteval shape_type shape() { return {}; }
    // TODO: get rid of the static_cast
    template< typename... Indices >
    static constexpr shape_type index( Indices... Is )
    { return { static_cast< size_t >( Is )... }; }

    template< typename OtherT, size_t... Is >
    constexpr void equal_helper( OtherT const& other, seq< Is... > )
    { (( get< Is >( *this ) = get< Is >( other )), ... ); }

    template< typename OtherT, size_t... Is >
    constexpr tuple< T, Ts... > tuple_init_helper( OtherT const& other, seq< Is... > )
    { return make_tuple( static_cast< tuple_element_t< Is, tuple< T, Ts... >>>( get< Is >( other ))... ); }

    template< size_t... Is >
    static constexpr Tensor zero_helper( seq< Is... > )
    { return { static_cast< tuple_element_t< Is, tuple< T, Ts... >>>( 0 )... }; }

    static constexpr Tensor zero()
    { return zero_helper( make_seq< size() >{} ); }

    template< typename... Us >
    constexpr Tensor& operator=( Tensor< shape_type, Us... > const& other )
    {
        if( &other != this )  
            equal_helper( other, make_seq< size() >{} );
        return *this;
    }

    template< typename... Us >
    requires( size() == sizeof...( Us ))
    constexpr Tensor& operator=( tuple< Us... > const& other )
    { 
        equal_helper( other, make_seq< size() >{} );
        return *this;
    }

private:
    template< typename U, size_t I, size_t... Is >
    void set_elements_helper( U value, seq< I, Is... > )
    { 
        get< I >( *this ) = static_cast< T >( value );
        (( get< Is >( *this ) = static_cast< Ts >( value )), ... );
    }

public:
    constexpr Tensor( ) = default;
    constexpr Tensor( int val )
    { set_elements_helper( val, make_seq< shape_type::size() >{} ); }
    constexpr Tensor( T t, Ts... ts ): tuple< T, Ts... >{ t, ts... } { }
    template< typename... Us >
    constexpr Tensor( Tensor< shape_type, Us... > const& other ):
        tuple< T, Ts... >{ tuple_init_helper( other, make_seq< size() >{} ) }
    { }
};

/////////////////////////////////
/// Tensor / Uniformly Typed ///
///////////////////////////////
///
/// @brief a shaped array of uniform arithmetic types.  This is useful for 
/// concrete expressions and general calculations. We aim it to be fully
/// constexpr so that it can be used at compile time.
///
/// example: 
/// auto v = make_tensor< Shape< 2, 2 >>( 
///     cos( angle ), -sin( angle ),
///     sin( angle ),  cos( angle ) );
///
///
/// @tparam T is the type of the elements of the tensor
/// @tparam ...Ts the remaining types which must be the same as T
/// @tparam S the shape of the tensor
template< shape S, typename T, typename... Ts >
requires( S::size() == 1 + sizeof...( Ts ) and ( is_same_v< T, Ts > and ... ))
struct Tensor< S, T, Ts... >: std::array< T, 1 + sizeof...( Ts )>
{
    using shape_type = S;
    using value_type = T;
    static consteval size_t size() { return 1 + sizeof...( Ts ); }
    using container_type = std::array< T, size() >;

    template< typename... Indices >
    static constexpr shape_type index( Indices... Is )
    { return { static_cast< size_t >( Is )... }; }

    using container_type::begin;
    using container_type::end;
    using container_type::operator[];

    static constexpr Tensor zero()
    { return zero_helper( make_seq< size() >{} ); }

    constexpr Tensor( ) = default;
    constexpr Tensor( int value )
    { std::fill( begin(), end(), static_cast< T >( value )); }
    constexpr Tensor( value_type t, Ts... ts ): 
        std::array< value_type, size() >{ t, ts... } 
    { }

    template< typename U, typename... Us >
    requires( is_convertible_v< value_type, U > and 
        ( is_convertible_v< value_type, Us > and ... ))
    constexpr Tensor( Tensor< shape_type, U, Us... > const& other ):
        std::array< value_type, size() >{ 
            array_init_helper( other, make_seq< size() >{} )}
    { }

private:
    template< size_t... Is >
    static constexpr Tensor zero_helper( seq< Is... > )
    { return { static_cast< T >( Is - Is )... }; }

    template< typename OtherT, size_t... Is >
    constexpr std::array< value_type, size() >
    array_init_helper( OtherT const& other, seq< Is... > )
    { return { get< Is >( other )... }; }

};

///////////////////////////
/// Tensor / Utilities ///
/////////////////////////
///
/// Traits and methods to manipulate tensors and tensor types
///

/// @brief type alias for uniformly typed tensors
template< shape S, typename T >
using uniform_tensor_t = 
    detail::UniformTensorHelper< S, T, make_seq< S::size() >>::type;

/// @brief get the Ith element of a tensor
/// @tparam TensorT the type of the tensor
/// @tparam I the element to get
/// @param ten an instance of the tensor containing the desired value
/// @return the value of the Ith element of tensor ten
template< size_t I, typename TensorT >
constexpr auto const& tensor_get( TensorT const& ten )
{ return get< I >( ten ); }

/// @brief get the Ith element of a tensor
/// @tparam TensorT the type of the tensor
/// @tparam I the element to get
/// @param ten an instance of the tensor containing the desired value
/// @return the value of the Ith element of tensor ten
template< size_t I, typename TensorT >
constexpr auto& tensor_get( TensorT& ten )
{ return get< I >( ten ); }

/// @brief extract an element from a tensor using a non-const index
/// @tparam T the type of the element to extract
/// @tparam TensorT the type of the tensor
/// @param element the runtime element to extract
/// @param ten the tensor containing the desired values
/// @return the value of ten at index element
template< typename T, typename TensorT >
constexpr T tensor_at( size_t element, TensorT const& ten )
{ return any_cast< T >( tuple_access( element, ten )); }

/// @brief trait to extract the shape of a tensor
/// @tparam TensorT the type of the tensor
template< typename TensorT >
struct tensor_shape
{ using type = TensorT::shape_type; };

/// @brief shape of a tensor
/// @tparam TensorT the tensor type
template< typename TensorT >
using tensor_shape_t = tensor_shape< TensorT >::type;

/// @brief the type of the Ith element of a tensor
/// @tparam TensorT the tensor type
template< size_t I, typename TensorT >
using tensor_element_t = detail::TensorElement< I, TensorT >::type;

/// @brief construct a tensor of shape S from the provided values
/// @tparam S the shape of the tensor
/// @tparam ...Ts the types of the tensor elements
/// @param ...ts the values of the tensor elements
/// @return a tensor of shape S with types ...Ts and values ...ts
template< shape S, typename... Ts >
requires( S::size() == sizeof...( Ts ))
constexpr Tensor< S, Ts... > make_tensor( Ts... ts )
{ return { ts... }; }

/// @brief determines if a type is a tensor
/// @tparam T the type to test
template< typename T >
constexpr bool is_tensor_v = detail::IsTensor< T >::value;

/// @brief the concept of a tensor
/// @tparam T the type to test
template< typename T >
concept tensor = is_tensor_v< T >;

//////////////////////
/// Tensor / Zero ///
////////////////////
///
/// methods to create zero tensors of uniform or arbitrary types

/// @brief creates a zero tensor with shape S and uniform type T for all 
/// elements
/// @tparam S the shape of the tensor
/// @tparam T the type of all the elements
/// @returns a tensor of uniform type T and shape S with each element
/// set to 0
template< shape S, typename T >
requires( isgreater( S::size(), 1 ))
constexpr uniform_tensor_t< S, T > zero_tensor()
{ return detail::zero_tensor_helper< uniform_tensor_t< S, T >>( 
    make_seq< S::size() >{} ); }

/// @brief creates a zero tensor with shape S and element types Ts...
/// @tparam S the shape of the tensor
/// @tparam ...Ts are the types of the tensor elements
/// @returns a tensor with shape S and element types Ts... where all elements
/// are set to 0
template< shape S, typename... Ts >
requires( S::size() == sizeof...( Ts ) )
constexpr Tensor< S, Ts... > zero_tensor()
{ return detail::zero_tensor_helper< Tensor< S, Ts... >>( 
    make_seq< S::size() >{} ); }

////////////////////////
/// Tensor / Vector ///
//////////////////////
///
/// A vector is a tensor with a single degree to its shape
///
/// @brief the concept of a vector is a tensor with a single dimension
/// @tparam T the type to test
template< typename T >
concept vector = ( is_tensor_v< T > and tensor_shape_t< T >::degree() <= 1 );

/// @brief a vector of uniform element types
/// @tparam N is the size of the vector
/// @tparam T is the type of the elements of the vector
template< size_t N, typename T >
using uniform_vector_t = uniform_tensor_t< Shape< N >, T >;

/// @brief uniform vector with zero values for it's elements
/// @tparam N is the size of the vector
/// @tparam T is the type of the elements
template< size_t N, typename T >
requires( isgreater( N, 1 ))
constexpr uniform_tensor_t< Shape< N >, T > zero_vector()
{ return zero_tensor< Shape< N >, T >(); }

/// @brief arbitrarily typed vector with element values equal to zero
/// @tparam N is the size of the vector
/// @tparam ...Ts are the types of the elements
template< size_t N, typename... Ts >
requires( N == sizeof...( Ts ))
constexpr Tensor< Shape< N >, Ts... > zero_vector()
{ return zero_tensor< Shape< N >, Ts... >(); }

/// @brief the type of a vector extended to contain new elements of the 
/// supplied types
/// @tparam V is the original vector type
/// @tparam ...Us are the element types to be extended to the vector type V
template< vector V, typename... Us >
using extend_vector_t = detail::ExtendVector< V, Us... >::type;

/// @brief adds a value to the end of a vector
/// @tparam T the type of the value
/// @tparam V the vector type
/// @param vec the vector values
/// @param value to be added
/// @return a new vector with the value as the last element
template< vector V, typename... Us >
extend_vector_t< V, Us... > extend_vector( V const& vec, Us const&... values )
{ return detail::ExtendVector< V, Us... >::make( vec, values... ); }

/// @brief a matrix is a tensor of degree 2 (a two dimensional tensor)
template< typename T >
concept matrix = ( is_tensor_v< T > and tensor_shape_t< T >::degree() == 2 );

/// @brief type helper for a matrix of uniform type
/// @tparam Rows in the matrix
/// @tparam Cols in the matrix
/// @tparam T is the type of the elements
template< size_t Rows, size_t Cols, typename T >
using uniform_matrix_t = uniform_tensor_t< Shape< Rows, Cols >, T >;

/// @brief concept of a tensor of a given shape
/// @tparam the type to test
/// @tparam ...Is are the sizes of the shape
template< typename T, size_t... Is >
concept tensor_shaped = detail::IsTensorShape< T, Shape< Is... >>::value;

////////////////////////////
/// Tensor / Operations ///
//////////////////////////
///
/// Operations on tensors include boolean and arithmetic operations and matrix-
/// like operations including determinant and transpose
///
/// details of tensor operations
namespace detail {

/// @brief helper to determine if two tensors are equal
/// @tparam LeftT the type of the left tensor operand
/// @tparam RightT the type of the right tensor operand
/// @tparam ...Is an index sequence of elements to test
/// @param left the left operand
/// @param right the right operand
/// @return true of the elements ...Is are equal in both operands
template< typename LeftT, typename RightT, size_t... Is >
constexpr auto equals_helper( LeftT const& left, RightT const& right,
    seq< Is... > )
{ return ((get< Is >( left ) == get< Is >( right )) and ... ); }

template< typename LeftT, typename RighT >
struct IsEqualityDefined;

/// @brief for non-tensors, equality is defined if the sub-types are convertible
/// NOTE: watch out for circular logic here!!!
/// @tparam LeftT 
/// @tparam RightT 
template< typename LeftT, typename RightT >
requires( not tensor< LeftT > and not tensor< RightT > )
struct IsEqualityDefined< LeftT, RightT >:
    integral_constant< bool, std::is_convertible_v< LeftT, RightT >> { };

/// @brief specialization of IsTensorEqualityDefined in the case where the 
/// tensor shapes are not the same.
template< typename LeftS, typename... Ts, typename RightS, typename... Us >
requires( not is_same_v< LeftS, RightS > )
struct IsEqualityDefined< Tensor< LeftS, Ts... >, 
    Tensor< RightS, Us... >>: integral_constant< bool, false > { };

template< typename LeftT, typename RightT, typename Seq >
struct IsTensorEqualityDefinedHelper;

template< typename LeftT, typename RightT, size_t... Is >
struct IsTensorEqualityDefinedHelper< LeftT, RightT,
    seq< Is... >>: integral_constant< bool, 
        ( IsEqualityDefined< tensor_element_t< Is, LeftT >, 
            tensor_element_t< Is, RightT >>::value and ... )> { };

/// @brief specialization of IsTensorEqualityDefined in the case where the 
/// tensor shapes are the same.  In this case we inherit to the helper value
template< typename LeftS, typename... Ts, typename RightS, typename... Us >
requires( is_same_v< LeftS, RightS > )
struct IsEqualityDefined< Tensor< LeftS, Ts... >, Tensor< RightS, Us... >>: 
    IsTensorEqualityDefinedHelper< 
        Tensor< LeftS, Ts... >, Tensor< RightS, Us... >, 
            make_seq< sizeof...( Ts )>> { };

/// @brief trait to check if two tensor types are compatible with the equality 
/// operator
/// @tparam LeftT 
/// @tparam RightT 
template< typename LeftT, typename RightT >
constexpr bool is_equality_defined_v = 
    IsEqualityDefined< LeftT, RightT >::value;

/// @brief helper to add two tensors
/// @tparam LeftT the type of the left tensor operand
/// @tparam RightT the type of the right tensor operand
/// @tparam ...Is an index sequence of elements to test
/// @param left the left operand
/// @param right the right operand
/// @return a tensor with elements summed from the operands
template< typename LeftT, typename RightT, size_t... Is >
constexpr auto plus_helper( LeftT const& left, RightT const& right, 
    seq< Is... > )
{ return make_tensor< typename LeftT::shape_type >(
    ( get< Is >( left ) + get< Is >( right ))... ); }

/// @brief helper subtract two tensors
/// @tparam LeftT the type of the left tensor operand
/// @tparam RightT the type of the right tensor operand
/// @tparam ...Is an index sequence of elements to test
/// @param left the left operand
/// @param right the right operand
/// @return a tensor with elements subtracted from the operands
template< typename LeftT, typename RightT, size_t... Is >
constexpr auto minus_helper( LeftT const& left, RightT const& right, 
    seq< Is... > )
{ return make_tensor< typename LeftT::shape_type >(
    ( get< Is >( left ) - get< Is >( right ))... ); }

/// @brief helper to calculate the product of two tensors
/// @tparam LeftT the type of the left tensor operand
/// @tparam RightT the type of the right tensor operand
/// @tparam ...Is an index sequence of elements of the tensor product
/// @param left the left operand
/// @param right the right operand
/// @return the tensor product of the operands
template< typename LeftT, typename RightT, size_t... Is >
constexpr auto product_helper( LeftT const& left, RightT const& right,
    seq< Is... > )
{
    using shape_type = shape_cat_t< typename LeftT::shape_type, 
        typename RightT::shape_type >;
    return make_tensor< shape_type >(( get< Is / right.size() >( left ) * 
        get< Is % right.size() >( right ))... );
}

/// @brief helper to scale the elements of a tensor
/// @tparam T the scalar type
/// @tparam TensorT the tensor type
/// @tparam ...Is index sequence of elements of the tensor
/// @param scalar the scalar value
/// @param ten the tensor to scale
/// @return the scaled tensor
template< typename TensorT, typename T, size_t... Is >
constexpr auto scale_helper( TensorT const& ten, T scalar, seq< Is... > )
{ return make_tensor< tensor_shape_t< TensorT >>
    (( get< Is >( ten ) * scalar )... ); }

/// @brief helper to divide the elements of a tensor by a scalar
/// @tparam T the type of the scalar
/// @tparam TensorT the tensor type
/// @tparam ...Is index sequence of elements of a tensor
/// @param scalar the scalar value
/// @param ten the tensor to scale
/// @return the scaled tensor
template< typename TensorT, typename T, size_t... Is >
constexpr auto divide_scale_helper( TensorT const& ten, T scalar, seq< Is... > )
{ return make_tensor< tensor_shape_t< TensorT >>
    (( get< Is >( ten ) / scalar )... ); }

/// @brief returns a tensor composed of op(E) where E is the corresponding 
/// element from ten
/// @tparam Op is the type of the unary operation
/// @tparam TensorT is the type of the tensor
/// @tparam ...Is are the elements of ten to operate on
/// @param op is the operation to execute on each element
/// @param ten is the tensor to operate on
template< typename Op, typename TensorT, size_t... Is >
constexpr auto op_helper( Op& op, TensorT const& ten, seq< Is... > )
{ return make_tensor< tensor_shape_t< TensorT >>( op( get< Is >( ten ))... ); }

// forward decl
template< size_t K, size_t I, size_t J, shape S, typename Seq >
struct ContractedElementSequenceHelper;

/// @brief resolves to an index_sequence of elements from a tensor with shape S 
/// which when summed together equal the Kth element of the same tensor 
/// contracted on indices I and J
/// @tparam K the element of the contracted tensor
/// @tparam I the first index to be contracted on
/// @tparam J the second index to be contracted on
/// @tparam S is the shape of the tensor to be contracted
/// @tparam Is... are 0, 1, ... N where N == shape_element_v< I, S >
template< size_t K, size_t I, size_t J, typename S, size_t... Is >
struct ContractedElementSequenceHelper< K, I, J, S, seq< Is... >>
{
    // calculate the contracted shape
    using contracted_shape = contract_shape_t< I, J, S >;
    static constexpr size_t contraction_size = shape_element_v< I, S >;

    // our elements will be the uncontracted index on Is
    using type = seq< uncontract_index< contraction_size, I, J >( Is, 
        contracted_shape::from_element( K ) )... >;
};

/// @brief defines an index sequence of elements to be summed for the Kth 
/// element of a tensor with shape S contracted along the Ith and Jth indices
/// @tparam K is the element of the original tensor
/// @tparam I the first contraction index
/// @tparam J the second contraction index
/// @tparam S the shape of the tensor
template< size_t K, size_t I, size_t J, typename S >
using contracted_element_seq = ContractedElementSequenceHelper< K, I, J, S, 
    make_seq< shape_element_v< I, S >>>::type;

/// @brief sums the Is... elements of ten
/// @tparam TensorT is the type of the tensor
/// @tparam Is are the elements to be summed
/// @param ten is the tensor storing the values
/// @returns the sum of the Is elements of the input tensor
template< typename TensorT, size_t... Is >
constexpr auto sum_elements_helper( TensorT const& ten, seq< Is... > )
{ return ( get< Is >( ten ) + ... ); }

/// @brief returns the Kth element of ten contracted along I and J
/// @tparam K the element of the contracted tensor
/// @tparam I the first contracted index
/// @tparam J the second contracted index
/// @tparam TensorT the type of the uncontracted tensor
/// @param ten is the original uncontracted tensor
template< size_t K, size_t I, size_t J, typename TensorT >
constexpr auto contracted_element( TensorT const& ten )
{ return sum_elements_helper( ten, 
    contracted_element_seq< K, I, J, typename TensorT::shape_type >{} ); }

/// @brief helper function for contracting a tensor
/// @tparam TensorT the type of the tensor to be contracted
/// @tparam I the first contraction index
/// @tparam J the second contraction index
/// @tparam ...Is an index sequence of elements of the contracted tensor
/// @param ten the uncontracted tensor
/// @return tensor ten contracted along I and J
template< size_t I, size_t J, typename TensorT, size_t... Is >
constexpr auto contract_helper( TensorT const& ten, seq< Is... > )
{
    static constexpr size_t i0 = min_v< I, J >;
    static constexpr size_t i1 = max_v< I, J >;
    using shape_type = contract_shape_t< i0, i1, typename TensorT::shape_type >;
    return make_tensor< shape_type >( contracted_element< Is, i0, i1 >( ten )... );
}

/// @brief the Kth element of a tensor transposed on index I and J
/// @tparam TensorT the type of the tensor to be transposed
/// @tparam K the element of the transposed tensor
/// @tparam I the first index to transpose
/// @tparam J the second index to transpose
/// @param ten the tensor to transpose the values of
/// @return ten transposed on indices I and J
template< size_t K, size_t I, size_t J, typename TensorT >
constexpr auto transposed_element( TensorT const& ten )
{
    using shape_type = tensor_shape_t< TensorT >;
    using transposed_shape_type = transpose_shape_t< I, J, tensor_shape_t< TensorT >>;

    static constexpr auto k = transpose_shape< I, J >( transposed_shape_type::from_element( K ));
    return get< k >( ten );
}

/// @brief helper function to transpose a tensor
/// @tparam TensorT the tensor type to transpose
/// @tparam I the first index to transpose
/// @tparam J the second index to transpose
/// @tparam ...Ks an index sequence of the elements of the transposed tensor
/// @param ten the tensor to transpose
/// @return ten transposed on indices I and J
template< size_t I, size_t J, typename TensorT, size_t... Ks >
constexpr auto transpose_helper( TensorT const& ten, seq< Ks... > )
{
    using shape_type = transpose_shape_t< I, J, tensor_shape_t< TensorT >>;
    return make_tensor< shape_type >( transposed_element< Ks, I, J >( ten )... );
}

/// @brief element of a subtensor of the element identified by a sequence Seq
/// @tparam Seq the element expressed as an index sequnce
/// @tparam TensorT the type of the original tensor
/// @tparam K the element of the subtensor
/// @param ten the original tensor
/// @return the value of the Kth element of a subtensor of ten around an element
/// identified by the index sequence Seq
template< size_t K, typename Seq, typename TensorT >
constexpr auto subtensor_element( TensorT const& ten )
{
    using shape_type = tensor_shape_t< TensorT >;
    using sub_shape_type = sub_tensor_shape_t< shape_type >;

    static constexpr const auto sub = sub_shape_type::from_element( K );
    static constexpr size_t L = shape_insert_at< Seq >( sub );
    return get< L >( ten );
}

/// @brief element of a subtensor of the Ith element of a TensorT tensor
/// @tparam TensorT the type of the original tensor
/// @tparam I the element of the subtensor
/// @tparam K the element to take the subtensor around
/// @param ten the tensor value
/// @return the value of element I of the subtensor at element K of ten
template< size_t I, size_t K, typename TensorT >
constexpr auto element_subtensor_element( TensorT const& ten )
{
    using shape_type = tensor_shape_t< TensorT >;
    using sub_shape_type = sub_tensor_shape_t< shape_type >;

    static constexpr const auto sub = sub_shape_type::from_element( K );
    static constinit const size_t L = shape_insert_at_element< I >( sub );
    return get< L >( ten );
}

/// @brief helper to calculate the subtensor of a given tensor
/// @tparam Seq is a sequence representing the position into the original tensor
/// @tparam TensorT the original tensor type
/// @tparam ...Ks an index sequence of elements of the subtensor
/// @param ten the original tensor
/// @return a subtensor of ten around the element identified by the index 
/// sequence Seq
template< typename Seq, typename TensorT, size_t... Ks >
constexpr auto subtensor_helper( TensorT const& ten, seq< Ks... > )
{ 
    using sub_shape_type = sub_tensor_shape_t< tensor_shape_t< TensorT >>;
    return make_tensor< sub_shape_type >( 
        subtensor_element< Ks, Seq >( ten )... ); 
}

// template< typename TensorT, size_t... Ks >
// constexpr auto subtensor_helper( tensor_shape_t< TensorT > shp, 
//     TensorT const& ten, seq< Ks... > )
// { 
//     using sub_shape_type = sub_tensor_shape_t< tensor_shape_t< TensorT >>;
//     return make_tensor< sub_shape_type >( 
//         subtensor_element< Ks >( shp, ten )... ); 
// }

/// @brief helper to calculate the subtensor around element I of the given 
/// tensor
/// @tparam TensorT the original tensor
/// @tparam I the element of the original tensor
/// @tparam ...Ks the elements of the subtensor
/// @param ten the original tensor
/// @return a subtensor around element I of ten
template< size_t I, typename TensorT, size_t... Ks >
constexpr auto element_subtensor_helper( TensorT const& ten, seq< Ks... > )
{ 
    using sub_shape_type = sub_tensor_shape_t< tensor_shape_t< TensorT >>;
    return make_tensor< sub_shape_type >( 
        element_subtensor_element< I, Ks >( ten )... ); 
}

/// @brief a helper for adding elements from a tensor
/// @tparam TensorT the type of the tensor
/// @tparam ...Ks are the elements to sum
/// @param ten is the tensor to sum elements from
/// @return the sum of elements ...Ks from tensor ten
template< typename TensorT, size_t... Ks >
constexpr auto sum_helper( TensorT const& ten, seq< Ks... > )
{ return ( tensor_get< Ks >( ten ) + ... ); }

/// @brief helper for matrix multiplication: a contraction over the middle
/// indices of the product of LeftT and RightT
/// @tparam LeftT the type of the left tensor
/// @tparam RightT the type of the right tensor
template< typename LeftT, typename RightT >
struct MatrixMultiplyHelper
{
    using left_shape = tensor_shape_t< LeftT >;
    using right_shape = tensor_shape_t< RightT >;
    static constexpr size_t first_contraction_index = 
        left_shape::degree() - 1;
    static constexpr bool is_compatible = 
        ( shape_element_v< first_contraction_index, left_shape > ==
            shape_element_v< 0, right_shape > );

    static constexpr auto matmul( LeftT const& left, RightT const& right )
    { return contract< first_contraction_index, first_contraction_index + 1 >
        ( tensor_product( left, right )); }
};

/// @brief trait to determine of the left and right tensors are compatible with
/// matrix multiplication
/// @tparam LeftT is the type of the left tensor
/// @tparam RightT is the type of the right tensor
template< typename LeftT, typename RightT >
constexpr bool matmul_compatible_v = 
    MatrixMultiplyHelper< LeftT, RightT >::is_compatible;

} // namespace detail

///////////////////////////
/// Tensor / Operators ///
/////////////////////////
/// 
/// Common boolean, arithmetic, matrix, and tensor operations defined below

/// @brief equality of two tensors
/// @tparam S is the shape of the operands (must be the same)
/// @tparam ...Ts the types of the left operand
/// @tparam ...Us the types of the right operand
/// @param left the left operand
/// @param right the right operand
/// @return true of the elements of the operands are equal
template< shape S, typename... Ts, typename... Us >
requires( detail::is_equality_defined_v< Tensor< S, Ts... >, Tensor< S, Us... >> )
constexpr auto operator ==( Tensor< S, Ts... > const& left, Tensor< S, Us... > const& right )
{ return detail::equals_helper( left, right, make_seq< sizeof...( Ts )>{} ); }

/// @brief inequality of two tensors
/// TODO: write a not_equals_helper to aid in disambiguation
/// @tparam S the shape of the operands
/// @tparam ...Ts the types of the left operand
/// @tparam ...Us the types of the right operand
/// @param left the left operand
/// @param right the right operand
/// @return true of any element of the tensors is not equal
template< shape S, typename... Ts, typename... Us >
requires( detail::is_equality_defined_v< Tensor< S, Ts... >, Tensor< S, Us... >> )
constexpr auto operator !=( Tensor< S, Ts... > const& left, Tensor< S, Us... > const& right )
{ return not detail::equals_helper( left, right, make_seq< sizeof...( Ts )>{} ); }

/// @brief the sum of two tensors
/// @tparam S the shape of the tensors
/// @tparam ...Ts the types of the left operand
/// @tparam ...Us the types of the right operand
/// @param left the left operand
/// @param right the right operand
/// @return the sum of the two operands
template< shape S, typename... Ts, typename... Us >
constexpr auto operator +( Tensor< S, Ts... > const& left, Tensor< S, Us... > const& right )
{ return detail::plus_helper( left, right, make_seq< sizeof...( Ts )>{} ); }

/// @brief the difference of two tensors
/// @tparam S the shape of the tensors
/// @tparam ...Ts the types of the left operand
/// @tparam ...Us the types of the right operands
/// @param left the left operand
/// @param right the right operand
/// @return the difference of the two operands
template< shape S, typename... Ts, typename... Us >
constexpr auto operator -( Tensor< S, Ts... > const& left, Tensor< S, Us... > const& right )
{ return detail::minus_helper( left, right, make_seq< sizeof...( Ts )>{} ); }

/// @brief the tensor product
/// @tparam A the shape of the left operand
/// @tparam ...Ts the types of the left operand
/// @tparam B the shape of the right operand
/// @tparam ...Us the types of the right operand
/// @param left the left operand
/// @param right the right operand
/// @return the tensor product of two tensors
template< shape A, typename... Ts, shape B, typename... Us >
constexpr auto tensor_product( Tensor< A, Ts... > const& left, Tensor< B, Us... > const& right )
{ return detail::product_helper( left, right, make_seq< sizeof...( Ts ) * sizeof...( Us )>{} ); }

/// @brief the tensor product
/// @tparam A the shape of the left operand
/// @tparam ...Ts the types of the left operand
/// @tparam B the shape of the right operand
/// @tparam ...Us the types of the right operand
/// @param left the left operand
/// @param right the right operand
/// @return the tensor product of two tensors
template< shape A, typename... Ts, shape B, typename... Us >
constexpr auto operator *( Tensor< A, Ts... > const& left, Tensor< B, Us... > const& right )
{ return detail::product_helper( left, right, make_seq< sizeof...( Ts ) * sizeof...( Us )>{} ); }

/// @brief add the elements of the tensor together
/// @tparam ...Ts are the types of the tensor elements
/// @tparam A is the shape of the tensor
/// @param ten is the tensor instance
/// @return the sum of the elements of the tensor
template< shape A, typename... Ts >
constexpr auto sum( Tensor< A, Ts... > const& ten )
{ return detail::sum_helper( ten, make_seq< sizeof...( Ts )>{} ); }

/// @brief multiplies a tensor by a scalar
/// @tparam T the scalar type
/// @tparam S the shape of the tensor
/// @tparam ...Ts the types of the tensor
/// @param scalar the scalar value
/// @param ten the tensor value
/// @return the scaled tensor
template< shape S, typename... Ts, typename T >
constexpr auto scale( Tensor< S, Ts... > const& ten, T scalar )
{ return detail::scale_helper( ten, scalar, make_seq< sizeof...( Ts )>{} ); }

/// @brief multiplies a tensor by a scalar
/// @tparam T the scalar type
/// @tparam S the shape of the tensor
/// @tparam ...Ts the types of the tensor
/// @param scalar the scalar value
/// @param ten the tensor value
/// @return the scaled tensor
template< shape S, typename... Ts, typename T >
requires( not tensor< T >)
constexpr auto operator*( Tensor< S, Ts... > const& ten, T scalar )
{ return detail::scale_helper( ten, scalar, make_seq< sizeof...( Ts )>{} ); }

/// @brief multiplies a tensor by a scalar
/// @tparam T the scalar type
/// @tparam S the shape of the tensor
/// @tparam ...Ts the types of the tensor
/// @param scalar the scalar value
/// @param ten the tensor value
/// @return the scaled tensor
template< typename T, shape S, typename... Ts >
requires( not tensor< T >)
constexpr auto operator*( T scalar, Tensor< S, Ts... > const& ten )
{ return detail::scale_helper( ten, scalar, make_seq< sizeof...( Ts )>{} ); }

/// @brief divides a tensor by a scalar
/// @tparam T the scalar type
/// @tparam S the shape of the tensor
/// @tparam ...Ts the tensor element types
/// @param scalar the scalar value
/// @param ten the tensor value
/// @return the scaled tensor
template< shape S, typename... Ts, typename T >
constexpr auto divide_scale( Tensor< S, Ts... > const& ten, T scalar ) 
{ return detail::divide_scale_helper( ten, scalar, make_seq< sizeof...( Ts )>{} ); }

/// @brief divides a tensor by a scalar
/// @tparam T the scalar type
/// @tparam S the shape of the tensor
/// @tparam ...Ts the tensor element types
/// @param scalar the scalar value
/// @param ten the tensor value
/// @return the scaled tensor
template< shape S, typename... Ts, typename T >
requires( not tensor< T > )
constexpr auto operator /( Tensor< S, Ts... > const& ten, T scalar )
{ return detail::divide_scale_helper( ten, scalar, make_seq< sizeof...( Ts )>{} ); }

// stacked tensor details
namespace detail {

/// @brief helper class for the Ith element of a stack of ...Tensors
/// @tparam I is the element of the stacked tensor
/// @tparam ...Tensors are the tensor types to be stacked
template< size_t I, tensor... Tensors >
requires(( is_same_v< tensor_shape_t< Tensors...[ 0 ]>, tensor_shape_t< Tensors >> and ... ))
struct StackedElement
{ 
    // the shape of the new tensor is the size of the input tensors
    // concatenated to the shape of ...Tensors.  All of the ...Tensors
    // must have the same shape so we choose the first one
    using shape_type = shape_cat_t< Shape< sizeof...( Tensors )>, 
        tensor_shape_t< Tensors...[ 0 ]>>;

    // get an instance of our tensor shape to use to index into the parameters
    static constexpr shape_type element = shape_type::from_element( I );

    // the index to the parameter we will pull the value of the stacked
    // tensor from
    static constexpr size_t parameter_index = element.first();

    // type of the Ith element of the stacked tensor depends on which
    // tensor parameter we get from the parameter_index
    using type = tensor_element_t< element.rest(), Tensors...[ parameter_index ]>;

    // value of the Ith element of the stacked tensor
    static constexpr type value( Tensors const&... tensors )
    { return tensor_get< element.rest() >( tensors...[ parameter_index ] ); }
};

/// @brief helper class for a stack of tensors
/// @tparam First is the type of the first tensor
/// @tparam ...Rest are the types of the remaining tensors
template< tensor First, tensor... Rest >
requires(( std::is_same_v< tensor_shape_t< First >, tensor_shape_t< Rest >> and
    ... ))
struct StackedTensor {
private:
    using shape_type = shape_cat_t< Shape< 1 + sizeof...( Rest )>, 
        tensor_shape_t< First >>;

    template< typename Seq >
    struct Helper;

    template< size_t... Is >
    struct Helper< seq< Is... >>
    {
        using type = Tensor< shape_type, typename 
            StackedElement< Is, First, Rest... >::type... >;
        
        static constexpr type value( First const& first, Rest const&... rest )
        { return { StackedElement< Is, First, Rest... >::
            value( first, rest... )... }; }
    };

    using helper_type = Helper< make_seq< shape_type::size() >>::type;

public:
    // type of the stacked tensor
    using type = Helper< make_seq< shape_type::size() >>::type;

    // value of a tensor stack
    static constexpr type value( First const& first, Rest const&... rest )
    { return helper_type::value( first, rest... ); }
};
} // namespace detail

/// @brief create a new tensor from the given tensor by adding a new index of
/// size 1 + sizeof...( Rest ) to the left of indices (which must be equal) of
/// First and ...Rest
/// @tparam First is the type of the first tensor
/// @tparam ...Rest are the types of the remaining tensors to be stacked
template< tensor First, tensor... Rest >
requires(( is_same_v< tensor_shape_t< First >, tensor_shape_t< Rest >> and ... ))
constexpr typename detail::StackedTensor< First, Rest... >::type 
stack( First const& first, Rest const&... rest )
{ return detail::StackedTensor< First, Rest... >::value( first, rest... ); }

#ifndef NDEBUG
static_assert( is_same_v< decltype( stack( Tensor< Shape< 2, 2 >, int, int, int, int >{},
    Tensor< Shape< 2, 2 >, float, float, float, float >{} )), 
        Tensor< Shape< 2, 2, 2 >, int, int, int, int, float, float, float, float >> );
static_assert( is_same_v< decltype( stack( Tensor< Shape< 2, 1 >, int, float >{},
    Tensor< Shape< 2, 1 >, char, bool >{} )), 
        Tensor< Shape< 2, 2, 1 >, int, float, char, bool >> );
#endif // DEBUG

// dot product details
namespace detail {

template< typename LeftT, typename RightT, size_t... Is >
constexpr auto dot_helper( LeftT const& left, RightT const& right, 
    seq< Is... > )
{ return (( tensor_get< Is >( left ) * tensor_get< Is >( right )) + ... ); }

} // namespace detail 

/// @brief dot product of two tensors with the same shape
/// @tparam S the shape of the tensors
/// @tparam ...Ts the types of the left tensor elements
/// @tparam ...Us the tyeps of the right tensor elements
/// @param left the left argument
/// @param right the right argument
/// @returns the sum of element-wise products of the tensor arguments
template< shape S, typename... Ts, typename... Us >
constexpr auto dot( Tensor< S, Ts... > const& left, Tensor< S, Us... > const& right )
{ return detail::dot_helper( left, right, make_seq< sizeof...( Us ) >{} ); }

/// @brief cross product of 3d vectors
/// @tparam T[012] are the tyeps of the left vector 
/// @tparam U[012] are the types of the right vector
/// @param left vector
/// @param right vector
/// @return a 3d vector normal to left and right by the right-hand rule and with
/// length equal to the area of the parallelogram formed by left and right.
/// NOTE: interesting that this has units of area (when the inputs have units of
/// length).  Do we need a cross_root function which divides the cross product
/// by the square-root of the length of the cross product?
/// NOTE: taking the dot product of a length-vector with this area-vector will
/// yield a volume-vector.
template< typename T0, typename T1, typename T2,
    typename U0, typename U1, typename U2 >
constexpr auto cross( Tensor< Shape< 3 >, T0, T1, T2 > const& left,
    Tensor< Shape< 3 >, U0, U1, U2 > const& right )
{ 
    return make_tensor< Shape< 3 >>(
        tensor_get< 1 >( left ) * tensor_get< 2 >( right ) -
            tensor_get< 2 >( left ) * tensor_get< 1 >( right ),
        tensor_get< 2 >( left ) * tensor_get< 0 >( right ) - 
            tensor_get< 0 >( left ) * tensor_get< 2 >( right ),
        tensor_get< 0 >( left ) * tensor_get< 1 >( right ) -
            tensor_get< 1 >( left ) * tensor_get< 0 >( right ));
}

// tensor norm details
namespace detail {

// forward decl
template< size_t I, typename TensorT >
struct TensorNorm;

/// @brief L1 norm is the sum of the elements of a tensor
/// @tparam TensorT is the type of the tensor
template< typename TensorT >
struct TensorNorm< 1, TensorT >
{ static constexpr auto value( TensorT const& ten ) { return sum( ten ); } };

/// @brief L2 norm is the sqrt( x_n^2 + ... ) of the tensor elements
/// @tparam TensorT 
template< typename TensorT >
struct TensorNorm< 2, TensorT >
{ 
    static constexpr auto value( TensorT const& ten ) 
    { return value_helper( ten, make_seq< TensorT::size() >{} ); }

    template< size_t... Is >
    static constexpr auto value_helper( TensorT const& ten, seq< Is... > )
    { 
        auto ten2 = make_tensor< tensor_shape_t< TensorT >>(
            ( tensor_get< Is >( ten ) * tensor_get< Is >( ten ) )... );
        return std::sqrt( sum( ten2 ));
    }
};
} // namespace detail

/// @brief Ith norm of a tensor
/// @tparam ...Ts the tensor element types
/// @tparam I the type of norm to calculate ( 1 or 2 )
/// @tparam S the shape of the tensor
/// @param ten the tensor value
/// @return the Ith norm of the tensor
template< size_t I, shape S, typename... Ts >
constexpr auto norm( Tensor< S, Ts... > const& ten )
{ return detail::TensorNorm< I, Tensor< S, Ts... >>::value( ten ); }

/// @brief L2 norm of a tensor is the default
/// @tparam ...Ts the tensor element types
/// @tparam S the shape of the tensor
/// @param ten the tensor value
/// @return the Ith norm of the tensor
template< shape S, typename... Ts >
constexpr auto norm( Tensor< S, Ts... > const& ten )
{ return detail::TensorNorm< 2, Tensor< S, Ts... >>::value( ten ); }

namespace detail {

template< typename MatrixT >
struct IdentityMatrixHelper;

template< shape S, typename... Ts >
requires( S::degree() == 2 and 
    shape_element_v< 0, S > == shape_element_v< 1, S > )
struct IdentityMatrixHelper< Tensor< S, Ts... >>
{ 
    using matrix_type = Tensor< S, Ts... >;

    template< size_t... Is >
    static constexpr matrix_type identity_helper( seq< Is... > )
    { return make_tensor< S >( static_cast< tensor_element_t< Is, matrix_type >>( 
        // are we on the diagonal?
        shape_get< 0 >( S::from_element( Is )) == shape_get< 1 >( S::from_element( Is )) ?
            1 : 0 )... ); }

    static constexpr matrix_type identity()
    { return identity_helper( make_seq< sizeof...( Ts )>{} ); }
};

} // namespace detail

template< typename MatrixT >
constexpr MatrixT identity_matrix()
{ return detail::IdentityMatrixHelper< MatrixT >::identity(); }

/// @brief contract a tensor along indices I and J
/// @tparam TensorT the type of the tensor to contract
/// @tparam I the first contraction index
/// @tparam J the second contraction index
/// @param ten the tensor value
/// @return ten contracted along indices I and J
template< size_t I, size_t J, typename TensorT >
requires( shape_element_v< I, tensor_shape_t< TensorT >> == 
    shape_element_v< J, tensor_shape_t< TensorT >> )
constexpr auto contract( TensorT const& ten )
{ 
    static constexpr size_t contracted_size = 
        contract_shape_t< I, J, tensor_shape_t< TensorT >>::size();

    return detail::contract_helper< I, J >( ten, make_seq< contracted_size >{} ); 
}

/// @brief transpose a tensor along indices I and J
/// @tparam TensorT the type of the tensor
/// @tparam I the first index to transpose
/// @tparam J the second index to transpose
/// @param ten the tensor value
/// @return ten transposed on the Ith and Jth indices
template< size_t I, size_t J, typename TensorT >
constexpr auto transpose( TensorT const& ten )
{ return detail::transpose_helper< I, J >( ten, 
    make_seq< tensor_shape_t< TensorT >::size() >{} ); }

/// @brief take the subtensor around the position identified by an index 
/// sequence
/// @tparam Seq an index sequence representing the position to take a subtensor
/// @tparam TensorT the tensor type
/// @param ten the tensor value
/// @return a subtensor around the element identified by Seq
template< typename Seq, typename TensorT >
constexpr auto subtensor( TensorT const& ten )
{
    using sub_shape_type = sub_tensor_shape_t< tensor_shape_t< TensorT >>;
    return detail::subtensor_helper< Seq >( ten, make_seq< sub_shape_type::size() >{} );
}

// template< typename TensorT >
// constexpr auto subtensor( tensor_shape_t< TensorT > shp, TensorT const& ten )
// {
//     using sub_shape_type = sub_tensor_shape_t< tensor_shape_t< TensorT >>;
//     return detail::subtensor_helper( shp, ten, make_seq< sub_shape_type::size() >{} );
// }

/// @brief take the subtensor around element I of the given tensor
/// @tparam TensorT the tensor type
/// @tparam I the element to take the subtensor around
/// @param ten the tensor value
/// @return a subtensor around element I of ten
template< size_t I, typename TensorT >
constexpr auto element_subtensor( TensorT const& ten )
{
    using sub_shape_type = sub_tensor_shape_t< tensor_shape_t< TensorT >>;
    return detail::element_subtensor_helper< I >( ten, 
        make_seq< sub_shape_type::size() >{} );
}

////////////////////////////////////
/// Tensor / Matrix Determinant ///
//////////////////////////////////
///
/// implementation of a matrix determinant for tensors
///

// forward decl
template< typename TensorT >
requires( tensor_shape_t< TensorT >::degree() == 2 and
    shape_element_v< 0, tensor_shape_t< TensorT >> ==
        shape_element_v< 1, tensor_shape_t< TensorT >> )
constexpr auto det( TensorT const& ten );

// determinant details
namespace detail {

/// @brief helper to calculate the determinant of a tensor
/// @tparam TensorT the tensor type
/// @tparam ...Is an index sequence of all elements of the tensor
/// @param ten the tensor value
/// @return the determinant of ten
template< typename TensorT, size_t... Is >
requires( isgreater( sizeof...( Is ), 1 ))
constexpr auto det_helper( TensorT const& ten, seq< Is... > )
{ 
    // we know this is a 2D tensor with equal sizes, sizeof...( Is )
    using shape = tensor_shape_t< TensorT >;
    return ((
        ( Is % 2 == 0 ? 1 : -1 ) * // odd or even permutation
        get< shape{ 0, Is } >( ten ) * // 
        det( subtensor< seq< 0, Is >>( ten ))) + ... );
}

/// @brief specialization for a tensor with a single element
/// @tparam TensorT the tensor type
/// @param ten the tensor value
/// @return the sole element of a single element tensor is it's determinant
template< typename TensorT >
constexpr auto det_helper( TensorT const& ten, seq< 0 > )
{ return get< 0 >( ten ); }

/// @brief helper to calculate the cofactor matrix of a tensor
/// @tparam TensorT the tensor type
/// @tparam ...Is a sequnce of elements of the tensor
/// @param ten the tensor value
/// @return a cofactor tensor 
template< typename TensorT, size_t... Is >
constexpr auto cofactor_helper( TensorT const& ten, seq< Is... > )
{ 
    using shape_type = tensor_shape_t< TensorT >;
    return make_tensor< shape_type >((
        ( is_element_even_v< Is, shape_type > ? 1 : -1 ) * 
        det( element_subtensor< Is >( ten )))... );
}
} // namespace detail

/// @brief determinant of a square tensor
/// @tparam TensorT the type of the tensor
/// @param ten the tensor itself
/// @return the determinant of tensor ten
template< typename TensorT >
requires( tensor_shape_t< TensorT >::degree() == 2 and
    shape_element_v< 0, tensor_shape_t< TensorT >> ==
        shape_element_v< 1, tensor_shape_t< TensorT >> )
constexpr auto det( TensorT const& ten )
{ return detail::det_helper( ten, 
    make_seq< shape_element_v< 0, tensor_shape_t< TensorT >> >{} ); }

/// @brief cofactor matrix of a square matrix
/// @tparam TensorT is the type of the tensor
/// @param ten is the tensor itself
/// @return a tensor of the same shape containing the cofactors of ten
template< typename TensorT >
requires( tensor_shape_t< TensorT >::degree() == 2 and
    shape_element_v< 0, tensor_shape_t< TensorT >> ==
        shape_element_v< 1, tensor_shape_t< TensorT >> )
constexpr auto cofactor( TensorT const& ten )
{ 
    using shape_type = tensor_shape_t< TensorT >;
    return detail::cofactor_helper( ten, make_seq< shape_type::size() >{} );
}

/// @brief inverse of a square matrix
/// @tparam TensorT the type of the tensor
/// @param ten the tensor value
/// @return the inverse of a tensor ten
template< typename TensorT >
requires( tensor_shape_t< TensorT >::degree() == 2 and
    shape_element_v< 0, tensor_shape_t< TensorT >> ==
        shape_element_v< 1, tensor_shape_t< TensorT >> )
constexpr auto inverse( TensorT const& ten )
{ return divide_scale( transpose< 0, 1 >( cofactor( ten )), det( ten )); }

/// @brief multiply the tensors as matrices, ie: contract the tensor product
/// along the conjoining indices
/// @tparam LeftT the type of the left Tensor
/// @tparam RightT the type of the right Tensor
/// @param left the left tensor instance
/// @param right the right tensor instance
/// @return the tensor product contracted along the conjoining indices
template< typename LeftT, typename RightT >
requires( detail::matmul_compatible_v< LeftT, RightT > )
constexpr auto matmul( LeftT const& left, RightT const& right )
{ return detail::MatrixMultiplyHelper< LeftT, RightT >::
    matmul( left, right ); }

/////////////////////////////////
/// Tensor / Type Operations ///
///////////////////////////////
/// 
/// Operations on the type of tensors used to create subtensors

template< shape A, shape B >
struct IsSubShape;

template< size_t... As, size_t... Bs >
requires( isless( sizeof...( As ), sizeof...( Bs )))
struct IsSubShape< Shape< As... >, Shape< Bs... >>: 
    integral_constant< bool, false > { };

template< size_t... As, size_t... Bs >
requires( not isless( sizeof...( As ), sizeof...( Bs )))
struct IsSubShape< Shape< As... >, Shape< Bs... >>:
    integral_constant< bool, (( As >= Bs ) and ... )> { };

template< shape A, shape B >
constexpr bool is_sub_shape_v = IsSubShape< A, B >::value;

/// @brief checks if two tensor types can be added together.
template< typename T, typename U >
concept is_compatible = tensor< T > and tensor< U > and requires( T t, U u )
{ t + u; };

namespace detail {

template< size_t I, typename V >
struct TranslationMatrixAugmentedElement
{ 
    using type = tensor_element_t< I, V >;
    static constexpr type value( V translation )
    { return tensor_get< I >( translation ); }
};

template< size_t I, size_t J, typename V >
struct TranslationMatrixSquareElement
{
    using type = scalar_type;
    static constexpr type value( V )
    { return ( I == J ? 1. : 0. ); }
};

template< size_t Dim, size_t Elem, typename V >
struct TranslationMatrixElement;

template< size_t Dim, size_t Elem, typename V >
requires( isless( Shape< Dim, Dim+1 >::from_element( Elem )[ 1 ], Dim ))
struct TranslationMatrixElement< Dim, Elem, V >:
    TranslationMatrixSquareElement< 
        Shape< Dim, Dim+1 >::from_element( Elem )[ 0 ], 
            Shape< Dim, Dim+1 >::from_element( Elem )[ 1 ], V >
{ };

template< size_t Dim, size_t Elem, typename V >
requires( Shape< Dim, Dim+1 >::from_element( Elem )[ 1 ] == Dim )
struct TranslationMatrixElement< Dim, Elem, V >:
    TranslationMatrixAugmentedElement< 
        Shape< Dim, Dim+1 >::from_element( Elem )[ 0 ], V >
{ };

template< typename V, typename Seq >
struct TranslationMatrixHelper;

template<>
struct TranslationMatrixHelper< Tensor< Shape< 0 >>, seq< >>
{
    using vector_type = Tensor< Shape< 0 >>;
    using type = Tensor< Shape< 1 >, scalar_type >;
    static constexpr type make( vector_type )
    { return { 0 }; }
};

template< size_t Dim, typename... Ts, size_t... Is >
struct TranslationMatrixHelper< Tensor< Shape< Dim >, Ts... >, seq< Is... >>
{
    static constexpr size_t dimensions = Dim;
    using vector_type = Tensor< Shape< Dim >, Ts... >;

    using type = Tensor< Shape< dimensions, dimensions+1 >, 
        typename TranslationMatrixElement< dimensions, Is, vector_type >
            ::type... >;

    static constexpr type make( vector_type translation )
    { return { TranslationMatrixElement< dimensions, Is, vector_type >::value( 
        translation )... }; }
};

template< vector V >
struct TranslationMatrix:
    TranslationMatrixHelper< V, 
        make_seq< shape_element_v< 0, tensor_shape_t< V >> * 
            ( shape_element_v< 0, tensor_shape_t< V >> + 1 )>> 
{ };

} // namespace detail

template< typename VectorT >
auto translation_matrix( VectorT offset )
{ return detail::TranslationMatrix< VectorT >::make( offset ); };

template< size_t Dim, typename T = scalar_type >
auto zero_translation_matrix()
{ return detail::TranslationMatrix< uniform_tensor_t< Shape< Dim >, T >>::make( 
    zero_vector< Dim, T >()); };

namespace detail {

template< size_t N, size_t Axis0, size_t Axis1, typename AngleT >
struct RotatePlaneDiagonalMatrixElement;

template< size_t I, size_t J, size_t Axis0, size_t Axis1, typename AngleT >
struct RotatePlaneOffDiagonalMatrixElement;

// the ones along the diagonal
template< size_t N, size_t Axis0, size_t Axis1, typename AngleT >
requires( N != Axis0 and N != Axis1 )
struct RotatePlaneDiagonalMatrixElement< N, Axis0, Axis1, AngleT >
{
    using type = scalar_type;
    static constexpr type value( AngleT )
    { return 1.l; }
};

// the zeros in the off-diagonal
template< size_t I, size_t J, size_t Axis0, size_t Axis1, typename AngleT >
requires( I != Axis1 and J != Axis0 )
struct RotatePlaneOffDiagonalMatrixElement< I, J, Axis0, Axis1, AngleT >
{
    using type = scalar_type;
    static constexpr type value( AngleT )
    { return 0.l; }
};

// the cosine elements of the diagonal
template< size_t N, size_t Axis0, size_t Axis1, typename AngleT >
requires( N == Axis0 or N == Axis1 )
struct RotatePlaneDiagonalMatrixElement< N, Axis0, Axis1, AngleT >
{ 
    using type = decltype( cos( AngleT{} ));
    static constexpr type value( AngleT angle )
    { return cos( angle ); }
};

// the negative sine element
template< size_t I, size_t J, size_t Axis0, size_t Axis1, typename AngleT >
requires( I == Axis0 and J == Axis1 )
struct RotatePlaneOffDiagonalMatrixElement< I, J, Axis0, Axis1, AngleT >
{
    using type = decltype( -sin( AngleT{} ));
    static constexpr type value( AngleT angle )
    { return -sin( angle ); }
};

// the positive sine element
template< size_t I, size_t J, size_t Axis0, size_t Axis1, typename AngleT >
requires( I == Axis1 and J == Axis0 )
struct RotatePlaneOffDiagonalMatrixElement< I, J, Axis0, Axis1, AngleT >
{
    using type = decltype( sin( AngleT{} ));
    static constexpr type value( AngleT angle )
    { return sin( angle ); }
};

template< size_t Elem, size_t Axis0, size_t Axis1, shape S, typename AngleT >
struct RotatePlaneMatrixElement;

// specialization along the diagonal
template< size_t Elem, size_t Axis0, size_t Axis1, shape S, typename AngleT >
requires( S::from_element( Elem )[ 0 ] == S::from_element( Elem )[ 1 ])
struct RotatePlaneMatrixElement< Elem, Axis0, Axis1, S, AngleT >: 
    RotatePlaneDiagonalMatrixElement< S::from_element( Elem )[ 0 ], 
        Axis0, Axis1, AngleT > { };

// specialization for the off-diagonal
template< size_t Elem, size_t Axis0, size_t Axis1, shape S, typename AngleT >
requires( S::from_element( Elem )[ 0 ] != S::from_element( Elem )[ 1 ])
struct RotatePlaneMatrixElement< Elem, Axis0, Axis1, S, AngleT >: 
    RotatePlaneOffDiagonalMatrixElement< 
        S::from_element( Elem )[ 0 ], S::from_element( Elem )[ 1 ], 
            Axis0, Axis1, AngleT > { };

template< size_t Dim, size_t Axis0, size_t Axis1, typename AngleT, typename Seq >
struct RotatePlaneMatrixHelper;

template< size_t Dim, size_t Axis0, size_t Axis1, typename AngleT, size_t... Is >
struct RotatePlaneMatrixHelper< Dim, Axis0, Axis1, AngleT, seq< Is... >>
{
    using shape_type = Shape< Dim, Dim >;
    using angle_type = AngleT;
    using type = Tensor< shape_type, 
        typename RotatePlaneMatrixElement< Is, Axis0, Axis1, 
            shape_type, angle_type >::type... >;

    constexpr type make( angle_type angle )
    { 
        return { RotatePlaneMatrixElement< Is, Axis0, Axis1, shape_type, 
            angle_type >::value( angle )... }; 
    }
};

template< size_t Dim, size_t Axis0, size_t Axis1, typename AngleT >
struct RotatePlaneMatrix: 
    RotatePlaneMatrixHelper< Dim, Axis0, Axis1, AngleT, make_seq< Dim * Dim >> 
{ };


} // namespace detail


template< size_t Dim, size_t Axis0, size_t Axis1, typename AngleT >
using rotate_plane_matrix_t = 
    detail::RotatePlaneMatrix< Dim, Axis0, Axis1, AngleT >::type;

/// @brief creates a possibly lazily-executed rotation matrix of dimension Dim
/// of the plane formed from Axis0 and Axis1
/// @tparam AngleT type of the angle parameter (must be a scalar)
/// @tparam Dim dimension of the matrix
/// @tparam Axis0 axis to begin angle measurement from
/// @tparam Axis1 second axis to identify the plane of rotation
/// @param angle scalar angle value in radians
/// @return 
template< size_t Dim, size_t Axis0, size_t Axis1, typename AngleT >
constexpr rotate_plane_matrix_t< Dim, Axis0, Axis1, AngleT > 
rotate_plane_matrix( AngleT angle )
{ return detail::RotatePlaneMatrix< Dim, Axis0, Axis1, AngleT >::make( angle ); }

// template< tensor T, shape S >
// requires( is_sub_shape_v< tensor_shape_t< T >, S > )
// struct TensorSplit
// { 
//     using first_shape = S;
//     using second_shape = shape_remainder_t< tensor_shape_t< T >, S >;

//     using first_type = TensorSplitFirstHelper< T, first_shape, 
//         make_seq< first_shape::size() >>::type;
//     using second_type = TensorSplitSecondHelper< T, second_shape, 
//         make_seq< second_shape::size() >>::type;
// };

} // namespace tensors

#endif
