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

    constexpr auto a = not_( or_( not_( and_( false, true )), false ));
    constexpr auto an = normalize< Or, And >( a );



    return EXIT_SUCCESS;
}


