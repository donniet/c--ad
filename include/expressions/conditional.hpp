#ifndef __EXPRESSIONS_CONDITIONAL_HPP__
#define __EXPRESSIONS_CONDITIONAL_HPP__

#include "expressions/expressions.hpp"

namespace expressions {

// inspiration:
//
// select( n < 100, n, 100 );
// 
// scope( n = 0, m = 0 );
//
// auto g = f( pred, true_case, false_case );
// auto h = g( n >= 100, m, m += n++ )(  ;
// 
//
// func do_while( pred, body )
// {
//     body();
//     if( not pred() )
//        return;
//     do_while( pred, body );
// }
//
// 
//
// 
//

//////////////////////
/// Select method ///
////////////////////
///
template< typename SelectorT, typename... Options >
struct Select;

template< integral IntT, typename First, typename... Rest >
//requires(( std::is_convertible_v< First, Rest > and ... and true ))
struct Select< IntT, First, Rest... >
{
    using type = First;
    static constexpr type
    value( IntT option, First first, Rest... rest )
    {
        if( option <= 0 or option >= 1 + sizeof...( Rest ))
            return first;

        std::array< First, sizeof...( Rest )> rest_array{ rest... };

        return rest_array[ option - 1 ];
    };
};

template< typename SelectorT, typename... Options >
using select_t = Select< SelectorT, Options... >::type;

template< typename SelectorT, typename... Options >
constexpr select_t< SelectorT, Options... >
select( SelectorT const& selector, Options const&... options )
{ return Select< SelectorT, Options... >::value( selector, options... ); }

/////////////////////////////
/// Selection expression ///
///////////////////////////
///
template< typename SelectorT, typename... Options >
requires( integral< result_t< SelectorT >> )
struct Selection;

template< >
struct IsCompoundOperation< Selection >: true_type { };

template< typename SelectorT,typename... Options >
requires( integral< result_t< SelectorT >> )
struct Selection: Compound< Selection< SelectorT, Options... >>
{
    static constexpr auto
    value( SelectorT const& selector, Options const&... options )
    { return select( selector, options... ); }

    using Compound< Selection< SelectorT, Options... >>::Compound;
};

// if any option is open we cannot determine the type of the respose so we 
// return a selection itself
template< typename SelectorT, typename... Options >
requires( expression< SelectorT > or 
    ( expression< Options > or ... or false ))
struct Select< SelectorT, Options... >
{
    using type = Selection< SelectorT, Options... >;

    static constexpr type
    value( SelectorT const& selector, Options const&... options )
    { return { selector, options... }; }
};

//////////////////////
/// If expression ///
////////////////////
///
template< typename Cond, typename ResultT >
constexpr ResultT
if_( Cond const& cond, ResultT const& true_value, ResultT const& false_value )
{ return cond ? true_value : false_value; }

template< typename Cond, typename ThenT, typename ElseT >
struct Conditional;

template< >
struct IsCompoundOperation< Conditional >: true_type { };

template< typename Cond, typename ThenT, typename ElseT >
requires( std::is_convertible_v< result_t< ThenT >, result_t< ElseT >> )
struct Conditional< Cond, ThenT, ElseT >: 
    Compound< Conditional< Cond, ThenT, ElseT >>
{
    static constexpr auto
    value( Cond const& cond, ThenT const& then_value, ElseT const& else_value )
    { return if_( cond, then_value, else_value ); }

    using Compound< Conditional< Cond, ThenT, ElseT >>::Compound;
};

template< typename Cond, typename ThenT, typename ElseT >
requires( expression< Cond > or expression< ThenT > or expression< ElseT >)
constexpr Conditional< Cond, ThenT, ElseT >
if_( Cond const& cond, ThenT const& then_expr, ElseT const& else_expr )
{ return { cond, then_expr, else_expr }; }

}; // namespace expressions

#endif // __EXPRESSIONS_CONDITIONAL_HPP__
