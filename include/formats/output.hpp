#ifndef __GEOMETRY_FORMATS_OUTPUT_HPP__
#define __GEOMETRY_FORMATS_OUTPUT_HPP__

#include "geometry/space.hpp"
#include "geometry/collection.hpp"

#include <vector>

namespace geometry { 

template< typename VertexT >
struct VertexChain: std::vector< VertexT >
{
    using vertex_type = VertexT;
    using base_type = std::vector< VertexT >;

    template< typename... Vertices >
    VertexChain( size_t dimensions, bool closed, Vertices... vertices ):
        dimensions{ dimensions }, closed{ closed }, base_type{ vertices... } { }

    size_t dimensions;
    bool closed;
};

template< typename First, typename... Rest >
requires(( is_same_v< First, Rest > and ... ))
VertexChain< First > make_vertex_chain_closed( size_t dimensions, First first, 
    Rest... rest )
{ return { dimensions, true, first, rest... }; }

template< typename First, typename... Rest >
requires(( is_same_v< First, Rest > and ... ))
VertexChain< First > make_vertex_chain( size_t dimensions, First first, 
    Rest... rest )
{ return { dimensions, false, first, rest... }; }



namespace formats {


// template< typename Out, typename T >
// constexpr auto output( Out& file, T const& obj );

template< typename OutT, typename CollectionT, size_t... Is >
constexpr void output_collection_helper( OutT& out, CollectionT const& col, 
    seq< Is... > )
{ ( output( out, get< Is >( col )), ... ); }

template< typename OutT, typename... Objects >
requires( isgreater( sizeof...( Objects ), 0 ))
constexpr void output( OutT& out, Collection< Objects... > const& col )
{ output_collection_helper( out, col, make_seq< sizeof...( Objects )>{} ); }

} // namespace formats
} // namespace geometry

#endif