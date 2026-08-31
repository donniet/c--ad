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

// iterative evaluator
template< typename Condition, typename Scope >
struct DoWhile
{
    template< typename BodyT >
    constexpr auto
    operator ()( BodyT const& body )
    {
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
