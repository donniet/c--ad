#ifndef __GEOMETRY_SIMPLEX_HPP__
#define __GEOMETRY_SIMPLEX_HPP__

#include "tensors.hpp"
#include "utility.hpp"

#include <vector>
#include <ranges>
#include <algorithm>

namespace geometry {
namespace simplex {

template< typename U >
struct Vertex: std::vector< U >
{
    constexpr bool operator ==( Vertex const& vec ) const;
    constexpr bool operator !=( Vertex const& vec ) const;
    constexpr Vertex& operator+=( Vertex const& );
    constexpr Vertex& operator-=( Vertex const& );
    template< typename VectorT >
    constexpr Vertex& operator+=( VectorT const& vec )
    {
        auto i = begin();
        auto j = std::ranges::begin( vec );

        for( ; i != end() and j != std::ranges::end( vec ); ++j, ++i )
            *i += *j;

        for( ; j != std::ranges::end( vec ); ++j )
            extend( *j );

        return *this;
    }
    constexpr Vertex& operator*=( long double );
    constexpr Vertex& operator/=( long double );
    constexpr U norm2() const;
    constexpr U norm() const;

    using std::vector< U >::begin;
    using std::vector< U >::end;

    constexpr void extend( U&& value )
    { std::vector< U >::push_back( std::move( value )); }

    constexpr void extend( U const& value )
    { std::vector< U >::push_back( std::move( value )); }

    using std::vector< U >::size;

    // vertices are projected out to 1 in any dimension beyond their definition
    constexpr U operator []( size_t I ) const
    { return ( I < size() ? std::vector< U >::at( I ) : static_cast< U >( 1 )); }

    Vertex( size_t dim ): std::vector< U >{ dim, static_cast< U >( 0 )} { }
};

/// @brief a mesh with a different name and arbirary dimensions, but a consistent
/// unit for the coordinates
/// @tparam U type of each individual coordinate
template< typename U >
struct Simplex: std::vector< Vertex< U >>
{
    using vertex_type = Vertex< U >;

    bool operator ()( vertex_type const& point ) const;
};

template< typename U >
struct Complex: std::vector< Simplex< U >>
{
    using vertex_type = Vertex< U >;
    using simplex_type = Simplex< U >;
    using container_type = std::vector< simplex_type >;
    
    template< std::ranges::input_range R >
    constexpr void append_range( R&& rng )
    { container_type::insert( container_type::end(), std::ranges::begin( rng ), 
        std::ranges::end( rng )); }

    bool operator ()( vertex_type const& point ) const;

};

template< typename U >
struct Builder
{
    using complex_type = Complex< U >;
    using simplex_type = Simplex< U >;
    using vertex_type = Vertex< U >;

    using value_type = complex_type;
    using iterator = stack_of< complex_type >::iterator;
    using const_iterator = stack_of< complex_type >::const_iterator;

    size_t size() { return shape_stack.size(); }

    iterator begin() { return shape_stack.begin(); }
    iterator end() { return shape_stack.end(); }

    const_iterator begin() const { return shape_stack.begin(); }
    const_iterator end() const { return shape_stack.end(); }

    void push_vertex( size_t dim = 0 )
    { shape_stack.push( {{ simplex_type{{ vertex_type{ dim } }}}} ); }

    void combine( size_t count )
    {
        if( count > shape_stack.size() )
            throw std::logic_error( "cannot combine more than we have on the stack" );

        complex_type combined;
        for( ; count > 0; --count )
            combined.append_range( shape_stack.pop() );
        shape_stack.push( combined );
    }

    template< size_t From, size_t To, typename ScalarT >
    void linear_transform( uniform_matrix_t< From, To, ScalarT > const& mat )
    {
        complex_type shape = shape_stack.pop();

        for( simplex_type& simp: shape )
            for( vertex_type& vert: simp )
                vert = matmul( mat, vert );

        shape_stack.push( shape );
    }

    template< typename VectorT >
    void translate( VectorT const& offset )
    { 
        complex_type shape = shape_stack.pop();

        for( simplex_type& simp: shape )
            for( vertex_type& vert: simp )
                vert += offset;

        shape_stack.push( shape );
    }

    void translate( null_tensor_t const& offset )
    { }

    void extrude( U from, U to, size_t steps = 1 )
    {  
        complex_type shape = shape_stack.pop();
        complex_type shape_ex;

        auto from_step = [from, to, steps]( size_t step ) -> U
        { 
            long double s = (long double)step/(long double)steps;
            return from * ( 1.0l - s ) + to * s;
        };
        auto to_step = [from, to, steps]( size_t step ) -> U
        { 
            long double s = (long double)( step + 1 )/(long double)steps;
            return from * ( 1.0l - s ) + to * s;
        };
        
        for( simplex_type& simp: shape )
            for( size_t step = 0; step < steps; ++step )
                shape_ex.append_range( 
                    extrude_simplex( simp, from_step( step ), to_step( step )));

        shape_stack.push( shape_ex );
    }

private:
    complex_type extrude_simplex( simplex_type const& s, U from, U to )
    {
        if( s.size() == 0 )
            throw std::logic_error( "cannot extrude an empty simplex" );
        
        auto s0_0 = s[0].extend( from );
        auto s0_1 = s[0].extend( to );

        // are we extruding a single point?
        if( s.size() == 1 )
            return {{ s0_0, s0_1 }};

        auto s1_0 = s[1].extend( from );
        auto s1_1 = s[1].extend( to );

        // are we extruding a line segment
        if( s.size() == 2 ) 
            return {{ s0_0, s0_1, s1_1 },
                    { s0_0, s1_1, s1_0 }};
        
        throw std::logic_error( "extrusions above a simplex of degree 2 are not"
            " supported at this time." );
    }


    stack_of< complex_type > shape_stack;
};

} // namespace geometry 
} // namespace simplex 

#endif