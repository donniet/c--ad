#ifndef __GEOMETRY_HPP__
#define __GEOMETRY_HPP__

#include "geometry/space.hpp"
#include "geometry/collection.hpp"
#include "geometry/compound.hpp"
#include "geometry/attribute.hpp"
#include "geometry/linear_transform.hpp"
#include "geometry/translate.hpp"
#include "geometry/project.hpp"
#include "geometry/orient.hpp"
#include "geometry/boundary.hpp"

namespace geometry {

/**
 * What is a component?
 * 
 * - a component of an extrusion is an extrusion of a component (along with the
 *   base and extruded object )
 * 
 */

/// @brief extrude a surface element from an object and return a compound
/// @tparam ObjectT 
/// @tparam SurfaceElementT 
template< typename ObjectT >
struct Padding
{
    using object_type = ObjectT;
    using space_type = space_of< object_type >;
    using vector_type = space_type::vector_type;

    static string object_name() 
    { return "padding_" + object_type::object_name(); }

    object_type object;
    vector_type direction;
};

template< size_t I, size_t J, typename ObjT >
requires( I != J and isleess( I, dimensions_of_v< space_of< ObjT >> and isless( J, dimensions_of_v< space_of< ObjT >> )))
struct Revolution
{
    static constexpr size_t first_axis = I;
    static constexpr size_t second_axis = J;
    using object_type = ObjT;

    static string object_name()
    { return "revolve_" + std::to_string( I ) + "_" + std::to_string( J ) + "_" + object_type::object_name(); }

    object_type object;
    long double angle;
};

//////////////
// operations


template< typename ObjT >
requires( is_empty< ObjT > )
Empty< space_of< ObjT >> pad( ObjT const& )
{ return { }; }

template< typename ObjT >
requires( not is_empty< ObjT > )
Padding< ObjT > pad( ObjT const& object )
{ return { object }; }


/////////////////////////////
// collection specializations

// /// @brief empty collection
// /// @return an empty collection
// template< > auto collection() { return Collection<>{}; }

// /// @brief collection of a single object
// /// @tparam T single object type
// /// @param obj single object 
// /// @return a collection containing a single object
// template< typename T >
// Collection< T > collection( T obj )
// { return { obj }; }

// /// @brief collection of a collection is the collection
// /// @tparam ...Ts objects in the collection
// /// @param col collection instance
// /// @return col itself
// template< typename... Ts >
// Collection< Ts... > collection( Collection< Ts... > col )
// { return col; }

// template< typename ReturnT, typename ColT, typename ColU, size_t... Is, size_t... Js >
// ReturnT concat_collections_helper( ColT first, ColU second,
//     seq< Is... >, seq< Js... > )
// { return { get< Is >( first )..., get< Js >( second )... }; }

// template< typename... Ts, typename... Us >
// auto concat_collections( Collection< Ts... > first, Collection< Us... > second )
// { return concat_collections_helper< Collection< Ts..., Us... >>( first, second, 
//     make_seq< sizeof...( Ts ) >{}, make_seq< sizeof...( Us ) >{}); }

// /// @brief collection of objects that ensures no collections of collections.  
// /// Each object must be in the same vector space
// /// @tparam First type of first object
// /// @tparam ...Rest types of the remaining objects
// /// @param first object instance
// /// @param ...rest remaining object instances
// /// @return a collection containing the objects, where nested collections are 
// /// instead enumerated in the output
// template< typename First, typename... Rest >
// auto collection( First first, Rest... rest )
// { return concat_collections( collection( first ), collection( rest... )); }

// /////////////////////////////////
// /// subdivide specializations ///
// /////////////////////////////////

// template< size_t Divisions >
// constexpr Collection<> subdivide( Point ) { return {}; }

// template< size_t Divisions, typename CollectionT, size_t... Is >
// constexpr auto subdivide_collection_helper( CollectionT col, seq< Is... > )
// { return colection( subdivide< Divisions >( get< Is >( col ))... ); }

// template< size_t Divisions, typename... Objects >
// constexpr auto subdivide( Collection< Objects... > col ) 
// { return subdivide_collection_helper< Divisions >( col, 
//     make_seq< sizeof...( Objects )>{} ); }

// template< size_t Divisions, typename ObjT >
// constexpr auto subdivide( Orientation< ObjT > oriented )
// { return orient( subdivide< Divisions >( oriented.object() ), 
//     oriented.orientation() ); }

// template< size_t Divisions, typename ObjT, typename U >
// constexpr auto subdivide( Projection< ObjT, U > proj )
// { return project( subdivide< Divisions >( proj.object() ), proj.amount() ); }

// template< size_t Divisions, typename ObjT >
// struct SubdivisionHelper;

// template< typename ObjT >
// struct SubdivisionHelper< 0, ObjT > 
// { static_assert( false, "cannot subdivide by zero"); };

// template< typename ObjT, typename U >
// struct SubdivisionHelper< 1, Extrusion< ObjT, U >>
// {
//     using type = Extruded< ObjT, U >;
//     static constexpr type subdivide( type ext )
//     { return ext; }
// };

// template< size_t Divisions, typename ObjT, typename U >
// requires( Divisions > 1 )
// struct SubdivisionHelper< Divisions, Extruded< ObjT, U >>
// {
//     using type = uniform_collection_t< 
//     static constexpr type subdivide( type ext )
//     { return ext; }
// };

// template< size_t Divisions, typename ObjT, typename U >
// constexpr auto subdivide( Extrusion< ObjT, U > ext )
// { return SubdivisionHelper< Divisions, Extrusion< ObjT, U >>::subdivide( ext ); }



///////////////////////////
// boundary specializations


/// @brief compound types of the boundary of a padding
struct padding_boundary {
    struct base { };
    struct shell { };
    struct cap { };
};

// a segment is the shell element of the boundary of a pad of a point
// or is it a padded point?

// pad( plane ) -> cube
// pad( segment ) -> plane
// pad( point ) -> segment


///////////////////
/// translation ///
///////////////////

///////////
/// pad ///
///////////

////////////////////////////////////
/// orientation helper functions ///
////////////////////////////////////

/// IsEmpty specializations ///
///

template< typename ObjT >
struct IsEmpty< Padding< ObjT >>
{ static constexpr bool value = IsEmpty< ObjT >::value; };








} // namespace geometry

/////////
// output



#endif // __GEOMETRY_HPP__