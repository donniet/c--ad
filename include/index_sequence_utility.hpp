#ifndef __INDEX_SEQUENCE_UTILITY_HPP__
#define __INDEX_SEQUENCE_UTILITY_HPP__

#include <utility>
#include <cstddef>

template< typename IndexSequence >
struct index_sequence_op;

template< typename IndexSequence1, typename IndexSequence2 >
struct index_sequence_concatenator;

template< size_t... Is >
struct index_sequence_concatenator< std::index_sequence<>, std::index_sequence< Is... >>
{ using type = std::index_sequence< Is... >; };

template< size_t... Is >
struct index_sequence_concatenator< std::index_sequence< Is... >, std::index_sequence<> >
{ using type = std::index_sequence< Is... >; };

template< size_t... Is, typename IndexSequence2 >
struct index_sequence_concatenator< std::index_sequence< Is... >, IndexSequence2 >
{ 
private:
    using second_sequence_op = index_sequence_op< IndexSequence2 >;
public:
    using type = index_sequence_concatenator< 
        std::index_sequence< Is..., second_sequence_op::first >,
        typename second_sequence_op::rest >; 
};

template< typename IndexSequence, size_t Begin, size_t End >
struct subsequencer;


template< size_t First, size_t... Is >
struct index_sequence_op< std::index_sequence< First, Is... > >
{
    template< size_t I >
    using append = std::index_sequence< First, Is..., I >;

    template< size_t I >
    using prepend = std::index_sequence< I, First, Is... >;

    template< typename IndexSequence >
    using concatenate = index_sequence_concatenator< 
        std::index_sequence< First, Is... >, IndexSequence >::type;

    template< size_t Begin, size_t End = sizeof...( Is ) >
    using subsequence = subsequencer< std::index_sequence< First, Is... >, Begin, End >::type;

    template< size_t N >
    using remove_nth = index_sequence_concatenator< 
        subsequence< 0, N >, subsequence< N + 1 >>::type;

    static constexpr size_t first = First;
    using rest = std::index_sequence< Is... >;
};

template< >
struct index_sequence_op< std::index_sequence< > >
{
    template< size_t I >
    using append = std::index_sequence< I >;

    template< size_t I >
    using prepend = std::index_sequence< I >;

    template< typename IndexSequence >
    using concatenate = IndexSequence;
};


template< size_t From, size_t To, size_t... Is>
struct make_index_sequence_range_helper
{ using type = make_index_sequence_range_helper< From+1, To, Is..., From >::type; };

template< size_t To, size_t... Is >
struct make_index_sequence_range_helper< To, To, Is... >
{ using type = std::index_sequence< Is... >; };

template< size_t From, size_t To >
struct make_index_sequence_range
{ using type = make_index_sequence_range_helper< From, To >::type; };

template< size_t From, size_t To >
using make_index_sequence_range_t = make_index_sequence_range< From, To >::type;

namespace pack 
{

template< typename PackType >
struct pack_size;

template< template< typename... > class Pack, typename... Ts >
struct pack_size< Pack< Ts... >>
{ static constexpr size_t size = sizeof...( Ts ); };


template< size_t N, typename PackType >
struct element;

template< size_t N, template< typename... > class Pack, typename... Ts >
struct element< N, Pack< Ts... >>
{ using type = std::tuple_element_t< N, std::tuple< Ts... > >; };

template< size_t N, typename PackType >
using element_t = element< N, PackType >::type;

template< typename PackType, size_t... Is >
struct select;

template< template< typename... > class Pack, typename... Ts, size_t... Is >
struct select< Pack< Ts... >, Is... >
{ using type = Pack< element< Is, Pack< Ts... > >... >; };

template< typename PackType, typename IndexSequence >
struct select_seq;

template< typename PackType, size_t... Is >
struct select_seq< PackType, std::index_sequence< Is... > >
{ using type = select< PackType, Is... >::type; };

template< typename PackType, typename IndexSequence >
using select_seq_t = select_seq< PackType, IndexSequence >::type;

template< size_t N, typename PackType>
struct right
{ 
    static constexpr size_t siz = pack_size< PackType >::size;

    using type = select_seq_t< PackType, make_index_sequence_range_t< N, siz >>; 
};

template< size_t N, typename PackType>
using right_t = right< N, PackType >::type;

template< size_t N, typename PackType>
struct left 
{ using type = select_seq_t< PackType, make_index_sequence_range_t< 0, N >>; };

template< size_t N, typename PackType>
using left_t = left< N, PackType >::type;


template< typename Pack, typename... Us >
struct append;

template< template< typename... > class Pack, typename... Ts, typename... Us >
struct append< Pack< Ts... >, Us... >
{ using type = Pack< Ts..., Us... >; };

template< typename Pack1, typename Pack2 >
struct concat;

template< typename Pack1, template< typename... > class Pack2, typename... Ts >
struct concat< Pack1, Pack2< Ts... > >
{ using type = append< Pack1, Ts... >::type; };

template< typename Pack1, typename Pack2 >
using concat_t = concat< Pack1, Pack2 >::type;

template< size_t N, typename PackType >
struct remove_nth
{ using type = concat_t< left_t< N, PackType >, right_t< N+1, PackType > >; };


} // namespace pack


#endif
