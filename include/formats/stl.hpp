#ifndef __STL_HPP__
#define __STL_HPP__

/**
 * Implementation of the STL file format.
 */

#include "units.hpp"
#include "geometry.hpp"

#include <iostream>
#include <vector>
#include <functional>

namespace formats {

using namespace units;
using namespace geometry;

struct STLFile
{
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
        using iterator = std::vector< Vertex >::iterator;
        using reverse_iterator = std::vector< Vertex >::reverse_iterator;

        size_t size() { return vertices.size(); }
        iterator begin() { return vertices.begin(); }
        iterator end() { return vertices.end(); }
        reverse_iterator rbegin() { return vertices.rbegin(); }
        reverse_iterator rend() { return vertices.rend(); }


        Vertex& add_vertex( Length x = 0_m, Length y = 0_m, Length z = 0_m )
        { return vertices.emplace_back( x, y, z ); }
        Vertex& add_vertex( Vertex const& v )
        { return vertices.emplace_back( v ); }

        Vertex& operator []( size_t n )
        { return vertices[ n ]; }

        Facet( Length nx = 0_m, Length ny = 0_m, Length nz = 0_m ):
            normal{ nx, ny, nz } { }

        Vertex normal;
        string comment;
        std::vector< Vertex > vertices;
    };

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

                for( size_t i = 1; i < facet.vertices.size() - 1; ++i )
                {
                    os << "facet normal " << facet.normal << "\n"
                       << "\touter loop\n"
                       << "\t\tvertex " <<  facet.vertices[ 0 ] << "\n";

                    for( size_t j = 0; j < 2; ++j )
                        os << "\t\tvertex " << facet.vertices[ i + j ] << "\n";

                    os << "\tendloop\n"
                    << "endfacet\n";
                }
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

/**
 * Here's what a cube looks like:
 * 
 * output<Solid, 
 *    Orientation<Projection<Extrusion<Extrusion<Point, Length, long double>>, Length, long double>>, Length, long double>>>, 
 *    Orientation<Projection<Extrusion<Extrusion<Point, Length, long double>>, Length, long double>>, Length, long double>>>, 
 *    Extrusion<Collection<
 *        Orientation<Projection<Extrusion<Point, Length, long double>>, Length, long double>>>, 
 *        Orientation<Projection<Extrusion<Point, Length, long double>>, Length, long double>>>, 
 *        Extrusion<Collection<
 *            Orientation<Projection<Point, Length, long double>>>, 
 *            Orientation<Projection<Point, Length, long double>>>, 
 *            Extrusion<Collection<>, Length, long double>>>, 
 *        Length, long double>>>, 
 *    Length, long double>>>
 * 
 * output<Collection<
 *    Orientation<Projection<Extrusion<Extrusion<Point, Length>, Length>, Length>>, 
 *    Orientation<Projection<Extrusion<Extrusion<Point, Length>, Length>, Length>>, 
 *    Extrusion<Collection<
 *       Orientation<Projection<Extrusion<Point, Length>, Length>>, 
 *       Orientation<Projection<Extrusion<Point, Length>, Length>>, 
 *       Extrusion<Collection<Orientation<Projection<Point, Length>>, 
 *          Orientation<Projection<Point, Length>>, 
 *          Extrusion<Collection<>, Length>>, Length>>, Length>>>
 * 
 * Collection<
 *    Orientation<Projection<Extrusion<Extrusion<Point, Length, 1>, Length, 1>, Length > >, 
 *    Collection<
 *      Extrusion<Orientation<Projection<Extrusion<Point, Length, 1>, Length > >, Length, 1>, 
 *      Extrusion<Orientation<Projection<Extrusion<Point, Length, 1>, Length > >, Length, 1>, 
 *      Collection<
 *         Extrusion<Extrusion<Orientation<Projection<Point, Length > >, Length, 1>, Length, 1>, 
 *         Extrusion<Extrusion<Orientation<Projection<Point, Length > >, Length, 1>, Length, 1> > > >
 */


/// @brief outputing a point to an STL facet
/// @param out facet to add a vertex to
/// @param p point to add
/// @return the vertex
constexpr STLFile::Vertex& output( STLFile::Facet& out, Point const& p )
{ 
    auto& v = out.add_vertex(); 
#ifndef NDEBUG
    v.comment = "Facet,Point";
#endif
    return v;
}

/// @brief output an extrusion to a facet.
/// @tparam ObjT extruded object type
/// @param out facet to write this extrusion to
/// @param object object to output
/// @return the facet
template< typename ObjT, size_t Steps >
constexpr STLFile::Facet& output( STLFile::Facet& out, 
    Extrusion< ObjT, Length, Steps > const& extrusion )
{ 
    static constexpr size_t dim = dimensions_of_v< space_of< ObjT >>;

    size_t verts = out.size();
    output( out, extrusion.object() );
    size_t i = out.size();
    verts = out.size() - verts;
    Length amount = {};
    for( Length const& step : extrusion.step_values())
    {
        amount += step;
        for( size_t j = 0; j < verts; ++j )
        {
            auto& v = out.add_vertex( out[--i] );
            v[ dim ] = amount;
#ifndef NDEBUG
            v.comment = "Facet,Extrusion";
#endif
        }
    }
    
    return out;
}

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
    {
        out.comment += "; Facet,Projection";
        (*j)[ dim ] = object.amount();
    }
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
    s.comment = "File,Attribute";
    return output( s, obj.object() ); 
}

template< typename ObjT >
constexpr auto output( STLFile::Solid& out, Orientation< ObjT > const& object )
{ 
    auto v = object.orientation();
    auto& facet = out.add_facet( tensor_get< 0 >( v ), tensor_get< 1 >( v ), 
        tensor_get< 2 >( v ));
    facet.comment += "; Solid,Orientation";
    output( facet, object.object() );
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