#ifndef __COMPOUNDS_HPP__
#define __COMPOUNDS_HPP__

#include "universe.hpp"
#include "units.hpp"

#include <string>

using std::string;

template< typename Object >
auto boundary_of( pointer_to< Object > ptr );

template< typename... ObjectTypes >
requires in_same_domain< ObjectTypes... >::value
struct CompoundOf : virtual Object
{
    using compound_tuple = std::tuple< pointer_to< ObjectTypes >... >;
    template< size_t I >
    using element_t = std::tuple_element_t< I, compound_tuple >;

    template< size_t I >
    element_t< I > at() { return std::get< I >( elements ); }

    CompoundOf( pointer_to< ObjectTypes >... objs ) : elements{ objs... } { }

    compound_tuple elements;
};

template< typename... ObjectTypes >
pointer_to< CompoundOf< ObjectTypes... >> compound_of( pointer_to< ObjectTypes >... objs )
{ return { objs... }; }

namespace detail
{
    template< typename CompoundType, size_t... Is >
    auto boundary_of_compound_helper( CompoundType compound, std::index_sequence< Is... > )
    { return compound_of( boundary_of( compound.template at< Is >()... )); }
}

template< typename... ObjectTypes >
auto boundary_of( pointer_to< CompoundOf< ObjectTypes... >> extruded )
{ return detail::boundary_of_compound_helper( extruded, 
    std::make_index_sequence< sizeof...( ObjectTypes )>{} ); }


template< typename ObjectType, unit Unit >
struct Extruded : virtual CompoundOf< ObjectType >
{
    using compound_type = CompoundOf< ObjectType >;
    using domain_type = domain_append< domain_of< ObjectType >, Unit >;
    enum : size_t { extruded_object = 0 };
    enum : size_t { extruded_amount = 0 };

    // the extruded object
    auto object() { return at< extruded_object >(); }

    // our predicate
    long double operator()( domain_type const& x )
    {
        // first cast down to the domain of our extruded type, and test our
        // wrapped object
        if( object()( (domain_of< ObjectType >)x ) == 0 )
            return 0;

        // TODO: get rid of the "template" keyword required here since eval is a dependent template
        Unit a = amount.template eval< extruded_amount >();
        Unit t = x.last();

        if( t <= 0 && t >= a ) return -1;
        if( t >= 0 && t <= a ) return 1;
        return 0;
    }

    Extruded( pointer_to< ObjectType > obj ) : compound_type{ obj } { }
    
    Dimensions< Unit > amount;

};

template< unit Unit, typename ObjectType >
pointer_to< Extruded< ObjectType, Unit >> extrude( pointer_to< ObjectType > obj )
{ return create_in< Extruded< ObjectType, Unit >>( obj.get_universe(), obj ); }

template< typename ObjectType, unit Unit >
auto boundary_of( pointer_to< Extruded< ObjectType, Unit >> extruded )
{
    // extrude the boundary of the extruded object by Unit
    auto ret = extrude< Unit >( boundary_of( extruded.object() ));
    // link the dimensions together
    ret->amount = reference_to( extruded.amount );
    return ret;
}

template< typename ObjectType >
struct Positioned : virtual CompoundOf< ObjectType >
{
    using domain_type = domain_of< ObjectType >;
    using dimensions_type = dimensions_for< domain_type >;
    using compound_type = CompoundOf< ObjectType >;

    enum : size_t { positioned_object = 0 };

    // the extruded object
    auto object() { return at< positioned_object >(); }

    // our predicate
    long double operator()( domain_type x )
    { 
        // subtract our position from x and test the positioned object
        return object()( x -= position() ); 
    }

    Positioned( pointer_to< ObjectType > obj ) : compound_type{ obj } { }

    dimensions_type position;
};

template< typename ObjectType >
pointer_to< Positioned< ObjectType >> make_positioned( pointer_to< ObjectType > obj )
{ return { obj }; }

template< typename ObjectType >
auto boundary_of( pointer_to< Positioned< ObjectType >> positioned )
{
    // extrude the boundary of the positioned object 
    auto ret = make_positioned( boundary_of( positioned.object() ));
    // link the dimensions together

    ret->position = reference_to( positioned->position );
    return ret;
}


template< typename... ObjectTypes >
struct Array : virtual Object, 
    std::tuple< pointer_to< ObjectTypes >... >
{
    static constexpr const size_t size = sizeof...( ObjectTypes );
    using container_type = std::tuple< pointer_to< ObjectTypes >... >;
    
    template< size_t I > using array_element_t = 
        std::tuple_element_t< I, container_type >;

    template< size_t I >
    array_element_t< I >& at()
    { return get< I >( *this ); }

    Array( pointer_to< ObjectTypes >... ptrs ) : container_type{ ptrs... } { }
};

template< typename... ObjectTypes >
Array< ObjectTypes... > array_of( pointer_to< ObjectTypes >... ptrs )
{ return { ptrs... }; }

template< typename Object >
concept bounded_object = requires( Object obj )
{ 
    typename Object::boundary_type; 
    obj.boundary();
};

template< typename ObjectPtr >
auto boundary_of( ObjectPtr ptr )
{ return ptr->boundary(); }

template< typename ObjectType = void >
struct Named;

template< > 
struct Named< void > : virtual Object
{
#ifdef DEFAULT_OBJECT_NAME
    static constexpr const string default_name = STRINGIZE(DEFAULT_OBJECT_NAME);
#else
    static constexpr const string default_name = "object";
#endif

    Named( string name = default_name ) : name{ name } { }

    string name;
};

template< typename ObjectType >
struct Named : virtual Object
{
    using object_pointer = typename object_traits< ObjectType >::pointer;
    static constexpr const string default_name = Named< void >::default_name;
    
    Named( string name = default_name ) : name{ name } { }

    template< typename ObjectPtr >
    // requires is_convertable_to< object_pointer, ObjectPtr >
    Named( string name, ObjectPtr ptr ) : object{ ptr }, name{ name } { }

    object_pointer object;
    string name; 
};

struct Point : virtual Object
{ };

struct Segment : virtual Extruded< Point, units::Length >
{ 
    using extruded_type = Extruded< Point, units::Length >;

    Segment() : length{ extruded_type::amount }
    { }

    DimensionAlias< units::Length > length;
};

struct Rect : virtual Extruded< Segment, units::Length >
{ };

struct Box : virtual Object 
{ 
    using dimensions_type = Dimensions< units::Length, units::Length, units::Length >;

    Box() { }

    dimensions_type dimensions;
};


#endif