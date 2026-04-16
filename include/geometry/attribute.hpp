#ifndef __GEOMETRY_ATTRIBUTE_HPP__
#define __GEOMETRY_ATTRIBUTE_HPP__

#include "geometry/space.hpp"

namespace geometry {

//////////////////
/// attributes ///
//////////////////

struct Named 
{ 
    static string object_name() { return "named"; }
    constexpr string const& name() const
    { return _name; }

    string _name; 
};

/// Attribute Operation
///

template< typename ObjT, typename AttributeT >
struct Attribution
{
    using object_type = ObjT;
    using space_type = space_of< object_type >;
    using attribute_type = AttributeT;

    static string object_name() { return "attribution__" + 
        AttributeT::object_name() + "__" + object_type::object_name(); }

    object_type object;
    attribute_type attribute;
};

template< typename ObjT, typename AttributeT >
struct IsEmpty< Attribution< ObjT, AttributeT >>
{ static constexpr bool value = IsEmpty< ObjT >::value; };

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

// template< typename AttT, typename ObjT >
// constexpr AttT get_attribute( Orientation< ObjT > const& oriented )
// { return get_attribute< AttT >( oriented.object() ); }

// template< typename AttT, typename ObjT, typename U >
// constexpr AttT get_attribute( Projection< ObjT, U > const& projection )
// { return get_attribute< AttT >( projection.object() ); }


} // namespace geometry 

#endif