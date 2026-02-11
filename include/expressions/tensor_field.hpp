#ifndef __TENSOR_FIELD_HPP__
#define __TENSOR_FIELD_HPP__

#include "expression_ops.hpp"

#if !NO_TENSOR_PRINTING
#include <format>
#include <sstream>
#include <string_view>
#endif

namespace expressions {

template< size_t... Is >
struct Shape
{
    static constexpr size_t size = sizeof...( Is );
    static constexpr size_t elements_size = ( ... * Is );
};

template< >
struct Shape<>
{
    static constexpr size_t size = 0;
    static constexpr size_t elements_size = 1; // should this be 0 too?
};

template< typename T >
struct is_shape 
{ static constexpr bool value = false; };

template< size_t... Is >
struct is_shape< Shape< Is... >>
{ static constexpr bool value = true; };

template< typename T >
constexpr bool is_shape_v = is_shape< T >::value;

template< typename T >
concept shape = is_shape_v< T >;

template< typename T >
struct is_tensor_like
{ static constexpr bool value = false; };

template< typename... Ts >
struct is_tensor_like< tuple< Ts... >>
{ static constexpr bool value = true; };

template< typename T >
constexpr bool is_tensor_like_v = is_tensor_like< T >::value;

template< typename T >
concept tensor_like = is_tensor_like_v< T >;

template< typename T >
concept scalar_like = not tensor_like< T >;

template< size_t I, typename T >
using tensor_element_t = dependent_t< I, T >;

template< size_t I, shape D >
struct shape_at;

template< size_t I, size_t J, size_t... Js >
requires( isless( I, 1 + sizeof...( Js ) ))
struct shape_at< I, Shape< J, Js... >>
{ static constexpr size_t value = shape_at< I-1, Shape< Js... >>::value; };

template< size_t J, size_t... Js >
struct shape_at< 0, Shape< J, Js... >>
{ static constexpr size_t value = J; };

template< size_t I, shape D >
constexpr size_t shape_at_v = shape_at< I, D >::value;

template< shape A, shape B >
struct shape_cat;

template< size_t... Is, size_t... Js >
struct shape_cat< Shape< Is... >, Shape< Js... >>
{ using type = Shape< Is..., Js... >; };

template< shape A, shape B >
using shape_cat_t = shape_cat< A, B >::type;

/**
 * pack struct to hold the indices referencing an element of a tensor
 */
template< size_t... Is >
struct Index 
{ };

template< typename T >
struct is_index
{ static constexpr bool value = false; };

template< size_t... Is >
struct is_index< Index< Is... >>
{ static constexpr bool value = true; };

template< typename T >
constexpr bool is_index_v = is_index< T >::value;

template< typename T >
concept index = is_index_v< T >;

template< size_t I, index D >
struct index_at;

template< size_t I, size_t J, size_t... Js >
requires( isless( I, 1 + sizeof...( Js ) ))
struct index_at< I, Index< J, Js... >>
{ static constexpr size_t value = index_at< I-1, Index< Js... >>::value; };

template< size_t J, size_t... Js >
struct index_at< 0, Index< J, Js... >>
{ static constexpr size_t value = J; };

template< size_t I, index D >
constexpr size_t index_at_v = index_at< I, D >::value;

template< index A, index B >
struct index_cat;

template< size_t... Is, size_t... Js >
struct index_cat< Index< Is... >, Index< Js... >>
{ using type = Index< Is..., Js... >; };

template< index A, index B >
using index_cat_t = index_cat< A, B >::type;

template< index Index, shape Shape >
struct is_valid_index
{ static constexpr bool value = false; };

template< index Index, shape Shape, typename Seq >
struct is_valid_index_helper;

template< index Index, shape Shape, size_t... Is >
struct is_valid_index_helper< Index, Shape, seq< Is... >>
{ static constexpr bool value = 
    ( ... and isless( index_at_v< Is, Index >, shape_at_v< Is, Shape > )); };

template< size_t... Is, size_t... Js >
requires( sizeof...(Is) == sizeof...(Js) )
struct is_valid_index< Index< Is... >, Shape< Js... >>
{ static constexpr bool value = 
    is_valid_index_helper< Index< Is... >, Shape< Js... >, 
        make_seq< sizeof...(Is)>>::value; };

template< index I, shape S >
constexpr bool is_valid_index_v = is_valid_index< I, S >::value;

template< index D, shape S >
struct index_to_element_helper;

template< size_t D, size_t S >
struct index_to_element_helper< Index< D >, Shape< S > >
{ static constexpr size_t value = D; };

template< size_t D, size_t... Ds, size_t S, size_t... Ss >
requires( isgreater( sizeof...( Ds ), 0 ) and isgreater( sizeof...( Ss ), 0 ))
struct index_to_element_helper< Index< D, Ds... >, Shape< S, Ss... >>
{ static constexpr size_t value = D * ( ... * Ss ) + 
    index_to_element_helper< Index< Ds... >, Shape< Ss... >>::value; };


template< size_t I, shape S >
struct element_to_index;

template< size_t I, size_t S, size_t... Ss >
struct element_to_index< I, Shape< S, Ss... >>
{ using type = index_cat_t< Index< I / ( ... * Ss )>, 
    typename element_to_index< I % ( ... * Ss ), Shape< Ss... >>::type >; };

template< size_t I, size_t S >
struct element_to_index< I, Shape< S >>
{ using type = Index< I >; };

// empty shape corresponds to a tensor contracted to a single value
template<>
struct element_to_index< 0, Shape< >>
{ using type = Index< >; };

/**
 * Converts an element offset into the Tensor tuple back into a set of 
 * subscripts (opposite of element_from)
 * 
 * @tparam Shape of the tensor
 * @tparam Element index into the values tuple of the tensor
 * @returns an index<...> of subscripts corresponding to the same element of 
 *     the Tensor
 */
template< size_t Element, shape S >
using element_to_index_t = element_to_index< Element, S >::type;

template< typename X, size_t I, size_t N >
struct insert_at;

template< size_t... Js, size_t N >
struct insert_at< Shape< Js... >, 0, N >
{ using type = Shape< N, Js... >; };

template< size_t J, size_t... Js, size_t I, size_t N >
requires ( I > 0 and I <= 1+sizeof...( Js ))
struct insert_at< Shape< J, Js... >, I, N >
{ using type = shape_cat_t< Shape< J >, typename insert_at< Shape< Js... >, I-1, N >::type >; };

template< size_t... Js, size_t N >
struct insert_at< Index< Js... >, 0, N >
{ using type = Index< N, Js... >; };

template< size_t J, size_t... Js, size_t I, size_t N >
requires ( I > 0 and I <= 1+sizeof...( Js ))
struct insert_at< Index< J, Js... >, I, N >
{ using type = index_cat_t< Index< J >, typename insert_at< Index< Js... >, I-1, N >::type >; };

/**
 * Insert an element in a shape or index object
 * 
 * @tparam X the shape or index object to have a new element inserted
 * @tparam I the position in X to insert a new element
 * @tparam N the value to insert into X
 */
template< typename X, size_t I, size_t N >
using insert_at_t = insert_at< X, I, N >::type;


template< index D, typename T >
struct tensor_index;

template< index D, typename T >
using tensor_index_t = tensor_index< D, T >::type;

template< index D, typename T >
struct index_to_element;

template< index D, typename T >
constexpr size_t index_to_element_v = index_to_element< D, T >::value;

/**
 * collection of expressions in a multi-dimensional shape specified by S
 * 
 * @tparam S is the shape< Is... > of the tensor
 * @tparam Es are the expression types of the elements of the tensor
 */
template< shape S, typename... Es >
requires( S::elements_size == sizeof...( Es ))
struct Tensor : DependsOn< Es... >
{ 
    using shape_type = S;
    using result_type = Tensor< S, result_t< Es >... >; 
    using elements_tuple_type = tuple< Es... >;

    template< size_t I >
    constexpr tuple_element_t< I, elements_tuple_type > elem()
    { return get< I >( DependsOn< Es... >::exprs ); }

    template< size_t... Is >
    constexpr tensor_index_t< Index< Is... >, Tensor > subscript()
    { return get< index_to_element_v< Index< Is... >, Tensor >>( 
        DependsOn< Es... >::exprs ); }

    constexpr result_type operator()( result_t< Es >... xs )
    { return { xs... }; }

    Tensor() = default;
    Tensor( Tensor const& ) = default;
    Tensor( Es... es ) : DependsOn< Es... >{ es... } { }
};

template< shape S, typename... Es >
struct tensor_helper;

template< typename E >
struct tensor_helper< Shape<>, E >
{ using type = E; };

template< size_t S, size_t... Ss, typename... Es >
struct tensor_helper< Shape< S, Ss... >, Es... >
{ using type = Tensor< Shape< S, Ss... >, Es... >; };

template< shape S, typename... Es >
requires( S::elements_size == sizeof...( Es ))
using tensor_t = tensor_helper< S, Es... >::type; 

template< shape S, typename... Ts >
struct is_tensor_like< Tensor< S, Ts... >>
{ static constexpr bool value = true; };

template< typename T >
struct shape_of
{ using type = T::shape_type; };

template< typename... Ts >
struct shape_of< tuple< Ts... >>
{ using type = Shape< sizeof...( Ts )>; };

template< typename T >
using shape_of_t = shape_of< T >::type;

template< index D, typename T >
struct index_to_element
{ static constexpr size_t value = 
    index_to_element_helper< D, shape_of_t< T >>::value; };

template< index D, shape S, typename... Ts >
requires( is_valid_index_v< D, S > )
struct tensor_index< D, Tensor< S, Ts... >> 
{ using type = tensor_element_t< index_to_element_helper< D, S >::value, Tensor< S, Ts... >>; };

template< index D, size_t Beg, typename Seq >
struct subindex_helper;

template< index D, size_t Beg, size_t... Is >
struct subindex_helper< D, Beg, seq< Is... >>
{ using type = Index< index_at_v< Beg + Is, D >... >; };


} // namespace expressions

#if !NO_TENSOR_PRINTING
namespace std {

// TODO: this isn't working
template< expressions::shape S, typename... Ts, class CharT >
struct formatter< expressions::Tensor< S, Ts... >, CharT >
{
    using type = expressions::Tensor< S, Ts... >;

    template< size_t... Is >
    void format_helper( basic_ostringstream< CharT >& os, type x, seq< Is... > )
    { (( os << x.template elem< Is >() << ", "), ... ); }

    // TODO: implement parsing of context
    template< typename ParseContext >
    constexpr ParseContext::iterator parse( ParseContext& ctx )
    { 
        auto it = ctx.begin();
        while( it != ctx.end() and *it != '}' )
            ++it;
        return it;
    }

    template< typename FormatContext >
    FormatContext::iterator format( type x, FormatContext& ctx )
    {
        basic_ostringstream< CharT > os;
        format_helper( os, x, make_seq< S::elements_size >{} );
        return ranges::copy(std::move(os).str(), ctx.out()).out;
    }
};

} // namespace std
#endif

#endif