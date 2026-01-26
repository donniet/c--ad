#ifndef __UNIVERSE_HPP__
#define __UNIVERSE_HPP__

#include "utility.hpp"

#include <memory>
#include <memory_resource>
#include <iostream>
#include <string>
#include <new>
#include <set>
#include <type_traits>

#ifndef STRINGIZE
#define STRINGIZE_(x) #x
#define STRINGIZE(x) STRINGIZE_(x)
#endif

using std::string;

struct Object;
struct Universe;

template< virtual_base_of< Object > T >
struct object_ptr
{
    friend struct Object;
    friend struct Universe;

    using value_type = T;
    using pointer = T*;
    using reference = T&;

    pointer operator->() const { return value; }
    reference operator*() const { return *value; }

    bool operator==( object_ptr< T > const& other ) const
    { return value == other.value; }
    bool operator!=( object_ptr< T > const& other ) const
    { return value != other.value; }

    operator bool() const { return value != nullptr; }

    object_ptr& operator=( object_ptr const& ) = default;
    object_ptr& operator=( object_ptr&& ) = default;
    object_ptr& operator=( std::nullptr_t ) { value = nullptr; return *this; }

    Universe& universe();

    object_ptr( object_ptr const& ) = default;
    object_ptr( object_ptr&& ) = default;

    // ensure initially null
    object_ptr() : value{ nullptr } { };
    object_ptr( std::nullptr_t ) : value{ nullptr } { }; // allow explicit null

// protected:
    object_ptr& operator=( pointer ptr ) { value = ptr; return *this; }
    object_ptr& operator=( Object* ptr )
    { value = dynamic_cast< T* >( ptr ); return *this; }
    operator pointer() const { return value; };
    object_ptr( pointer ptr ) : value{ ptr } { };
    object_ptr( Object* ptr ) : value{ dynamic_cast< T* >( ptr ) } { }

private:
    pointer value;
};


/**
 * Domains are a list of numeric types representing the arguments passed to
 * object predicates.
 */
template< typename... Ts >
struct Domain { };

namespace detail 
{
    template< typename... Ds >
    struct domain_cat_helper;

    template< typename... Ts >
    struct domain_cat_helper< Domain< Ts... >>
    { using type = Domain< Ts... >; };

    template< typename... Ts, typename... Us >
    struct domain_cat_helper< Domain< Ts... >, Domain< Us... >>
    { using type = Domain< Ts..., Us... >; };
    
    template< typename D1, typename D2, typename... Ds >
    struct domain_cat_helper< D1, D2, Ds... >
    { using type = domain_cat_helper< 
        typename domain_cat_helper< D1, D2 >::type, 
        typename domain_cat_helper< Ds... >::type >::type; }; 
}

template< typename... Ds >
using domain_cat = detail::domain_cat_helper< Ds... >::type;

template< typename D, typename... Ts >
using domain_append = detail::domain_cat_helper< D, Domain< Ts... >>::type;

namespace detail
{
    template< typename... Ts >
    struct domain_of_helper;

    template< >
    struct domain_of_helper< >
    { using type = Domain< >; };

    template< typename... Us, typename... Ts >
    struct domain_of_helper< Domain< Us... >, Ts... >
    { using type = domain_of_helper< Us..., Ts... >::type; };

    template< typename T, typename... Ts >
    struct domain_of_helper< T, Ts... >
    { using type = domain_cat< Domain< T >, 
        typename domain_of_helper< Ts... >::type >; };

}

template< typename... Ts >
using domain_of = detail::domain_of_helper< Ts... >::type;

template< typename ObjectType >
struct object_traits
{
    using pointer = object_ptr< ObjectType >;
    // using domain_type = typename ObjectType::domain_type;
};

template< typename ObjectType >
using pointer_to = object_traits< ObjectType >::pointer;

template< typename ObjectType >
using domain_of = object_traits< ObjectType >::domain_type;

template< typename ObjectType, typename... ArgTypes >
// requires std::is_virtual_base_of< Object, ObjectType >::value
pointer_to< ObjectType > create_in( Universe& world, ArgTypes... args )
{ return new ( world ) ObjectType{ args... }; }

struct Object 
{
    using domain_type = Domain< >;

    long double operator()( domain_type const& x )
    { return 1.; }

    long double operator()()
    { return 1.; }

    Object(); 
    virtual ~Object();

    /**
     * allocate in the default_universe
     */
    static void* operator new( size_t size );
    static void operator delete( void* ptr );

    /**
     * allocate within the given Universe
     */
    static void* operator new( size_t size, Universe& here );
    static void operator delete( void* ptr, Universe& here );
};




template< typename ObjectType >
Universe& universe_of( pointer_to< ObjectType > obj )
{ return obj.universe(); }

// getting a pointer to a temporary (rvalue reference) requires a move 
// allocation placed in the object's universe
template< typename ObjectType >
pointer_to< ObjectType > make_object_ptr( ObjectType&& object )
{ return new ( universe_of( &object )) ObjectType{ std::move( object ) }; }

template< typename ObjectType, typename... ArgTypes >
pointer_to< ObjectType > new_object_of( Universe& universe, ArgTypes... args )
{ return new ( universe ) ObjectType{ args... }; }


struct Universe
{
    friend struct Object;
    template< virtual_base_of< Object >> 
    friend struct object_ptr;

#ifdef DEFAULT_UNIVERSE_NAME
    static constexpr const char* default_universe_name = STRINGIZE(DEFAULT_UNIVERSE_NAME);
#else
    static constexpr const char* default_universe_name = "default";
#endif

    Universe() : m_name{ default_universe_name }, 
        m_memory{ std::pmr::get_default_resource() }
    { }

    Universe( string name, std::pmr::memory_resource* memory = 
        std::pmr::get_default_resource() ) : m_name{ name }, m_memory{ memory } 
    { }

    // use placement new to create new objects
    template< typename ObjectType, typename... ArgTypes >
    // requires { std::is_virtual_base_of< Object, ObjectType > }
    pointer_to< ObjectType > create( ArgTypes... args )
    { return new ( *this ) ObjectType{ args... }; }

    template< typename ObjectType >
    // requires { std::is_virtual_base_of< Object, ObjectType > }
    pointer_to< ObjectType > duplicate( pointer_to< ObjectType > obj )
    { return new ( *this ) ObjectType{ *obj }; }

    virtual ~Universe()
    { delete_all_objects(); }

    string name() const { return m_name; }

protected:
    /**
     * header_type stores data for every allocated Object.  It appears
     * immediately before the Object* returned by new_Object_of
     */
    struct header_type
    {
        static header_type* from_address( void* address )
        { return reinterpret_cast< header_type* >( address ) - 1; }

        static header_type const* from_address( void const* address )
        { return reinterpret_cast< header_type const* >( address ) - 1; }

        Universe* universe;
        size_t allocated_size;
    };

    static constexpr size_t header_size = sizeof( header_type );

    static Universe* from_address( void* address )
    { return header_type::from_address( address )->universe; }

    Object* new_Object_of( size_t bytes ) 
    {  
        void* ptr = m_memory->allocate( header_size + bytes );
        auto* header = reinterpret_cast< header_type* >( ptr );
        // write the header
        *header = { this, bytes };
        // save the Object* for deconstruction
        auto* obj = reinterpret_cast< Object* >( header + 1 );
        m_objects.insert( obj );
        // return an Object* pointing after the header
        return obj;
    }
    void delete_Object( Object* ptr )
    {
        auto* header = header_type::from_address( ptr ); 

        if( header->universe != this )
            throw std::logic_error( "object from another universe cannot be deleted" );

        auto allocated_size = header->allocated_size;
        // unlink
        m_objects.erase( ptr );
        header->universe = nullptr;
        header->allocated_size = 0;
        // deallocate
        m_memory->deallocate( reinterpret_cast< void* >( header ), 
            header->allocated_size );
    }

    void delete_all_objects()
    { while( !m_objects.empty() )
        delete m_objects.extract( m_objects.begin() ).value(); }

    string m_name;
    std::pmr::memory_resource* m_memory;
    std::set< Object* > m_objects;
};

template< virtual_base_of< Object > T >
Universe& object_ptr< T >::universe() 
{ return *Universe::from_address( value ); }

/**
 * Default Universe global variable
 */
static inline Universe default_universe = {};

/**
 * Object implementation
 */
Object::Object() { }
Object::~Object() { }

void* Object::operator new( size_t size )
{ return default_universe.new_Object_of( size ); }

void Object::operator delete( void* ptr )
{
    auto* universe = Universe::from_address( ptr );
    universe->delete_Object( reinterpret_cast< Object* >( ptr ));
}

void* Object::operator new( size_t size, Universe& here )
{ return here.new_Object_of( size ); }

void Object::operator delete( void* ptr, Universe& here )
{
    auto* obj = reinterpret_cast< Object* >( ptr );
    here.delete_Object( obj );
}

/**
 * Concepts
 */

template< typename... ObjectTypes >
struct in_same_domain;

// define only when the domain_type's are equal
template< typename ObjectType > struct in_same_domain< ObjectType >
{ 
    static constexpr const bool value = true;
    using domain_type = object_traits< ObjectType >::domain_type; 
};

template< typename First, typename... Rest >
requires std::is_same_v< typename object_traits< First >::domain_type, 
    typename in_same_domain< Rest... >::domain_type >
struct in_same_domain< First, Rest... > : in_same_domain< Rest... >
{ };


#endif