#ifndef __EXPRESSIONS_STATIC_VALUE_HPP__
#define __EXPRESSIONS_STATIC_VALUE_HPP__

#include "expressions/forward_decl.hpp"

namespace expressions {

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
struct IsExpression< StaticValue< T >>: std::true_type { };

/// @brief result of a static value is the result of it's value type
template< typename T >
struct Result< StaticValue< T >>
{ using type = std::remove_cv_t< T >; };

template< typename T >
requires( not is_expression_v< T > ) // not sure if we need this.. meta expressions?
struct StaticValue< T >
{ 
    using value_type = std::remove_cv_t< T >;

    // casting to and from an expression should be explicit
    explicit constexpr operator value_type() const
    { return _value; } 

    constexpr value_type get_value() const
    { return _value; }

    constexpr value_type operator ()() const
    { return _value; }

    constexpr StaticValue(): _value{} { }
    constexpr StaticValue( value_type const& other ): _value{ other } { }
    constexpr StaticValue( StaticValue const& ) = default;
    constexpr StaticValue( StaticValue&& ) = default;

private:
    value_type _value;
};

/// @brief helper to create expressions with unchanging values
/// @tparam T the type of this expression
/// @param value the value of this expression
/// @return returns an expression that will always evaluate to value
///
template< typename T >
requires( not is_expression_v< T > )
constexpr StaticValue< T > static_expr( T const& value )
{ return StaticValue< T >{ value }; }

// results in a static expression unless T is already an expression type
template< typename T >
struct MakeExpression
{ 
    using type = StaticValue< T >;
    static constexpr type 
    value( T const& value )
    { return { value }; }
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
