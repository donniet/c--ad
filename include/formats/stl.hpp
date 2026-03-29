#ifndef __STL_HPP__
#define __STL_HPP__

/**
 * Implementation of the STL file format.
 */

#include "units.hpp"
#include "tensors.hpp"
#include "geometry.hpp"

#include <iostream>
#include <vector>
#include <list>
#include <functional>
#include <algorithm>

namespace formats {

using namespace units;
using namespace tensors;
using namespace geometry;


struct STLFile
{
    using vector_type = Tensor< Shape< 3 >, Length, Length, Length >;

    struct Vertex
    {
        Length& operator []( size_t n )
        {
            switch( n ) {
            case 0: return x;
            case 1: return y;
            case 2: 
            default:
                return z;
            }
        }

        vector_type as_vector() const
        { return { x, y, z }; }

        Vertex( Length x, Length y, Length z ):
            x{ x }, y{ y }, z{ z }
        { }

        Vertex( Vertex const& v ):
            x{ v.x }, y{ v.y }, z{ v.z }
        { }

        Length x;
        Length y;
        Length z;
        string comment;
    };

    // implemented using an outer loop of vertices
    struct Facet
    {
        using iterator = std::list< Vertex >::iterator;
        using const_iterator = std::list< Vertex >::const_iterator;
        using reverse_iterator = std::list< Vertex >::reverse_iterator;

        Vertex& front() { return vertices.front(); }
        Vertex const& front() const { return vertices.front(); }
        Vertex& back() { return vertices.back(); }
        Vertex const& back() const { return vertices.back(); }
        size_t size() { return vertices.size(); }
        iterator begin() { return vertices.begin(); }
        iterator end() { return vertices.end(); }
        reverse_iterator rbegin() { return vertices.rbegin(); }
        reverse_iterator rend() { return vertices.rend(); }

        void reverse_vertex_order()
        { std::reverse( vertices.begin(), vertices.end() ); }

        Vertex& add_vertex( Length x = 0_m, Length y = 0_m, Length z = 0_m )
        { return vertices.emplace_back( x, y, z ); }
        Vertex& add_vertex( Vertex const& v )
        { return vertices.emplace_back( v ); }

        iterator emplace( const_iterator here, 
            Length x = 0_m, Length y = 0_m, Length z = 0_m )
        { return vertices.emplace( here, x, y, z ); }
        iterator emplace( const_iterator here, Vertex const& v )
        { return vertices.emplace( here, v ); }

        Facet( Length nx = 0_m, Length ny = 0_m, Length nz = 0_m ):
            normal{ nx, ny, nz } { }

        Vertex normal;
        string comment;
        std::list< Vertex > vertices;
    };

    /// @brief output a string representing this STLFile. 
    /// NOTE: this is for debug purposes.  Use the STL class to output an STL 
    /// file format
    /// @return a string containing the solids, facets and vertices of this STLFile
    string to_string() const
    {
        string ret = "{\n";
        ret += "\t.solids = {\n";

        for( auto const& solid: solids )
        {
            ret += "\t\t.name = \"" + solid.name + "\";\n";
            ret += "\t\t.comment = \"" + solid.comment + "\";\n";
            ret += "\t\t.facets = {\n";

            for( auto const& facet: solid.facets )
            {
                ret += "\t\t\t.normal = {" 
                    + std::to_string( facet.normal.x ) + ", " 
                    + std::to_string( facet.normal.y ) + ", " 
                    + std::to_string( facet.normal.z ) + " };\n";
                ret += "\t\t\t.comment = \"" + facet.comment + "\";\n";
                ret += "\t\t\t.vertices = {\n";

                for( auto const& vertex: facet.vertices )
                {
                    ret += "\t\t\t\t{ " 
                        + std::to_string( vertex.x ) + ", " 
                        + std::to_string( vertex.y ) + ", "
                        + std::to_string( vertex.z ) + " };\n";
                }

                ret += "\t\t\t};\n";
            }

            ret += "\t\t}\n"
                   "\t};\n";
        }
        ret += "}\n";
        return ret;
    }

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

    struct Solid
    {
        using iterator = std::vector< Facet >::iterator;
        using reverse_iterator = std::vector< Facet >::reverse_iterator;

        size_t size() { return facets.size(); }
        iterator begin() { return facets.begin(); }
        iterator end() { return facets.end(); }
        reverse_iterator rbegin() { return facets.rbegin(); }
        reverse_iterator rend() { return facets.rend(); }

        Facet& add_facet( Length nx = 0_m, Length ny = 0_m, Length nz = 0_m )
        { return facets.emplace_back( nx, ny, nz ); }

        Solid( string const& name ): name{ solid_name( name ) } { }

        string name;
        string comment;
        std::vector< Facet > facets;
    };

    Solid& add_solid( string name = "solid" )
    { return solids.emplace_back( name ); }

    std::vector< Solid > solids;
};

std::ostream& operator <<( std::ostream& os, STLFile::Vertex const& v )
{ return os << meters( v.x ) << " " << meters( v.y ) << " " << meters( v.z ); }

template< typename ObjT >
struct STL
{
    using object_type = ObjT;

    std::ostream& write_to( std::ostream& os ) const
    {
        STLFile out;
        string (*cmt)( string const& ) = STLFile::comment;

        // stls are the boundary of a solid object
        output( out, boundary( object() ));

        for( auto const& solid : out.solids )
        {
            os << "solid " << solid.name << "\n";

            for( auto const& facet : solid.facets )
            {
                // malformed facet
                if( facet.vertices.size() < 3 )
                    continue;

                auto i = facet.vertices.begin();
                while( i != facet.vertices.end() )
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

                // for( size_t i = 1; i < facet.vertices.size() - 1; ++i )
                // {
                //     os << "facet normal " << facet.normal << "\n"
                //        << "\touter loop\n"
                //        << "\t\tvertex " <<  facet.vertices[ 0 ] << "\n";

                //     for( size_t j = 0; j < 2; ++j )
                //         os << "\t\tvertex " << facet.vertices[ i + j ] << "\n";

                //     os << "\tendloop\n"
                //     << "endfacet\n";
                // }
            }
            os << "endsolid " << solid.name << "\n";
        }

        return os;
    }

    constexpr object_type object() const 
    { return _object; }

    constexpr STL( object_type object, string name = "object" ): 
        _object{ object }, _name{ name } { }

    object_type _object;
    string _name;
};

/// @brief ostream operator
/// @tparam ObjT solid object type
/// @param os ostream instance
/// @param stl the STL object wrapper
/// @return the ostream operator
template< typename ObjT >
std::ostream& operator <<( std::ostream& os, STL< ObjT > const& stl )
{ return stl.write_to( os ); }

/// @brief outputing a point to an STL facet
/// @param out facet to add a vertex to
/// @param p point to add
/// @return the vertex
constexpr STLFile::Vertex& output( STLFile::Facet& out, Point const& p )
{ return out.add_vertex(); }

/// @brief extrusion of a point
template< typename ObjT, size_t Steps >
constexpr STLFile::Facet& output( STLFile::Facet& out, 
    Extrusion< ObjT, Length, Steps > const& extrusion )
{
    using iterator = STLFile::Facet::iterator;
    static constexpr size_t dim = dimensions_of_v< space_of< ObjT >>;
    // gather the vertex
    output( out, extrusion.object() );

    if( out.size() == 1 )
    {
        // expects only a single vertex and adds a string of vertices
        Length amount = {};
        for( Length const& step: extrusion.step_values() )
        {
            amount += step;
            auto v = out.front();
            v[ dim ] = amount;
            out.add_vertex( v );
        }

        return out;
    }

    // expects a string of vertices and creates triplets
    Length amount = {};
    size_t s = 0;

    std::list< STLFile::Vertex > layer;
    // transfer our string to the layer
    layer.splice( layer.end(), out.vertices );

    for( Length const& step: extrusion.step_values() )
    {
        amount += step;
        iterator p1, p2;
        p1 = p2 = layer.begin();
        ++p2;

        while( p2 != layer.end() )
        {
            auto v1 = *p1;
            auto v2 = *p2;
            auto v3 = *p1;
            v3[ dim ] = amount;
            auto v4 = *p2;
            v4[ dim ] = amount;

            // first triangle uses the original v1 vertices
            // v1-v2-v3
            out.add_vertex( v1 );
            out.add_vertex( v2 );
            out.add_vertex( v3 );

            // second triangle inserted after first
            // v4-v3-v2
            out.add_vertex( v4 );
            out.add_vertex( v3 );
            out.add_vertex( v2 );

            p1 = p2;
            ++p2;
        }

        // move layer one step forward
        for( auto& v: layer )
            v[ dim ] = amount;
    }

    return out;
}

/// @brief extrusion of a.  Assumes the facet is linear
/// NOTE: vertices should be in sets of three
/// @tparam ObjT extruded object type
/// @param out facet to write this extrusion to
/// @param object object to output
/// @return the facet
// template< typename ObjT, size_t Steps >
// constexpr STLFile::Facet& output( STLFile::Facet& out, 
//     Extrusion< ObjT, Length, Steps > const& extrusion )
// { 
//     static constexpr size_t dim = dimensions_of_v< space_of< ObjT >>;
//     using iterator = STLFile::Facet::iterator;

//     // add vertices for the sub object
//     output( out, extrusion.object() );
//     Length amount = {};
//     iterator last_step = out.begin();

//     switch( dim ) {
//     case 0:
//         // extrusion of a single point
//         for( Length const& step: extrusion.step_values() )
//         {

//         }
//         break;
//     case 1:
//         break;
//     case 2:
//         break;
//     }

//     if constexpr( dim == 0 )
//     {

//     }

//     for( Length const& step : extrusion.step_values() )
//     {
//         amount += step;
//         iterator v1, v2;
//         v1 = v2 = last_step;
//         if( v2 == out.end() )
//             break;

//         ++v2;
//         while( v2 != out.end() )
//         {
//             // we have two vertices on the last step
//         }

//         for( size_t j = 0; j < verts; ++j )
//         {
//             auto& v = out.add_vertex( out[--i] );
//             v[ dim ] = amount;
//         }
//     }
    
//     return out;
// }

/// @brief output an extrusion to a facet
/// @tparam ObjT 
/// @param out 
/// @param object 
/// @return 
template< typename ObjT >
constexpr auto output( STLFile::Facet& out, 
    Projection< ObjT, Length > const& object )
{
    static constexpr size_t dim = dimensions_of_v< space_of< ObjT >>;
    size_t verts = out.size();
    // output the projected object
    output( out, object.object());
    verts = out.size() - verts;
    for( auto j = out.rbegin(); verts > 0; --verts, ++j )
        (*j)[ dim ] = object.amount();
}

/// @brief outputing an orientation to a facet is duplicative since we already
/// identified the normal vector
/// TODO: what if we hane't set a normal vector yet?  
/// @param out facet to add a vertex to
/// @param p 
/// @return the vertex
template< typename ObjT >
constexpr auto output( STLFile::Facet& out, Orientation< ObjT > const& object )
{ return output( out, object.object() ); }

// extrusion of a segment
template< typename ObjT >
// requires( isless( space_of< ObjT >::dimensions(), 3 ))
constexpr auto output( STLFile::Solid& out, Extrusion< ObjT, Length > const& object )
{ return output( out.add_facet(), object ); }

template< typename ObjT >
// requires( isless( space_of< ObjT >::dimensions(), 3 ))
constexpr auto output( STLFile::Solid& out, Projection< ObjT, Length > const& object )
{ return output( out.add_facet(), object ); }

template< typename CollectionT, size_t... Is >
constexpr auto output_collection_helper( STLFile::Solid& out, CollectionT const& col, 
    seq< Is... > )
{ return ( output( out, get< Is >( col )), ... ); }

// template< typename CollectionT, size_t... Is >
// constexpr auto output_collection_helper( STLFile::Solid& out, CollectionT const& col, 
//     seq< Is... > )
// { return ( output( out, get< Is >( col )), ... ); }

template< typename... Objects >
constexpr auto output( STLFile& out, Collection< Objects... > const& col )
{ return output_collection_helper( out.add_solid( "collection"), col, 
    make_seq< sizeof...( Objects )>{} ); } 

template< typename ObjT >
constexpr auto output( STLFile& out, Extrusion< ObjT, Length > const& obj )
{ return output( out.add_solid( "extrusion" ), obj ); } 

template< typename ObjT >
constexpr auto output( STLFile& out, Projection< ObjT, Length > const& obj )
{ return output( out.add_solid( "projection" ), obj ); } 

template< typename ObjT >
constexpr auto output( STLFile& out, Attribution< ObjT, Named > const& obj )
{ 
    auto& s = out.add_solid( obj.attribute().name() );
    return output( s, obj.object() ); 
}

template< typename ObjT >
constexpr auto output( STLFile::Solid& out, Orientation< ObjT > const& object )
{ 
    auto v = object.orientation();
    STLFile::Facet& facet = out.add_facet( tensor_get< 0 >( v ), 
        tensor_get< 1 >( v ), tensor_get< 2 >( v ));
    output( facet, object.object() );

    // check if the triangle order matches the orientation
    if( facet.vertices.size() < 3 )
        return out;

    auto n = facet.normal.as_vector();

    auto i = facet.vertices.begin();
    auto const& v0 = (i++)->as_vector();
    auto const& v1 = (i++)->as_vector();
    auto const& v2 = i->as_vector();

    auto x1 = v1 - v0;
    auto x2 = v2 - v0;
    
    if( is_positive( dot( cross( x1, x2 ), n )))
        facet.reverse_vertex_order();
    
    return out;
}

// template< >
// constexpr auto output( STLFile::Facet& out, 
//     Projected< Projected< Projected< Point, Length >, Length >, Length > const& p )
// { 
//     out.add_vertex( p.object().object().amount(), 
//         p.object().amount(), p.amount() );
// }

// extrusion of a point 
// template< unit U >
// constexpr auto output( STLFile::Facet& out, Extrusion< Point, U > const& object )
// { 
//     out.add_vertex();
//     out.add_vertex( object.amount() );
// }


} // namespace formats


#endif // __STL_HPP__