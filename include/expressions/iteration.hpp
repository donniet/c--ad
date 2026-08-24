#ifndef __EXPRESSIONS_ITERATION_HPP__
#define __EXPRESSIONS_ITERATION_HPP__

#include "expressions/expressions.hpp"
#include "expressions/conditional.hpp"

namespace expressions {

// inspiration:
//
// scope( n = 0, m = 0 );
// while_do( n < 100, m += n, n++ ) | scope;
//
// while_( n < 100 ).do_( m += n, n++ ) | iterate_on( scope );
//
// assert( scope( n ) == 100 and scope( m ) = 5500 );

template< typename Cond, typename... Body >
struct WhileDoer;

template< >
struct IsCompoundOperation< WhileDoer >: true_type { };

template< typename Cond, typename... Body >
struct WhileDoer: Compound< WhileDoer< Cond, Body... >>
{
    static constexpr size_t next_id = next_var_id_v< 
        WhileDoer< Cond, Body... >>;
    using place_type = Var< next_id, result_t< Cond >>;
    using var_type = Var< next_id, 
        Func< select_t< Cond, link_t< Body..., place_type >, Cond >, 
            place_type >>;

    using func_var = Func< var_type, var_type >;
    using expr_type = 
        Func< select_t< Cond, link_t< Body..., place_type >, Cond >,
            place_type >;

    
    // expr_type f;
    // auto 


    // 
    //  
    // 

    // select_t< Var< N, bool >, 
    //      
    
    static constexpr select_t< Cond, link_t< Body... >, Cond >
    value( Cond const& cond, Body const&... body )
    { return select( cond, link( body... ), cond ); }

    using Compound< WhileDoer< Cond, Body... >>::Compound;
};

template< expression Cond, typename... Body >
struct WhileDoer< Cond, Body... >: Compound< WhileDoer< Cond, Body... >>
{
    static constexpr auto
    value( Cond const& cond, Body const&... body )
    { 
        auto self = WhileDoer< Cond, Body... >{ cond, body... };
        return select( cond(), link( body..., self ), cond ); 
    }

    using Compound< WhileDoer< Cond, Body... >>::Compound;
};

template< typename Cond, typename... Body >
constexpr WhileDoer< Cond, Body... >
while_do( Cond const& cond, Body const&... body )
{ return { cond, body... }; }

} // namespace expressions


#endif // __EXPRESSIONS_ITERATION_HPP__
