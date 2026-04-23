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

#include <iostream>
#include <vector>
#include <list>
#include <functional>
#include <algorithm>

namespace geometry{
namespace formats {

using namespace units;
using namespace tensors;

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

        Solid( string const& name ): name{ solid_name( name ) } { }

        string name;
    };

    struct Cursor
    {
        Cursor( STLFile* file ): file{ file } { }

        STLFile* file;
        builder_type builder;
    };

    Cursor cursor() { return Cursor{ this }; }

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

    Length& set_minimum_length( Length const& min_length )
    { return _minimum_length = min_length; }

    Length const& minimum_length() const
    { return _minimum_length; }

    std::vector< Solid > solids;
    Length _minimum_length;
};

std::ostream& operator <<( std::ostream& os, STLFile::vec3 const& v )
{ return os << meters( v[0] ) << " " << meters( v[1] ) << " " << meters( v[2] ); }

template< typename ObjT >
struct STL
{
    using object_type = ObjT;
    using cursor_type = STLFile::Cursor;

    void output( cursor_type cursor, Point object ) const
    { cursor.builder.push_vertex(); }

    void output( cursor_type cursor, Extrusion< ObjT, Length, Length > ext ) const
    { 
        output( cursor, ext.object() );
        cursor.builder.extrude( ext.from(), ext.to() ); 
    }

    template< size_t From, typename... Us >
    static constexpr uniform_vector_t< From + sizeof...( Us ), Length > 
    translation_vector_from( ProjectAt< From, Us... > proj )
    { return proj( uniform_vector_t< From, Length >::zero() ); }

    template< typename ObjU, size_t From, typename... Us >
    requires(( is_same_v< Length, Us > and ... ))
    void output( cursor_type cursor, Projection< ObjU, ProjectAt< From, Us... >> ext ) const
    { 
        output( cursor, ext.object() );
        cursor.builder.translate( translation_vector_from( ext.transform() )); 
    }

    /// output compounds and attributed objects
    template< typename ComponentsU, typename... Objs >
    void output( cursor_type cursor, Compound< ComponentsU, Objs... > compound ) const
    {
        auto output_compound_helper = [&]< size_t... Is >( seq< Is... > )
        {( output( cursor, get_component< Is >( compound )), ... ); };

        output_compound_helper( make_seq< sizeof...( Objs )>{} );
        cursor.builder.combine( sizeof...( Objs ));
    }

    template< typename... Objs >
    void output( cursor_type cursor, Collection< Objs... > col ) const
    {
        auto output_collection_helper = [&]< size_t... Is >( seq< Is... > )
        {( output( cursor, get< Is >( col )), ... ); };

        output_collection_helper( make_seq< sizeof...( Objs )>{} );
        cursor.builder.combine( sizeof...( Objs ));
    }
    template< typename ObjU, typename AttU >
    void output( cursor_type cursor, Attribution< ObjU, AttU > att ) const
    { output( cursor, att.object() ); }

    template< typename ObjU, size_t Rows, size_t Cols >
    void output( cursor_type cursor,
        Projection< ObjU, Linear< uniform_matrix_t< Rows, Cols, Length >>> proj ) const
    {
        output( cursor, proj.object() );
        cursor.builder.linear_transform( proj.transformation() );
    }

    std::ostream& write_to( std::ostream& os ) const;

    constexpr object_type const& object() const 
    { return _object; }

    constexpr STL& with_minimum_length( Length const& minimum_length )
    { 
        _minimum_length = minimum_length;
        return *this;
    }

    constexpr STL( object_type const& object, string name = "object" ): 
        _object{ object }, _name{ name }, _minimum_length{ /* 0 */ } { }

    object_type _object;
    string _name;
    Length _minimum_length;
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

template< typename ObjT >
std::ostream& STL< ObjT >::write_to( std::ostream& os ) const
{
    STLFile out;
    out.set_minimum_length( _minimum_length );

    // stls are the boundary of a solid object
    output( out.cursor(), boundary( object() ));

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
template< typename ObjT >
std::ostream& operator <<( std::ostream& os, STL< ObjT > const& stl )
{ return stl.write_to( os ); }

} // namespace formats
} // namespace geometry


#endif // __STL_HPP__