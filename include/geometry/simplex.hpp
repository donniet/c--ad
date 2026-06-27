#ifndef __GEOMETRY_SIMPLEX_HPP__
#define __GEOMETRY_SIMPLEX_HPP__

#include "utility.hpp"
#include "tensors.hpp"
#include "expressions/expressions.hpp"

#include <vector>
#include <ranges>
#include <algorithm>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <memory>

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

    Simplex project_at( U here ) const
    {
        Simplex ret;
        for( Vertex v: *this )
            ret.push_back( v.extend( here ));
        return ret;
    }

    constexpr Simplex(): container_type{} { }
    constexpr Simplex( Simplex const& other ): container_type{ other } { }
    constexpr Simplex( std::initializer_list< vertex_type >&& vertex_list ):
        container_type{ vertex_list } { }
};

/// @brief base class for any vertex manipulation method
/// @tparam U 
template< typename U >
struct VertexTransformer
{
    using vertex_type = Vertex< U >;

    size_t from_dimensions() const { return _from; }
    size_t to_dimensions() const { return _to; }

    virtual vertex_type transform( vertex_type const& vert ) const = 0;

    VertexTransformer( size_t from, size_t to ): _from{ from }, _to{ to } { }

    virtual ~VertexTransformer() { }

protected:
    void set_from_dimensions( size_t from ) { _from = from; }
    void set_to_dimensions( size_t to ) { _to = to; }

private:
    size_t _from, _to;
};

template< size_t N, typename U >
struct VertexTranslator: virtual VertexTransformer< U >
{
    using vector_type = uniform_vector_t< N, U >;

    VertexTranslator( size_t from = N ): VertexTransformer< U >{ from, N }, 
        _offset{} { }
    VertexTranslator( size_t from, vector_type const& vect ): 
        VertexTransformer< U >{ from, N }, _offset{ vect } { }

    virtual ~VertexTranslator() { }

    vector_type _offset;
};

template< size_t Rows, size_t Cols, typename U >
struct VertexMatrixMultiply: virtual VertexTransformer< U >
{
    using matrix_type = uniform_matrix_t< Rows, Cols, U >;

    VertexMatrixMultiply(): VertexTransformer< U >{ Cols, Rows } { }
    VertexMatrixMultiply( matrix_type const& mat ):
        VertexTransformer< U >{ Cols, Rows }, _matrix{ mat } { }

    virtual ~VertexMatrixMultiply() { }

    uniform_matrix_t< Rows, Cols, U > _matrix;
};

template< typename U >
struct SimplexSpecification
{
    using simplex_type = Simplex< U >;

    U maximum_extrusion( simplex_type const& simp, U from, U to )
    {

    }
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

    template< typename... Specs >
    void push_vertex( size_t dim = 0, Specs... specs )
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

    template< size_t From, size_t To, typename ScalarT, typename... Specs >
    void linear_transform( uniform_matrix_t< From, To, ScalarT > const& mat, Specs... specs )
    {
        complex_type shape = shape_stack.pop();

        for( simplex_type& simp: shape )
            for( vertex_type& vert: simp )
                vert = matmul( mat, vert );

        shape_stack.push( shape );
    }

    template< typename VectorT, typename... Specs >
    void translate( VectorT const& offset, Specs... specs )
    { 
        complex_type shape = shape_stack.pop();

        for( simplex_type& simp: shape )
            for( vertex_type& vert: simp )
                vert += offset;

        shape_stack.push( shape );
    }

    template< typename... Specs >
    void translate( null_tensor_t const&, Specs... )
    { }

    template< typename... Specs >
    void extrude( U from, U to, Specs... specs )
    {  
        complex_type shape = shape_stack.pop();
        size_t count = 0;
        for( ; from < to ; ++count )
        {
            U step = maximum_extrusion_step( shape, from, to, specs... );
            shape_stack.push( extrude_complex( shape, from, step ));
            from = step;
        }
        combine( count );
    }

    template< typename SpecT, size_t Rows, size_t Cols >
    auto adjust_spec_linear( SpecT spec, uniform_matrix_t< Rows, Cols, units::Scalar > const& mat )
    {
        
    }

    template< typename SpecT, size_t N >
    auto adjust_spec_translate( SpecT spec, uniform_vector_t< N, U > const& mat )
    {

    }

private:
    template< typename... Specs >
    U maximum_extrusion_step( complex_type const& plex, U from, U to, 
        Specs... specs )
    {
        U step = to;
        for( simplex_type const& simp: plex )
            step = maximum_extrusion_step( simp, from, step, specs... );
        return step;
    }

    template< typename... Specs >
    U maximum_extrusion_step( simplex_type const& simp, U from, U to, 
        Specs... specs )
    { return from; }
    //    return min_of( specs.maximum_extrusion( simp, from, to )... ); }

    static complex_type extrude_complex( complex_type const& c, U from, U to )
    { 
        complex_type ret;
        for( simplex_type const& s: c )
            ret.push_back( extrude_simplex( s, from, to ));
        return ret;
    }

    static complex_type extrude_simplex( simplex_type const& s, U from, U to )
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

        auto s2 = s[2];
        auto s2_0 = s2.extend( from );
        auto s2_1 = s2.extend( to );

        // are we extruding a plane
        // TODO:
        //  s0_0, s1_0, s2_0
        //  
        // 
        //  s0_1, s1_1, s2_1
        // if( s.size() == 3 )
        //     return {{ s0_0, s0_1, s1_1, s2_1 },
        //             { s0_0, s1_1, }}
        
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