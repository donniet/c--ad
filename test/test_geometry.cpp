
#include "index_sequence_utility.hpp"

#include <array>
#include <iostream>

/**
 * c++ad is a library to build 3D models and vector drawings from expressive c++
 * code. Complex constraints and expressions for dimensions of parametric parts
 * that are not expressable in GUI based cad. True volumetric and parts with 
 * lazy evaluation of surfaces will allow for more accurate boolean operations.
 * 
 * arch: types represent basic geometric elements and operations on them.  The
 * c++ type system can keep track of these operations along with the links to 
 * the parameters/dimensions and expressions.
 * instances of a class hold the parameters/dimensions and are returned by
 * method calls to the classes
 * 
 * terms:
 * - geometry_type: the base geometry devoid of dimensions and manipulations
 *   like translations and rotations.  These can have components like edges and
 *   vertices, faces, and volumes
 * - dimensions_type: this is a reference type which holds the parameters that
 *   all operations use
 * - operations: these are classes which modify geometries and other operations
 *  
 */


namespace units
{
    static constexpr long double meters_per_inch = 0.0254;
    static constexpr long double meters_per_foot = meters_per_inch / 12.;
    static constexpr long double meters_per_mile = meters_per_foot / 5280.;

    struct Length
    {
        Length& operator+=(Length const& other) 
        { 
            m_meters += other.m_meters;
            return *this;
        }

        long double in_meters() const
        { return m_meters; }
        operator long double() const
        { return m_meters; }

        Length( int value = 0 ) : m_meters{ (long double)value } { }
        Length( unsigned int value ) : m_meters{ (long double)value } { }
        Length( long value ) : m_meters{ (long double)value } { }
        Length( unsigned long value ) : m_meters{ (long double)value } { }
        Length( float value ) : m_meters{ value } { }
        Length( double value ) : m_meters{ value } { }
        Length( long double value ) : m_meters{ value } { }

        long double m_meters;
    };

    struct AutomaticValue
    { 
        template< typename UnitType >
        UnitType get();

        operator Length() const { return { -1 }; }
    };

    namespace literals
    {
        static constexpr AutomaticValue automatic = {};

        Length operator""_in(long double inches)
        { return { inches * meters_per_inch }; }
        Length operator""_in(unsigned long long inches)
        { return { (long double)inches * meters_per_inch }; }
        Length operator""_ft(long double feet)
        { return { feet * meters_per_foot }; }
        Length operator""_ft(unsigned long long feet)
        { return { (long double)feet * meters_per_foot }; }
        Length operator""_mi(long double miles)
        { return { miles * meters_per_mile }; }
        Length operator""_mi(unsigned long long miles)
        { return { (long double)miles * meters_per_mile }; }
        Length operator""_m(long double meters)
        { return { meters }; }
        Length operator""_m(unsigned long long meters)
        { return { (long double)meters }; }
        Length operator""_mm(long double millimeters)
        { return { 0.001 * millimeters }; }
        Length operator""_mm(unsigned long long millimeters)
        { return { 0.001 * (long double)millimeters }; }
        Length operator""_km(long double kilometers)
        { return { 1000. * kilometers }; }
        Length operator""_km(unsigned long long kilometers)
        { return { 1000. * (long double)kilometers }; } 
    }
};

/**
 * Domain is a closed coordinate space of a given number of dimensions
 */
template< typename... UnitTypes >
struct Domain;

/**
 * Subdomain is a helper class to reduce the dimensionality of a Domain
 */
template< typename DomainType, size_t Of >
struct Subdomain;

template< typename... UnitTypes >
struct Domain
{ 
    static constexpr size_t dimensionality = sizeof...( UnitTypes );

    template< size_t N >
    using nth_type = std::tuple_element_t< N, std::tuple< UnitTypes... > >;

    template< size_t Of >
    using boundary_type = Subdomain< Domain, Of >::domain_type;

    struct coordinates_type : std::tuple< UnitTypes... >
    { 
        static coordinates_type zero() 
        { return zero_helper(
            std::make_index_sequence< sizeof...( UnitTypes )>{} ); }

        template< size_t I >
        void set( auto value )
        { std::get< I >( *this ) = value; }

        template< size_t I >
        auto get() const
        { return std::get< I >( *this ); }

        coordinates_type& operator=( coordinates_type const& ) = default;
        coordinates_type& operator=( coordinates_type&& ) = default;

        coordinates_type() : std::tuple< UnitTypes... >{ zero() } { }
        coordinates_type( UnitTypes... units ) : 
            std::tuple< UnitTypes... >{ units... } 
        { }
        // coordinates_type( coordinates_type const& ) = default;
        // coordinates_type( coordinates_type&& ) = default;

    private:
        template< size_t... Is >
        static coordinates_type zero_helper( std::index_sequence<Is...> )
        { return { (Is & 0)... }; }
    };

    coordinates_type origin() const
    { return coordinates_type::zero(); }

    Domain operator~() const;

    template< size_t Of >
    boundary_type< Of > boundary();
};

template< typename DomainType, typename UnitType >
struct ExtendedDomain;

template< typename UnitType, typename... UnitTypes >
struct ExtendedDomain< Domain< UnitTypes... >, UnitType >
{ using domain_type = Domain< UnitTypes..., UnitType >; };

template< typename DomainType, typename UnitType >
using extend_domain = ExtendedDomain< DomainType, UnitType >::domain_type;

template< size_t Of, typename... UnitTypes >
struct Subdomain< Domain< UnitTypes... >, Of >
{
    using domain_type = pack::remove_nth< Of, Domain< UnitTypes... >>::type;
};

template<typename UnitType, unsigned ... Sizes>
struct Dimensions : std::tuple< std::array< UnitType, Sizes >... >
{ 
    template< size_t I >
    auto value( unsigned j ) { return std::get<I>(*this)[j]; }
};

template< typename DomainType >
using domain_coordinates = typename DomainType::coordinates_type;

template< typename UnitType, unsigned Dimension >
struct Range : std::array< std::pair< UnitType, UnitType >, Dimension >
{ };

/**
 * object_traits is the base-traits object for all cad objects
 * 
 */
template< typename ObjectType >
struct object_traits
{
    using domain_type = typename ObjectType::domain_type;
    using geometry_type = typename ObjectType::geometry_type;
    using dimensions_type = typename ObjectType::dimensions_type;

    // using range_type = typename ObjectType::range_type;
};

template< typename ObjectType >
using domain_of = object_traits< ObjectType >::domain_type;

template< typename ObjectType >
using geometry_of = object_traits< ObjectType >::geometry_type;

/**
 * geometry_traits gets traits for geometric objects including components
 */
template< typename ObjectType >
struct geometry_traits
{
    using geometry_type = typename object_traits< ObjectType >::geometry_type;
    using components_type = typename geometry_type::components_type;

    static constexpr size_t components_size = 
        std::tuple_size_v< components_type >;
};

template< size_t I, typename ObjectType >
requires ( I < geometry_traits< ObjectType >::components_size )
struct ComponentGetter
{
    using components_type = geometry_traits< ObjectType >::components_type;
    using type = std::tuple_element_t< I, components_type >;

    static constexpr type get( ObjectType const& geometry )
    { std::get< I >( geometry.components() ); }

    type operator()( ObjectType const& geometry ) const 
    { return get( geometry ); }
};

template< size_t I, typename ObjectType >
auto component( ObjectType const& geometry )
{ return ComponentGetter< I, ObjectType >::get( geometry ); }

// template< typename ObjectType >
// struct dimensional_traits;

template< typename ObjectType, typename... ObjectTypes >
requires ( 
    std::is_same_v< domain_of< ObjectType >, domain_of< ObjectTypes >> && ... )
struct Collection
{
    using domain_type = domain_of< ObjectType >;
    using geometry_type = Collection< ObjectType, ObjectTypes... >;
    using dimensions_type = Dimensions< void >;
    using components_type = std::tuple< ObjectType, ObjectTypes... >;

    components_type components() const
    { return m_components; }

    components_type m_components;
};

template< template< typename... > class Wrapper, typename ComponentsType >
struct ComponentWrapperFromComponentsType;

template< template< typename... > class Wrapper, typename... ComponentTypes >
struct ComponentWrapperFromComponentsType< Wrapper, Collection< ComponentTypes... >>
{
    using type = Collection< Wrapper< ComponentTypes >... >;
};

template< template< typename... > class Wrapper, typename ObjectType >
struct ComponentWrapper
{
    using geometry_type = object_traits< ObjectType >::geometry_type;
    using components_type = geometry_traits< geometry_type >::components_type;

    using type = ComponentWrapperFromComponentsType< Wrapper, components_type >::type;
};

template< template< typename... > class Wrapper, typename ObjectType >
using component_wrapper = ComponentWrapper< Wrapper, ObjectType >::type;



template< typename ObjectType, typename DomainType >
struct Projected
{
    using domain_type = DomainType;
    using base_type = ObjectType;
    using geometry_type = geometry_of< base_type >;
    using dimensions_type = typename ObjectType::dimensions_type;

    base_type object;
};

template< typename ObjectType, typename DomainType >
struct Projector
{
    using type = Projected< ObjectType, DomainType >;

    type operator()( ObjectType object )
    { return { object }; }
};


template< typename ObjectType, typename DomainType >
using projection_of = Projector< ObjectType, DomainType >::type;

/**
 * Point is a selected element from a domain
 */
template< typename DomainType >
struct Point
{
    using domain_type = DomainType;
    using dimensions_type = Dimensions< domain_coordinates< domain_type > >;
    using geometry_type = Point< DomainType >;

    template< typename DomainType2 > 
    Point( Point< DomainType2 > const& other ) : 
        dimensions{ other.dimensions }
    { }

    Point( Point&& other ) = default;
    Point( Point const& other ) = default;
    Point() : dimensions{} { }

    dimensions_type dimensions;
};

template< typename DomainTypeFrom, typename DomainTypeTo >
struct Projector< Point< DomainTypeFrom >, DomainTypeTo >
{ 
    using type = Point< DomainTypeTo >;

    type operator()() const;
};


/**
 * Segment is a linear interpolation between two objects using a single parameter
 */
template< typename PointType, typename PointType1 >
requires ( std::same_as< geometry_of< PointType >, geometry_of< PointType1 >> )
struct Segment 
{
    using domain_type = object_traits< PointType >::domain_type;
    using dimensions_type = Dimensions< void >;
    using geometry_type = Segment< PointType, PointType1 >;



    std::tuple< PointType, PointType1 > points;
    dimensions_type dimensions;
};

template< typename UnitType0, typename UnitType1 >
struct Rectangle 
{
    using dimensions_type = std::tuple< UnitType0, UnitType1 >;
    // using range_type = Range< units::Length, 2 >;
    using domain_type = Domain< UnitType0, UnitType1 >;
    using geometry_type = Rectangle;
    using point_type = Point< domain_type >;
    using segment_type = Segment< point_type, point_type >;
    using components_type = Collection< segment_type, segment_type, 
        segment_type, segment_type >;

    enum side_identifier : int
    {
        bottom_edge = 0b00,
        top_edge = 0b01,
        left_edge = 0b10,
        right_edge = 0b11,
    };

    struct side_type : Domain< units::Length, units::Length >
    {
        Rectangle const& quad;
        side_identifier edge_id;

        side_type( Rectangle const& quad, side_identifier edge_id ) :
            quad{ quad }, edge_id{ edge_id }
        { }
    };

    side_type operator[]( side_identifier edge_id ) const
    { return { *this, edge_id }; }

    dimensions_type dimensions;
};


template< typename FaceType >
struct face_traits : object_traits< FaceType >
{
    using side_type = typename FaceType::side_type;
    using side_identifier = typename FaceType::side_identifier;
};

// template< typename ObjectType, typename PositionType >
// struct Positioned;

// template< typename ObjectType, typename PositionType >
// Positioned< ObjectType, PositionType > pose( ObjectType object, PositionType at_position );

// template< typename ObjectType, typename PositionType >
// struct Positioned
// {
//     using object_type = ObjectType;
//     using position_type = PositionType;
//     using domain_type = object_traits< object_type >::domain_type;
//     // geometry doesn't change when positioned
//     using geometry_type = geometry_of< object_type >;
//     // using range_type = typename object_type::range_type;

//     Positioned( object_type object ) : object{ object }, position{} { }
//     Positioned( object_type object, position_type position ) :
//         object{ object }, position{ position }
//     { }

//     template< typename CompositeIdentifier >
//     auto operator[]( CompositeIdentifier id ) const
//     { return pose( object[id], position ); }

//     object_type object;
//     position_type position;
// };

// template< typename FaceType, typename PositionType >
// struct face_traits< Positioned< FaceType, PositionType > >
// {
//     using side_type = Positioned< typename face_traits< FaceType >::side_type, PositionType >;
//     using side_identifier = typename face_traits< FaceType >::side_identifier;
// };

template< typename ObjectType >
struct Translated
{
    using base_type = ObjectType;
    using domain_type = object_traits< ObjectType >::domain_type;
    using dimensions_type = Dimensions< domain_coordinates< domain_type > >;
    // geometry doesn't change when translated
    using geometry_type = geometry_of< ObjectType >;

    base_type base() const
    { return m_base; }

    dimensions_type dimensions() const
    { return m_dimensions; }

    base_type m_base;
    dimensions_type m_dimensions;
};


template< typename ObjectType >
struct Extruded 
{
    using base_type = ObjectType;
    using base_domain_type = object_traits< ObjectType >::domain_type;
    using dimensions_type = Dimensions< units::Length >;
    using domain_type = extend_domain< base_domain_type, units::Length >;
    using geometry_type = Extruded< ObjectType >;

    using proximal_type = projection_of< base_type, domain_type >;
    using distal_type = Translated< proximal_type >;
    using extrusion_type = component_wrapper< Extruded, ObjectType >;



    enum component_type : size_t
    { proximal = 0, distal = 1, extrusion = 2 };

    using components_type = 
        Collection< proximal_type, distal_type, extrusion_type >;

    proximal_type proximal_component() const
    { return { base() }; }

    distal_type distal_component() const
    {

    }

    extrusion_type extrution_component() const
    { return { proximal_component(), distal_component() }}

    components_type components() const
    { return { 
        proximal_component(), distal_component(), extrusion_component() }; }

    dimensions_type dimensions() const
    { return m_dimensions; }

    base_type base() const
    { return m_base; }

    base_type m_base;
    dimensions_type m_dimensions;
};

// template< typename DomainType >
// struct Extruded< Point< DomainType >>
// {
//     using domain_type = extend_domain< DomainType, units::Length >;
//     using base_type = Point< domain_type >;
//     using dimensions_type = Dimensions< units::Length, 1 >;

//     base_type proximal()
//     { return base; }

//     Translated< base_type > distal() 
//     { return { base, dimensions.value<0>(0) }; }

//     Segment< base_type, Translated< base_type >> segment() const
//     { return {{ base , distal() }}; }

//     base_type base;
//     dimensions_type dimensions;
// };

template< typename ObjectType >
Extruded< ObjectType > extrude( ObjectType object )
{ return { object, { } }; }


/**
 * Prism is a 3D shape built from a 2D profile, a vector direction and a length
 */
// template<typename FaceType>
// struct Prism
// { 
//     using dimensions_type = Dimensions< units::Length, 1 >;
//     using face_type = FaceType;
//     using base_domain_type = object_traits< face_type >::domain_type;
//     using domain_type = extend_domain< base_domain_type, units::Length >;

//     using side_type = face_traits< FaceType >::side_type;
//     using side_identifier = face_traits< FaceType >::side_identifier;

//     face_type base() const
//     { return { m_base }; }

//     auto side( side_identifier side )
//     {  
//         auto edge = m_base[side];


//     }

//     Prism( face_type face ) : 
//         m_base{face}, dimensions{{ units::literals::automatic }} 
//     { }

//     // boolean negation
//     Prism operator~() const;

//     face_type m_base;
//     dimensions_type dimensions;
// };

// template< typename FaceType >
// Prism< FaceType > prism_from( FaceType face )
// { return { face }; }


template< typename ObjectType >
struct Interpolated
{
    // using range_type = typename ObjectType::range_type;
    using domain_type = typename ObjectType::domain_type;
    using dimensions_type = typename ObjectType::dimensions_type;
    using geometry_type = geometry_of< ObjectType >;

    ObjectType object;
    range_type range;
};

template< typename FaceType >
struct face_traits< Interpolated< FaceType > >
{
    using side_type = face_traits< FaceType >::side_type;
    using side_identifier = face_traits< FaceType >::side_identifier;
};

/**
 * Grid is a way to divide a quad into sub-quads
 */
template<typename UnitType, unsigned... Cells>
struct Grid
{ 
    static constexpr size_t number_of_dimensions = sizeof...( Cells );
    static constexpr auto sequnce_of_dimensions = std::make_index_sequence< number_of_dimensions >{};
    using dimensions_type = Dimensions< UnitType, Cells... >;
    using index_type = std::array< unsigned, number_of_dimensions >;
    using range_type = Range< UnitType, number_of_dimensions >;

    template<size_t ... Is>
    range_type range_of_helper( index_type index, std::index_sequence<Is...> )
    { return { dimensional_range<Is>( index[Is] )... }; }

    template< size_t Dimension >
    std::pair< UnitType, UnitType > dimensional_range( unsigned index )
    { 
        auto rules = std::get< Dimension >( dimensions );

        UnitType lower = 0;
        int i = 0;
        for( ; i < (int)index - 1; ++i )
            lower += rules[i];
        
        return { lower, lower + rules[i] };
    }

    template< typename... Indices >
    range_type range_of( Indices... indices )
    { return range_of_helper( { (unsigned)indices... }, sequnce_of_dimensions ); }

    template<typename FaceType>
    struct attached_type
    {
        using object_type = FaceType;

        template<typename... Indices>
        Interpolated< FaceType > cell(Indices... indices)
        { return { object, grid.range_of( indices... ) }; }

        Grid & grid;
        object_type object;
    };
  

    template<typename FaceType>
    attached_type<FaceType> of(FaceType face)
    { return { *this, face }; }

    dimensions_type dimensions;
};

// template<typename FaceType, typename UnitType, unsigned Dimensions>
// auto extrude( Positioned< FaceType > positioned_face,
//               Vector< Dimensions > direction = { 0., 0., 0. } )
// {
//     auto sides = positioned_face.sides() | make_face( direction );

//     return Prism{  }
// }

template<typename ... ObjectTypes>
struct STL 
{ 
    static constexpr size_t object_count = sizeof...(ObjectTypes);

    std::ostream& print( std::ostream& os) const
    { 
        print_helper( os, std::make_index_sequence<object_count>{} ); 
        return os;
    }

    template<size_t ... Is>
    void print_helper( std::ostream& os, std::index_sequence<Is...> ) const
    { ( print_object( os, std::get<Is>(objects) ), ... ); }

    template<typename ObjectType>
    static void print_object( std::ostream& os, ObjectType& object, std::string const& name = "solid" )
    {
        os << "# STL of " << typeid(ObjectType).name() << "\n";

        os << "\nsolid " << name << "\n\n";

        auto shell = boundary_of( object );
        auto simplex = simplex_of( shell );

        for( auto const& facet : facets_of( simplex ) )
            print_facet( os, facet );
        
        os << "\nendsolid " << name << "\n\n";
    }

    template< typename FaceType >
    static void print_facet( std::ostream& os, FaceType& face )
    {
        auto n = normal_of( face );
        os << "facet normal " << n[0] << ", " << n[1] << ", " << n[2] << "\n"
           << "\touter loop\n";
        
        for( auto const& v : vertex_loop_of( face ) )
            os << "\t\tvertex " << v[0] << ", " << v[1] << ", " << v[2] << "\n";

        os << "\tendloop\n"
           << "endfacet\n";
    }

    STL( ObjectTypes&... objects ) : objects{objects...} { }

    std::tuple<ObjectTypes&...> objects;
};

template<typename... ObjectTypes>
std::ostream& operator<<( std::ostream& os, STL< ObjectTypes... > const& stl )
{ return stl.print( os ); }


int main( int ac, char* av[] )
{   
    using namespace units::literals;
    using std::cout, std::endl;

    using rect = Rectangle< units::Length, units::Length >;

    auto profile = rect{};
    auto stile = extrude( profile );
    auto rail = extrude( profile );
    auto rail_extrusion = component< rail.extrusion >( rail );

    auto layout = Grid< units::Length, 3, 3 >{};


    auto tenon_layout = layout.of( component< stile.proximal >( stile ));
    auto mortise_layout = layout.of( component< rect::bottom >( rail_extrusion ));

    // extrude the middle face of the 3x3 grid on the xz face of the stile prism
    auto tenon = extrude( tenon_layout.cell(1, 1) );
    auto mortise = ~prism_from( mortise_layout.cell(1, 1) );

    // all dimensions
    profile.dimensions = {{ 1.5_in, 3_in }};
    stile.dimensions = {{ 2.0_ft }};
    rail.dimensions = {{ 4.0_ft }};
    layout.dimensions = {
        { 0.25_in, 0.5_in, 0.25_in }, // x cell width
        { 0.5_in,  2.0_in, automatic  }, // y cell height
    };
    tenon.dimensions = mortise.dimensions = {{ 2.0_in }};
    
    cout << STL{ tenon, mortise } << endl; 

    return 0;
}