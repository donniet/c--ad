#ifndef __UNIVERSE_HPP__
#define __UNIVERSE_HPP__

#include "forward_declarations.hpp"

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

template< typename T >
struct initialized_ptr
{
    using value_type = T;
    using pointer = T*;
    using reference = T&;

    pointer operator->() const { return value; }
    reference operator*() const { return *value; }
    operator pointer() const { return value; };

    initialized_ptr& operator=( initialized_ptr const& ) = default;
    initialized_ptr& operator=( initialized_ptr&& ) = default;
    initialized_ptr( initialized_ptr const& ) = default;
    initialized_ptr( initialized_ptr&& ) = default;
    initialized_ptr( pointer ptr ) : value{ ptr } { };

    // it's all for this
    initialized_ptr() : value{ nullptr } { };

    pointer value;
};

template< typename ObjectType >
struct object_traits
{
    using pointer = typename ObjectType::pointer;
};

template< typename ObjectType >
using pointer_to = object_traits< ObjectType >::pointer;

template< typename ObjectType >
pointer_to< ObjectType > get_pointer_to( ObjectType& object )
{ return { &object }; }



struct Universe;

struct Object 
{
    using pointer = initialized_ptr< Object >;

    Universe& get_universe();

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
{ return obj->get_universe(); }

// getting a pointer to a temporary (rvalue reference) requires a move 
// allocation placed in the object's universe
template< typename ObjectType >
pointer_to< ObjectType > get_pointer_to( ObjectType&& object )
{ return new ( universe_of( &object )) ObjectType{ std::move( object ) }; }

template< typename ObjectType, typename... ArgTypes >
pointer_to< ObjectType > new_object_of( Universe& universe, ArgTypes... args )
{ return new ( universe ) ObjectType{ args... }; }


struct Universe
{
    friend struct Object;
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

#endif