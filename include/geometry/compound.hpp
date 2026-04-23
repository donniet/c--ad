#ifndef __GEOMETRY_COMPOUND_HPP__
#define __GEOMETRY_COMPOUND_HPP__

namespace geometry {

template< typename... Ts >
using ComponentList = std::tuple< Ts... >;

// this can't make boundaries of line segments because the extruded component is
// empty.  but why?  they should all be the same space_type
template< typename ComponentsT, typename First, typename... Rest >
requires((( First::dimensions() == Rest::dimensions() ) and ... ) and 
   (( First::parameters() == Rest::parameters() ) and ... ))
struct Compound: tuple< First, Rest... >
{
    using components_type = ComponentsT;
    static constexpr size_t dimensions() { return First::dimensions(); }
    static constexpr size_t parameters() { return First::parameters(); }

    static string object_name() 
    { return "compound__" + First::object_name() + (( "-" + Rest::object_name() ) + ... ) + "__"; }

    constexpr Compound( First const& first, Rest const&... rest ): 
        tuple< First, Rest... >{ first, rest... } { }
    constexpr Compound() { };
};

template< typename ComponentT, typename CompoundT, typename ObjT >
struct add_component_helper;

template< typename ComponentT, typename... Components, typename... Objs, typename ObjT >
struct add_component_helper< ComponentT, 
    Compound< ComponentList< Components... >, Objs... >, ObjT >
{
    using compound_type = Compound< ComponentList< Components... >, Objs... >;
    using object_type = ObjT;
    using components_type = ComponentList< ComponentT,  Components... >;
    using type = Compound< components_type, object_type, Objs... >;

    static type add( compound_type compound, object_type object )
    {
        type ret = compound;
        get< 0 >( ret ) = object;
        return ret;
    } 
};

template< typename ComponentT, typename CompoundT >
struct remove_component_helper;

template< size_t I, typename CompoundT >
struct remove_component_by_index_helper;

struct components {
    /// @brief global component type representing all components in a compound
    struct all { };

    /// @brief global component type representing no component
    /// this allows for defaults and empty pads
    struct none { };
};

template< typename... Ts >
struct CompoundPath { };

template< typename... Ts >
using path = CompoundPath< Ts... >;

template< typename... Components >
struct MakeCompound
{
    template< typename... Objects >
    requires( sizeof...( Objects ) == sizeof...( Components ))
    static constexpr Compound< ComponentList< Components... >, Objects... >
    make( Objects const&... objects )
    { return { objects... }; }
};


/// GetComponent ///
///
template< typename ComponentT, typename CompoundT >
struct GetComponent;

/// @brief idempotent call to get_components by specifying all_copmonents
/// @tparam CompoundT 
template< typename CompoundT >
struct GetComponent< components::all, CompoundT >
{
    using compound_type = CompoundT;

    static constexpr compound_type get( compound_type const& compound )
    { return compound; }
};

/// @brief get_component that always returns an Empty
/// @tparam CompoundT 
template< typename CompoundT >
struct GetComponent< components::none, CompoundT >
{
    using compound_type = CompoundT;

    static constexpr Empty< compound_type::dimensions > 
    get( compound_type const& )
    { return { }; }
};

template< typename ComponentT, typename ComponentListT, typename... Objects >
struct GetComponent< ComponentT, Compound< ComponentListT, Objects... > >
{ 
    using compound_type = Compound< ComponentListT, Objects... >;
    using component_type = tuple_element_t< 
        tuple_index_v< ComponentT, ComponentListT >, tuple< Objects... >>;

    static constexpr component_type get( compound_type const& compound ) 
    { return std::get< tuple_index_v< ComponentT, ComponentListT >>( compound ); } 
};

template< typename First, typename... Rest, typename ComponentListT, typename... Objects >
requires( isgreater( sizeof...( Rest ), 0 ))
struct GetComponent< path< First, Rest... >, Compound< ComponentListT, Objects... >>
{ 
    using compound_type = Compound< ComponentListT, Objects... >;
    using first_getter = GetComponent< First, compound_type >;
    using path_getter = GetComponent< path< Rest... >, 
        typename first_getter::component_type >;
    using component_type = path_getter::component_type;

    static constexpr component_type get( compound_type const& compound ) 
    { return path_getter::get( first_getter::get( compound )); } 
};

template< typename First, typename ComponentListT, typename... Objects >
struct GetComponent< path< First >, Compound< ComponentListT, Objects... >>:
    GetComponent< First, Compound< ComponentListT, Objects... >> { };

template< typename ComponentT, typename CompoundT >
constexpr typename GetComponent< ComponentT, CompoundT >::component_type
get_component( CompoundT const& compound )
{ return GetComponent< ComponentT, CompoundT >::get( compound ); }

template< size_t I, typename ComponentListT, typename... Objects >
constexpr tuple_element_t< I, tuple< Objects... >>
get_component( Compound< ComponentListT, Objects... > const& compound )
{ return std::get< I >( compound ); }


// represents a call to get_component
template< typename ElementT, typename ObjT >
struct Component
{
    using element_type = ElementT;
    using object_type = ObjT;

    object_type object;
};

/// @brief lazily extract a component from a compound
/// @tparam ComponentT 
/// @tparam ObjT 
/// @param object 
/// @return 
template< typename ComponentT, typename ObjT >
constexpr Component< ComponentT, ObjT > component( ObjT object )
{ return { object }; }


} // namespace geometry 

#endif