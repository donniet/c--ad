#ifndef __EXPRESSIONS_CONDITIONAL_HPP__
#define __EXPRESSIONS_CONDITIONAL_HPP__

#include "expressions/expressions.hpp"

namespace expressions {

//////////////////////
/// Select method ///
////////////////////
///
template< typename SelectorT, typename... Options >
struct Select;

template< integral IntT, typename First, typename... Rest >
requires(( is_same_v< First, Rest > and ... and true )
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

template< typename SelectorT, typename... Options >
requires( integral< result_t< SelectorT >>
struct Selection;

template< >
struct IsCompoundOperation< Selection >: true_type { };

template< typename SelectorT, typename... Options >
requires( integral< result_t< SelectorT >>
struct Selection: Compound< Select< SelectorT, Options... >>
{
    static constexpr auto
    value( SelectorT const& selector, Options const&... options )
    { return select( selector, options... ); }

    using Compound< Select< SelectorT, Options... >>::Compound;
};

// if any option is open we cannot determine the type of the respose so we 
// return a selection itself
template< typename SelectorT, typename... Options >
requires( open_expresssion< SelectorT > or 
    ( open_expression< Options > or ... or false ))
struct Select< SelectorT, Options... >
{
    using type = Selection< SelectorT, Options... >;

    static constexpr type
    value( SelectorT const& selector, Options const&... options )
    { return { selector, options... }; }
};


}; // namespace expressions

#endif // __EXPRESSIONS_CONDITIONAL_HPP__
