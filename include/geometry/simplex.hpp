#ifndef __GEOMETRY_SIMPLEX_HPP__
#define __GEOMETRY_SIMPLEX_HPP__

#include "tensors.hpp"
#include "utility.hpp"

#include <vector>
#include <ranges>
#include <algorithm>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <algorithm>

namespace geometry {
namespace simplex {

template< typename U >
struct Vertex: std::vector< U >
{
    using value_type = U;
    using container_type = std::vector< U >;
    
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
            container_type::push_back( *j );

        return *this;
    }
    constexpr Vertex& operator*=( long double );
    constexpr Vertex& operator/=( long double );
    constexpr U norm2() const;
    constexpr U norm() const;

    using std::vector< U >::begin;
    using std::vector< U >::end;

    constexpr Vertex extend( U&& value ) const
    { 
        Vertex ret = *this;
        ret.push_back( value );
        return ret;
    }

    constexpr Vertex extend( U const& value ) const
    { 
        Vertex ret = *this;
        ret.push_back( value );
        return ret;
    }

    using std::vector< U >::size;

    // vertices are projected out to 1 in any dimension beyond their definition
    constexpr U operator []( size_t I ) const
    { return ( I < size() ? std::vector< U >::at( I ) : static_cast< U >( 1 )); }

    Vertex( size_t dim ): std::vector< U >{ dim, static_cast< U >( 0 )} { }
    Vertex(): container_type{} { }
    Vertex( std::initializer_list< value_type >&& coords ):
        container_type{ coords } { }
};

/// @brief a mesh with a different name and arbirary dimensions, but a consistent
/// unit for the coordinates
/// @tparam U type of each individual coordinate
template< typename U >
struct Simplex: std::vector< Vertex< U >>
{
    using vertex_type = Vertex< U >;
    using value_type = vertex_type;
    using container_type = std::vector< vertex_type >;

    bool operator ()( vertex_type const& point ) const;

    constexpr Simplex& operator =( Simplex const& other )
    {
        container_type::operator =( other );
        return *this;
    }

    constexpr Simplex(): container_type{} { }
    constexpr Simplex( Simplex const& other ): container_type{ other } { }
    constexpr Simplex( std::initializer_list< vertex_type >&& vertex_list ):
        container_type{ vertex_list } { }
};

template< typename U >
struct Complex: std::vector< Simplex< U >>
{
    using vertex_type = Vertex< U >;
    using simplex_type = Simplex< U >;
    using value_type = simplex_type;
    using container_type = std::vector< simplex_type >;
    
    template< std::ranges::input_range R >
    constexpr void append_range( R&& rng )
    { container_type::insert( container_type::end(), std::ranges::begin( rng ), 
        std::ranges::end( rng )); }

    bool operator ()( vertex_type const& point ) const;

    constexpr Complex& operator =( Complex const& other )
    {  
        container_type::operator =( other );
        return *this;
    }

    constexpr Complex( std::initializer_list< simplex_type >&& simplex_list ):
        container_type{ simplex_list } { }
    constexpr Complex( Complex const& other ):
        container_type{ other } { }
    constexpr Complex(): container_type{} { }

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

    value_type const& result() const
    { return shape_stack.top(); }

    void push_vertex( size_t dim = 0 )
    { 
        vertex_type v{ dim };
        simplex_type s{{ v }};
        complex_type c{{ s }};

        shape_stack.push( c ); 
    }

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

        auto s0 = s[0];
        auto s0_0 = s0.extend( from );
        auto s0_1 = s0.extend( to );

        // are we extruding a single point?
        if( s.size() == 1 )
            return {{ s0_0, s0_1 }};

        auto s1 = s[1];
        auto s1_0 = s1.extend( from );
        auto s1_1 = s1.extend( to );

        // are we extruding a line segment
        if( s.size() == 2 ) 
            return {{ s0_0, s0_1, s1_1 },
                    { s0_0, s1_1, s1_0 }};
        
        throw std::logic_error( "extrusions above a simplex of degree 2 are not"
            " supported at this time." );
    }

    stack_of< complex_type > shape_stack;
};

template< typename U >
std::ostream& operator <<( std::ostream& os, Vertex< U > const& vert )
{  
    os << "(";
    std::copy( vert.begin(), vert.end(), std::ostream_iterator< U >( os, " " ));
    return os << ")";
}

template< typename U >
std::ostream& operator <<( std::ostream& os, Simplex< U > const& facet )
{  
    os << "{";
    std::copy( facet.begin(), facet.end(), 
        std::ostream_iterator< Vertex< U >>( os, " " ));
    return os << "}";
}

template< typename U >
std::ostream& operator <<( std::ostream& os, Complex< U > const& complex )
{  
    os << "[\n";
    std::copy( complex.begin(), complex.end(), 
        std::ostream_iterator< Simplex< U >>( os, "\n" ));
    return os << "]";
}

} // namespace simplex 
} // namespace geometry 


#endif