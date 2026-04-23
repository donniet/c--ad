#ifndef __GEOMETRY_COLLECTION_HPP__
#define __GEOMETRY_COLLECTION_HPP__

#include "geometry/space.hpp"

namespace geometry {

template< typename... >
struct Collection;

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

/// @brief an empty collection
template< >
struct Collection< >: Object
{ 
    static constexpr size_t size() { return 0; }
};

/// @brief a tuple-like collection of objects
/// @tparam First type of first object
/// @tparam ...Rest type of remaining objects
template< typename First, typename... Rest >
requires((( First::dimensions() == Rest::dimensions() ) and ... ))
struct Collection< First, Rest... >: Collection< Rest... >
{ 
    static constexpr size_t dimensions() { return First::dimensions(); }
    static constexpr size_t parameters() { return First::parameters() + 
        ( Rest::parameters() + ... ); }
    static constexpr size_t size() { return 1 + sizeof...( Rest ); }

    constexpr First const& first() const { return _first; }
    constexpr First& first() { return _first; }

    constexpr Collection< Rest... > const& rest() const 
    { return *this; }
    constexpr Collection< Rest... >& rest() 
    { return *this; }

    static string object_name() { return "collection__" + First::object_name() + 
        (( "_" + Rest::object_name() ) + ... ) + "__"; }

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

template< typename... Objects >
struct IsEmpty< Collection< Objects... >>:
    integral_constant< bool, ( IsEmpty< Objects >::value and ... )> { };

template< typename... >
struct FlattenCollections;

template< >
struct FlattenCollections< >
{ 
    using type = Collection<>;
    static constexpr type make() { return {}; } 
};

template< typename First, typename... Rest >
struct FlattenCollections< First, Rest... >
{  
    using type = collection_add_t< First, 
        typename FlattenCollections< Rest... >::type >;
    static constexpr type make( First const& first, Rest const&... rest )
    { return { first, FlattenCollections< Rest... >::make( rest... )}; }
};

template< typename... Rest >
struct FlattenCollections< Collection<>, Rest... >
{  
    using type = FlattenCollections< Rest... >::type;
    static constexpr type make( Collection<> const&, Rest const&... rest )
    { return FlattenCollections< Rest... >::make( rest... ); }
};

template< typename LeadingFirst, typename... LeadingRest, typename... Rest >
struct FlattenCollections< Collection< LeadingFirst, LeadingRest... >, Rest... >
{  
    using tail_maker = FlattenCollections< Collection< LeadingRest... >, Rest... >;
    using maker = FlattenCollections< LeadingFirst, typename tail_maker::type >;
    using type = maker::type;

    static constexpr type make( 
        Collection< LeadingFirst, LeadingRest... > const& first, 
        Rest const&... rest )
    { 
        auto tail = tail_maker::make( first.rest(), rest... );
        return maker::make( first.first(), tail ); 
    }
};


/// @brief creates a collection of geometric objects which can be operated on
/// together. 
/// @tparam ...Objects types in the collection
/// @param ...objects in the collection
/// @return a collection where any nested collections are enumerated into the 
/// returned collection
template< typename... Objects >
typename FlattenCollections< Objects... >::type 
flatten( Objects const&... objects )
{ return FlattenCollections< Objects... >::make( objects... ); }

// NOTE: original dumb implementation
template< typename... Objects >
Collection< Objects... > collection( Objects const&... objects )
{ return { objects... }; }

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

#endif