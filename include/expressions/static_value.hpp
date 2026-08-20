#ifndef __EXPRESSIONS_STATIC_VALUE_HPP__
#define __EXPRESSIONS_STATIC_VALUE_HPP__

#include "expressions/forward_decl.hpp"
#include "expressions/constant.hpp"

#include <type_traits>

namespace expressions {

using std::integral_constant;
using std::true_type, std::false_type;

////////////////////
/// StaticValue ///
//////////////////
/// 
/// Class to hold a value considered to be unchanging in an expression
///
/// @brief wrapper to turn any type into an expression
/// @tparam T the wrapped type
///
///
template< typename T >
struct StaticValue;

template< typename T >
struct IsExpression< StaticValue< T >>: integral_constant< bool,
    not IsExpression< T >::value > { };

/// @brief result of a static value is the result of it's value type
template< typename T >
struct Result< StaticValue< T >>
{ using type = std::remove_cv_t< T >; };

template< typename T >
// requires( not is_expression_v< T > ) // not sure if we need this.. meta expressions?
struct StaticValue //< T >
{ 
    using value_type = std::remove_cv_t< T >;

    // casting to and from an expression should be explicit
    explicit constexpr 
    operator value_type() const
    { return _value; } 

    constexpr value_type 
    get_value() const
    { return _value; }

    template< typename... Ts >
    constexpr value_type 
    operator ()( Ts const&... ) const
    { return _value; }

    template< typename U >
    constexpr Chain< StaticValue< T >, U >
    operator ,( U const& next ) const;

    constexpr StaticValue(): _value{} { }
    constexpr StaticValue( value_type const& other ): _value{ other } { }
    constexpr StaticValue( StaticValue const& ) = default;
    constexpr StaticValue( StaticValue&& ) = default;

private:
    value_type _value;
};

template< typename T >
struct MakeStaticExpr
{ 
    using type = StaticValue< T >;
    static constexpr type
    value( T const& val )
    { return { val }; }
};

template< arithmetic auto X >
requires( X == 0 )
struct MakeStaticExpr< Constant< X >>
{
    using type = Constant< 0 >;
    static consteval type
    value( Constant< X > )
    { return {}; }
};

template< >
struct MakeStaticExpr< exact_zero >
{
    using type = Constant< 0 >;
    static consteval type
    value( exact_zero )
    { return {}; }
};

/// @brief helper to create expressions with unchanging values
/// @tparam T the type of this expression
/// @param value the value of this expression
/// @return returns an expression that will always evaluate to value
///
template< typename T >
requires( not is_expression_v< T > )
constexpr typename MakeStaticExpr< T >::type 
static_expr( T const& val )
{ return MakeStaticExpr< T >::value( val ); }

/// we return a Constant< 0 > if the value is exactly zero
//template< > 
//consteval Constant< 0 >
//static_expr( exact_zero )
//{ return {}; }
//
//template< auto N >
//consteval Constant< N >
//static_expr( Constant< N > )
//{ return {}; }

// results in a static expression unless T is already an expression type
template< typename T >
struct MakeExpression
{ 
    using type = StaticValue< T >;
    static constexpr type 
    value( T const& value )
    { return type{ value }; }
};

template< >
struct MakeExpression< exact_zero >
{
    using type = Constant< 0 >;
    static consteval type
    value( exact_zero )
    { return {}; }
};

template< expression T >
struct MakeExpression< T >
{
    using type = T;
    static constexpr type
    value( T const& value )
    { return value; }
};

template< typename T >
using make_expression_t = MakeExpression< T >::type;

template< typename T >
constexpr make_expression_t< T >
make_expression( T const& value )
{ return MakeExpression< T >::value( value ); }

} // namespace expressions

#endif
