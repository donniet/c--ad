#include "units.hpp"
#include "dimensions.hpp"
#include "universe.hpp"

#include <iostream>

using std::string;

template< typename Object >
auto boundary_of( pointer_to< Object > ptr );

template< typename ObjectType, size_t Dimensionality = 3, typename Unit = units::Length >
struct Positioned : virtual Object
{
    using unit_type = Unit;
    using object_pointer = typename ObjectType::pointer;
    static constexpr size_t dimensionality = Dimensionality;
    using dimensions_type = repeated_dim_t< unit_type, dimensionality >;

    dimensions_type& position() { return dimensions; }

    object_pointer object;
    dimensions_type dimensions;
};

template< typename Object, size_t Dimensionality = 3, typename Unit = units::Length >
pointer_to< Positioned< Object, Dimensionality, Unit >> positioned( pointer_to< Object > object )
{ return { object }; }

template< typename Object, size_t Dimensionality, typename Unit >
auto& position_of( Positioned< Object, Dimensionality, Unit >* object )
{ return object->position(); }

template< typename ObjectType, size_t Dimensionality = 3, typename Unit = units::Length >
struct Oriented : virtual Object
{
    using unit_type = Unit;
    using object_pointer = typename ObjectType::pointer;
    static constexpr size_t dimensionality = Dimensionality;
    using dimensions_type = repeated_dim_t< unit_type, dimensionality >;

    Oriented( object_pointer obj, dimensions_type const& dim ) :
        object{ obj }, dimensions{ dim }
    { }
    Oriented( object_pointer obj, dimensions_type&& dim ) :
        object{ obj }, dimensions{ std::move( dim )}
    { }
    Oriented( object_pointer obj ) : object{ obj } { }
    Oriented() : object{ nullptr } { }

    Oriented& operator=( Oriented const& ) = default;
    Oriented& operator=( Oriented&& ) = default;
    Oriented( Oriented const& ) = default;
    Oriented( Oriented&& ) = default;

    dimensions_type& orientation() { return dimensions; }

    object_pointer object;
    dimensions_type dimensions;
};

template< typename ObjectType >
using oriented_1d = Oriented< ObjectType, 1 >;
template< typename ObjectType >
using oriented_2d = Oriented< ObjectType, 2 >;
template< typename ObjectType >
using oriented_3d = Oriented< ObjectType, 3 >;

template< typename Object, size_t Dimensionality = 3, typename Unit = units::Length >
Oriented< Object, Dimensionality, Unit > oriented( Object object )
{ return { object }; }

template< typename Object, size_t Dimensionality, typename Unit >
auto& orientation_of( Oriented< Object, Dimensionality, Unit > object )
{ return object->orientation(); }

template< typename... ObjectTypes >
struct Array : virtual Object, 
    std::tuple< pointer_to< ObjectTypes >... >
{
    static constexpr const size_t size = sizeof...( ObjectTypes );
    using container_type = std::tuple< pointer_to< ObjectTypes >... >;

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
concept bounded_object_ptr = requires( ObjectPtr ptr )
{ bounded_object< decltype( *ptr )>; };

template< typename... ObjectTypes >
struct BoundedBy
{
    using boundary_type = Array< ObjectTypes... >;

    BoundedBy( pointer_to< ObjectTypes >... ptrs ) : boundary_type{ ptrs... } { }

    boundary_type boundary_objects;

    pointer_to< boundary_type > boundary() 
    { return { &boundary_objects }; }
};

template< bounded_object_ptr ObjectPtr >
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

struct Point : virtual Object, BoundedBy< >
{ 
    using bounded_by_type = BoundedBy< >;
    using pointer = initialized_ptr< Point >;

    long double operator()()
    { return 1.; }
    
    // Point() : bounded_by_type{ this } { };
};

struct Segment : virtual Object, BoundedBy< 
    Oriented< Point >, 
    Positioned< Oriented< Point >> >
{
    using dimensions_type = Dimensions< units::Length >;
    using bounded_by_type = BoundedBy< Oriented< Point >, Positioned< 
        Oriented< Point >>>::boundary_type;

    // predicate in dimension_type space
    long double operator()( units::Length value )
    {  
        if( value < 0 || value > length() )
            return 0;
        return 1;
    }

    units::Length length()
    { return dimensions.eval<0>(); }

    // Segment() : bounded_by_type{ this, new_object_of< Point >( universe() ) } { }

    dimensions_type dimensions;
};

struct Rect : virtual Object
{
    using dimensions_type = Dimensions< units::Length, units::Length >;
    
    dimensions_type dimensions;
};

struct Box : virtual Object 
{ 
    using dimensions_type = Dimensions< units::Length, units::Length, units::Length >;

    Box() { }

    dimensions_type dimensions;
};

int main( int ac, char* av[] )
{
    using namespace units::operators;
    using namespace units::literals;
    using namespace units;

    using std::cout, std::cin, std::endl;

    Universe world( "cabinet" );

    // Tensor< Contravariant<>, Covariant< Length, Length >> g;
    // Vector< Length > v;

    // auto y = d( exp( expression_of( v )));

    auto stile = new ( world ) Box{ };
    auto rail = new ( world ) Box{ };

    auto cabinet_width  = constant( 48.00_in );
    auto cabinet_height = constant( 34.50_in );
    auto cabinet_depth  = constant( 24.00_in );
    auto kick_height    = constant(  4.00_in );
    auto door_relief    = constant(  0.25_in );
    auto door_gap       = constant( 0.125_in );
    auto frame_width    = constant(  1.50_in );
    auto door_thickness = constant( 0.625_in );

    auto door_height = ( cabinet_height - kick_height ) - door_relief * 2.0;

    stile->dimensions = { door_height, frame_width, door_thickness };

    cout << stile->dimensions.to_string() << endl;

    return EXIT_SUCCESS;
}