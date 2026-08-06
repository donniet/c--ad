#ifndef __EXPRESSIONS_CONSTANT_HPP__
#define __EXPRESSIONS_CONSTANT_HPP__

#include "expressions/forward_decl.hpp"

namespace expressions {

/////////////////
/// Constant ///
///////////////
///
/// Wraps a compile-time constant as an expression. This allows
/// for expression sipmlification at compile-time.
///
/// @brief a compile-time constant
/// @tparam T the type of the constant
/// @tparam Value of the constant
///
template< auto >
struct Constant;

template< auto Value >
struct IsExpression< Constant< Value >>: std::true_type { };

template< auto Value >
struct Result< Constant< Value >>
{ using type = std::remove_cv_t< decltype( Value )>; };

template< auto Value >
struct Constant
{
    using value_type = std::remove_cv_t< decltype( Value )>;

    static constexpr value_type value = Value;

    constexpr operator value_type() const
    { return value; }

    constexpr value_type operator ()() const
    { return value; }

    constexpr Constant() = default;
};

template< auto Value >
using constant = Constant< Value >;

// NOTE: not sure about the constants
constexpr constant< 0 > constant_zero = constant< 0 >{};
constexpr constant< 1 > constant_one = constant< 1 >{};
constexpr constant< true > constant_true = constant< true >{};
constexpr constant< false > constant_false = constant< false >{};

} // namespace expressions

#endif
