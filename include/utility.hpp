#ifndef __UTILITY_HPP__
#define __UTILITY_HPP__

#include <utility>
#include <tuple>
#include <ranges>
#include <memory>
#include <type_traits>

using std::size_t;

/**
 * math utilities
 */
template< size_t X >
struct Identity { static constexpr size_t value = X; };

// min_v and max_v details
namespace detail {

template< size_t I, size_t... Is >
struct Min
{ static constexpr size_t value = ( I < Min< Is... >::value ? I : Min< Is... >::value ); };

template< size_t I >
struct Min< I >
{ static constexpr size_t value = I; };

template< size_t I, size_t... Is >
struct Max
{ static constexpr size_t value = ( I > Max< Is... >::value ? I : Max< Is... >::value ); };

template< size_t I >
struct Max< I >
{ static constexpr size_t value = I; };

} // namespace detail

/**
 * Evaluates to the minimum (or maximum) value of the parameter pack
 * 
 * @tparam Is are the values to extract the minimum of
 * @return the minimum of the parameters Is
 */
template< size_t... Is >
static constexpr size_t min_v = detail::Min< Is... >::value;

template< size_t... Is >
static constexpr size_t max_v = detail::Max< Is... >::value;

/**
 * noop_t always evaluates to it's template parameter no matter what size_t is
 * passed as the secon parameter.  This is useful for constructing uniform
 * pack types from sequences
 */
namespace detail {
template< typename T, size_t Ignored >
struct NoopType { using type = T; };

} // namespace detail (noop_t)

template< typename T, size_t Ignored >
using noop_t = detail::NoopType< T, Ignored >::type;

/**
 * Sequence helpers
 */
template< size_t... Is >
using seq = std::index_sequence< Is... >;

template< size_t Size >
using make_seq = std::make_index_sequence< Size >;

/**
 * override of std::get<I> that can call Type::template at<I>
 */
template< typename T >
concept has_at = requires( T t )
{ t.template at< 0 >(); };

template< size_t I, size_t J, has_at T >
auto const& get( T const& obj )
{ return obj.template at< I, J >(); }

template< size_t I, size_t J, has_at T >
auto& get( T& obj )
{ return obj.template at< I, J >(); }


/** 
 * is_virtual_base_of implementation taken from 
 * [https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2985r0.html]
 */
template< typename Base, typename Derived >
struct is_virtual_base_of
{ static constexpr const bool value = requires { 
    (Base *)std::declval<Derived *>();
    (Derived *)std::declval<Base *>(); }; };

template< typename Base, typename Derived >
using is_virtual_base_of_v = is_virtual_base_of< Base, Derived >::value;

template< typename Derived, typename Base >
concept virtual_base_of = is_virtual_base_of< Base, Derived >::value;

/**
 * compile time math
 */
namespace detail {

// IsSquareHelper subtracts the NextOdd until NextOdd is bigger than N
// n^2 == sum_{ 0 <= i <= n }{ 2i + 1 }
// i.e. n^2 is the sum of the first n odd numbers
template< size_t N, size_t Next = 1 >
struct IsSquareHelper;

template< size_t N, size_t Next >
requires ( N < Next )
struct IsSquareHelper< N, Next >
{ static constexpr bool value = (N == 0); };

template< size_t N, size_t Next >
requires ( N >= Next )
struct IsSquareHelper< N, Next >
{ static constexpr bool value = IsSquareHelper< N - Next, Next + 2 >::value; };

template< size_t N, size_t R = 1 >
struct SquareRootHelper;

template< size_t N, size_t R >
requires ( N < R )
struct SquareRootHelper< N, R >
{ static constexpr size_t value = R >> 1; }; // R == 2X + 1 so R/2 == X

template< size_t N, size_t R >
requires ( N >= R )
struct SquareRootHelper< N, R >
{ static constexpr size_t value = SquareRootHelper< N - R, R + 2 >::value; };

// NOTE: the largest numbers that can be checked this way depends on the -ftemplate-depth 
//       parameter. For me this is about 500^2, but i've set to 256^2 to be safe
static constexpr size_t maximum_square_root_value = 256;
static constexpr size_t maximum_is_square_value = 
    maximum_square_root_value * maximum_square_root_value;

} // namespace detail


template< size_t N >
requires ( N <= detail::maximum_is_square_value )
struct is_square
{ static constexpr bool value = detail::IsSquareHelper< N >::value; };

template< size_t N >
requires ( N <= detail::maximum_is_square_value )
struct square_root
{ static constexpr size_t value = detail::SquareRootHelper< N >::value; };

static_assert( is_square< detail::maximum_is_square_value >::value );
static_assert( square_root< detail::maximum_is_square_value >::value == 
    detail::maximum_square_root_value );

/**
 * remove_nth removes an index from a run-time RandomAccessContainer
 */
template< std::ranges::random_access_range Container, typename AllocType >
requires std::uses_allocator_v< Container, AllocType >
Container remove_nth( size_t N, Container const& list )
{
    auto i = std::ranges::begin( list );
    auto e = std::ranges::end( list );
    size_t size = std::distance( i, e );

    auto ret = Container{ std::allocator_arg, size, list.allocator() };

    auto j = std::ranges::begin( ret );
    for( size_t n = 0; i != e; ++n, ++i, ++j )
    {
        if( n == N && ++i == e ) // skip if this one is the Nth
            break;               // break if we reach the end when we skip
        
        *j = *i;
    }

    return ret;
}

/**
 * are_equal variadic compile time predicate
 */
template< typename... Ts >
constexpr bool are_equal( Ts... ts );

template< >
constexpr bool are_equal()
{ return true; }

template< typename T >
constexpr bool are_equal( T )
{ return true; }

template< typename T, typename U >
constexpr bool are_equal( T t, U u )
{ return t == u; }

template< typename T, typename U, typename... Ts >
constexpr bool are_equal( T t, U u, Ts... ts )
{ return t == u && are_equal( u, ts... ); }

/**
 * sequence helpers (credit to Google Gemini)
 */

// 1. Primary template: The "worker" that performs the recursion
template <typename T, typename Result, T... Rest>
struct reverse_helper;

// 2. Base case: When the input sequence is empty, return the accumulated result
template <typename T, T... Reversed>
struct reverse_helper<T, std::integer_sequence<T, Reversed...>> {
    using type = std::integer_sequence<T, Reversed...>;
};

// 3. Recursive step: Move the first element of the "Rest" pack 
// to the front of the "Reversed" pack
template <typename T, T... Reversed, T First, T... Rest>
struct reverse_helper<T, std::integer_sequence<T, Reversed...>, First, Rest...> {
    using type = typename reverse_helper<T, std::integer_sequence<T, First, Reversed...>, Rest...>::type;
};

// 4. Public Interface
template <typename T>
struct reverse_integer_sequence;

template <typename T, T... Is>
struct reverse_integer_sequence<std::integer_sequence<T, Is...>> {
    using type = typename reverse_helper<T, std::integer_sequence<T>, Is...>::type;
};

/**
 * reverses an integer sequence
 */
template <typename T>
using reverse_integer_sequence_t = typename reverse_integer_sequence<T>::type;

#ifndef NDEBUG
namespace test {
static_assert( std::is_same_v< std::index_sequence< 0, 1, 2, 3 >,
    reverse_integer_sequence_t< std::index_sequence< 3, 2, 1, 0 >>> );
} // namespace test
#endif


/**
 * Tuple helpers
 */
namespace detail {

using std::index_sequence;

template< typename TupleType, size_t... Is >
struct TupleSelect
{ using type = std::tuple< std::tuple_element_t< Is, TupleType >... >; };

template< typename TupleType, size_t... Is >
using tuple_select_t = typename TupleSelect< TupleType, Is... >::type;

template< typename TupleType, size_t... Is >
TupleSelect< TupleType, Is... >::type 
tuple_select( TupleType const& tup, index_sequence< Is... > )
{ return std::make_tuple( std::get< Is >( tup )... ); }

template< typename TupleType, size_t N, typename IndexSequence >
struct RemoveNthHelper;

template< typename TupleType, size_t N, size_t... Is >
struct RemoveNthHelper< TupleType, N, index_sequence< Is... >>
{ using type = std::tuple< std::tuple_element_t< ( Is < N ? Is : Is + 1 ), TupleType >... >; };

template< typename TupleType, size_t N >
struct RemoveNth
{ 
    static constexpr const size_t tuple_size = std::tuple_size_v< TupleType >;
    using type = RemoveNthHelper< TupleType, N, std::make_index_sequence< tuple_size >>::type; 
};

template< size_t N, typename TupleType >
using remove_nth_t = typename RemoveNth< TupleType, N >::type;

template< size_t N, typename TupleType, size_t... Is >
remove_nth_t< N, TupleType >
remove_nth_helper( TupleType const& tup, index_sequence< Is... > )
{ return tuple_select( tup, std::index_sequence< ( Is < N ? Is : Is + 1 )... >{} ); }

} // namespace detail

template< size_t N, typename... Ts >
requires ( N < sizeof...( Ts ) )
auto remove_nth( std::tuple< Ts... > const& tup )
{ return detail::remove_nth_helper< N >( tup, 
    std::make_index_sequence< sizeof...( Ts ) - 1 >{} ); }




/**
 * Pack helpers
 */
template< typename PackType >
struct PackSize;

template< template< typename... > class Pack, typename... Ts >
struct PackSize< Pack< Ts... > >
{ static constexpr const size_t size = sizeof...( Ts ); };

template< typename T, typename... >
using pack_front = T;

namespace detail {

template< typename... >
struct pack_back_helper;

template< typename T >
struct pack_back_helper< T >
{ using type = T; };

template< typename T, typename... Ts >
struct pack_back_helper< T, Ts... >
{ using type = typename pack_back_helper< Ts... >::type; };

} // detail

template< typename... Ts >
using pack_back = typename detail::pack_back_helper< Ts... >::type;

namespace detail {

template< template < typename... > class Pack, typename TupleType >
struct PackFromTuple;

template< template < typename... > class Pack, typename... Ts >
struct PackFromTuple< Pack, std::tuple< Ts... >>
{ using type = Pack< Ts... >; };

template< typename PackType >
struct TupleFromPack;

template< template < typename... > class Pack, typename... Ts  >
struct TupleFromPack< Pack< Ts... >>
{ using type = std::tuple< Ts... >; };

template< typename PackedType, size_t... Is >
struct PackSelect;

template< template< typename... > class Pack, typename... Ts, size_t... Is >
struct PackSelect< Pack< Ts... >, Is... >
{
private:
    using tuple_type = std::tuple< Ts... >;
public:
    using type = PackFromTuple< Pack, detail::tuple_select_t< tuple_type, Is... >>::type;
};

template< typename PackedType, size_t... Is >
using pack_select_t = typename PackSelect< PackedType, Is... >::type;

template< size_t N, typename PackType, typename IndexSequence >
struct PackRemoveNthHelper;

template< size_t N, typename PackType, size_t... Is >
struct PackRemoveNthHelper< N, PackType, std::index_sequence< Is... >>
{  using type = pack_select_t< PackType, ( Is < N ? Is : Is + 1 )... >; };

template< size_t N, typename PackType >
struct PackRemoveNth
{ 
private:
    static constexpr const size_t pack_size = PackSize< PackType >::size;
    using pack_index_type = std::make_index_sequence< pack_size >;

public:
    using type = typename PackRemoveNthHelper< N, PackType, pack_index_type >::type; 
};

template< size_t N, typename PackType, size_t... Is >
remove_nth_t< N, PackType >
pack_remove_nth_helper( PackType const& pack, index_sequence< Is... > )
{ return pack_select( pack, std::index_sequence< ( Is < N ? Is : Is + 1 )... >{} ); }

} // namespace detail

template< typename PackType, size_t... Is >
requires (( Is < PackSize< PackType >::size ) and ... )
detail::pack_select_t< PackType, Is... > 
pack_select( PackType const& pack, std::index_sequence< Is... > )
{ return { std::get< Is >( pack )... }; }

template< size_t N, typename PackType >
requires ( N < PackSize< PackType >::size )
detail::remove_nth_t< N, PackType >
remove_nth( PackType const& pack )
{ 
    static constexpr size_t pack_size = PackSize< PackType >::size;
    return detail::pack_remove_nth_helper< N >( pack, std::make_index_sequence< pack_size >{} ); 
}

#endif