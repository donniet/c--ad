#ifndef __UTILITY_HPP__
#define __UTILITY_HPP__

#include <utility>
#include <tuple>
#include <ranges>
#include <memory>
#include <type_traits>
#include <string>
#include <locale>
#include <cmath>
#include <array>
#include <functional>
#include <any>

using std::size_t;
using size_diff_t = std::ptrdiff_t;

using std::integral_constant;
using std::tuple, std::tuple_cat, std::tuple_size_v, std::tuple_element_t, 
    std::make_tuple;
using std::is_same_v, std::is_convertible_v;
using std::string, std::to_string;
using std::isspace;
using std::isless, std::isgreater;
using std::array;
using std::function;
using std::any, std::make_any, std::any_cast;

////////////////
/// Simple Stack
///
template< typename T >
struct stack_of: protected std::vector< T >
{
    constexpr void push( T&& value ) 
    { std::vector< T >::push_back( value ); }

    constexpr void push( T const& value ) 
    { std::vector< T >::push_back( value ); }

    constexpr T const& top() const
    { return std::vector< T >::back(); }

    constexpr T pop()
    {
        if( size() == 0 )
            throw std::logic_error( "stack is empty" );
            
        T ret = top();
        std::vector< T >::pop_back();
        return ret;
    }

    using value_type = T;
    using iterator = std::vector< T >::iterator;
    using reverse_iterator = std::vector< T >::reverse_iterator;
    using const_iterator = std::vector< T >::const_iterator;
    using const_reverse_iterator = std::vector< T >::const_reverse_iterator;

    using std::vector< T >::begin;
    using std::vector< T >::end;
    using std::vector< T >::rbegin;
    using std::vector< T >::rend;
    using std::vector< T >::cbegin;
    using std::vector< T >::cend;
    using std::vector< T >::size;
    using std::vector< T >::clear;

    constexpr stack_of& operator=( stack_of const& ) = default;

    constexpr stack_of() = default;
    constexpr stack_of( stack_of const& ) = default;
    constexpr stack_of( stack_of&& ) = default;
};


/////////////////////
/// Sequence Helpers
///
/// @brief shorthand for an std::index_sequence
/// @tparam ...Is sequence of indices
template< size_t... Is >
using seq = std::index_sequence< Is... >;

/// @brief shorthand for std::make_index_sequence
/// @tparam Size is the number of elements in the index sequence
template< size_t Size >
using make_seq = std::make_index_sequence< Size >;

////////////////
/// String Helpers
///
/// @brief trim the left spaces from a string
/// @param s the string
/// @return the string with the left-most spaces removed
constexpr string& ltrim( string& s )
{
    auto i = s.begin();
    for(; i != s.end() and isspace( *i ); ++i ) {}
    s.erase( s.begin(), i );
    return s;
}

/// @brief trim the left spaces from a constant string and return the result
/// @param s the string
/// @return the string with the left-most spaces removed
constexpr string ltrim( string const& s )
{
    string ret = s;
    ltrim( ret );
    return ret;
}

/// @brief trim the right spaces from a string
/// @param s a reference to a string
/// @return the string reference
constexpr string& rtrim( string& s )
{
    auto i = s.rbegin();
    for(; i != s.rend() and isspace( *i ); ++i ) {}
    s.erase( i.base(), s.end() );
    return s;
}

/// @brief trim the right spaces from a constant string
/// @param s the constant string reference
/// @return the trimmed string
constexpr string rtrim( string const& s )
{
    string ret = s;
    rtrim( ret );
    return ret;
}

/// @brief trim the spaces from both the left and right of a string
/// @param s the string reference
/// @return the trimmed string reference
constexpr string& trim( string& s )
{ return ltrim( rtrim( s )); }

/// @brief trim a constant string 
/// @param s the reference to the constant string
/// @return the string s with spaces trimmed from the left and right
constexpr string trim( string const& s )
{ 
    string ret = s;
    ltrim( rtrim( ret ));
    return ret;
}

/////////////////
/// Math Utilities
///
namespace std {

/// @brief power function with templated exponent
/// @tparam Exp the exponent
/// @param arg the base
/// @return arg raised to the Exp power
template< int Exp >
constexpr auto pow( float arg )
{ return std::pow( arg, Exp ); }

/// @brief power function with templated exponent
/// @tparam Exp the exponent
/// @param arg the base
/// @return arg raised to the Exp power
template< int Exp >
constexpr auto pow( double arg )
{ return std::pow( arg, Exp ); }

/// @brief power function with templated exponent
/// @tparam Exp the exponent
/// @param arg the base
/// @return arg raised to the Exp power
template< int Exp >
constexpr auto pow( long double arg )
{ return std::pow( arg, Exp ); }

/// @brief power function with templated exponent
/// @tparam Exp the exponent
/// @param arg the base
/// @return arg raised to the Exp power
template< int Exp >
constexpr auto pow( short arg )
{ return std::pow( arg, Exp ); }

/// @brief power function with templated exponent
/// @tparam Exp the exponent
/// @param arg the base
/// @return arg raised to the Exp power
template< int Exp >
constexpr auto pow( int arg )
{ return std::pow( arg, Exp ); }

/// @brief power function with templated exponent
/// @tparam Exp the exponent
/// @param arg the base
/// @return arg raised to the Exp power
template< int Exp >
constexpr auto pow( long arg )
{ return std::pow( arg, Exp ); }

/// @brief power function with templated exponent
/// @tparam Exp the exponent
/// @param arg the base
/// @return arg raised to the Exp power
template< int Exp >
constexpr auto pow( long long arg )
{ return std::pow( arg, Exp ); }

/// @brief power function with templated exponent
/// @tparam Exp the exponent
/// @param arg the base
/// @return arg raised to the Exp power
template< int Exp >
constexpr auto pow( unsigned short arg )
{ return std::pow( arg, Exp ); }

/// @brief power function with templated exponent
/// @tparam Exp the exponent
/// @param arg the base
/// @return arg raised to the Exp power
template< int Exp >
constexpr auto pow( unsigned int arg )
{ return std::pow( arg, Exp ); }

/// @brief power function with templated exponent
/// @tparam Exp the exponent
/// @param arg the base
/// @return arg raised to the Exp power
template< int Exp >
constexpr auto pow( unsigned long arg )
{ return std::pow( arg, Exp ); }

/// @brief power function with templated exponent
/// @tparam Exp the exponent
/// @param arg the base
/// @return arg raised to the Exp power
template< int Exp >
constexpr auto pow( unsigned long long arg )
{ return std::pow( arg, Exp ); }

} // namespace std;

namespace detail {
template< typename T, size_t Size, size_t... Is >
constexpr T sum_helper( std::array< T, Size > const& values, seq< Is... > )
{ return ( values[ Is ] + ... ); }
} // namespace detail

/// @brief sum an array of values together by folding the + operator
/// @tparam T the type to be summed
/// @tparam Size of the array
/// @param values to be summed
/// @return the sum of values
template< typename T, size_t Size >
constexpr T sum( std::array< T, Size > const& values )
{ return detail::sum_helper( values, make_seq< Size >{} ); }

/// @brief permutations of n
/// @param n number of distict items
/// @return the number of permutations of n distinct items (n!)
constexpr size_t perm( size_t n )
{
    size_t p = 1;
    for(; n > 0; --n ) 
        p *= n;
    return p;
}

/// @brief a trait which returns a size_t equal to the parameter
/// @tparam X parameter returned as a static value member
template< size_t X >
struct Identity: integral_constant< size_t, X > { };

// min_v and max_v details
namespace detail {

/// @brief minimum of a parameter pack of size_t's
/// @tparam I the first value
/// @tparam ...Is the remaining values
template< size_t I, size_t... Is >
struct Min: integral_constant< size_t, 
    ( isless( I,  Min< Is... >::value ) ? I : Min< Is... >::value ) > { };

/// @brief minimum of a single value is the value
/// @tparam I the value
template< size_t I >
struct Min< I >: integral_constant< size_t, I > { };

/// @brief maximum of a parameter pack of size_t's
/// @tparam I the first value
/// @tparam ...Is the remaining values
template< size_t I, size_t... Is >
struct Max: integral_constant< size_t, 
    ( isgreater( I,  Max< Is... >::value) ? I : Max< Is... >::value )> { };

/// @brief maximum of a single value is the value
/// @tparam I the value
template< size_t I >
struct Max< I >: integral_constant< size_t, I > { };

/// @brief the first element greater than every element in the parameter pack
/// @tparam ...Is the values
template< size_t... Is >
struct LeastUpperBound: integral_constant< size_t, 1 + Max< Is... >::value > { };

/// @brief the least upper bound of an empty set is 0
template<>
struct LeastUpperBound<>
{ static constexpr size_t value = 0; };

} // namespace detail

/// @brief minimum of a size_t parameter pack
/// @tparam ...Is the values
template< size_t... Is >
static constexpr size_t min_v = detail::Min< Is... >::value;

/// @brief maximum of a size_t parameter pack
/// @tparam ...Is the values
template< size_t... Is >
static constexpr size_t max_v = detail::Max< Is... >::value;

/// @brief the smallest size_t greater than every element of the parameter pack
/// @tparam ...Is the values
template< size_t... Is >
static constexpr size_t least_upper_bound_v = detail::LeastUpperBound< Is... >::value;

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

/// @brief checks if a template parameter is a square integer
/// @tparam N the value to check
template< size_t N >
requires ( N <= detail::maximum_is_square_value )
struct is_square: integral_constant< bool, detail::IsSquareHelper< N >::value >
{ };

/// @brief evaluates the integer square root of a size_t template parameter
/// @tparam N the value to determine the integer square root of
template< size_t N >
requires ( N <= detail::maximum_is_square_value )
struct square_root: integral_constant< size_t, 
    detail::SquareRootHelper< N >::value > { };

/// verify that this compiler supports the maximums for this square root op
static_assert( is_square< detail::maximum_is_square_value >::value );
static_assert( square_root< detail::maximum_is_square_value >::value == 
    detail::maximum_square_root_value );

/// @brief remove_nth removes an index from a run-time RandomAccessContainer
/// @tparam AllocT type of allocator used in container
/// @tparam ContainerT type of the container
/// @param N index of element to remove
/// @param list container values
/// @return a new container with the Nth element removed
template< std::ranges::random_access_range ContainerT, typename AllocT >
requires std::uses_allocator_v< ContainerT, AllocT >
constexpr ContainerT remove_nth( size_t N, ContainerT const& list )
{
    auto i = std::ranges::begin( list );
    auto e = std::ranges::end( list );
    size_t size = std::distance( i, e );

    auto ret = ContainerT{ std::allocator_arg, size, list.allocator() };

    auto j = std::ranges::begin( ret );
    for( size_t n = 0; i != e; ++n, ++i, ++j )
    {
        if( n == N && ++i == e ) // skip if this one is the Nth
            break;               // break if we reach the end when we skip
        
        *j = *i;
    }

    return ret;
}

template< typename T, typename... Ts >
requires(( is_same_v< T, Ts > and ... ))
constexpr T min_of( T first, Ts... rest )
{
    if constexpr( sizeof...( Ts ) == 0 )
        return first;

    T rest_min = min_of( rest... );
    return first <= rest_min ? first : rest_min;
}

template< typename T, typename... Ts >
requires(( is_same_v< T, Ts > and ... ))
constexpr T max_of( T first, Ts... rest )
{
    if constexpr( sizeof...( Ts ) == 0 )
        return first;

    T rest_max = max_of( rest... );
    return first >= rest_max ? first : rest_max;
}

/// @brief determines if the values passed are equal even if they are different
/// types
/// @tparam ...Ts the types of the parameters
/// @param ...ts the values
/// @return true of all the elements are equal. Assumes operator== is transitive
template< typename... Ts >
constexpr bool are_equal( Ts... ts );

/// @brief null case of are_equal always returns true
/// @return true identically
template< >
constexpr bool are_equal()
{ return true; }

/// @brief terminal case of are_equal for a single element must be true to 
/// ensure recursive case works
/// @tparam T type of parameter
/// @return true identically
template< typename T >
constexpr bool are_equal( T )
{ return true; }

/// @brief terminal case of are_equal for two elements is true if t == u
/// @tparam T the type of the first parameter
/// @tparam U the type of the second parameter
/// @param t the first value
/// @param u the second value
/// @return t == u
template< typename T, typename U >
constexpr bool are_equal( T t, U u )
{ return t == u; }

/// @brief recursive case of are_equal
/// @tparam T the first parameter type
/// @tparam U the second parameter type
/// @tparam ...Ts the remaining parameter types
/// @param t the first parameter
/// @param u the second parameter
/// @param ...ts the reamining parameters
/// @return true if all parameters are equal
template< typename T, typename U, typename... Ts >
constexpr bool are_equal( T t, U u, Ts... ts )
{ return t == u && are_equal( u, ts... ); }

/////////////////////
/// Sequence Helpers
/// credit: Google Gemini

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

/// @brief evaluates to an integer_sequence which is the reverse of the input
/// @tparam T the input sequence
template <typename T>
using reverse_integer_sequence_t = typename reverse_integer_sequence<T>::type;

namespace detail {
template< size_t index, typename Seq >
struct sequence_at_helper;

template< size_t index, size_t I, size_t... Is >
struct sequence_at_helper< index, seq< I, Is... >>
{ static constexpr size_t value = 
    sequence_at_helper< index-1, seq< Is... >>::value; };

template< size_t I, size_t... Is >
struct sequence_at_helper< 0, seq< I, Is... >>
{ static constexpr size_t value = I; };

/// @brief trait to determine if an index is in an index sequence
template< size_t I, typename Seq >
struct SequenceContains;

/// @brief null case for testing if a sequence contains an index
template< size_t I >
struct SequenceContains< I, seq< >>: 
    integral_constant< bool, false > { };

/// @brief success case 
template< size_t I, size_t J, size_t... Js >
requires( I == J )
struct SequenceContains< I, seq< J, Js... >>: 
    integral_constant< bool, true > { };

/// @brief recurse case
template< size_t I, size_t J, size_t... Js >
requires( I != J )
struct SequenceContains< I, seq< J, Js... >>:
    SequenceContains< I, seq< Js... >> { };

} // namespace detail

/// @brief trait to determine of a sequence contains an index
/// @tparam I is the index to search for
/// @tparam Seq is the index sequence to search
/// @returns true of Seq contains the index I
template< size_t I, typename Seq >
constexpr size_t sequence_contains_v = 
    detail::SequenceContains< I, Seq >::value;

/// @brief value of the sequence at a given index
/// @tparam Seq the integer sequence
/// @tparam index of the desired element
template< size_t index, typename Seq >
static constexpr size_t sequence_at = 
    detail::sequence_at_helper< index, Seq >::value;

namespace detail {

template< typename SeqT, typename SeqU >
struct ConcatSeq;

template< size_t... Is, size_t... Js >
struct ConcatSeq< seq< Is... >, seq< Js... >>
{ using type = seq< Is..., Js... >; };

template< typename Seq >
struct MinimumSequenceElement;

template< size_t I >
struct MinimumSequenceElement< seq< I >>
{ static constexpr size_t value = I; };

template< size_t I, size_t... Is >
requires( isgreater( sizeof...( Is ), 0 ))
struct MinimumSequenceElement< seq< I, Is... >>
{ static constexpr size_t value = (( I <= Is ) and ... ) ? I : 
    MinimumSequenceElement< seq< Is... >>::value; };

template< size_t I, typename Seq >
struct RemoveSequenceElement;
        
template< size_t I >
struct RemoveSequenceElement< I, seq< >>
{ using type = seq< >; };

template< size_t I, size_t J, size_t... Js >
requires( I == J )
struct RemoveSequenceElement< I, seq< J, Js... >>
{ using type = RemoveSequenceElement< I, seq< Js... >>::type; };

template< size_t I, size_t J, size_t... Js >
requires( I != J )
struct RemoveSequenceElement< I, seq< J, Js... >>
{ using type = ConcatSeq< seq< J >, 
    typename RemoveSequenceElement< I, seq< Js... >>::type >::type; };

template< typename Seq >
struct SortUniqueSequence;

template< >
struct SortUniqueSequence< seq< >>
{ using type = seq< >; };

template< size_t I >
struct SortUniqueSequence< seq< I >>
{ using type = seq< I >; };

template< size_t... Is >
requires( isgreater( sizeof...( Is ), 0 ))
struct SortUniqueSequence< seq< Is... >>
{ 
protected:
    static constexpr size_t min_element = 
        MinimumSequenceElement< seq< Is... >>::value;

public:
    using type = ConcatSeq< seq< min_element >, 
        typename SortUniqueSequence< typename RemoveSequenceElement< 
            min_element, seq< Is... >>::type >::type >::type;
};

static_assert( is_same_v< seq< >, SortUniqueSequence< seq< >>::type > );
static_assert( is_same_v< seq< 1 >, SortUniqueSequence< seq< 1 >>::type > );
static_assert( is_same_v< seq< 1, 2 >, SortUniqueSequence< seq< 1, 2 >>::type > );
static_assert( is_same_v< seq< 1, 2 >, SortUniqueSequence< seq< 1, 1, 2 >>::type > );
static_assert( is_same_v< seq< 1, 2 >, SortUniqueSequence< seq< 1, 2, 1 >>::type > );
static_assert( is_same_v< seq< 1, 2, 3 >, SortUniqueSequence< seq< 3, 2, 1 >>::type > );
static_assert( is_same_v< seq< 1, 2, 3 >, 
    SortUniqueSequence< seq< 3, 2, 3, 3, 2, 2, 1, 1, 1, 2, 3, 3 >>::type > );

template< typename... Seqs >
struct MergeUniqueSortedSequences;

template< size_t... Is >
struct MergeUniqueSortedSequences< seq< Is... >>
{ using type = seq< Is... >; };

template< size_t... Js >
requires( isgreater( sizeof...( Js ), 0 )) // for disambiguation
struct MergeUniqueSortedSequences< seq< Js... >, seq< >>
{ using type = seq< Js... >; };

template< size_t... Js >
struct MergeUniqueSortedSequences< seq< >, seq< Js... >>
{ using type = seq< Js... >; };

template< size_t I, size_t... Is, size_t J, size_t... Js >
requires( isless( I, J ))
struct MergeUniqueSortedSequences< seq< I, Is... >, seq< J, Js... >>
{ using type = ConcatSeq< seq< I >, typename MergeUniqueSortedSequences< 
    seq< Is... >, seq< J, Js... >>::type >::type; };

template< size_t I, size_t... Is, size_t J, size_t... Js >
requires( not isless( I, J ))
struct MergeUniqueSortedSequences< seq< I, Is... >, seq< J, Js... >>
{ using type = ConcatSeq< seq< J >, typename MergeUniqueSortedSequences< 
    seq< I, Is... >, seq< Js... >>::type >::type; };

template< typename First, typename... Rest >
requires( isgreater( sizeof...( Rest ), 1 ))
struct MergeUniqueSortedSequences< First, Rest... >
{ using type = MergeUniqueSortedSequences< First, 
        typename MergeUniqueSortedSequences< Rest... >::type >::type; };

} // namespace detail

template< typename Seq >
using sort_unique_seq = detail::SortUniqueSequence< Seq >::type;

template< typename... Seqs >
using merge_unique_sorted_seq = 
    detail::MergeUniqueSortedSequences< Seqs... >::type;

////////////////////
/// Tuple Helpers
///
namespace detail {

using std::index_sequence;

template< typename TupleT, size_t... Is >
struct TupleSelect
{ using type = std::tuple< std::tuple_element_t< Is, TupleT >... >; };

template< typename TupleT, typename Seq >
struct TupleSelectSeq;

template< typename TupleT, size_t... Is >
struct TupleSelectSeq< TupleT, seq< Is... >>
{ using type = TupleSelect< TupleT, Is... >::type; };

template< typename TupleT, size_t... Is >
using tuple_select_t = typename TupleSelect< TupleT, Is... >::type;

template< typename TupleT, size_t... Is >
constexpr TupleSelect< TupleT, Is... >::type 
tuple_select( TupleT const& tup, index_sequence< Is... > )
{ return std::make_tuple( std::get< Is >( tup )... ); }

template< typename TupleT, size_t N, typename IndexSequence >
struct RemoveNthHelper;

template< typename TupleT, size_t N, size_t... Is >
struct RemoveNthHelper< TupleT, N, index_sequence< Is... >>
{ using type = std::tuple< std::tuple_element_t< ( Is < N ? Is : Is + 1 ), TupleT >... >; };

template< typename TupleT, size_t N >
struct RemoveNth
{ 
    static constexpr const size_t tuple_size = std::tuple_size_v< TupleT >;
    using type = RemoveNthHelper< TupleT, N, std::make_index_sequence< tuple_size >>::type; 
};

template< size_t N, typename TupleT >
using remove_nth_t = typename RemoveNth< TupleT, N >::type;

template< size_t N, typename TupleT, size_t... Is >
constexpr remove_nth_t< N, TupleT >
remove_nth_helper( TupleT const& tup, index_sequence< Is... > )
{ return tuple_select( tup, std::index_sequence< ( Is < N ? Is : Is + 1 )... >{} ); }

template< typename RetT, typename TupleT, size_t... Is >
RetT tuple_rest_helper( TupleT const& tup, seq< Is... > )
{ return { get< 1 + Is >( tup )... }; }

template< typename T, typename TupleT >
struct IsTupleElement;

template< typename T, typename... Ts >
struct IsTupleElement< T, tuple< Ts... >>:
    integral_constant< bool, ( is_same_v< T, Ts > or ... )> { };

template< typename T, typename TupleT, size_t Start = 0 >
struct TupleIndex;

template< typename T, size_t Start >
struct TupleIndex< T, tuple<>, Start >: integral_constant< size_t, Start > { };

template< typename T, typename First, typename... Rest, size_t Start >
requires( is_same_v< T, First > ) 
struct TupleIndex< T, tuple< First, Rest... >, Start >: 
    integral_constant< size_t, Start > { };

template< typename T, typename First, typename... Rest, size_t Start >
requires( not is_same_v< T, First > )
struct TupleIndex< T, tuple< First, Rest... >, Start >:
    TupleIndex< T, tuple< Rest... >, Start + 1 > { };

template< typename T, typename TupleT >
constexpr size_t tuple_index_v = TupleIndex< T, TupleT, 0 >::value;

template< typename TupleT, typename... Us >
struct IsTupleMadeFrom;

template< >
struct IsTupleMadeFrom< tuple<> >: integral_constant< bool, true > { };

template< typename TupleT, typename... Us >
requires(( not IsTupleElement< Us, TupleT >::value or ... ))
struct IsTupleMadeFrom< TupleT, Us... >: integral_constant< bool, false > { };

template< typename... Ts, typename First, typename... Rest >
requires( IsTupleElement< First, tuple< Ts... >>::value and
    ( IsTupleElement< Rest, tuple< Ts... >>::value and ... ))
struct IsTupleMadeFrom< tuple< Ts... >, First, Rest... >:
    IsTupleMadeFrom< remove_nth_t< tuple_index_v< First, tuple< Ts... >>, 
        tuple< Ts... >>, Rest... > 
{ };

template< typename TupleT, typename... Ts >
constexpr bool is_tuple_made_from_v = IsTupleMadeFrom< TupleT, Ts... >::value;

} // namespace detail

/// @brief returns the first element of a tuple
/// @tparam T the type of the first element
/// @tparam ...Ts types of the remaining elements
/// @param tup the tuple value
/// @return the value of the first element
template< typename T, typename... Ts >
constexpr T const& tuple_first( tuple< T, Ts... > const& tup )
{ return get< 0 >( tup ); }

/// @brief returns the non-first elements of a tuple as a new tuple
/// @tparam T the type of the first element of the tuple
/// @tparam ...Ts types of the remaining elements
/// @param tup the tuple values
/// @return a tuple containing the non-first elements
template< typename T, typename... Ts >
constexpr tuple< Ts... > tuple_rest( tuple< T, Ts... > const& tup )
{ return detail::tuple_rest_helper< tuple< Ts... >>( tup, make_seq< sizeof...( Ts ) >{} ); }

/// @brief removes the Nth element of a tuple
/// @tparam ...Ts the types in the tuple
/// @tparam N index to the element to be removed
/// @param tup the tuple value
/// @return a new tuple with the Nth element removed
template< size_t N, typename... Ts >
requires ( N < sizeof...( Ts ) )
constexpr auto remove_nth( std::tuple< Ts... > const& tup )
{ return detail::remove_nth_helper< N >( tup, 
    std::make_index_sequence< sizeof...( Ts ) - 1 >{} ); }

/// @brief removes the first element of a tuple (same as tuple_rest?)
/// @tparam First 
/// @tparam ...Rest 
/// @param tup 
/// @return 
template< typename First, typename... Rest >
constexpr std::tuple< Rest... > remove_first( std::tuple< First, Rest... > const& tup )
{ return remove_nth< 0 >( tup ); }

template< typename T, typename TupleT >
constexpr bool is_tuple_element = detail::IsTupleElement< T, TupleT >::value;

template< typename TupleT >
struct IsTuple: integral_constant< bool, false > {};

template< typename... Ts >
struct IsTuple< tuple< Ts... >>: integral_constant< bool, true > {};

template< typename TupleT >
constexpr bool is_tuple_v = IsTuple< TupleT >::value;

////////////////////////////
/// runtime access to tuples
///
namespace detail {
/// forward decl
template< typename TupleT >
struct TupleAccessor;

/// forward decl
template< typename TupleT, typename Seq >
struct TupleAccessorBase;

/// @brief helper base class for TupleAccessor. 
/// @tparam TupleT the tuple type to be accessed
/// @tparam ...Is are an index sequence into TupleT
template< typename TupleT, size_t... Is >
struct TupleAccessorBase< TupleT, seq< Is... >>
{
    using accessor_type = function< any( TupleT const& )>;
    
    template< size_t I >
    static any accessor( TupleT const& tup )
    { return { get< I >( tup )}; }

    static constexpr array< accessor_type, sizeof...( Is )> at = 
    { accessor< Is >... };
};

/// @brief helper class for tuple_access method
/// @tparam ...Ts are the types of the tuple
template< typename... Ts >
struct TupleAccessor< tuple< Ts... >>:
    TupleAccessorBase< tuple< Ts... >, make_seq< sizeof...( Ts )>>
{ };
} // namespace detail

/// @brief Provides runtime access to a tuple by instantiating a static 
/// std::array of accessor functions for the TupleT. This is essentially like an 
/// array of std::get< I > functions for this tuple where I is also the index 
/// into the array for that accessor. See detail::TupleAccessorBase for the 
/// implementation
/// @tparam TupleT type of the tuple
/// @param i index of requested tuple element
/// @param tup tuple to access
/// @return an std::any containing the value of the ith element of tup
template< typename TupleT >
any tuple_access( size_t i, TupleT const& tup )
{ return detail::TupleAccessor< TupleT >::at[i](tup); }

//////////////////////////////////////////////////////////
/// tuple cat ( may not need this as std provides one... )
///
namespace detail {
/// forward decl
template< typename... TupleTypes >
struct TupleCat;

/// @brief concatenation of no tuples is an empty tuple
template<>
struct TupleCat<> 
{ using type = tuple<>; };

/// @brief concatenation of a single tuple is the tuple
/// @tparam ...Ts are the types of the tuple
template< typename... Ts >
struct TupleCat< tuple< Ts... >>
{ using type = tuple< Ts... >; };

/// @brief concatenation of two or more tuples is a tuple of the first two types
/// concatenated with the remaining types
template< typename... Ts, typename... Us, typename... Rest >
struct TupleCat< tuple< Ts... >, tuple< Us... >, Rest... >
{ using type = TupleCat< tuple< Ts..., Us... >, Rest... >::type; };
} // namespace detail

/// @brief concatenate a list of tuples together
/// @tparam ...Tuples to couple
template< typename... Tuples >
using tuple_cat_t = detail::TupleCat< Tuples... >::type;

namespace detail {
template< size_t I, typename T, typename TupleT >
struct TupleInsert;

template< typename T, typename... Ts >
struct TupleInsert< 0, T, tuple< Ts... >>
{ using type = tuple< T, Ts... >; };

template< size_t I, typename T, typename First, typename... Rest >
requires( isgreater( I, 0 ))
struct TupleInsert< I, T, tuple< First, Rest... >>
{ using type = tuple_cat_t< tuple< First >, 
    typename TupleInsert< I-1, T, tuple< Rest... >>::type >; };
} // namespace detail

/// @brief insert a type into a tuple at an index
/// @tparam T type to insert
/// @tparam TupleT tuple type to insert into
/// @tparam I position of T in the resultant tuple type
template< size_t I, typename T, typename TupleT >
using tuple_insert_t = detail::TupleInsert< I, T, TupleT >::type;

namespace detail {
template< typename TupleT >
struct TupleUnique;

template<>
struct TupleUnique< tuple<> >
{ using type = tuple<>; };

template< typename T >
struct TupleUnique< tuple< T >>
{ using type = tuple< T >; };

template< typename T, typename... Ts >
requires ( ... and not is_same_v< T, Ts > )
struct TupleUnique< tuple< T, Ts... >>
{ using type = tuple_cat_t< tuple< T >, 
    typename TupleUnique< tuple< Ts... >>::type >; };

template< typename T, typename... Ts >
requires ( ... or is_same_v< T, Ts > )
struct TupleUnique< tuple< T, Ts... >>
{ using type = TupleUnique< tuple< Ts... >>::type; };
} // namespace detail

/// @brief remove duplicate types from a tuple type
/// @tparam TupleT 
template< typename TupleT >
using tuple_unique_t = detail::TupleUnique< TupleT >::type;

namespace detail {

template< typename T, typename TupleT, size_t Index = 0 >
struct TupleIndexHelper;

template< typename T, size_t Index >
struct TupleIndexHelper< T, tuple< >, Index >
{ static constexpr size_t value = Index; };

template< typename T, typename U, typename... Us, size_t Index >
requires( is_same_v< T, U > )
struct TupleIndexHelper< T, tuple< U, Us... >, Index >
{ static constexpr size_t value = Index; };

template< typename T, typename U, typename... Us, size_t Index >
requires( not is_same_v< T, U > )
struct TupleIndexHelper< T, tuple< U, Us... >, Index >
{ static constexpr size_t value = 
    TupleIndexHelper< T, tuple< U, Us... >, 1 + Index >::value; };

} // namespace detail 

template< typename T, typename TupleT >
constexpr size_t tuple_index_v = detail::TupleIndexHelper< T, TupleT >::value;

/**
 * Pack helpers
 */

/**
 * noop_t always evaluates to it's template parameter no matter what size_t is
 * passed as the second parameter.  This is useful for constructing uniform
 * pack types from sequences
 */
namespace detail {
template< typename T, size_t Ignored >
struct NoopType { using type = T; };

} // namespace detail (noop_t)

template< typename T, size_t Ignored >
using noop_t = detail::NoopType< T, Ignored >::type;


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

template< template < typename... > class Pack, typename TupleT >
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





#endif
