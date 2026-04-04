#ifndef __GEOMETRY_HPP__
#define __GEOMETRY_HPP__

#include "tensors.hpp"
#include "units.hpp"
// #include "expressions.hpp"

#include <print>
#include <iostream>
#include <vector>

namespace geometry {

using namespace tensors;
using namespace units;

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

template< typename T >
using vector_for = space_of< T >::vector_type;

template< typename U, size_t Sections, size_t... Is >
constexpr U divisions_helper( U amount, std::array< U, Sections >& ret, 
    seq< Is... > )
{ return amount - (( ret[ Is ] = amount / Sections ) + ... ); }

template< typename U, size_t Sections >
struct IgnoreRemainder
{ constexpr void operator()( U remainder, std::array< U, Sections >& divisions )
    { return; } };

/// @brief divide a measurement into an array such that the sum of the array 
/// equals the amount
template< size_t Sections, typename U, 
    typename RemainderStrategy = IgnoreRemainder< U, Sections >>
requires( isgreater( Sections, 0 ))
constexpr std::array< U, Sections > divisions( U amount, 
    RemainderStrategy handle_remainder = RemainderStrategy{} )
{ 
    std::array< U, Sections > ret;
    U rem = divisions_helper( amount, ret, make_seq< Sections >{} );
    handle_remainder( rem, ret );
    return ret;
}



template< typename... >
struct Collection;

template< >
struct Collection< >
{ 
    using space_type = null_space;
    using boundary_type = Collection< >; // no boundary
    static constexpr size_t size() { return 0; }
    constexpr boundary_type boundary() { return {}; }
};

/// @brief a collection-like object that inhabits the space of an object
/// but acts as a empty collection
template< typename ObjT >
struct Ignored : Collection< >
{
    using space_type = space_of< ObjT >;
    Ignored( ObjT const& ) { };
};

// a zero dimensional geometric element
struct Point
{ 
    using space_type = null_space;
    using boundary_type = Collection< >; // no boundary
    constexpr boundary_type boundary() { return {}; }
    // all zero dimensional
};

/// @brief a tuple-like collection of objects
/// @tparam First type of first object
/// @tparam ...Rest type of remaining objects
template< typename First, typename... Rest >
requires(( is_same_v< space_of< First >, space_of< Rest >> and ... ))
struct Collection< First, Rest... >: Collection< Rest... >
{ 
    using space_type = space_of< First >;
    using boundary_type = Collection< typename First::boundary_type, 
        typename Rest::boundary_type... >;
    static constexpr size_t size() { return 1 + sizeof...( Rest ); }

    constexpr First const& first() const { return _first; }
    constexpr First& first() { return _first; }

    constexpr Collection< Rest... > const& rest() const 
    { return *this; }
    constexpr Collection< Rest... >& rest() 
    { return *this; }

    constexpr boundary_type boundary() const
    { return { First::boundary(), Rest::boundary()... }; }

    constexpr Collection( First const& first, Rest const&... rest ): 
        Collection< Rest... >{ rest... }, _first{ first }
    { }
    constexpr Collection( First const& first, Collection< Rest... > const& rest ):
        Collection< Rest... >{ rest }, _first{ first }
    { }

    constexpr Collection( Collection const& ) = default;
    // constexpr Collection() = default;

    First _first;
};

template< typename ObjT, typename CollectionT >
struct CollectionAdd;

template< typename ObjT, typename... Objects >
struct CollectionAdd< ObjT, Collection< Objects... >>
{ using type = Collection< ObjT, Objects... >; };

template< typename... Leading, typename... Following >
struct CollectionAdd< Collection< Leading... >, Collection< Following... >>
{ using type = Collection< Leading..., Following... >; };


template< typename ObjT, typename CollectionT >
using collection_add_t = CollectionAdd< ObjT, CollectionT >::type;

template< typename... >
struct MakeCollection;

template< >
struct MakeCollection< >
{ 
    using type = Collection<>;
    static constexpr type make() { return {}; } 
};

template< typename First, typename... Rest >
struct MakeCollection< First, Rest... >
{  
    using type = collection_add_t< First, 
        typename MakeCollection< Rest... >::type >;
    static constexpr type make( First const& first, Rest const&... rest )
    { return { first, MakeCollection< Rest... >::make( rest... )}; }
};

template< typename... Rest >
struct MakeCollection< Collection<>, Rest... >
{  
    using type = MakeCollection< Rest... >::type;
    static constexpr type make( Collection<> const&, Rest const&... rest )
    { return MakeCollection< Rest... >::make( rest... ); }
};

template< typename LeadingFirst, typename... LeadingRest, typename... Rest >
struct MakeCollection< Collection< LeadingFirst, LeadingRest... >, Rest... >
{  
    using tail_maker = MakeCollection< Collection< LeadingRest... >, Rest... >;
    using maker = MakeCollection< LeadingFirst, typename tail_maker::type >;
    using type = maker::type;

    static constexpr type make( 
        Collection< LeadingFirst, LeadingRest... > const& first, 
        Rest const&... rest )
    { 
        auto tail = tail_maker::make( first.rest(), rest... );
        return maker::make( first.first(), tail ); 
    }
};




} // namespace geometry
namespace std {
template< typename... Ts >
struct tuple_size< geometry::Collection< Ts... >>: 
    integral_constant< size_t, sizeof...( Ts )> { };

template< size_t I, typename... Ts >
struct tuple_element< I, geometry::Collection< Ts... >>
{ using type = tuple_element< I, tuple< Ts... >>::type; };

template< typename U, typename T, typename... Ts >
constexpr U const& get( geometry::Collection< T, Ts... > const& col )
{
    if constexpr( is_same_v< U, T > )
        return col.first();
    else
        return get< U >( col.rest() );
}

template< typename U, typename T, typename... Ts >
constexpr U& get( geometry::Collection< T, Ts... >& col )
{
    if constexpr( is_same_v< U, T > )
        return col.first();
    else
        return get< U >( col.rest() );
}

template< size_t I, typename T, typename... Ts >
requires( isless( I, 1+sizeof...( Ts )))
constexpr tuple_element_t< I, tuple< T, Ts... >> const&
get( geometry::Collection< T, Ts... > const& col )
{ 
    if constexpr( I == 0 )
        return col.first();
    else
        return get< I-1 >( col.rest() );
}

template< size_t I, typename T, typename... Ts >
requires( isless( I, 1+sizeof...( Ts )))
constexpr tuple_element_t< I, tuple< T, Ts... >>&
get( geometry::Collection< T, Ts... >& col )
{ 
    if constexpr( I == 0 )
        return col.first();
    else
        return get< I-1 >( col.rest() );
}
} // namespace std
namespace geometry {

using std::get;


//////////////
// attributes
struct Named 
{ 
    constexpr string const& name() const
    { return _name; }

    string _name; 
};
struct Boundary 
{ 
    constexpr int count() const
    { return _count; }

    int _count; 
};

template< typename ObjT, typename AttributeT >
struct Attribution
{
    using object_type = ObjT;
    using space_type = space_of< object_type >;
    using boundary_type = Attribution< 
        typename object_type::boundary_type, AttributeT >;
    using attribute_type = AttributeT;

    constexpr object_type object() const
    { return _object; }

    constexpr attribute_type attribute() const
    { return _attribute; }

    constexpr boundary_type boundary() const
    { return { object().boundary(), attribute() }; }

    Attribution( object_type object, attribute_type attribute ):
        _object{ object }, _attribute{ attribute }
    { }

    object_type _object;
    attribute_type _attribute;
};

/// @brief an oriented object
/// @tparam ObjectT the type of the object
template< typename ObjT >
struct Orientation
{
    using object_type = ObjT;
    using space_type = space_of< object_type >;
    using boundary_type = Orientation< typename object_type::boundary_type >;
    using vector_type = space_type::vector_type;

    constexpr object_type object() const
    { return _object; }

    constexpr vector_type orientation() const
    { return _orientation; }

    constexpr boundary_type boundary() const
    { return { object().boundary(), orientation() }; }

    Orientation( object_type object, vector_type orientation ):
        _object{ object }, _orientation{ orientation }
    { }

    object_type _object;
    vector_type _orientation;
};

////////////////////////////
/// space helper classes ///
////////////////////////////

template< typename T, typename U >
struct ExtrudeSpace;

template< typename U >
struct ExtrudeSpace< Space< Tensor< Shape<> >>, U >
{
    using unit_type = U;
    using vector_type = Tensor< Shape< 1 >, unit_type >;
    using matrix_type = Tensor< Shape< 1, 1 >, unit_type >;
    using space_type = Space< vector_type >;

    static constexpr vector_type base( Tensor< Shape<> > )
    { return { static_cast< unit_type >( 0 ) }; }

    static constexpr vector_type extruded( Tensor< Shape<> >, unit_type u )
    { return { u }; }
};

template< size_t D, typename... Ts, typename U >
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
    { return vector_helper( v, static_cast< U >( 0 ), make_seq< D >{} ); }

    static constexpr vector_type extruded(
        Tensor< Shape< D >, Ts... > const& v, U u )
    { return vector_helper( v, u, make_seq< D >{} ); }
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

template< typename ObjectT >
struct LinearTransformation
{
    using object_type = ObjectT;
    using space_type = space_of< object_type >;
    static constexpr size_t dim = dimensions_of_v< space_type >;
    using matrix_type = uniform_tensor_t< Shape< dim, dim >, Scalar >;
    using boundary_type = LinearTransformation< typename object_type::boundary_type >;

    constexpr object_type const& object() const { return _object; }
    constexpr object_type& object() { return _object; }
    constexpr matrix_type const& transformation() const { return _transform; }
    constexpr matrix_type& transformation() { return _transform; }

    constexpr LinearTransformation( object_type const& obj, matrix_type const& mat ):
        _object{ obj }, _transform{ mat }
    { }
    constexpr LinearTransformation(): _object{}, _transform{} { }

    object_type _object;
    matrix_type _transform;
};

template< typename ObjectT >
struct Translation
{
    using object_type = ObjectT;
    using space_type = space_of< object_type >;
    static constexpr size_t dim = dimensions_of_v< space_type >;
    using vector_type = space_type::vector_type;
    using boundary_type = Translation< typename object_type::boundary_type >;

    constexpr object_type const& object() const { return _object; }
    constexpr object_type& object() { return _object; }
    constexpr vector_type const& translation() const { return _transl; }
    constexpr vector_type& translation() { return _transl; }

    constexpr Translation( object_type const& obj, vector_type const& mat ):
        _object{ obj }, _transl{ mat }
    { }
    constexpr Translation(): _object{}, _transl{} { }

    object_type _object;
    vector_type _transl;
};

template< typename ObjectT, typename U >
struct Projection
{
    using object_type = ObjectT;
    using unit_type = U;
    using extrude_space = ExtrudeSpace< space_of< object_type >, unit_type >;
    using space_type = extrude_space::space_type;
    using boundary_type = Projection< typename ObjectT::boundary_type, U >;
    using vector_type = extrude_space::vector_type;

    constexpr object_type const& object() const { return _object; }
    constexpr object_type&       object()       { return _object; }
    constexpr unit_type   const& amount() const { return _amount; }
    constexpr unit_type&         amount()       { return _amount; }

    constexpr boundary_type boundary() const
    { return { object().boundary(), amount() }; }

    constexpr Projection( object_type object, unit_type amount = 
        static_cast< unit_type >( 0 )): _object{ object }, _amount{ amount } 
    { }

    object_type _object;
    unit_type _amount;
};

template< typename ObjT >
requires( isgreater( dimensions_of_v< space_of< ObjT >>, 0 ))
struct Intrusion
{
    using object_type = ObjT;
    using helper_type = IntrudedSpace< space_of< object_type >>;
    static constexpr size_t dim = dimensions_of_v< space_of< object_type >> - 1;

    constexpr object_type const& object() const { return _object; }
    constexpr object_type& object() { return _object; }

    Intrusion( object_type const& obj ): _object{ obj } { }

    object_type _object;
};

// template< typename SpaceT, typename ObjT, unit... Us >
// Projection< ObjT, SpaceT > project( ObjT object, Us... offsets )
// { return { project, offsets... }; }

// forward decl
template< typename ObjT, typename U, size_t Steps >
struct Extrusion;

template< typename ObjT, typename U, size_t Steps >
struct ExtrusionBase: Orientation< Projection< ObjT, U >>
{ 
    using object_type = ObjT;
    using unit_type = U;
    using step_values_type = std::array< unit_type, Steps >;
    static constexpr size_t dim = dimensions_of_v< space_of< ObjT >>;
    using helper_type = ExtrudeSpace< space_of< ObjT >, U >;
    using base_vector_type = space_of< ObjT >::vector_type;

    ExtrusionBase( object_type const& object, step_values_type const& step_values ):
        Orientation< Projection< object_type, U >>{{ object }, 
            helper_type::extruded( base_vector_type{}, -sum( step_values ) )}
    { }
};

template< typename ObjT, typename U, size_t Steps >
struct ExtrusionCap: Orientation< Projection< ObjT, U >>
{ 
    using object_type = ObjT;
    using unit_type = U;
    using step_values_type = std::array< unit_type, Steps >;
    static constexpr size_t dim = dimensions_of_v< space_of< ObjT >>;
    using helper_type = ExtrudeSpace< space_of< ObjT >, U >;
    using base_vector_type = space_of< ObjT >::vector_type;

    ExtrusionCap( object_type const& object, step_values_type const& step_values ):
        Orientation< Projection< object_type, U >>{{ object, sum( step_values ) }, 
            helper_type::extruded( base_vector_type{}, sum( step_values ))}
    { }
};

template< typename ObjT, typename U, size_t Steps = 1 >
struct Extrusion
{
    using object_type = ObjT;
    using unit_type = U;
    using step_values_type = std::array< unit_type, Steps >;
    using helper_type = ExtrudeSpace< space_of< object_type >, unit_type >;
    using space_type = helper_type::space_type;

    using base_type = ExtrusionBase< ObjT, U, Steps >;
    using cap_type = ExtrusionCap< ObjT, U, Steps >;
    using shell_type = Extrusion< typename ObjT::boundary_type, unit_type, Steps >;

    using boundary_type = Collection< base_type, cap_type, shell_type >;

    static constexpr size_t steps() { return Steps; }

    object_type const& object() const 
    { return _object; }
    step_values_type const& step_values() const 
    { return _step_values; }

    base_type base() const 
    { return { object(), step_values() }; }
    cap_type cap() const 
    { return { object(), step_values() }; }
    shell_type shell() const 
    { return { object().boundary(), step_values() }; }

    step_values_type& step_values() 
    { return _step_values; }
    constexpr U step_from( size_t step ) const
    {
        U value = {};
        for( size_t i = 0; i < step; ++i )
            value += step_values()[ i ];
        return value;
    }
    constexpr U step_to( size_t step ) const
    {
        U value = {};
        for( size_t i = 0; i <= step; ++i )
            value += step_values()[ i ];
        return value;
    }
    unit_type amount() const
    { return sum( step_values() ); }
    object_type& object() 
    { return _object; }

    constexpr boundary_type boundary() const
    { return { base(), cap(), shell() }; }

    constexpr Extrusion( object_type const& object, step_values_type const& step_values ): 
        _object{ object }, _step_values{ step_values }
    { }

    constexpr Extrusion( object_type const& object, unit_type const& amount ): 
        _object{ object }, _step_values{ divisions< steps() >( amount )}
    { }

    constexpr Extrusion(): 
        _object{ }, _step_values{} 
    { }

    object_type _object;
    step_values_type _step_values;
};

template< size_t Step, typename ExtrusionT >
struct ExtrusionStep;

/// @brief represents a single step of an extrusion
template< size_t Step, typename ObjT, typename U, size_t Steps >
requires( isless( Step, Steps ))
struct ExtrusionStep< Step, Extrusion< ObjT, U, Steps >>
{
    using boundary_type = Extrusion< typename ObjT::boundary_type, U, 1 >;
    using extrusion_type = Extrusion< ObjT, U, Steps >;
    using space_type = space_of< extrusion_type >;
    static constexpr size_t step() { return Step; }

    constexpr U from() const
    { return extrusion().step_from( Step ); }

    constexpr U to() const
    { return extrusion().step_to( Step ); }

    constexpr extrusion_type const& extrusion() const
    { return _extrusion; }

    constexpr ExtrusionStep( extrusion_type const& ext ):
        _extrusion{ ext }
    { }
    constexpr ExtrusionStep(): _extrusion{} { }

    extrusion_type _extrusion;
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
 * attribute > extrude > project > orient > collection << boundary
 * 
 * subdivide( project(...) )   => project( subdivide(...) )
 * subdivide( orient(...) )    => orient( subdivide(...) )
 * subdivide( collection(...)) => collection( subdivide(...) )
 * subdivide( extrude(...) )   => collection( project( extrude(...)) ... )
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
 * orient( collection(...) )   
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

template< size_t... Is >
struct IndexSelector
{
    template< size_t I, typename >
    struct selector: integral_constant< bool, (( I == Is ) or ... ) > { };
};

template< size_t Step, typename ObjT, typename U, size_t Steps >
requires( isless( Step, Steps ))
ExtrusionStep< Step, Extrusion< ObjT, U, Steps >>
extrusion_step( Extrusion< ObjT, U, Steps > const& ext )
{ return { ext }; }

template< template< size_t, typename > class Selector, typename CollectionT, size_t... Is >
auto collection_select_helper( CollectionT const& col, seq< Is... > )
{
    return MakeCollection< std::conditional_t< 
        Selector< Is, tuple_element_t< Is, CollectionT >>::value,
        tuple_element_t< Is, CollectionT >,
        Ignored< tuple_element_t< Is, CollectionT >>>... >::make( 
            get< Is >( col )... );
}

/// @brief extract components using predicate into new collection
template< template< size_t, typename > class Selector, typename... Objects >
auto select( Collection< Objects... > const& col )
{ return collection_select_helper< Selector >( col, make_seq< sizeof...( Objects )>{} ); }

template< template< size_t, typename > class Selector, typename ExtrusionT, size_t... Is >
auto extrusion_select_helper( ExtrusionT const& ext, seq< Is... > )
{    
    return MakeCollection< std::conditional_t< 
        Selector< Is, ExtrusionStep< Is, ExtrusionT >>::value,
        ExtrusionStep< Is, ExtrusionT >,
        Ignored< ExtrusionStep< Is, ExtrusionT >>>... >::make(
            extrusion_step< Is >( ext )... );
}

/// @brief 
template< template< size_t, typename > class Selector, 
    typename ObjT, typename U, size_t Steps >
auto select( Extrusion< ObjT, U, Steps > const& ext )
{ return extrusion_select_helper< Selector >( ext, make_seq< Steps >{} ); }

/// @brief tag an object with an attribute
/// @tparam ObjectT object type
/// @tparam T attribute type
/// @param object instance
/// @param attribute value
/// @return an attributed object
template< typename ObjT, typename AttT >
constexpr Attribution< ObjT, AttT > attribute( ObjT const& object, 
    AttT const& attribute )
{ return { object, attribute }; }

template< typename AttT, typename ObjT >
constexpr AttT get_attribute( ObjT const& object )
{ return { }; }

// found the attribute
template< typename AttT, typename ObjT >
constexpr AttT get_attribute( Attribution< ObjT, AttT > const& attributed )
{ return attributed.attribute(); }

// found a different attribute
template< typename AttT, typename ObjT, typename OtherT >
requires( not is_same_v< AttT, OtherT > )
constexpr AttT get_attribute( Attribution< ObjT, OtherT > const& attributed )
{ return get_attribute< AttT >( attributed.object() ); }

template< typename AttT, typename ObjT >
constexpr AttT get_attribute( Orientation< ObjT > const& oriented )
{ return get_attribute< AttT >( oriented.object() ); }

template< typename AttT, typename ObjT, typename U >
constexpr AttT get_attribute( Projection< ObjT, U > const& projection )
{ return get_attribute< AttT >( projection.object() ); }

template< typename AttT, typename ObjT, typename U, size_t Steps >
constexpr AttT get_attribute( Extrusion< ObjT, U, Steps > const& extrusion )
{ return get_attribute< AttT >( extrusion.object() ); }

/// @brief orienting an object
/// @tparam ObjectT type of the object
/// @param object instance
/// @param orientation vector
/// @return an object + orientation pair
template< typename ObjectT >
Orientation< ObjectT > orient( ObjectT const& object, 
    typename space_of< ObjectT >::vector_type orientation )
{ return { object, orientation }; }

/// @brief re-orienting an object overrides the orientation
/// @tparam ObjectT type of the object
/// @param oriented instance
/// @param orientation vector
/// @return an object + orientation pair
/// TODO: re-orient? 
template< typename ObjT >
Orientation< ObjT > orient( Orientation< ObjT > const& oriented, 
    typename space_of< ObjT >::vector_type orientation )
{ return { oriented.object(), orientation }; }

template< typename ObjT >
vector_for< ObjT > get_orientation( ObjT const& object )
{ return { }; }

template< typename ObjT >
vector_for< ObjT > get_orientation( Orientation< ObjT > const& object )
{ return object.orientation(); }

template< typename ObjT, typename AttT >
vector_for< ObjT > get_orientation( Attribution< ObjT, AttT > const& attributed )
{ return get_orientation( attributed.object() ); }

template< typename ObjT, typename U >
vector_for< ObjT > get_orientation( Projection< ObjT, U > const& projected )
{ return ExtrudeSpace< space_of< ObjT >, U >::base( get_orientation( 
    projected.object() )); }

template< typename ObjT, typename U, size_t Steps >
vector_for< ObjT > get_orientation( Extrusion< ObjT, U, Steps > const& extruded )
{ return ExtrudeSpace< space_of< ObjT >, U >::base( get_orientation( 
    extruded.object() )); }

/// @brief interpolate an object in a new dimension out to amount
/// @tparam ObjT type of object to be extruded
/// @tparam U dimension to interpolate through
/// @param object to be extruded
/// @param amount of extrusion
/// @return the extruded object
template< size_t Steps, typename ObjT, typename U >
Extrusion< ObjT, U, Steps > extrude( ObjT const& object, 
    std::array< U, Steps > const& step_values )
{ return { object, step_values }; }

template< typename ObjT, typename First, typename... Rest >
requires(( is_same_v< First, Rest > and ... ))
Extrusion< ObjT, First, 1 + sizeof...( Rest ) > extrude( ObjT const& object, 
    First const& first, Rest const&... rest )
{ return { object, { first, rest... }}; }

/// @brief projecting an object into an extrude space
/// @tparam ObjectT type of the object
/// @tparam U unit of additional dimension
/// @param object instance
/// @param here position of projection
/// @return an object + unit pair in the extrude space
template< typename ObjectT, typename U >
constexpr Projection< ObjectT, U > project( ObjectT const& object, U here )
{ return { object, here }; }

/// @brief creates a collection of geometric objects which can be operated on
/// together. 
/// @tparam ...Objects types in the collection
/// @param ...objects in the collection
/// @return a collection where any nested collections are enumerated into the 
/// returned collection
template< typename... Objects >
typename MakeCollection< Objects... >::type 
collection( Objects const&... objects )
{ return MakeCollection< Objects... >::make( objects... ); }

/////////////////////////////
// projection specializations

// /// @brief projection of an orientation is the orientation of a projection
// /// @tparam ObjectT oriented object type
// /// @tparam U projected location
// /// @param oriented object to be projected
// /// @param here projection location
// /// @return an oriented, projected object
// template< typename ObjectT, typename U >
// constexpr Oriented< Projected< ObjectT, U >> 
// project( Oriented< ObjectT > oriented, U here )
// {  
//     using extruded_space = ExtrudeSpace< space_of< ObjectT >, U >;
//     return orient( project( oriented.object(), here ),
//         extruded_space::extruded( oriented.orientation(), here ));
// }

// /// @brief projection of an attributed object is an attributed projection
// /// @tparam ObjT type of the attributed object
// /// @tparam T attribute type
// /// @tparam U projection type
// /// @param obj attributed object instance
// /// @param amount projection amount
// /// @return an attributed projection
// template< typename ObjT, typename T, typename U >
// constexpr auto project( Attributed< ObjT, T > obj, U amount )
// { return attribute( project( obj.object(), amount ), obj.attribute() ); }

// template< typename CollectionT, typename U, size_t... Is >
// constexpr auto collection_project_helper( CollectionT col, U here, 
//     seq< Is... > )
// { return collection( project( get< Is >( col ), here )... ); }

// /// @brief projection of a collection is the collection of the projected objects
// /// @tparam ...Objects object types to be projected
// /// @tparam U projection unit
// /// @param col collection of objects
// /// @param here amount of projection
// /// @return a collection of projected objects
// template< typename... Objects, typename U >
// constexpr auto project( Collection< Objects... > col, U here )
// { return collection_project_helper( col, here, 
//     make_seq< sizeof...( Objects )>{} ); }


//////////////////////////
// extrude specializations

/// @brief extrusion of an oriented object is an orientation of the extrusion
/// of the oriented object.
/// @tparam ObjT the oriented object type
/// @tparam U the unit of extrusion
/// @param oriented the oriented object
/// @param amount thee amount to be extruded
/// @return the extruded object
template< size_t Steps, typename ObjT, typename U >
constexpr Orientation< Extrusion< ObjT, U, Steps >> extrude( 
    Orientation< ObjT > const& oriented, 
    std::array< U, Steps > const& step_values )
{
    using extrusion_space = ExtrudeSpace< space_of< ObjT >, U >;
    
    return orient( extrude< Steps >( oriented.object(), step_values ), 
        extrusion_space::base( oriented.orientation() ));
}

template< size_t Step, typename ObjT, typename U, size_t Steps >
constexpr auto extrusion_step( Orientation< Extrusion< ObjT, U, Steps >> const& ori )
{ 
    using extrusion_space = ExtrudeSpace< space_of< ObjT >, U >;

    return orient( extrusion_step< Step >( ori.object() ), 
        ori.orientation() );
        // extrusion_space::base( ori.orientation() )); 
}

// template< typename Steps, typename ObjT, typename AttT, typename U >
// Extrusion< Attribution< ObjT, AttT >, U, Steps > extrude( Attribution< ObjT, AttT > const& attributed, 
//     std::array< U, Steps > const& step_values )
// { return { attributed, step_values }; }

// template< typename ObjT, typename U, typename V, size_t Steps >
// Extrusion< Projection< ObjT, U >, V, Steps > extrude( 
//     Projection< ObjT, U > const& projected, 
//     std::array< U, Steps > const& step_values )
// { return { projected, step_values }; }

// /// @brief extrusion of an attributed object is an attributed extrusion
// /// @tparam ObjT type of the attributed object
// /// @tparam T attribute type
// /// @tparam U extrusion type
// /// @param obj attributed object instance
// /// @param amount extrusion amount
// /// @return an attributed extrusion
// template< typename ObjT, typename T, typename U >
// constexpr auto extrude( Attributed< ObjT, T > obj, U amount )
// { return attribute( extrude( obj.object(), amount ), obj.attribute() ); }

template< size_t Steps, typename CollectionT, typename U, size_t... Is >
auto collection_extrude_helper( CollectionT col, U amount, seq< Is... > )
{ return collection( extrude< Steps >( get< Is >( col ), amount )... ); }

/// @brief the extrusion of a collection is a collection of extrusions
/// @tparam ...Objects the object types in the collection
/// @tparam U the unit of extrusion
/// @param collection the collection of objects
/// @param amount the amount to be extruded
/// @return a collection of extruded objects
template< size_t Steps, typename... Objects, typename U >
auto extrude( Collection< Objects... > const& col, 
    std::array< U, Steps > const& amount )
{ return collection_extrude_helper< Steps >( col, amount, 
    make_seq< sizeof...( Objects )>{} ); }

template< size_t Step, typename CollectionT, size_t... Is >
auto collection_extrusion_step_helper( CollectionT const& col, seq< Is... > )
{ return collection( extrusion_step< Step >( get< Is >( col ))... ); }

template< size_t Step, typename... Objects >
auto extrusion_step( Collection< Objects... > const& col )
{ return collection_extrusion_step_helper< Step >( col, 
    make_seq< sizeof...( Objects )>{} ); }


/// @brief linear transformation
template< typename ObjT >
constexpr LinearTransformation< ObjT > 
transform_linear( ObjT const& obj, 
    typename LinearTransformation< ObjT >::matrix_type const& transform )
{ return { obj, transform }; }

template< typename MatT, size_t Axis0, size_t Axis1 >
constexpr MatT rotate_plane_matrix( Scalar angle )
{
    using shape_type = MatT::shape_type;
    auto rot = identity_matrix< MatT >();
    
    tensor_get< shape_type( Axis0, Axis0 )>( rot ) = cos( angle );
    tensor_get< shape_type( Axis0, Axis1 )>( rot ) = -sin( angle );
    tensor_get< shape_type( Axis1, Axis0 )>( rot ) = sin( angle );
    tensor_get< shape_type( Axis1, Axis1 )>( rot ) = cos( angle );

    return rot;
}

template< size_t Axis0, size_t Axis1, typename ObjT >
requires( Axis0 != Axis1 and isless( Axis0, dimensions_of_v< space_of< ObjT >> ) 
    and isless( Axis1, dimensions_of_v< space_of< ObjT >> ))
constexpr LinearTransformation< ObjT >
rotate_plane( ObjT const& obj, Scalar angle )
{ 
    using matrix_type = LinearTransformation< ObjT >::matrix_type;

    return transform_linear( obj, 
        rotate_plane_matrix< matrix_type, Axis0, Axis1 >( angle ));
}

template< typename ObjT >
constexpr Translation< ObjT >
translate( ObjT const& obj, typename space_of< ObjT >::vector_type const& by )
{ return { obj, by }; }

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
constexpr auto boundary( Attribution< ObjT, T > attributed )
{ return attribute( boundary( attributed.object() ), attributed.attribute() ); }

/// @brief boundary of an extrusion is a collection including the 
/// @tparam ObjT extruded object
/// @tparam U unit of extrusion
/// @param ext extruded object
/// @return the boundary of an extruded object
template< typename ObjT, typename U, size_t Steps >
// requires( isgreater( dimensions_of_v< space_of< ObjT >>, 0 ))
constexpr auto boundary( Extrusion< ObjT, U, Steps > ext )
{ 
    using extrusion_space = ExtrudeSpace< space_of< ObjT >, U >;
    using vector_type = extrusion_space::vector_type;

    static constexpr size_t dim = dimensions_of_v< space_of< ObjT >>;
    vector_type extrusion_direction, intrusion_direction;
    get< dim >( extrusion_direction ) = static_cast< U >( 1 );
    get< dim >( intrusion_direction ) = static_cast< U >( -1 );

    return collection( 
        orient( project( ext.object(), static_cast< U >( 0 )), extrusion_direction ),      // proxinal
        orient( project( ext.object(), ext.amount() ), intrusion_direction ),              // distal
        extrude< Steps >( boundary( ext.object() ), ext.step_values() )); // lateral
}

template< typename ObjT >
constexpr Collection< > boundary( Ignored< ObjT > const& )
{ return {}; }

template< size_t Step, typename ObjT, typename U, size_t Steps >
constexpr auto boundary( ExtrusionStep< Step, Extrusion< ObjT, U, Steps >> const& ext_step )
{
    using extrusion_type = Extrusion< ObjT, U, Steps >;
    using extrusion_space = space_of< extrusion_type >;
    using vector_type = extrusion_space::vector_type;

    static constexpr size_t dim = dimensions_of_v< space_of< ObjT >>;
    vector_type extrusion_direction, intrusion_direction;
    get< dim >( extrusion_direction ) = static_cast< U >( 1 );
    get< dim >( intrusion_direction ) = static_cast< U >( -1 );

    extrusion_type const& ext = ext_step.extrusion();
    U from = ext_step.from(), to = ext_step.to();

    return collection(
        orient( project( ext.object(), from ), extrusion_direction ),
        orient( project( ext.object(), to ), intrusion_direction ),
        extrusion_step< Step >( extrude< Steps >( 
            boundary( ext.object() ), ext.step_values() )));
}

template< typename ObjT >
constexpr auto boundary( LinearTransformation< ObjT > const& transformed )
{ return transform_linear( boundary( transformed.object() ), 
    transformed.transformation() ); }

template< typename ObjT >
constexpr auto boundary( Translation< ObjT > const& translated )
{ return translate( boundary( translated.object() ), 
    translated.translation() ); }

template< size_t FirstStep, size_t... Rest >
struct ExtrudedSurface
{
    // since the type tracks the element, the implementation can store the traversal
    template< typename ObjT, typename U, size_t Steps >
    requires( isless( FirstStep, Steps ))
    static constexpr auto select( Extrusion< ObjT, U, Steps > const& extrusion )
    {
        U from, to = extrusion.step_values()[ FirstStep ];
        if constexpr( FirstStep > 0 )
            from = extrusion.step_values()[ FirstStep - 1 ];
        
        
    }
};


/// @brief pad an element of a geometric shape
template< typename SurfaceElement, typename ObjT, typename U, size_t Steps, typename V >
auto pad( Extrusion< ObjT, U, Steps > const& extrusion, V amount )
{
    // first we identify the element possibly through recursion
    // Element is a type and types contain the information needed to identify 
    // the geometric piece to be padded. So it's two types talking to one another.
    

    // then we mimic an extrusion by finding the boundary of the padded element
    // and extruding it in the direction of the orientation of the extruded
    // element, and projecting the extruded element by amount
    auto surface_element = SurfaceElement::select( extrusion );

    // extrude won't work here becuase it creates a new dimension
    auto padded_surface_element = extrude( surface_element, amount );

    return collection( extrusion, padded_surface_element );
}

/// @brief boundary of a projection is a projection of the boundary
/// @tparam ObjT projected object type
/// @tparam U projection unit
/// @param ext projected object
/// @return boundary of the projected object
template< typename ObjT, typename U >
constexpr auto boundary( Projection< ObjT, U > pro )
{ return project( boundary( pro.object() ), pro.amount() ); }

/// @brief boundary of an oriented object is the orientation of the boundary
/// @tparam ObjT oriented object type
/// @param ori oriented object
/// @return the boundary of the oriented object re-oriented
template< typename ObjT >
constexpr auto boundary( Orientation< ObjT > ori )
{ return orient( boundary( ori.object() ), ori.orientation() ); }

/// @brief helper function for calculating the boundary of a collection
/// @tparam CollectionT collection
/// @tparam ...Is index sequence to elements in the collection
/// @param collection collection
/// @return 
template< typename CollectionT, size_t... Is >
constexpr auto collection_boundary_helper( CollectionT col, 
    seq< Is... > )
{ return collection( boundary( get< Is >( col ))... ); }

/// @brief the boundary of a collection is a collection of all the boundaries
/// @tparam ...Objects
/// @param collection 
/// @return 
template< typename... Objects >
constexpr auto boundary( Collection< Objects... > col )
{ return collection_boundary_helper( col, 
    make_seq< sizeof...( Objects )>{} ); }

///////////////////
/// translation ///
///////////////////

// template< typename ObjT >
// auto translate( ObjT obj, vector_for< ObjT > rel );

// template< typename ObjT >
// requires( dimensions_of_v< space_of< ObjT >> == 0 )
// ObjT translate( ObjT obj, null_tensor_t )
// { return obj; }

// template< typename ObjT, typename U, size_t Steps >
// auto translate( Extrusion< ObjT, U, Steps > proj, 
//     vector_for< Projection< ObjT, U >> rel )
// {
//     // get the dimensions of the projected object
//     // this will also be the index to the offset in rel
//     static constexpr size_t dim = dimensions_of_v< space_of< ObjT >>;

//     // adjust the projection by the last dimension of our relative vector
//     // then recurse
//     return project( translate( proj.object(), element_subtensor< dim >( rel )), 
//         proj.amount() + tensor_get< dim >( rel ));
// }

///////////
/// pad ///
///////////

/// @brief construct a prism from the object along the orientation by the 
/// specified amount.  This is different than an extrusion because a pad does 
/// not increase the dimensionality of the space of the object.
/// @tparam ObjT oriented object type
/// @tparam U distance of the pad
/// @param object to be padded
/// @param amount pad distance
template< typename ObjT, typename U >
// TODO: write requirements on U
// requires( norm_compable_v< typename Oriented< ObjT >::vector_type, U > )
constexpr auto pad( Orientation< ObjT > object, U amount )
{
    auto direction = object.orientation();

    // calculate the distance vector
    // we want the final distance along direction to be amount
    auto rel = scale( divide_scale( direction, norm( direction )), amount );

    return collection( translate( object, rel ), 
        extrude_along( boundary( object ), rel ));
}

template< typename U >
Extrusion< Point, U > segment( U length )
{ return { {}, length }; }

template< typename U >
Extrusion< Extrusion< Point, U >, U > rectangle( U length, U width )
{ return {{ {}, length }, width }; }

template< typename U >
Attribution< Extrusion< Extrusion< Extrusion< Point, U >, U >, U >, Named >
box( U length, U width, U height )
{ return {{{{ {}, length }, width }, height }, { "box" }}; }

/*

template< typename ObjT >
struct Padded 
{ 

};

template< typename ObjT >
Padded< ObjT > pad( ObjT object );

*/

} // namespace geometry

/////////
// output

namespace formats {

using namespace geometry;

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

#endif // __GEOMETRY_HPP__