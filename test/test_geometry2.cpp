#include "units.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include <cmath>

#include <type_traits>

using std::string;



template< typename ObjectType >
struct DomainOf
{ using type = typename ObjectType::domain_type; };

template< typename ObjectType >
using domain_of = DomainOf< ObjectType >::type;

template< typename UnitType >
struct Dimensions
{
    // ensure our unit is a Product<...>
    using unit_type = units::ProductType< UnitType >::type;

    static constexpr size_t dimensionality = 
        units::Dimensionality< UnitType >::value;

    auto dimensions() const { return *m_dimensions; }
    // auto dimensions( UnitType value )
    // {
    //     m_dimensions = value;
    //     return m_dimensions;
    // }
    template< typename... UnitTypes >
    auto dimensions( units::Space< UnitTypes... > units )
    { return setting_helper( units, 
        std::make_index_sequence< 
            sizeof...( UnitTypes ) < dimensionality ? 
                sizeof...( UnitTypes ) : dimensionality >{} ); }

    template< typename... UnitTypes >
    auto dimensions( UnitTypes... units )
    { return setting_helper( units::Space< UnitTypes... >{{ units... }}, 
        std::make_index_sequence< 
            sizeof...( UnitTypes ) < dimensionality ?
                sizeof...( UnitTypes ) : dimensionality >{} ); }

    Dimensions& operator=( Dimensions const& d )
    {
        m_dimensions = d.m_dimensions;
        return *this;
    }
    Dimensions& operator=( Dimensions&& d )
    {
        m_dimensions = std::move(d.m_dimensions);
        return *this;
    }

    Dimensions() : m_dimensions{ std::make_shared< unit_type >() } { }
    Dimensions( Dimensions const& d ) : m_dimensions{ d.m_dimensions } { }
    Dimensions( Dimensions&& d ) : m_dimensions{ std::move(d.m_dimensions) } { }
private:
    template< typename ProductType, size_t... Is >
    auto setting_helper( ProductType unit, std::index_sequence< Is... > )
    {
        (( std::get< Is >( *m_dimensions ) = std::get< Is >( unit )), ... );
        return *m_dimensions;
    }

    std::shared_ptr< unit_type > m_dimensions;
};

template<>
struct Dimensions<void>
{
    static constexpr size_t dimensionality = 0;
    
    units::Space<> dimensions() const { return {}; }

    template< typename... UnitTypes >
    units::Space<> dimensions( UnitTypes... )
    { return dimensions(); }

};

template< typename ObjectType >
struct Collection
{
    using domain_type = domain_of< ObjectType >;

    std::vector< ObjectType > m_objects;
};

/**
 * Simplex is a collection of faces that approximate the referenced object
 * the inherited dimensions are the maximum error allowed
 */
template< typename ObjectType >
struct Simplex : Dimensions< domain_of< ObjectType >>
{
    using domain_type = domain_of< ObjectType >;

    auto facets();

    Simplex( ObjectType object ) : object{ object } { }

    ObjectType object;
};

template< typename ObjectType >
struct Boundary
{
    // TODO: we need a Quotient or Derivative type of a domain...
    using domain_type = domain_of< ObjectType >;

    ObjectType object;
};

template< typename ObjectType >
Boundary< ObjectType > boundary_of( ObjectType object )
{ return { object }; }

struct Point 
{ 
    using domain_type = units::Space< >;

    long double operator()( domain_type ) const { return 0; }
};

/**
 * Project maps the domain of the ObjectType to DomainType directly
 */
template< typename ObjectType, typename DomainType >
struct Project 
{
    using domain_type = DomainType;

    long double operator()( domain_type x ) const
    { return object( units::project< domain_of< ObjectType >>( x ) ); }

    ObjectType object;
};

template< typename DomainType, typename ObjectType >
Project< ObjectType, DomainType > project( ObjectType object )
{ return { object }; }

template< typename ObjectType >
struct Translate : public Dimensions< domain_of< ObjectType >>
{
    using domain_type = domain_of< ObjectType >;

    long double operator()( domain_type x ) const
    { return object(x - my::dimensions()); }

    Translate( ObjectType object ) : Dimensions< domain_of< ObjectType >>{ },
        object{ object }
    { }

    ObjectType object;

private:
    using my = Dimensions< domain_type >;
};

template< typename ObjectType >
Translate< ObjectType > translate( ObjectType object )
{ return { object }; }

template< typename BaseType, typename UnitType >
struct Extrude : Dimensions< UnitType >
{
    using domain_type = 
        units::ProductType< domain_of< BaseType >, UnitType >::type;

    long double operator()( domain_type x ) const
    {
        auto extruded_dimension = 
            std::get< domain_of< BaseType >::dimensionality >( x );
        if( extruded_dimension < 0 )
            return -extruded_dimension;
        if( extruded_dimension >= my::dimensions() )
            return extruded_dimension - my::dimensions();

        return base( (domain_of< BaseType >)x );
    }

    Extrude( BaseType base ) : Dimensions< UnitType >{ }, base{ base } { }

    BaseType base;
private:
    using my = Dimensions< UnitType >;
};

template< typename UnitType, typename BaseType >
Extrude< BaseType, UnitType > extrude( BaseType object )
{ return { object }; }

template< typename ObjectType0, typename ObjectType1 >
requires ( std::is_same_v< domain_of< ObjectType0 >, domain_of< ObjectType1 >> )
struct Interpolate : Dimensions< long double >
{
    using domain_type = domain_of< ObjectType0 >;

    long double operator()( domain_type x ) const
    { 
        auto d0 = object0( x );
        auto d1 = object1( x );
        auto t = my::dimensions();

        return d0 * ( 1. - t ) + d1 * t;
    }

    Interpolate( ObjectType0 object0, ObjectType1 object1 ) :
        object0{ object0 }, object1{ object1 }
    { }

    ObjectType0 object0;
    ObjectType1 object1;
private:
    using my = Dimensions< long double >;
};

template< typename ObjectType0, typename ObjectType1 >
Interpolate< ObjectType0, ObjectType1 > interpolate( 
    ObjectType0 object0, ObjectType1 object1 )
{ return { object0, object1 }; }

template< typename BaseType >
struct Pad : Dimensions< domain_of< BaseType >>
{
    using domain_type = domain_of< BaseType >;
};


/**
 * print functions for debugging
 */
template< typename PrintableType >
void print( PrintableType ) { }

template< typename TupleType, size_t ... Is >
void print_helper( TupleType const& tuple, std::index_sequence< Is... > )
{
    std::cout << "(";
    ( ( std::cout << std::to_string( std::get< Is >( tuple )) << 
        ( Is < sizeof...( Is ) - 1  ? ", " : "" )), ... );
    std::cout << ")";
}

void print( units::Length length )
{ std::cout << std::to_string( length ); }

template< typename... UnitTypes >
void print( units::Space< UnitTypes... > unit )
{ print_helper( unit, std::make_index_sequence< sizeof...( UnitTypes ) >{} ); }

void print( Point point )
{
    std::cout << "point";
}

template< typename ObjectType0, typename ObjectType1 >
void print( Interpolate< ObjectType0, ObjectType1 > interpolated )
{
    std::cout << "interpolated{from:";
    print( interpolated.object0 );
    std::cout << " to:";
    print( interpolated.object1 );
    std::cout << " by:";
    print( interpolated.dimensions() );
    std::cout << "}";
}

template< typename BaseType, typename UnitType >
void print( Extrude< BaseType, UnitType > extruded )
{
    std::cout << "extruded{";
    print( extruded.base );
    std::cout << " by:";
    print( extruded.dimensions() );
    std::cout << "}";
}

template< typename BaseType >
void print( Translate< BaseType > translated )
{
    std::cout << "translated{";
    print( translated.object );
    std::cout << " by ";
    print( translated.dimensions() );
    std::cout << "}";
}

template< typename BaseType, typename DomainType >
void print( Project< BaseType, DomainType > projected )
{
    std::cout << "projected{";
    print( projected.object );
    std::cout << "}";
}

// template< typename... ObjectTypes >
// void print( ObjectTypes... objects )
// { ( print( objects ), ... ); }

int main(int ac, char * av[])
{
    std::cout << "## test_geometry2" << std::endl;

    using units::Length;
    using cartesian = units::Space< Length, Length >;

    Point a, b;
    auto segment = extrude< Length >( b );

    auto a2 = project< cartesian >( a );

    auto p = translate( a2 );
    auto q = translate( a2 );
    auto m = interpolate(p, q);

    segment.dimensions( 1 );
    p.dimensions( 2, 3 );
    q.dimensions( 4, 5 );
    m.dimensions( 0.5 );


    print( segment );
    std::cout << "\n";
    print( p );
    std::cout << "\n";
    print( q );
    std::cout << "\n";
    print( m );
    std::cout << "\n";

    static_assert( std::is_same_v< cartesian, decltype(p)::domain_type > );

    cartesian x{{ 0.5, 0.5 }};

    std::cout<< "distance of (0.5, 0.5) from p: " << p(x) << std::endl;

    return 0;
}

