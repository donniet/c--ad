// recursion tests

#include "expressions/expressions.hpp"
#include "expressions/arithmetic.hpp"
#include "expressions/conditional.hpp"
#include "expressions/logical.hpp"
#include "expressions/comparison.hpp"

#include "testing.hpp"

#include <print>

using std::print, std::println;
using namespace expressions;

template< size_t N, typename FuncT, typename OmegaT >
struct PrimitiveRecurse;

// recursive case
template< size_t N, typename FuncT, typename OmegaT >
struct PrimitiveRecurse
{
    using recurance_type = PrimitiveRecurse< N - 1, FuncT, OmegaT >;
    
    static constexpr auto
    value( FuncT const& f, OmegaT const& omega )
    { return sub( f, recurance_type::value( f, omega )); }
};

// base case
template< typename FuncT, typename OmegaT >
struct PrimitiveRecurse< 0, FuncT, OmegaT >
{
    static constexpr OmegaT const&
    value( FuncT const&, OmegaT const& omega )
    { return omega; }
};

template< size_t N, typename FuncT, typename OmegaT >
constexpr auto 
primitive_recurse( FuncT const& func, OmegaT const& omega )
{ return PrimitiveRecurse< N, FuncT, OmegaT >::value( func, omega ); }

bool test_recursion()
{
    Var< 0, int > n;
    Var< 1, int > r;

    //auto fact = func( if_( n > 1_c, n * r( n - 1_c ), 1_c ), n, r );
    auto fact = func( n * r(n - 1_c), n, r );
    //static_assert( closed_expression< std::remove_cvref_t< decltype( sub( sub( fact, 1_c ), 1_c ))>> );

    // TODO: continue on fixing this recursion
    auto do_fact = primitive_recurse< 5 >( substitute( fact, 5_c ), 1_c );

    //static_assert( is_same_v< void, decltype( do_fact )> );
    static_assert( not open_expression< std::remove_cvref_t< decltype( do_fact )>> );

    println( "primitive_recurse< 5 >( fact, 1 ) = {}", do_fact );

    return true;
}

int main( int ac, char* av[] )
{
    println( "EXPRESSION RECURSION TESTS..." );
    ensure( test_recursion, "recursion" );
    println( "SUCCESS." );
    return EXIT_SUCCESS;
}
