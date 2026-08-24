// expression result test

#include "expressions/expressions.hpp"

#include <print>
#include <type_traits>

using namespace expressions;
using std::print, std::println;

int main( int ac, char* av[] )
{
    static_assert( is_same_v< result_t< Constant< (int)1 >>, int >, 
        "constant int result is int" );
    static_assert( is_same_v< result_t< Var< 0, int >>, int >,
        "int variable result is int" );
    static_assert( is_same_v< result_t< Var< 0, Var< 0, int >>>, int >,
        "second order var int result is int" );
    static_assert( is_same_v< result_t< Var< 0, Constant< (int)1 >>>, int >,
        "formula variable int result is int" );

    return EXIT_SUCCESS;
}
