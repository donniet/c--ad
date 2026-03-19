#include "units.hpp"
#include "tensors.hpp"
// #include "expressions.hpp"

#include <print>
#include <iostream>
#include <vector>

using namespace units;
using namespace tensors;

/**
 * What is a component?
 * 
 * - a component of an extrusion is an extrusion of a component (along with the
 *   base and extruded object )
 * 
 */

// how do we setup vectors?
template< vector T >
struct Space 
{
    using vector_type = T;
    static constexpr size_t dimensions();
};

using null_space = Space< Tensor< Shape< >>>;

template< typename T >
struct DimensionsOf;

template< size_t D, typename... Ts >
struct DimensionsOf< Space< Tensor< Shape< D >, Ts... >>>:
    integral_constant< size_t, D > { };

template< >
struct DimensionsOf< Space< Tensor< Shape <> >>>:
    integral_constant< size_t, 0 > { };

template< typename T >
constexpr size_t dimensions_of_v = DimensionsOf< T >::value;

template< vector T >
constexpr size_t Space< T >::dimensions()
{ return dimensions_of_v< Space< T >>; }

template< typename T >
struct SpaceOf
{ using type = T::space_type; };

template< typename T >
using space_of = SpaceOf< T >::type;

// a zero dimensional geometric element
struct Point
{ 
    using space_type = null_space;
    // all zero dimensional
};

template< typename ObjectT, typename AttributeT >
struct Attributed
{
    using object_type = ObjectT;
    using space_type = space_of< object_type >;
    using attribute_type = AttributeT;

    constexpr object_type object() const
    { return _object; }

    constexpr attribute_type attribute() const
    { return _attribute; }

    Attributed( object_type object, attribute_type attribute ):
        _object{ object }, _attribute{ attribute }
    { }

    object_type _object;
    attribute_type _attribute;
};

/// @brief an oriented object
/// @tparam ObjectT the type of the object
template< typename ObjectT >
struct Oriented
{
    using space_type = space_of< ObjectT >;
    using vector_type = space_type::vector_type;
    using object_type = ObjectT;

    constexpr object_type object() const
    { return _object; }

    constexpr vector_type orientation() const
    { return _orientation; }

    Oriented( object_type object, vector_type orientation ):
        _object{ object }, _orientation{ orientation }
    { }

    object_type _object;
    vector_type _orientation;
};

template< typename T, unit U >
struct ExtrudeSpace;

template< unit U >
struct ExtrudeSpace< Space< Tensor< Shape<> >>, U >
{
    using unit_type = U;
    using vector_type = Tensor< Shape< 1 >, unit_type >;
    using space_type = Space< vector_type >;

    static constexpr vector_type base( Tensor< Shape<> > )
    { return { static_cast< unit_type >( 0 ) }; }

    static constexpr vector_type extruded( Tensor< Shape<> >, unit_type u )
    { return { u }; }
};

template< size_t D, typename... Ts, unit U >
struct ExtrudeSpace< Space< Tensor< Shape< D >, Ts... >>, U >
{ 
    using vector_type = Tensor< Shape< D + 1 >, Ts..., U >;
    using space_type = Space< vector_type >; 

    template< size_t... Is >
    static constexpr vector_type vector_helper( 
        Tensor< Shape< D >, Ts... > const& v, U val, seq< Is... > )
    { return { tensor_get< Is >( v )..., val }; }

    static constexpr vector_type base( 
        Tensor< Shape< D >, Ts... > const& v )
    { return vector_helper( v, static_const< U >( 0 ), make_seq< D >{} ); }

    static constexpr vector_type extruded(
        Tensor< Shape< D >, Ts... > const& v, U u )
    { return vector_helper( v, u, make_seq< D >{} ); }
};

template< typename ObjectT, unit U >
struct Projected
{
    using object_type = ObjectT;
    using unit_type = U;
    using extrude_space = ExtrudeSpace< space_of< object_type >, unit_type >;
    using space_type = extrude_space::space_type;
    using vector_type = extrude_space::vector_type;

    constexpr object_type object() const
    { return _object; }

    constexpr unit_type amount() const
    { return _amount; }

    constexpr Projected( object_type object, unit_type amount = 
        static_cast< unit_type >( 0 )): _object{ object }, _amount{ amount } { }

    object_type _object;
    unit_type _amount;
};

template< typename T, size_t I = ( dimensions_of_v< T > - 1 )>
struct IntrudedSpace;

template< size_t D, typename... Ts, size_t I >
struct IntrudedSpace< Space< Tensor< Shape< D >, Ts... >>, I >
{
    template< typename Seq >
    struct VectorHelper;

    template< size_t... Is >
    struct VectorHelper< seq< Is... >>
    { using type = Tensor< Shape< D - 1 >, 
        tensor_element_t<( isless( Is, I ) ? Is : Is + 1 ), 
            Tensor< Shape< D >, Ts... >>... >; };

    using vector_type = VectorHelper< make_seq< D - 1 >>::type;
    using type = Space< vector_type >;

    template< size_t... Is >
    static constexpr vector_type vector_helper(
        Tensor< Shape< D >, Ts... > const& v, seq< Is... > )
    { return { tensor_get< isless( Is, I ) ? Is : Is + 1 >( v )... }; }

    static constexpr vector_type intruded( Tensor< Shape< D >, Ts... > const& v )
    { return vector_helper( v, make_seq< D - 1 >{} ); }
};

template< typename... >
struct Collection;

template< >
struct Collection< >
{ };

/// @brief a collection of objects
/// @tparam First type of first object
/// @tparam ...Rest type of remaining objects
template< typename First, typename... Rest >
requires(( is_same_v< space_of< First >, space_of< Rest >> and ... ))
struct Collection< First, Rest... >: tuple< First, Rest... >
{ 
    using space_type = space_of< First >;
    static constexpr size_t size() { return 1 + sizeof...( Rest ); }

    Collection( First first, Rest... rest ): 
        tuple< First, Rest... >{ first, rest... }
    { }
};


// template< typename SpaceT, typename ObjT, unit... Us >
// Projection< ObjT, SpaceT > project( ObjT object, Us... offsets )
// { return { project, offsets... }; }

// forward decl
template< typename ObjT, unit U >
struct Extrusion;

template< typename ObjT, unit U >
struct Extrusion
{
    using object_type = ObjT;
    using unit_type = U;
    using helper_type = ExtrudeSpace< space_of< object_type >, unit_type >;
    using space_type = helper_type::space_type;

    enum component_type: size_t 
    { base = 0, extruded };

    object_type object() const
    { return _object; }

    unit_type amount() const
    { return _amount; }

    constexpr Extrusion( object_type object, unit_type amount ): 
        _object{ object }, _amount{ amount } { }
    constexpr Extrusion(): _object{ }, _amount{ 0 } { }

    object_type _object;
    unit_type _amount;
};

//////////////
// operations

/**
 * point: represents an element of a space
 * boundary: returns the boundary of the object
 * orient: an object and a vector
 * extrude: interpolate an object into a new dimension
 * project: place an object in a higher dimensional space
 * collection: group of objects
 * 
 * extrude > project > orient > attribute > collection << boundary
 * 
 * boundary( project(...) )    => project(       boundary(...) )
 * boundary( extrude(...) )    => collection( 
 *     project(..., 0), project(...,x), extrude( boundary(...) ))
 * boundary( oriented(...) )   ?> oriented(      boundary(...) )
 * boundary( collection(...) ) => collection(    boundary(...) )
 * boundary( point )           => point
 * 
 * orient( project(...) )
 * orient( extrude(...) )
 * orient( orient(...) )       => orient(...)
 * orient( collection(...) )   => collection(    orient(...) )
 * 
 * extrude( project(...) )     == ...
 * extrude( orient(...) )      => orient(        extrude(...) )
 * extrude( point )            == segment.
 * extrude( segment )          == plane.
 * ...                         == ...
 * extrude( collection(...) )  => collection(    extrude(...) )
 * 
 * project( extrude(...) )     == ...
 * project( orient(...) )      => orient(        project(...) )
 * project( point )            == 1D location
 * project( project( point ))  == 2D location
 * ...
 * project( collection(...) )  => collection(    project(...) )
 * 
 * collection(...)
 */

/// @brief tag an object with an attribute
/// @tparam ObjectT object type
/// @tparam T attribute type
/// @param object instance
/// @param attribute value
/// @return an attributed object
template< typename ObjectT, typename T >
constexpr Attributed< ObjectT, T > attribute( ObjectT object, T attribute )
{ return { object, attribute }; }

/// @brief interpolate an object in a new dimension out to amount
/// @tparam ObjT type of object to be extruded
/// @tparam U dimension to interpolate through
/// @param object to be extruded
/// @param amount of extrusion
/// @return the extruded object
template< typename ObjT, unit U = units::Length >
Extrusion< ObjT, U > extrude( ObjT object, U amount )
{ return { object, amount }; }

/// @brief projecting an object into an extrude space
/// @tparam ObjectT type of the object
/// @tparam U unit of additional dimension
/// @param object instance
/// @param here position of projection
/// @return an object + unit pair in the extrude space
template< typename ObjectT, unit U >
constexpr Projected< ObjectT, U > project( ObjectT object, U here )
{ return { object, here }; }

/// @brief orienting an object
/// @tparam ObjectT type of the object
/// @param object instance
/// @param orientation vector
/// @return an object + orientation pair
template< typename ObjectT >
Oriented< ObjectT > orient( ObjectT object, 
    typename space_of< ObjectT >::vector_type orientation )
{ return { object, orientation }; }

// forward decl
template< typename... Objects >
auto collection( Objects... );


//////////////////////////////
// orientation specializations

/// @brief re-orienting an object overrides the orientation
/// @tparam ObjectT type of the object
/// @param oriented instance
/// @param orientation vector
/// @return an object + orientation pair
template< typename ObjectT >
Oriented< ObjectT > orient( Oriented< ObjectT > oriented, 
    typename space_of< ObjectT >::vector_type orientation )
{ return { oriented.object(), orientation }; }

/// @brief orientation of an attributed object attributes the orientation
/// @tparam ObjectT attributed object type
/// @tparam T attribute type
/// @param obj attributed object
/// @param orientation vector
/// @return returns an attributed oriented object
template< typename ObjectT, typename T >
Attributed< Oriented< ObjectT >, T > orient( Attributed< ObjectT, T > obj, 
    typename space_of< ObjectT >::vector_type orientation )
{ return attribute( orient( obj.object(), orientation ), obj.attribute() ); }

template< typename CollectionT, typename VectorT, size_t... Is >
constexpr auto collection_orient_helper( CollectionT col, VectorT orientation, 
    seq< Is... > )
{ return collection( orient( get< Is >( col ), orientation )... ); }

/// @brief re-orienting an object overrides the orientation
/// @tparam ObjectT type of the object
/// @param oriented instance
/// @param orientation vector
/// @return an object + orientation pair
template< typename First, typename... Rest >
Collection< Oriented< First >, Oriented< Rest >... > 
orient( Collection< First, Rest... > col, 
    typename space_of< First >::vector_type orientation )
{ return collection_orient_helper( col, orientation, 
    make_seq< 1 + sizeof...( Rest )>{} ); }


/////////////////////////////
// projection specializations

/// @brief projection of an orientation is the orientation of a projection
/// @tparam ObjectT oriented object type
/// @tparam U projected location
/// @param oriented object to be projected
/// @param here projection location
/// @return an oriented, projected object
template< typename ObjectT, unit U >
constexpr Oriented< Projected< ObjectT, U >> 
project( Oriented< ObjectT > oriented, U here )
{  
    using extruded_space = ExtrudeSpace< space_of< ObjectT >, U >;
    return orient( project( oriented.object(), here ),
        extruded_space::extruded( oriented.orientation(), here ));
}

/// @brief projection of an attributed object is an attributed projection
/// @tparam ObjT type of the attributed object
/// @tparam T attribute type
/// @tparam U projection type
/// @param obj attributed object instance
/// @param amount projection amount
/// @return an attributed projection
template< typename ObjT, typename T, unit U >
constexpr auto project( Attributed< ObjT, T > obj, U amount )
{ return attribute( project( obj.object(), amount ), obj.attribute() ); }

template< typename CollectionT, unit U, size_t... Is >
constexpr auto collection_project_helper( CollectionT col, U here, 
    seq< Is... > )
{ return collection( project( get< Is >( col ), here )... ); }

/// @brief projection of a collection is the collection of the projected objects
/// @tparam ...Objects object types to be projected
/// @tparam U projection unit
/// @param col collection of objects
/// @param here amount of projection
/// @return a collection of projected objects
template< typename... Objects, unit U >
constexpr auto project( Collection< Objects... > col, U here )
{ return collection_project_helper( col, here, 
    make_seq< sizeof...( Objects )>{} ); }


//////////////////////////
// extrude specializations

/// @brief extrusion of an oriented object is an orientation of the extrusion
/// of the oriented object.
/// @tparam ObjT the oriented object type
/// @tparam U the unit of extrusion
/// @param oriented the oriented object
/// @param amount thee amount to be extruded
/// @return the extruded object
template< typename ObjT, unit U >
Oriented< Extrusion< ObjT, U >> extrude( Oriented< ObjT > oriented, U amount )
{
    using extrusion_space = ExtrudeSpace< space_of< ObjT >, U >;
    
    return { extrude( oriented.object(), amount ), 
        extrusion_space::base( oriented.orientation() ) };
}

/// @brief extrusion of an attributed object is an attributed extrusion
/// @tparam ObjT type of the attributed object
/// @tparam T attribute type
/// @tparam U extrusion type
/// @param obj attributed object instance
/// @param amount extrusion amount
/// @return an attributed extrusion
template< typename ObjT, typename T, unit U >
constexpr auto extrude( Attributed< ObjT, T > obj, U amount )
{ return attribute( extrude( obj.object(), amount ), obj.attribute() ); }

template< typename CollectionT, unit U, size_t... Is >
auto collection_extrude_helper( CollectionT col, U amount, seq< Is... > )
{ return collection( extrude( get< Is >( col ), amount )... ); }

/// @brief the extrusion of a collection is a collection of extrusions
/// @tparam ...Objects the object types in the collection
/// @tparam U the unit of extrusion
/// @param collection the collection of objects
/// @param amount the amount to be extruded
/// @return a collection of extruded objects
template< typename... Objects, unit U >
Collection< Extrusion< Objects, U >... > 
extrude( Collection< Objects... > col, U amount )
{ return collection_extrude_helper( col, amount, 
    make_seq< sizeof...( Objects )>{} ); }


/////////////////////////////
// collection specializations

/// @brief empty collection
/// @return an empty collection
template< > auto collection() { return Collection<>{}; }

/// @brief collection of a single object
/// @tparam T single object type
/// @param obj single object 
/// @return a collection containing a single object
template< typename T >
Collection< T > collection( T obj )
{ return { obj }; }

/// @brief collection of a collection is the collection
/// @tparam ...Ts objects in the collection
/// @param col collection instance
/// @return col itself
template< typename... Ts >
Collection< Ts... > collection( Collection< Ts... > col )
{ return col; }

template< typename ReturnT, typename ColT, typename ColU, size_t... Is, size_t... Js >
ReturnT concat_collections_helper( ColT first, ColU second,
    seq< Is... >, seq< Js... > )
{ return { get< Is >( first )..., get< Js >( second )... }; }

template< typename... Ts, typename... Us >
auto concat_collections( Collection< Ts... > first, Collection< Us... > second )
{ return concat_collections_helper< Collection< Ts..., Us... >>( first, second, 
    make_seq< sizeof...( Ts ) >{}, make_seq< sizeof...( Us ) >{}); }

template< typename First, typename... Rest >
auto collection( First first, Rest... rest )
{ return concat_collections( collection( first ), collection( rest... )); }


///////////////////////////
// boundary specializations

/// @brief boundary of a point is a point
/// @param Point parameter
/// @return a Point
constexpr Collection<> boundary( Point ) { return {}; }

/// @brief boundary of an attributed object attributes the boundary
/// @tparam ObjT attributed object type
/// @tparam T attribute type
/// @param attributed attribute object
/// @return the attributed boundary object
template< typename ObjT, typename T >
constexpr auto boundary( Attributed< ObjT, T > attributed )
{ return attribute( boundary( attributed.object() ), attributed.attribute() ); }

/// @brief boundary of an extrusion is a collection including the 
/// @tparam ObjT extruded object
/// @tparam U unit of extrusion
/// @param ext extruded object
/// @return the boundary of an extruded object
template< typename ObjT, unit U >
constexpr auto boundary( Extrusion< ObjT, U > ext )
{ 
    using extrusion_space = ExtrudeSpace< space_of< ObjT >, U >;

    return collection( 
        project( ext.object(), static_cast< U >( 0 )),      // proxinal
        project( ext.object(), ext.amount() ),              // distal
        extrude( boundary( ext.object() ), ext.amount() )); // lateral
}

/// @brief boundary of a projection is a projection of the boundary
/// @tparam ObjT projected object type
/// @tparam U projection unit
/// @param ext projected object
/// @return boundary of the projected object
template< typename ObjT, unit U >
constexpr auto boundary( Projected< ObjT, U > pro )
{ return project( boundary( pro.object() ), pro.amount() ); }

/// @brief boundary of an oriented object is the orientation of the boundary
/// @tparam ObjT oriented object type
/// @param ori oriented object
/// @return the boundary of the oriented object re-oriented
template< typename ObjT >
constexpr auto boundary( Oriented< ObjT > ori )
{ return orient( boundary( ori.object() ), ori.orientation() ); }

/// @brief helper function for calculating the boundary of a collection
/// @tparam CollectionT collection
/// @tparam ...Is index sequence to elements in the collection
/// @param collection collection
/// @return 
template< typename CollectionT, size_t... Is >
constexpr auto collection_boundary_helper( CollectionT collection, seq< Is... > )
{ return collection( boundary( get< Is >( collection ))... ); }

/// @brief the boundary of a collection is a collection of all the boundaries
/// @tparam ...Objects
/// @param collection 
/// @return 
template< typename... Objects >
constexpr auto boundary( Collection< Objects... > collection )
{ return collection_boundary_helper( collection, make_seq< sizeof...( Objects )>{} ); }


template< unit U >
Extrusion< Point, U > segment( U length )
{ return { {}, length }; }

template< unit U >
Extrusion< Extrusion< Point, U >, U > rectangle( U length, U width )
{ return {{ {}, length }, width }; }

template< unit U >
Extrusion< Extrusion< Extrusion< Point, U >, U >, U >
box( U length, U width, U height )
{ return {{{ {}, length }, width }, height }; }

template< typename ObjT >
struct Padded 
{ 

};

template< typename ObjT >
Padded< ObjT > pad( ObjT object );


template< typename Out, typename T >
constexpr auto output( Out& file, T const& obj );

template< typename OutT, typename CollectionT, size_t... Is >
constexpr void output_collection_helper( OutT& out, CollectionT const& col, 
    seq< Is... > )
{ ( output( out, get< Is >( col )), ... ); }

template< typename OutT, typename... Objects >
constexpr void output( OutT& out, Collection< Objects... > const& col )
{ output_collection_helper( out, col, make_seq< sizeof...( Objects )>{} ); }


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
    };

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
        std::vector< Vertex > vertices;
    };

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

        Solid( string const& name ): name{ name } { }

        string name;
        std::vector< Facet > facets;
    };

    Solid& add_solid( string name = "solid" )
    { return solids.emplace_back( name ); }

    std::vector< Solid > solids;
};


template< typename ObjT >
struct STL
{
    using object_type = ObjT;

    void write_to( std::ostream& os ) const
    {
        STLFile out;
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
                    os << "facet normal " << 
                        meters( facet.normal.x ) << " " << 
                        meters( facet.normal.y ) << " " << 
                        meters( facet.normal.z ) << "\n"
                       << "\touter loop\n"
                       << "\t\tvertex " << 
                        meters( facet.vertices[ 0 ].x ) << " " << 
                        meters( facet.vertices[ 0 ].y ) << " " << 
                        meters( facet.vertices[ 0 ].z ) << "\n";

                    
                    for( size_t j = 0; j < 2; ++j )
                        os << "\t\tvertex " << 
                            meters( facet.vertices[ i + j ].x ) << " " << 
                            meters( facet.vertices[ i + j ].y ) << " " << 
                            meters( facet.vertices[ i + j ].z ) << "\n";

                    os << "\tendloop\n"
                    << "endfacet\n";
                }
            }
            os << "endsolid " << solid.name << "\n";
        }
    }

    constexpr object_type object() const 
    { return _object; }

    constexpr STL( object_type object, string name = "object" ): 
        _object{ object }, _name{ name } { }

    object_type _object;
    string _name;
};

template< typename ObjT >
std::ostream& operator<<( std::ostream& os, STL< ObjT > const& stl )
{ 
    stl.write_to( os ); 
    return os;
}

template<>
constexpr auto output( STLFile::Facet& out, Point const& p )
{ out.add_vertex(); }

// extrusion of a segment
template< typename ObjT, unit U >
// requires( isless( dimensions_of_v< space_of< ObjT >>, 3 ))
constexpr auto output( STLFile::Facet& out, Extrusion< ObjT, U > const& object )
{ 
    static constexpr size_t dim = dimensions_of_v< space_of< ObjT >>;

    size_t verts = out.size();
    output( out, object.object() );
    // add the y dimension
    size_t i = out.size();
    verts = out.size() - verts;
    for( size_t j = 0; j < verts; ++j )
        out.add_vertex( out[--i] )[ dim ] = object.amount();
}

template< typename ObjT, unit U >
constexpr auto output( STLFile::Facet& out, Projected< ObjT, U > const& object )
{
    static constexpr size_t dim = dimensions_of_v< space_of< ObjT >>;
    size_t verts = out.size();
    // output the projected object
    output( out, object.object());
    verts = out.size() - verts;
    for( auto j = out.rbegin(); verts > 0; --verts, ++j )
        (*j)[ dim ] = object.amount();
}

// extrusion of a segment
template< typename ObjT, unit U >
// requires( isless( space_of< ObjT >::dimensions(), 3 ))
constexpr auto output( STLFile::Solid& out, Extrusion< ObjT, U > const& object )
{ return output( out.add_facet(), object ); }

template< typename ObjT, unit U >
// requires( isless( space_of< ObjT >::dimensions(), 3 ))
constexpr auto output( STLFile::Solid& out, Projected< ObjT, U > const& object )
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

template< typename ObjT, unit U >
constexpr auto output( STLFile& out, Extrusion< ObjT, U > const& obj )
{ return output( out.add_solid( "extrusion" ), obj ); } 

template< typename ObjT, unit U >
constexpr auto output( STLFile& out, Projected< ObjT, U > const& obj )
{ return output( out.add_solid( "projection" ), obj ); } 

// template< typename ObjT >
// constexpr auto output( STLFile::Solid& out, Oriented< ObjT > const& object )
// { 
//     auto v = object.orientation();
//     auto facet = out.add_facet( get_tensor< 0 >( v ), get_tensor< 1 >( v ), 
//         get_tensor< 2 >( v ));
//     output( facet, object.object() );
// }

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




auto mortise_and_tenon()
{

    // auto tenon_layout = Grid< 3, 3 >{};
    // auto mortise_layout = Grid< 3, 3 >{};

    // auto rail_end = component< Box::Top >( rail );
    // auto stile_inside = component< Rectangle::Bottom >( stile );

    // auto tenon = pad( component< grid_cell< 1, 1 >>( 
    //     tenon_layout( rail_end )));
    // auto mortise = pad( component< grid_cell< 1, 1 >>( 
    //     mortise_layout ( stile_inside )));

    return box( 1_m, 1_m, 1_m );
}

int main( int ac, char* av[] )
{
    auto obj = mortise_and_tenon();

    std::cout << STL{ obj } << std::endl;

    return 0;
}