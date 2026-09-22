#ifndef __EXPRESSIONS_ITERATION_HPP__
#define __EXPRESSIONS_ITERATION_HPP__

#include "expressions/expressions.hpp"
#include "expressions/conditional.hpp"

namespace expressions {

// inspiration:
//
// scope( n = 0, m = 0 );
// ( n++, m += n ) | do_while( n < 100, scope )
//
// assert( scope( n ) == 100 and scope( m ) = 5500 );

template< typename ConditionT, typename BodyT >
constexpr ConditionT const&
loop_while( ConditionT const& cond, BodyT const& body )
{ return cond; }

template< size_t MaximumIterations, typename ConditionT, typename BodyT >
struct LoopWhile;

template< >
struct IsDiscriminatedOperation< LoopWhile >: true_type { };

template< size_t MaximumIterations, typename ConditionT, typename BodyT >
struct LoopWhile: Compound< LoopWhile< MaximumIterations, ConditionT, BodyT >>
{
    using this_type = LoopWhile< MaximumIterations, ConditionT, BodyT >;
    using next_type = LoopWhile< MaximumIterations - 1, ConditionT, BodyT >;

    static constexpr auto
    value( ConditionT cond, BodyT body )
    { return if_( cond, link( body, next_type{ cond, body }), false ); }

    using Compound< this_type >::Compound;
};

template< typename ConditionT, typename BodyT >
struct LoopWhile< 0, ConditionT, BodyT >: Compound< LoopWhile< 0, ConditionT, BodyT >>
{
    using this_type = LoopWhile< 0, ConditionT, BodyT >;

    static constexpr bool
    value( ConditionT, BodyT body )
    { return true; }

    using Compound< this_type >::Compound;
};

static constexpr size_t default_maximum_iterations = 0x10;

template< typename ConditionT, typename BodyT >
requires( expression< ConditionT > or expression< BodyT > )
constexpr LoopWhile< default_maximum_iterations, ConditionT, BodyT >
loop_while( ConditionT const& cond, BodyT const& body )
{ return { cond, body }; }


// iterative evaluator
template< typename Condition, typename Scope >
struct DoWhile
{
    // DT: this is called by the processor on each variable in the expression
    //     so we can either create specializations of this for variables
    //     or put this loop into the static value method
    template< typename BodyT >
    constexpr auto
    operator ()( BodyT const& body )
    {
        // return if_( cond(), body )
        auto pred = cond() | scope();
        while( pred )
        {
            body | scope();
            pred = cond() | scope();
        }
        return pred;
    }

    constexpr Condition const&
    cond() const
    { return _cond; }

    constexpr Scope&
    scope() const
    { return *_scope_ptr; }

    constexpr DoWhile() = delete;
    constexpr DoWhile( DoWhile const& ) = default;
    constexpr DoWhile( Condition const& cond, Scope& scope ):
        _cond{ cond }, _scope_ptr(&scope) { };
private:
    Condition _cond;
    Scope* _scope_ptr;
};

template< typename Condition, typename Scope >
constexpr DoWhile< Condition, Scope >
do_while( Condition const& cond, Scope& scope )
{ return { cond, scope }; }

} // namespace expressions

#endif // __EXPRESSIONS_ITERATION_HPP__
