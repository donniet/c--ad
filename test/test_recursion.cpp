// recursion tests

#include "expressions/expressions.hpp"
#include "expressions/arithmetic.hpp"
#include "expressions/conditional.hpp"
#include "expressions/logical.hpp"
#include "expressions/comparison.hpp"

#include "expressions/predicate.hpp"

#include "testing.hpp"

#include <print>

#define MAXIMUM_RECURSION_DEPTH 100

using std::print, std::println;
using namespace expressions;

//static_assert( is_same_v< void, free_variables_t< Conditional<GreaterThan<Var<0, int>, Constant<0L>>, Product<Var<0, int>, Sub<Var<1, Var<1, int>>, Difference<Var<0, int>, Constant<1L>>>>, Constant<1L>> >>);

//static_assert( is_same_v< void, free_variables_t< Func<Conditional<GreaterThan<Var<0, int>, Constant<0L>>, Product<Var<0, int>, Sub<Var<1, Var<1, int>>, Difference<Var<0, int>, Constant<1L>>>>, Constant<1L>>, Var<0, int>, Var<1, int>> >>);

// problem with open functions...
static_assert( open_expression< Func<Conditional<GreaterThan<Var<0, int>, Constant<0L>>, Product<Var<0, int>, Sub<Var<1, Var<1, int>>, Difference<Var<0, int>, Constant<1L>>>>, Constant<1L>>, Var<0, int>, Var<1, int>> >);

template< size_t N, size_t RId, typename FuncT, typename AlphaT, typename OmegaT >
struct PrimitiveRecurse;

template< size_t N, size_t RId, typename FuncT, typename AlphaT, typename OmegaT >
constexpr auto 
primitive_recurse( FuncT const& func, AlphaT const& alpha, OmegaT const& omega )
{ 
    return PrimitiveRecurse< N, RId, FuncT, AlphaT, OmegaT >::
        value( func, alpha, omega ); 
}


// recursive case
template< size_t N, size_t RId, typename FuncT, typename AlphaT, 
    typename OmegaT >
struct PrimitiveRecurse
{
    static constexpr auto
    value( FuncT const& f, AlphaT const& alpha, OmegaT const& omega )
    { 
        return primitive_recurse< N-1, RId >( f, substitute( f, alpha, f ), 
            omega );
    }
};

// base case
template< size_t RId, typename FuncT, typename AlphaT, typename OmegaT >
struct PrimitiveRecurse< 0, RId, FuncT, AlphaT, OmegaT >
{
    static constexpr OmegaT const&
    value( FuncT const&, AlphaT const& alpha, OmegaT const& omega )
    { return omega; }
};

// by default recursion just returns the argument
template< size_t Depth, size_t RId, typename ExprT, typename T >
constexpr auto
recurse_value( ExprT const& expr, T const& arg )
{ return arg; }

// if R isn't free in the expression, return it as a simple substitution
template< size_t Depth, size_t RId, expression ExprT, typename T >
requires( not free_variables_t< ExprT >::contains_id( RId ))
constexpr auto
recurse_value( ExprT const& expr, T const& arg )
{ return expr( arg ); }

template< size_t Depth, size_t RId, expression ExprT, auto Value >
requires( free_variables_t< ExprT >::contains_id( RId ) and
    free_variables_t< ExprT >::size == 2 )
constexpr auto
recurse_value( ExprT const& expr, Constant< Value > const& arg )
{
    static_assert( Depth < MAXIMUM_RECURSION_DEPTH, 
        "recusion depth limited to " TO_STRING(MAXIMUM_RECURSION_DEPTH) "." );
};

template< size_t I >
struct ForRecurse
{
    template< typename TestT >
    struct Is: integral_constant< bool, false > { };

    // a recursive expression is a substitution into a variable whose id is I
    // that takesa single argument.  We return a match using the recursive
    // variable as the variable match
    template< variable Var, typename ExprT >
    struct Is< Sub< Var, ExprT >>: 
        integral_constant< bool, I == var_id_v< Var >>
    { using matches_type = tuple< match< Var, ExprT >>; };
};

template< size_t RId, typename ExprT >
struct Recursion;

template< >
struct IsDiscriminatedOperation< Recursion >: true_type { };

// the result of a recursion is defined to be the same as the result when the
// recursed variable is replaced with the identity
//template< size_t RId, typename ExprT >
//struct Result< Recursion< RId, ExprT >>: 
//    Result< SubFor< RId, ExprT, Identity >::type >
//{ };

template< size_t RId, typename ExprT >
struct Recursion: Compound< Recursion< RId, ExprT >>
{
    template< auto Value >
    static constexpr ExprT 
    value( ExprT const& expr )
    { return expr; }

    using Compound< Recursion< RId, ExprT >>::Compound;
};

template< size_t RId, expression ExprT >
requires( not free_variables_t< ExprT >::contains_id( RId ))
struct Recursion< RId, ExprT >: Compound< Recursion< RId, ExprT >>
{
    template< auto Value >
    static constexpr auto
    value( ExprT const& expr )
    { return expr(); }

    using Compound< Recursion< RId, ExprT >>::Compound;
};

template< size_t RId, expression ExprT >
requires( free_variables_t< ExprT >::contains_id( RId ) and
    is_greater( free_variables_t< ExprT >::size, 1 ))
struct Recursion< RId, ExprT >: Compound< Recursion< RId, ExprT >>
{
    // what is the first variable that will be substituted besides R in this 
    // expression?
    template< typename SubOrder >
    struct FirstVarId;

    template< variable First, variable... Rest >
    requires( var_id_v< First > == RId )
    struct FirstVarId< tuple< First, Rest... >>: 
        FirstVarId< Rest... > { };

    template< variable Last >
    struct FirstVarId< Last >: 
        std::integral_constant< size_t, var_id_v< Last >> { };

    static constexpr size_t parameter_var_id = 
        FirstVarId< typename DirectSubOrder< ExprT >::type >::value;

    template< size_t Depth, typename ArgT >
    struct Helper
    {
        static_assert( Depth < MAXIMUM_RECURSION_DEPTH,
            "maximum recursion depth limited to " 
                TO_STRING(MAXIMUM_RECURSION_DEPTH) );

        // substitute the new argument in for the parameter in the pattern
        using param_sub_type = SubFor< parameter_var_id, ExprT, ArgT >::type;

        // then substitute the recursion using a predicate

    };

    template< size_t Depth, constant_expression U >
    struct Helper< Depth, U >
    {
        using type = U;
        static constexpr type
        value( U const& expr )
        { return expr; }
    };

    // we override the operator() from Compound 
    template< constant_expression ArgT >
    constexpr auto
    operator ()( ArgT const& arg ) const
    { return Helper< 0, ArgT >::value( arg ); }

    using Compound< Recursion< RId, ExprT >>::Compound;
};

bool test_recursion()
{
    Var< 0, int > n;
    Var< 1, int > r;

    // we can't define the type of a recursion without the initial value

    //auto fact = func( if_( n > 1_c, n * r( n - 1_c ), 1_c ), n, r );
//    auto fact = recurse< r.id >( if_( n > 0_c, n * r(n - 1_c), 1_c ));

 //   auto ten_fact = fact( 10_c );
    //static_assert( closed_expression< std::remove_cvref_t< decltype( sub( sub( fact, 1_c ), 1_c ))>> );

    // TODO: continue on fixing this recursion
    //
    // DT: it's always returning the omega value 
    //static_assert( is_same_v< void, decltype( do_fact )> );
//    static_assert( not open_expression< std::remove_cvref_t< decltype( do_fact )>> );

//    println( "primitive_recurse< 5 >( fact, 1 ) = {}", do_fact );

    return true;
}

int main( int ac, char* av[] )
{
    println( "EXPRESSION RECURSION TESTS..." );
    test::ensure( test_recursion, "recursion" );
    println( "SUCCESS." );
    return EXIT_SUCCESS;
}
