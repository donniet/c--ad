#ifndef __STL_HPP__
#define __STL_HPP__

/**
 * Implementation of the STL file format.
 */

#include "units.hpp"
#include "tensors.hpp"
#include "geometry.hpp"
#include "formats/output.hpp"
#include "geometry/simplex.hpp"
#include "geometry/processors.hpp"

#include <iostream>
#include <vector>
#include <list>
#include <functional>
#include <algorithm>

namespace geometry{
namespace formats {

using namespace units;
using namespace tensors;
using namespace geometry::simplex;
using std::optional;

/// 
/// 

struct STLFile
{
    using vec2 = uniform_tensor_t< Shape< 2 >, Length >;
    using vec3 = uniform_tensor_t< Shape< 3 >, Length >;
    using vec4 = uniform_tensor_t< Shape< 4 >, Length >;
    using chain_type = VertexChain< vec3 >;

    using mat2 = uniform_tensor_t< Shape< 2, 2 >, Scalar >;
    using mat3 = uniform_tensor_t< Shape< 3, 3 >, Scalar >;
    using mat4 = uniform_tensor_t< Shape< 4, 4 >, Scalar >;
    using mat3x4 = uniform_tensor_t< Shape< 3, 4 >, Scalar >;

    using builder_type = simplex::Builder< Length >;

    struct Solid;
    struct Facet;

    using solid_iterator = std::vector< Solid >::iterator;
    using facet_iterator = std::vector< Facet >::iterator; 
    using chain_iterator = chain_type::iterator;

    // implemented using an outer loop of vertices
    struct Facet
    {
        using vertex_type = vec3;
        using value_type = vertex_type;
        using iterator_type = vertex_type*;
        using const_iterator_type = vertex_type const*;

        static Facet from_simplex( Simplex< Length > const& simp )
        {
            Facet ret;
            int i = 0;
            for( Vertex< Length > const& vert: simp )
                ret.v[ i++ ]= vec3{ vert[0], vert[1], vert[2] };

            return ret;
        }
        
        void reverse_vertex_order()
        { std::swap( v[0], v[2] ); }

        iterator_type begin()
        { return &v[0]; }
        iterator_type end()
        { return &v[0] + 3; }

        const_iterator_type begin() const
        { return &v[0]; }
        const_iterator_type end() const
        { return &v[0] + 3; }

        Facet( Length nx = 0_m, Length ny = 0_m, Length nz = 0_m ):
            normal{ nx, ny, nz } { }

        vertex_type normal;
        vertex_type v[3];
    };

    struct Solid: std::vector< Facet >
    {
        Facet& add_facet( Length nx = 0_m, Length ny = 0_m, Length nz = 0_m )
        { return emplace_back( nx, ny, nz ); }
        Facet& add_facet( Facet const& facet )
        { return emplace_back( facet ); }

        static Solid from_complex( Complex< Length > const& plex );

        Solid( string const& name ): name{ solid_name( name ) } { }
        Solid(): name{ } { }

        string name;
    };

    void collect( builder_type const& builder );
    string to_string() const;

    // TODO: format the string name for use as a solid name in STL
    static string solid_name( string const& name )
    { return name; }

    // TODO: format the string as a comment
    static string comment( string const& comment )
    { 
        if( comment == "" )
            return comment;
        return "; " + comment;
    }

    solid_iterator add_solid( string name = "solid" )
    { return solids.emplace( solids.end(), name ); }

    solid_iterator add_solid( Solid&& solid )
    { return solids.emplace( solids.end(), solid ); }

    // Length& set_minimum_length( Length const& min_length )
    // { return _minimum_length = min_length; }

    // Length const& minimum_length() const
    // { return _minimum_length; }

    std::vector< Solid > solids;
    // Length _minimum_length;
};

std::ostream& operator <<( std::ostream& os, STLFile::vec3 const& v )
{ return os << meters( v[0] ) << " " << meters( v[1] ) << " " 
            << meters( v[2] ); }

template< typename ObjT >
struct STL
{
    using object_type = ObjT;
    using builder_type = STLFile::builder_type;

    template< typename... Specs >
    void output( builder_type& builder, Point point, Specs... specs ) const
    { builder.push_vertex( specs... ); }

    template< typename ObjU, typename... Specs >
    void output( builder_type& builder, Extrusion< ObjU, Length, Length > ext, 
        Specs... specs ) const
    { 
        output( builder, ext.object(), specs... );

        builder.extrude( ext.from(), ext.to(), specs... ); 
    }

    template< size_t From, typename... Us >
    static constexpr uniform_vector_t< From + sizeof...( Us ), Length > 
    translation_vector_from( ProjectAt< From, Us... > proj )
    { return proj( uniform_vector_t< From, Length >::zero() ); }

    template< typename ObjU, size_t From, typename... Us, typename... Specs >
    requires(( is_same_v< Length, Us > and ... ))
    void output( builder_type& builder, 
        Projection< ObjU, ProjectAt< From, Us... >> trans, 
        Specs... specs ) const
    { 
        output( builder, trans.object(), 
            builder_type::adjust_spec_translate( specs, 
                trans.transform().offset() )... );
                
        builder.translate( translation_vector_from( trans.transform() ), 
            specs... ); 
    }

    /// output compounds and attributed objects
    template< typename ComponentsU, typename... Objs, typename... Specs >
    void output( builder_type& builder, 
        Compound< ComponentsU, Objs... > compound, Specs... specs ) const
    {
        auto output_compound_helper = [&]< size_t... Is >( seq< Is... > )
        {( output( builder, get_component< Is >( compound ), specs... ), 
            ... ); };

        output_compound_helper( make_seq< sizeof...( Objs )>{} );
        builder.combine( sizeof...( Objs ));
    }

    template< typename... Objs, typename... Specs >
    void output( builder_type& builder, Collection< Objs... > col, 
        Specs... specs ) const
    {
        auto output_collection_helper = [&]< size_t... Is >( seq< Is... > )
        {( output( builder, get< Is >( col ), specs... ), ... ); };

        output_collection_helper( make_seq< sizeof...( Objs )>{} );
        builder.combine( sizeof...( Objs ));
    }

    // TODO: let attributes manipulate specs
    template< typename ObjU, typename AttU, typename... Specs >
    void output( builder_type& builder, Attribution< ObjU, AttU > att, 
        Specs... specs ) const
    { output( builder, att.object(), specs... ); }

    template< typename ObjU, size_t Rows, size_t Cols, typename... Specs >
    void output( builder_type& builder,
        Projection< ObjU, Linear< uniform_matrix_t< Rows, Cols, Scalar >>> proj, 
        Specs... specs ) const
    {
        output( builder, proj.object(), builder_type::adjust_spec_linear( specs, 
            proj.transformation().matrix())... );
        builder.linear_transform( proj.transformation(), specs... );
    }

    std::ostream& write_to( std::ostream& os ) const;

    constexpr object_type const& object() const 
    { return _object; }

    constexpr STL& with_minimum_length( Length const& minimum_length )
    { 
        _minimum_length = minimum_length;
        return *this;
    }
    constexpr Length minimum_length() const
    { return _minimum_length.value(); }

    constexpr STL& with_chordal_tolerance( Length const& chordal_tolerance )
    {
        _chordal_tolerance = chordal_tolerance;
        return *this;
    }
    constexpr Length chordal_tolerance() const
    { return _chordal_tolerance; }

    constexpr STL& with_angular_tolerance( Scalar const& angular_tolerance )
    {
        _angular_tolerance = angular_tolerance;
        return *this;
    }
    constexpr Scalar angular_tolerance() const
    { return _angular_tolerance; }

    constexpr STL( object_type const& object, string name = "object" ): 
        _object{ object }, _name{ name }, _minimum_length{ },
        _chordal_tolerance{ 0.1_mm }, _angular_tolerance{ 1_deg } 
    { }

    object_type _object;
    string _name;
    optional< Length > _minimum_length;
    Length _chordal_tolerance;
    Scalar _angular_tolerance;
};


/// @brief output a string representing this STLFile. 
/// NOTE: this is for debug purposes.  Use the STL class to output an STL 
/// file format
/// @return a string containing the solids, facets and vertices of this STLFile
string STLFile::to_string() const
{
    string ret = "{\n";
    ret += "\t.solids = {\n";

    for( auto const& solid: solids )
    {
        ret += "\t\t.name = \"" + solid.name + "\";\n";
        // ret += "\t\t.comment = \"" + solid.comment + "\";\n";
        ret += "\t\t.facets = {\n";

        for( auto const& facet: solid )
        {
            ret += "\t\t\t.normal = {" 
                + std::to_string( facet.normal[0] ) + ", " 
                + std::to_string( facet.normal[1] ) + ", " 
                + std::to_string( facet.normal[2] ) + " };\n";
            // ret += "\t\t\t.comment = \"" + facet.comment + "\";\n";
            ret += "\t\t\t.vertices = {\n";

            for( auto const& vertex: facet )
            {
                ret += "\t\t\t\t{ " 
                    + std::to_string( vertex[0] ) + ", " 
                    + std::to_string( vertex[1] ) + ", "
                    + std::to_string( vertex[2] ) + " };\n";
            }

            ret += "\t\t\t};\n";
        }

        ret += "\t\t}\n"
                "\t};\n";
    }
    ret += "}\n";
    return ret;
}

void STLFile::collect( STLFile::builder_type const& builder )
{
    for( Complex< Length > const& plex: builder )
        add_solid( Solid::from_complex( plex ));
}

STLFile::Solid STLFile::Solid::from_complex( Complex< Length > const& plex )
{
    Solid ret;

    // TODO: turn a complex into an STL Solid
    for( Simplex< Length > const& simp: plex )
        ret.add_facet( Facet::from_simplex( simp ));

    return ret;
}

template< typename ObjT, typename SpecProcessorT >
std::ostream& STL< ObjT, SpecProcessorT >::write_to( std::ostream& os ) const
{
    STLFile out;
    // out.set_minimum_length( _minimum_length );

    // stls are the boundary of a solid object
    auto builder = STLFile::builder_type{};
    output( builder, boundary( object() ));

    // DEBUG
    // std::cerr << "SIMPLEX: " << builder.result() << std::endl;

    out.collect( builder );

    for( auto const& solid : out.solids )
    {
        os << "solid " << solid.name << "\n";

        for( auto const& facet : solid )
        {
            // TODO: should each facet just have 3 vertices?
            auto i = facet.begin();
            while( i != facet.end() )
            {
                // we should always have three...
                auto v0 = *i++;
                auto v1 = *i++;
                auto v2 = *i++;

                os << "facet normal " << facet.normal << "\n"
                    << "\touter loop\n"
                    << "\t\tvertex " << v0 << "\n"
                    << "\t\tvertex " << v1 << "\n"
                    << "\t\tvertex " << v2 << "\n"
                    << "\tendloop\n"
                    << "enfacet\n";
            }
        }
        os << "endsolid " << solid.name << "\n";
    }

    return os;
}

/// @brief ostream operator
/// @tparam ObjT solid object type
/// @param os ostream instance
/// @param stl the STL object wrapper
/// @return the ostream operator
template< typename ObjT, typename SpecProcessorT >
std::ostream& operator <<( std::ostream& os, STL< ObjT, SpecProcessorT > const& stl )
{ return stl.write_to( os ); }

} // namespace formats
} // namespace geometry


#endif // __STL_HPP__