#include <print>
#include <tuple>
#include <utility>
#include <type_traits>

#include <cmath>
#include <cassert>
#include <array>

#include "normalize.hpp"

int main( int ac, char* av[] )
{
    using namespace expressions::normalization;
    using namespace expressions::normalization::test;

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


