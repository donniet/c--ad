/**
 * A Tensor is a shaped array with arbirary types
 */

#ifndef __TENSOR_HPP__
#define __TENSOR_HPP__

#include "utility.hpp"

namespace tensor {

/**
 * Shape of Array
 */
template< size_t... >
struct Shape;

/**
 * an empty shape describes a single-valued object like a scalar
 */
template< >
struct Shape< >
{  
    // empty shapes have size == 1
    static constexpr size_t size() { return 1; }
    // the number of dimensions in this shape
    static constexpr size_t dimensions() { return 0; }

    // instances of shapes are always equivalent 
    constexpr bool operator ==( Shape const& other ) const
    { return true; }
    constexpr bool operator !=( Shape const& other ) const
    { return false; }

    // the element and first index of an instance of an empty shape are always
    // zero since they describe a single-valued object
    constexpr size_t element() const { return 0; }
    constexpr operator size_t() const { return element(); }
    constexpr size_t first() const { return element(); }

    constexpr Shape insert( Shape here ) const
    { return {}; }

    // TODO: throw an error if passed something larger than 1
    static consteval Shape from_element( size_t )
    { return {}; }
};

/**
 * a non-empty shape of an array.  An instance of this shape acts as an index
 * into an array:
 * 
 * int arr[] = { 0b00, 0b01, 0b10, 0b11 };
 * assert( arr[ Shape< 2, 2 >{ 1, 0 }] == 0b10 );
 * 
 * @tparam First is the size of the first dimension
 * @tparam Rest... are the remaining dimension sizes
 */
template< size_t First, size_t... Rest >
struct Shape< First, Rest... >: Shape< Rest... >
{ 
    // the size of the first dimension is First
    static constexpr size_t first_size() { return First; }
    // the overall size is the product of each size dimension
    static constexpr size_t size() { return First * ( Rest * ... * 1 ); }
    // the number of dimensions in this shape
    static constexpr size_t dimensions() { return 1 + sizeof...( Rest ); }
    
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

    constexpr Shape< First+1, ( Rest+1 )... > 
    insert( Shape< First+1, ( Rest+1 )... > here ) const
    { return Shape< First+1, ( Rest+1 )... >{ first() < here ? first() : 1 + first(), 
        Shape< Rest... >::insert( here )}; }

    constexpr operator size_t() const 
    { return element(); }

    constexpr size_t element() const 
    { return _element; }

    constexpr size_t first() const 
    { return element() / Shape< Rest... >::size(); }

    // TODO: figure out how to get rid of the static_cast
    template< typename... Indices >
    constexpr Shape( size_t first, Indices... rest ):
        Shape{ first, Shape< Rest... >{ static_cast< size_t >( rest )... }}
    { }

    explicit constexpr Shape( size_t first, Shape< Rest... > rest ): 
        Shape< Rest... >{ rest }, 
        _element{ rest.element() + first * Shape< Rest... >::size() }
    { }

    // index values
    size_t _element;
};

template< size_t I, typename S >
struct ShapeGet;

template< size_t First, size_t... Rest >
struct ShapeGet< 0, Shape< First, Rest... >>
{ 
    static constexpr size_t value( Shape< First, Rest... > shp ) 
    { return shp.first(); } 
};

template< size_t I, size_t First, size_t... Rest >
requires( isgreater( I, 0 ))
struct ShapeGet< I, Shape< First, Rest... >>
{ 
    static constexpr size_t value( Shape< First, Rest... > shp ) 
    { return ShapeGet< I-1, Shape< Rest... >>::value( shp ); } 
};

template< size_t I, typename S >
constexpr size_t shape_get( S shp )
{ return ShapeGet< I, S >::value( shp ); }

static_assert( Shape< 2, 2 >{ 1, 1 } == Shape< 2, 2 >{ 1, 1 } );
static_assert( Shape< 2, 2 >{ 1, 1 }.element() == Shape< 2, 2, 2 >{ 0, 1, 1 }.element() );
static_assert( Shape< 2, 2 >{ 1, 1 } == Shape< 2 >{ 1 }.cat( Shape< 2 >{ 1 }));
static_assert( Shape< 2, 2 >{ 1, 1 }.element() == 3 );
static_assert( Shape< 2, 2 >::from_element( 3 ) == Shape< 2, 2 >{ 1, 1 });
// static_assert( Shape< 2, 2 >{ 1, 1 }.of( make_tuple( 0, 1, 2, 3 )) == 3 );
// static_assert( )

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

static_assert( Shape< 2, 2 >{ 1, 1 }.cat( Shape< 1 >{ 0 }) == Shape< 2, 2, 1 >{ 1, 1, 0 });

static_assert( std::array< int, 4 >{ 0b00, 0b01, 0b10, 0b11 }[ Shape< 2, 2 >{ 1, 0 }] == 0b10 );

static_assert( std::array< int, 16 >{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }
    [ Shape< 4, 4 >{ 1, 0 }] == 4 );



template< typename T, typename U >
struct ShapeCat;

template< size_t... Ts, size_t... Us >
struct ShapeCat< Shape< Ts... >, Shape< Us... >>
{ using type = Shape< Ts..., Us... >; };

template< typename T, typename U >
using shape_cat_t = ShapeCat< T, U >::type;

template< typename... Ts >
consteval auto shape_from_tuple( tuple< size_t, Ts... > tup )
{ return Shape< get< 0 >( tup ) >::cat( shape_from_tuple< Ts... >( remove_first( tup ))); }

consteval Shape< > shape_from_tuple( tuple< > tup )
{ return {}; }

template< typename T >
struct IsShape : std::integral_constant< bool, false > { };

template< size_t... Is >
struct IsShape< Shape< Is... >> : std::integral_constant< bool, true > { };

template< typename T >
static constexpr bool is_shape_v = IsShape< T >::value;

/**
 * the concept of a shape
 */
template< typename T >
concept shape = is_shape_v< T >;

template< size_t I, shape S >
struct IsElementEven;

template<>
struct IsElementEven< 0, Shape<>>
{ static constexpr bool value = true; };

template< size_t I, size_t First, size_t... Rest >
struct IsElementEven< I, Shape< First, Rest... >>
{
    static constexpr size_t first = I / Shape< Rest... >::size();
    static constexpr size_t rest = I % Shape< Rest... >::size();

    static constexpr bool is_first_even = ( first % 2 == 0 );
    static constexpr bool is_rest_even = IsElementEven< rest, Shape< Rest... >>::value;
    static constexpr bool value = ( is_first_even and is_rest_even ) or
        (not is_first_even and not is_rest_even );
};

template< size_t I, shape S >
static constexpr bool is_element_even_v = IsElementEven< I, S >::value;

/**
 * creates a super shape object by inserting 1 into each dimension
 */
template< shape S, typename Seq >
struct ShapeInsertAt;

template< >
struct ShapeInsertAt< Shape< >, seq< >>
{ static consteval Shape< > insert( Shape< > ) { return {}; } };

template< size_t First, size_t... Rest, size_t I, size_t... Is >
struct ShapeInsertAt< Shape< First, Rest... >, seq< I, Is... >>
{
    using type = Shape< 1+First, (1+Rest)... >;
    static constexpr type insert( Shape< First, Rest... > from )
    { return type{ from.first() < I ? from.first() : 1 + from.first(),  
        ShapeInsertAt< Shape< Rest... >, seq< Is... >>::insert( from ) }; }
};  

template< shape S, size_t I >
struct ShapeInsertAtElement;

template< >
struct ShapeInsertAtElement< Shape< >, 0 >
{ static consteval Shape< > insert( Shape< > ) { return {}; } };

template< size_t First, size_t... Rest, size_t I  >
struct ShapeInsertAtElement< Shape< First, Rest... >, I >
{
    using type = Shape< 1+First, (1+Rest)... >;
    using rest_type = Shape<( 1+Rest )... >;
    static constexpr size_t first = I / rest_type::size();
    static constexpr size_t rem = I % rest_type::size();

    static constexpr type insert( Shape< First, Rest... > from )
    { return type{ from.first() < first ? from.first() : 1 + from.first(),  
        ShapeInsertAtElement< Shape< Rest... >, rem >::insert( from ) }; }
};  

template< typename Seq, shape S >
consteval auto shape_insert_at( S shp )
{ return ShapeInsertAt< S, Seq >::insert( shp ); }

template< size_t First, size_t... Rest >
consteval auto shape_insert_at( Shape< 1+First, (1+Rest)... > here, 
    Shape< First, Rest... > shp )
{ return shp.insert( here ); }

template< size_t I, shape S >
consteval auto shape_insert_at_element( S shp )
{ return ShapeInsertAtElement< S, I >::insert( shp ); }

/**
 * shape elements
 */
template< size_t I, shape S >
struct ShapeElement;

template< size_t First, size_t... Rest >
struct ShapeElement< 0, Shape< First, Rest... >>:
    std::integral_constant< size_t, First > { };

template< size_t I, size_t First, size_t... Rest >
struct ShapeElement< I, Shape< First, Rest... >>:
    std::integral_constant< size_t, 
        ShapeElement< I-1, Shape< Rest... >>::value > 
{ };

template< size_t I, shape S >
static constexpr size_t shape_element_v = ShapeElement< I, S >::value;

/**
 * contraction of a shape
 */
template< size_t I, shape S >
struct RemoveShapeElement;

template< size_t First, size_t... Rest >
struct RemoveShapeElement< 0, Shape< First, Rest... >>
{ 
    using type = Shape< Rest... >; 
    static constexpr type value( Shape< First, Rest... > shp )
    { return shp; }
};

template< size_t I, size_t First, size_t... Rest >
requires( isgreater( I, 0 ))
struct RemoveShapeElement< I, Shape< First, Rest... >>
{ 
    using type = shape_cat_t< Shape< First >, 
        typename RemoveShapeElement< I - 1, Shape< Rest... >>::type >; 

    static constexpr type value( Shape< First, Rest... > shp )
    { return Shape< First >{ shp.first() }.cat(
            RemoveShapeElement< I-1, Shape< Rest... >>::value( shp )); }
};

template< size_t I, shape S >
constexpr auto remove_shape_element( S shp )
{ return RemoveShapeElement< I, S >::value( shp ); }

template< size_t I, size_t J, shape S >
requires( isless( I, J ) and shape_element_v< I, S > == shape_element_v< J, S > )
struct ContractShape
{ using type = RemoveShapeElement< I, 
    typename RemoveShapeElement< J, S >::type >::type; };

template< size_t I, size_t J, shape S >
using contract_shape_t = ContractShape< I, J, S >::type;

static_assert( is_same_v< Shape< 1, 2, 4 >, contract_shape_t< 2, 3, Shape< 1, 2, 3, 3, 4 >>> );

template< size_t K, size_t I, shape S >
struct InsertShapeElement;

template< size_t K, size_t... Is >
struct InsertShapeElement< K, 0, Shape< Is... >>
{ 
    using type = Shape< K, Is... >; 

    static constexpr type value( size_t k, Shape< Is... > shp )
    { return type{ k, shp }; }
};

template< size_t K, size_t I, size_t First, size_t... Rest >
requires( isgreater( I, 0 ))
struct InsertShapeElement< K, I, Shape< First, Rest... >>
{ 
    using type = shape_cat_t< Shape< First >, 
        typename InsertShapeElement< K, I-1, Shape< Rest... >>::type >; 

    static constexpr type value( size_t k, Shape< First, Rest... > shp )
    { return Shape< First >{ shp.first() }.cat( 
        InsertShapeElement< K, I-1, Shape< Rest... >>::value( k, shp )); }
};

template< size_t K, size_t I, shape S >
constexpr InsertShapeElement< K, I, S >::type 
insert_shape_element( size_t k, S shp )
{ return InsertShapeElement< K, I, S >::value( k, shp ); }

template< shape S >
struct SubTensorShape;

template< size_t... Sizes >
requires(( isgreater( Sizes, 1 ) and ... ))
struct SubTensorShape< Shape< Sizes... >>
{ using type = Shape< (Sizes - 1)... >; };

template< shape S >
using sub_tensor_shape_t = SubTensorShape< S >::type;


static_assert( Shape< 2 >{ 1 }.element() == insert_shape_element< 2, 0 >( 1, Shape< >{} ).element() );
static_assert( Shape< 2, 2 >{ 1, 0 }.element() == insert_shape_element< 2, 0 >( 1, Shape< 2 >{ 0 } ).element() );
static_assert( Shape< 2, 2 >{ 1, 1 }.element() == insert_shape_element< 2, 0 >( 1, Shape< 2 >{ 1 } ).element() );
static_assert( Shape< 2, 3 >{ 1, 2 }.element() == insert_shape_element< 2, 0 >( 1, Shape< 3 >{ 2 } ).element() );
static_assert( Shape< 2, 3 >{ 1, 2 }.element() == insert_shape_element< 3, 1 >( 2, Shape< 2 >{ 1 } ).element() );

static_assert( Shape< 2, 2 >{ 0, 1 } == insert_shape_element< 2, 0 >( 0, Shape< 2 >{ 1 }) );
static_assert( Shape< 3, 2 >{ 2, 1 } == insert_shape_element< 3, 0 >( 2, Shape< 2 >{ 1 }) );
static_assert( Shape< 2, 3 >{ 1, 2 }.element() == insert_shape_element< 3, 1 >( 2, Shape< 2 >{ 1 }).element() );
static_assert( Shape< 3, 3 >{ 2, 2 } == insert_shape_element< 3, 0 >( 2, Shape< 3 >{ 2 }) );

template< size_t K, size_t I, size_t J, shape S >
requires( isless( I, J ))
struct UncontractShape
{ 
    using type = InsertShapeElement< K, J, 
        typename InsertShapeElement< K, I, S >::type >::type; 

    static constexpr type value( size_t k, S shp )
    { return insert_shape_element< K, J >( k, 
        insert_shape_element< K, I >( k, shp )); }
};

template< size_t K, size_t I, size_t J, shape S >
using uncontract_shape_t = UncontractShape< K, I, J, S >::type;

static_assert( is_same_v< Shape< 3, 3 >, uncontract_shape_t< 3, 0, 1, Shape<>>> );
static_assert( is_same_v< Shape< 1, 2, 3, 3, 4 >, uncontract_shape_t< 3, 2, 3, Shape< 1, 2, 4 >>> );

template< size_t K, size_t I, size_t J, shape S >
constexpr uncontract_shape_t< K, I, J, S >
uncontract_index( size_t k, S shp )
{ return UncontractShape< K, I, J, S >::value( k, shp ); }


static_assert( uncontract_index< 1, 0, 1 >( 0, Shape< >{} ).element() == Shape< 1, 1 >{ 0, 0 }.element() );
static_assert( uncontract_index< 2, 0, 1 >( 0, Shape< >{} ).element() == Shape< 2, 2 >{ 0, 0 }.element() );
static_assert( uncontract_index< 2, 0, 1 >( 0, Shape< 1 >{ 0 } ).element() == Shape< 2, 2, 1 >{ 0, 0, 0 }.element() );
static_assert( uncontract_index< 2, 0, 1 >( 1, Shape< 1 >{ 0 } ).element() == Shape< 2, 2, 1 >{ 1, 1, 0 }.element() );

static_assert( uncontract_index< 2, 0, 1 >( 1, Shape< >{} ).element() == Shape< 2, 2 >{ 1, 1 }.element() );
static_assert( uncontract_index< 3, 0, 1 >( 1, Shape< >{} ).element() == 1 + 3*1 );
static_assert( uncontract_index< 3, 2, 3 >( 2, Shape< 1, 2, 4 >{ 0, 1, 2 }).element() == 2 + 4*( 2 + 3*( 2 + 3*( 1 + 1 * 0 ))) );
// static_assert( Shape< 1, 2, 3, 3, 4 >{ 0, 1, 2, 2, 3 } == uncontract< 3, 2, 3 >( 2, Shape< 1, 2, 4 >{ 0, 1, 3 }) );

template< size_t I, size_t J, shape S >
struct TransposeShape
{ 
    static constexpr size_t ith = shape_element_v< I, S >;
    static constexpr size_t jth = shape_element_v< J, S >;

    using base_type = RemoveShapeElement< I, 
        typename RemoveShapeElement< J, S >::type >::type;
    using type = InsertShapeElement< ith, J, 
        typename InsertShapeElement< jth, I, base_type >::type >::type;

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

template< size_t I, size_t J, shape S >
using transpose_shape_t = TransposeShape< I, J, S >::type;

template< size_t I, size_t J, shape S >
constexpr transpose_shape_t< I, J, S > transpose_shape( S shp )
{ return TransposeShape< I, J, S >::value( shp ); }

// template< shape S, typename Seq >
// struct ElementToIndexSequenceHelper;

// template< shape S, size_t.. Is >
// struct ElementToIndexSequenceHelper< S, seq< Is... >>
// { using type = seq< shape_get< Is >( S )... >; };

// template< shape S >
// struct ElementToIndexSequence
// { using type = ElementToIndexSequenceHelper< S, make_seq< S::dimensions() >>::type };

// template< shape S >
// 

/**
 * A tensor is a shaped array
 */
template< shape S, typename T, typename... Ts >
requires( S::size() == 1 + sizeof...( Ts ))
struct Tensor : tuple< T, Ts... >
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

    template< typename... Us >
    constexpr Tensor& operator=( Tensor< shape_type, Us... > const& other )
    {
        if( &other != this )  
            equal_helper( other, make_seq< size() >{} );
        return *this;
    }

    constexpr Tensor( ) = default;
    constexpr Tensor( T t, Ts... ts ): tuple< T, Ts... >{ t, ts... } { }
    template< typename... Us >
    constexpr Tensor( Tensor< shape_type, Us... > const& other )
    { equal_helper( other, make_seq< size() >{} ); }
};

template< size_t I, typename TensorT >
constexpr auto tensor_get( TensorT const& ten )
{ return get< I >( ten ); }

template< typename T, typename TensorT >
constexpr T tensor_at( size_t element, TensorT const& ten )
{ return any_cast< T >( tuple_access( element, ten )); }

template< typename TensorT >
struct tensor_shape
{ using type = TensorT::shape_type; };

template< typename TensorT >
using tensor_shape_t = tensor_shape< TensorT >::type;

template< shape S, typename... Ts >
requires( S::size() == sizeof...( Ts ))
constexpr Tensor< S, Ts... > make_tensor( Ts... ts )
{ return { ts... }; }

namespace detail {
template< typename LeftT, typename RightT, size_t... Is >
constexpr bool equals_helper( LeftT const& left, RightT const& right,
    seq< Is... > )
{ return ((get< Is >( left ) == get< Is >( right )) and ... ); }

template< typename LeftT, typename RightT, size_t... Is >
constexpr auto plus_helper( LeftT const& left, RightT const& right, 
    seq< Is... > )
{ return make_tensor< typename LeftT::shape_type >(
    ( get< Is >( left ) + get< Is >( right ))... ); }

template< typename LeftT, typename RightT, size_t... Is >
constexpr auto minus_helper( LeftT const& left, RightT const& right, 
    seq< Is... > )
{ return make_tensor< LeftT::shape_type >(
    ( get< Is >( left ) - get< Is >( right ))... ); }

template< typename LeftT, typename RightT, size_t... Is >
constexpr auto product_helper( LeftT const& left, RightT const& right,
    seq< Is... > )
{
    using shape_type = shape_cat_t< typename LeftT::shape_type, 
        typename RightT::shape_type >;
    return make_tensor< shape_type >(( get< Is / right.size() >( left ) * 
        get< Is % right.size() >( right ))... );
}

template< typename T, typename TensorT, size_t... Is >
constexpr auto scale_helper( T scalar, TensorT const& ten, 
    seq< Is... > )
{ return make_tensor< tensor_shape_t< TensorT >>(
    ( get< Is >( ten ) * scalar )... ); }

template< typename T, typename TensorT, size_t... Is >
constexpr auto divide_scale_helper( T scalar, TensorT const& ten, 
    seq< Is... > )
{ return make_tensor< tensor_shape_t< TensorT >>(
    ( get< Is >( ten ) / scalar )... ); }

/**
 * returns a tensor composed of op(E) where E is the corresponding element from 
 * ten
 */
template< typename Op, typename TensorT, size_t... Is >
constexpr auto op_helper( Op& op, TensorT const& ten, seq< Is... > )
{ return make_tensor< tensor_shape_t< TensorT >>( op( get< Is >( ten ))... ); }

// forward decl
template< size_t K, size_t I, size_t J, shape S, typename Seq >
struct ContractedElementSequenceHelper;

/**
 * resolves to an index_sequence of elements from a tensor with shape S which 
 * when summed together equal the Kth element of the same tensor contracted on 
 * indices I and J
 * 
 * @tparam K the element of the contracted tensor
 * @tparam I the first index to be contracted on
 * @tparam J the second index to be contracted on
 * @tparam S is the shape of the tensor to be contracted
 * @tparam Is... are 0, 1, ... N where N == shape_element_v< I, S >
 */
template< size_t K, size_t I, size_t J, shape S, size_t... Is >
struct ContractedElementSequenceHelper< K, I, J, S, seq< Is... >>
{
    // calculate the contracted shape
    using contracted_shape = contract_shape_t< I, J, S >;
    static constexpr size_t contraction_size = shape_element_v< I, S >;

    // our elements will be the uncontracted index on Is
    using type = seq< uncontract_index< contraction_size, I, J >( Is, 
        contracted_shape::from_element( K ) )... >;
};

/**
 * defines an index sequence of elements to be summed for the Kth element
 * of a tensor with shape S contracted along the Ith and Jth indices
 */
template< size_t K, size_t I, size_t J, shape S >
using contracted_element_seq = ContractedElementSequenceHelper< K, I, J, S, 
    make_seq< shape_element_v< I, S >>>::type;

/**
 * sums the Is... elements of ten
 * 
 * @tparam TensorT is the type of the tensor
 * @tparam Is are the elements to be summed
 * @param ten is the tensor storing the values
 * @returns the sum of the Is elements of the input tensor
 */
template< typename TensorT, size_t... Is >
constexpr auto sum_elements_helper( TensorT const& ten, seq< Is... > )
{ return ( get< Is >( ten ) + ... ); }

/**
 * returns the Kth element of ten contracted along I and J
 */
template< size_t K, size_t I, size_t J, typename TensorT >
constexpr auto contracted_element( TensorT const& ten )
{ return sum_elements_helper( ten, 
    contracted_element_seq< K, I, J, typename TensorT::shape_type >{} ); }

template< size_t I, size_t J, typename TensorT, size_t... Is >
constexpr auto contract_helper( TensorT const& ten, seq< Is... > )
{
    static constexpr size_t i0 = min_v< I, J >;
    static constexpr size_t i1 = max_v< I, J >;
    using shape_type = contract_shape_t< i0, i1, typename TensorT::shape_type >;
    return make_tensor< shape_type >( contracted_element< Is, i0, i1 >( ten )... );
}

template< size_t K, size_t I, size_t J, typename TensorT >
constexpr auto transposed_element( TensorT const& ten )
{
    using shape_type = tensor_shape_t< TensorT >;
    using transposed_shape_type = transpose_shape_t< I, J, tensor_shape_t< TensorT >>;

    static constexpr auto k = transpose_shape< I, J >( transposed_shape_type::from_element( K ));
    return get< k >( ten );
}

template< size_t I, size_t J, typename TensorT, size_t... Ks >
constexpr auto transpose_helper( TensorT const& ten, seq< Ks... > )
{
    using shape_type = transpose_shape_t< I, J, tensor_shape_t< TensorT >>;
    return make_tensor< shape_type >( transposed_element< Ks, I, J >( ten )... );
}

template< size_t K, typename Seq, typename TensorT >
constexpr auto subtensor_element( TensorT const& ten )
{
    using shape_type = tensor_shape_t< TensorT >;
    using sub_shape_type = sub_tensor_shape_t< shape_type >;

    static constexpr const auto sub = sub_shape_type::from_element( K );
    static constexpr size_t L = shape_insert_at< Seq >( sub );
    return get< L >( ten );
}

template< size_t I, size_t K, typename TensorT >
constexpr auto element_subtensor_element( TensorT const& ten )
{
    using shape_type = tensor_shape_t< TensorT >;
    using sub_shape_type = sub_tensor_shape_t< shape_type >;

    static constexpr const auto sub = sub_shape_type::from_element( K );
    static constinit const size_t L = shape_insert_at_element< I >( sub );
    return get< L >( ten );
}

template< typename Seq, typename TensorT, size_t... Ks >
constexpr auto subtensor_helper( TensorT const& ten, seq< Ks... > )
{ 
    using sub_shape_type = sub_tensor_shape_t< tensor_shape_t< TensorT >>;
    return make_tensor< sub_shape_type >( subtensor_element< Ks, Seq >( ten )... ); 
}

// template< typename TensorT, size_t... Ks >
// constexpr auto subtensor_helper( tensor_shape_t< TensorT > shp, 
//     TensorT const& ten, seq< Ks... > )
// { 
//     using sub_shape_type = sub_tensor_shape_t< tensor_shape_t< TensorT >>;
//     return make_tensor< sub_shape_type >( subtensor_element< Ks >( shp, ten )... ); 
// }

template< size_t I, typename TensorT, size_t... Ks >
constexpr auto element_subtensor_helper( TensorT const& ten, seq< Ks... > )
{ 
    using sub_shape_type = sub_tensor_shape_t< tensor_shape_t< TensorT >>;
    return make_tensor< sub_shape_type >( 
        element_subtensor_element< I, Ks >( ten )... ); 
}

} // namespace detail

/**
 * operators
 */
template< shape S, typename... Ts, typename... Us >
constexpr auto operator ==( Tensor< S, Ts... > const& left, Tensor< S, Us... > const& right )
{ return detail::equals_helper( left, right, make_seq< sizeof...( Ts )>{} ); }

template< shape S, typename... Ts, typename... Us >
constexpr auto operator !=( Tensor< S, Ts... > const& left, Tensor< S, Us... > const& right )
{ return not detail::equals_helper( left, right, make_seq< sizeof...( Ts )>{} ); }

template< shape S, typename... Ts, typename... Us >
constexpr auto operator +( Tensor< S, Ts... > const& left, Tensor< S, Us... > const& right )
{ return detail::plus_helper( left, right, make_seq< sizeof...( Ts )>{} ); }

template< shape S, typename... Ts, typename... Us >
constexpr auto operator -( Tensor< S, Ts... > const& left, Tensor< S, Us... > const& right )
{ return detail::minus_helper( left, right, make_seq< sizeof...( Ts )>{} ); }

template< shape A, typename... Ts, shape B, typename... Us >
constexpr auto operator *( Tensor< A, Ts... > const& left, Tensor< B, Us... > const& right )
{ return detail::product_helper( left, right, make_seq< sizeof...( Ts ) * sizeof...( Us )>{} ); }

template< typename T, shape S, typename... Ts >
constexpr auto scale( T scalar, Tensor< S, Ts... > const& ten )
{ return detail::scale_helper( scalar, ten, make_seq< sizeof...( Ts )>{} ); }

template< typename T, shape S, typename... Ts >
constexpr auto divide_scale( T scalar, Tensor< S, Ts... > const& ten )
{ return detail::divide_scale_helper( scalar, ten, make_seq< sizeof...( Ts )>{} ); }


/**
 * contracts a tensor along equinumerous indices
 */
template< size_t I, size_t J, typename TensorT >
requires( shape_element_v< I, tensor_shape_t< TensorT >> == 
    shape_element_v< J, tensor_shape_t< TensorT >> )
constexpr auto contract( TensorT const& ten )
{ 
    static constexpr size_t contracted_size = 
        contract_shape_t< I, J, tensor_shape_t< TensorT >>::size();

    return detail::contract_helper< I, J >( ten, make_seq< contracted_size >{} ); 
}

/**
 * transpose a tensor along two indices
 */
template< size_t I, size_t J, typename TensorT >
constexpr auto transpose( TensorT const& ten )
{ return detail::transpose_helper< I, J >( ten, 
    make_seq< tensor_shape_t< TensorT >::size() >{} ); }

/**
 * returns a sub-tensor of the original by removing the dimensions containing  
 * Index
 */
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

/**
 * returns the subtensor of element I in tensor ten
 */
template< size_t I, typename TensorT >
constexpr auto element_subtensor( TensorT const& ten )
{
    using sub_shape_type = sub_tensor_shape_t< tensor_shape_t< TensorT >>;
    return detail::element_subtensor_helper< I >( ten, 
        make_seq< sub_shape_type::size() >{} );
}

// forward decl
template< typename TensorT >
requires( tensor_shape_t< TensorT >::dimensions() == 2 and
    shape_element_v< 0, tensor_shape_t< TensorT >> ==
        shape_element_v< 1, tensor_shape_t< TensorT >> )
constexpr auto det( TensorT const& ten );

namespace detail {
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

template< typename TensorT >
constexpr auto det_helper( TensorT const& ten, seq< 0 > )
{ return get< 0 >( ten ); }

template< typename TensorT, size_t... Is >
constexpr auto cofactor_helper( TensorT const& ten, seq< Is... > )
{ 
    using shape_type = tensor_shape_t< TensorT >;
    return make_tensor< shape_type >((
        ( is_element_even_v< Is, shape_type > ? 1 : -1 ) * // i think this works for element ids too...
        det( element_subtensor< Is >( ten )))... );
}

} // namespace detail

/**
 * determinant of a square tensor
 * 
 * @tparam TensorT the type of the tensor
 * @param ten the tensor itself
 */
template< typename TensorT >
requires( tensor_shape_t< TensorT >::dimensions() == 2 and
    shape_element_v< 0, tensor_shape_t< TensorT >> ==
        shape_element_v< 1, tensor_shape_t< TensorT >> )
constexpr auto det( TensorT const& ten )
{ return detail::det_helper( ten, 
    make_seq< shape_element_v< 0, tensor_shape_t< TensorT >> >{} ); }


/**
 * cofactor matrix of a square matrix
 * 
 * @tparam TensorT is the type of the tensor
 * @param ten is the tensor itself
 * @returns a tensor of the same shape containing the cofactors of ten
 */
template< typename TensorT >
requires( tensor_shape_t< TensorT >::dimensions() == 2 and
    shape_element_v< 0, tensor_shape_t< TensorT >> ==
        shape_element_v< 1, tensor_shape_t< TensorT >> )
constexpr auto cofactor( TensorT const& ten )
{ 
    using shape_type = tensor_shape_t< TensorT >;
    return detail::cofactor_helper( ten, make_seq< shape_type::size() >{} );
}

/**
 * inverse of a square matrix
 */
template< typename TensorT >
requires( tensor_shape_t< TensorT >::dimensions() == 2 and
    shape_element_v< 0, tensor_shape_t< TensorT >> ==
        shape_element_v< 1, tensor_shape_t< TensorT >> )
constexpr auto inverse( TensorT const& ten )
{ return divide_scale( det( ten ), transpose< 0, 1 >( cofactor( ten ))); }


} // namespace tensor

#endif