#include "units.hpp"
#include "tensors.hpp"
// #include "expressions.hpp"

#include <print>

using namespace units;
using namespace tensors;

// how do we setup vectors?
template< unit U >
struct Space 
{
    using unit_type = U;
};

template< typename T >
struct SpaceOf
{ using type = T::space_type; };

template< typename T >
using space_of = SpaceOf< T >::type;

template< typename T, typename U >
struct SpaceQuotient
{ using type = Space< 
    unit_quotient< typename T::unit_type, typename U::unit_type >>; };

template< typename T, typename U >
using space_quotient_t = SpaceQuotient< T, U >::type;

using null_space = Space<>;

template< typename T, typename U >
struct SpaceProduct
{ using type = Space< 
    unit_product< typename T::unit_type, typename U::unit_type >>; };

template< typename T, typename U >
using space_product_t = SpaceProduct< T, U >::type;

// a zero dimensional geometric element
struct Point
{ 
    using space_type = null_space;
    // all zero dimensional
};

template< typename ObjT, typename ComponentT >
struct HasComponent: integral_constant< bool, false > { };

template< typename ObjT, typename ComponentT >
constexpr bool has_component_v = HasComponent< ObjT, ComponentT >::value;

// forward decl
template< typename ObjT, typename SpaceT >
struct Projection;

// projections have the same components as their base object
template< typename ObjT, typename SpaceT, typename ComponentT >
struct HasComponent< Projection< ObjT, SpaceT >, ComponentT >: 
    integral_constant< bool, has_component_v< ObjT, ComponentT >> { };

template< typename SpaceT, typename ObjT, typename OffsetT >
Projection< ObjT, SpaceT > project( ObjT object, OffsetT offset );

template< typename ObjT, typename SpaceT >
struct Projection
{
    using object_type = ObjT;
    using space_type = SpaceT;
    using offset_type = space_quotient_t< SpaceT, space_of< ObjT >>;

    template< typename ComponentT >
    requires( has_component_v< object_type, ComponentT > )
    auto operator[]( ComponentT component )
    { return project< space_type >( _base[ component ], _offset ); }

    offset_type offset()
    { return _offset; }

    Projection( object_type object, offset_type offset ): 
        _base{ object }, _offset{ offset } { }
    Projection( ): 
        _base { }, _offset{ } { }

    object_type _base;
    offset_type _offset;
};

template< typename SpaceT, typename ObjT, typename OffsetT >
Projection< ObjT, SpaceT > project( ObjT object, OffsetT offset )
{ return { project, offset }; }

// forward decl
template< typename ObjT, unit U >
struct Extrusion;

template< typename ObjT, unit U, typename ComponentT >
struct HasComponent< Extrusion< ObjT, U >, ComponentT >: 
    integral_constant< bool, has_component_v< ObjT, ComponentT >> { };

template< typename ObjT, unit U = units::Length >
Extrusion< ObjT, U > extrude( ObjT object, U amount );

template< typename ObjT, unit U >
struct Extrusion
{
    using object_type = ObjT;
    using unit_type = U;
    using space_type = space_product_t< space_of< ObjT >, Space< unit_type >>;
    enum component_type: size_t 
    { base = 0, extruded };

    auto operator []( component_type component )
    { return project< space_type >( _base, component == base ? 0 : _amount ); }

    template< typename ComponentT >
    requires( has_component_v< ObjT, ComponentT > )
    auto operator []( ComponentT base_component )
    { return extrude< unit_type >( _base[ base_component ], _amount ); }

    unit_type amount()
    { return _amount; }

    constexpr Extrusion( object_type object, unit_type amount ): 
        _base{ object }, _amount{ amount } { }
    constexpr Extrusion(): _base{ }, _amount{ 0 } { }

    object_type _base;
    unit_type _amount;
};

template< typename ObjT, unit U >
Extrusion< ObjT, U > extrude( ObjT object, U amount )
{ return { object, amount }; }

template< unit U = units::Length >
struct Segment: Extrusion< Point, U >
{
    
};

template< unit U = units::Length >
using Rectangle = Extrusion< Segment< U >, U >;

template< unit U = units::Length >
struct Box: Extrusion< Rectangle< U >, U >
{

};

template< typename ObjT >
struct Padded 
{ 

};

template< typename ObjT >
Padded< ObjT > pad( ObjT object );

void mortise_and_tenon()
{
    Box stile;
    Box rail;

    // auto tenon_layout = Grid< 3, 3 >{};
    // auto mortise_layout = Grid< 3, 3 >{};

    // auto rail_end = component< Box::Top >( rail );
    // auto stile_inside = component< Rectangle::Bottom >( stile );

    // auto tenon = pad( component< grid_cell< 1, 1 >>( 
    //     tenon_layout( rail_end )));
    // auto mortise = pad( component< grid_cell< 1, 1 >>( 
    //     mortise_layout ( stile_inside )));

    
}

int main( int ac, char* av[] )
{
    


    return 0;
}