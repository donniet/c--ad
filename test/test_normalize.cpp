#include <print>
#include <tuple>
#include <utility>
#include <type_traits>

#include <cmath>
#include <cassert>
#include <array>

#include "normalize.hpp"

using namespace expressions::normalization;

// forward decls
template< typename... Ts > struct And;
template< typename... Ts > struct Or;
template< typename T > struct Not;

template< typename... Ts >
constexpr And< Ts... > and_( Ts... );

template< typename... Ts >
constexpr Or< Ts... > or_( Ts... );

template< typename T >
constexpr Not< T > not_( T );

// disjunctive normal form helper
template< typename T >
using dnf = Normalizer< Or, And, Not, T >;

template< typename T >
using dnf_result_t = Normalizer< Or, And, Not, T >::type;

// some empty distinct types for testing
struct b0 { }; struct b1 { }; struct b2 { }; struct b3 { }; struct b4 { }; 
struct b5 { }; struct b6 { }; struct b7 { }; struct b8 { }; struct b9 { };

// class implementations
template< typename... Ts >
struct And: tuple< Ts... >
{ 
    template< size_t... Is >
    constexpr bool result_helper( seq< Is... > ) const
    { return ( get< Is >( *this ) and ... ); }

    constexpr operator bool() const
    { return result_helper( make_seq< sizeof...( Ts )>{} ); }

    constexpr And( Ts... ts ): tuple< Ts... >{ ts... } { }
};

template< typename... Ts >
struct Or: tuple< Ts... >
{ 
    template< size_t... Is >
    constexpr bool result_helper( seq< Is... > ) const
    { return ( get< Is >( *this ) or ... ); }

    constexpr operator bool() const
    { return result_helper( make_seq< sizeof...( Ts )>{} ); }

    constexpr Or( Ts... ts ): tuple< Ts... >{ ts... } { }
};

template< typename T >
struct Not: tuple< T >
{ 
    constexpr operator bool() const
    { return not get< 0 >( *this ); }

    constexpr Not( T t ): tuple< T >{ t } { }
};

template< typename... Ts >
constexpr And< Ts... > and_( Ts... ts )
{ return { ts... }; }

template< typename... Ts >
constexpr Or< Ts... > or_( Ts... ts )
{ return { ts... }; }

template< typename T >
constexpr Not< T > not_( T t )
{ return { t }; }

// template< typename T >
// constexpr T not_( Not< T > t )
// { return t; }

int main( int ac, char* av[] )
{
    // type tests
    static_assert( is_same_v< dnf_result_t< bool >, bool > );
    static_assert( is_same_v< dnf_result_t< b0 >, b0 > );
    static_assert( is_same_v< dnf_result_t< 
        And<And<b0,b1>,b2>>, 
        And< b0, b1, b2 >> );
    static_assert( is_same_v< dnf_result_t< 
        Or<b0,Or<b1,b2>>>,
        Or< b0,b1,b2 >> );
    static_assert( is_same_v< dnf_result_t< 
        Or<b0,Or<b1,Not<b2>>>>,
        Or< b0,b1,Not<b2>>> );
    static_assert( is_same_v< dnf_result_t< 
        Not< Or<b0,Or<b1,b2>>> >,
        And< Not<b0>, Not<b1>, Not<b2> >> );
    static_assert( is_same_v< dnf_result_t< 
        Not< Or<b0,Or<b1,Not<b2>>>> >,
        And< Not<b0>, Not<b1>, b2 >> );
    static_assert( is_same_v< dnf_result_t< 
        And<Or<b0,b1>,b2>>,
        Or< And<b0,b2>, And<b1,b2>>> );
    static_assert( is_same_v< dnf_result_t< 
        Or< And<b0,b1>, Or<b2,b3>>>,
        Or< And< b0,b1 >, b2, b3 >> );
    static_assert( is_same_v< dnf_result_t< 
        And< Or< b0,b1 >, Or< b2,b3,b4 > >>,
        Or< And< b0,b2 >, And< b1,b2 >, 
            And< b0,b3 >, And< b1,b3 >, 
            And< b0,b4 >, And< b1,b4 >>> );
    static_assert( is_same_v< dnf_result_t< 
        And< Or< b0,b1,b2 >, Or< b3,b4 > >>, 
        Or< And< b0,b3 >, And< b1,b3 >, And< b2,b3 >, 
            And< b0,b4 >, And< b1,b4 >, And< b2,b4 >>> );


    // ensuring test methods behave as expected
    static_assert( and_( true, true ));
    static_assert( not and_( true, false ));
    static_assert( or_( true, false ));
    static_assert( not or_( false, false ));
    static_assert( not not_( true ));
    static_assert( not_( false ));

    // ~(~(F&T)|F) == ~(~(F&T)) == ~~F == F
    // ~((~F|~T)|F)
    // ~(~F|~T)&~F
    // (F&T)&~F == F

    // TEST: ( F and T ) or F => 
    //  ( F and T ) or F =>
    //  F or F =>
    //  F
    constexpr auto a = or_( and_( false, true ), false );
    constexpr auto an = normalize< Or, And, Not >( a );
    static_assert( not a and (bool)an == (bool)a );
    static_assert( is_same_v< std::remove_cvref_t< decltype( an ) >, 
        Or< And< bool, bool >, bool >> );

    // TEST: not (( F and T ) or F ) => 
    //  not ( F and T ) and not F =>
    //  ( not F or not T ) and not F =>
    //  ( not F and not F ) or ( not T and not F ) =>
    //  ( T and T ) or ( F and T ) =>
    //  T or F =>
    //  T
    constexpr auto b = not_( or_( and_( false, true ), false ));
    constexpr auto bn = normalize< Or, And, Not >( b );
    static_assert( b and (bool)bn == (bool)b );
    static_assert( is_same_v< std::remove_cvref_t< decltype( bn ) >, 
        Or< And< Not< bool >, Not< bool >>, And< Not< bool >, Not< bool >>> > );



    return EXIT_SUCCESS;
}



