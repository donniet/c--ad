#ifndef __EXPRESSIONS_COMPARISON_HPP__
#define __EXPRESSIONS_COMPARISON_HPP__

#include "expressions/expressions.hpp"

namespace expressions {

////////////////
/// Compare ///
//////////////
/// 
/// @brief method that will result in a 3-way comparison
template< typename T, typename U >
struct Compare;

template< >
struct IsCompoundOperation< Compare >: std::true_type { };

template< typename T, typename U >
struct Compare: Compound< Compare< T, U >>
{
    static constexpr auto
    value( T const& t, U const& u )
    { return t <=> u; }

    using Compound< Compare< T, U >>::Compound;
};

template< typename T, expression U >
Compare< T, U > operator <=>( T const& t, U const& u )
{ return { t, u }; }


///////////////////
/// EqualsZero ///
/////////////////
///
template< typename T >
struct EqualsZero;

template< >
struct IsCompoundOperation< EqualsZero >: std::true_type { };

/// @brief equals zero expression
///
/// This is not intended to be used to construct expressions, but 
/// instead is used in the canonical form of all other comparisons
template< typename T >
struct EqualsZero: Compound< EqualsZero< T >>
{
    static constexpr auto 
    value( T const& arg )
    { return arg == 0; }

    using Compound< EqualsZero< T >>::Compound;
};

///////////////
/// Equals ///
/////////////
///
template< typename T, typename U >
struct Equals;

template< >
struct IsCompoundOperation< Equals >: std::true_type { };

/// @brief equality expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct Equals: Compound< Equals< T, U >>
{ 
    static constexpr auto
    value( T const& left, U const& right )
    { return left == right; }
    
    using Compound< Equals< T, U >>::Compound;
};

//////////////////
/// NotEquals ///
////////////////
///
template< typename T, typename U >
struct NotEquals;

template< >
struct IsCompoundOperation< NotEquals >: std::true_type { };

/// @brief non-equality expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct NotEquals: Compound< NotEquals< T, U >>
{ 
    static constexpr auto
    value( T const& left, U const& right )
    { return left != right; }
    
    using Compound< NotEquals< T, U >>::Compound;
};

////////////////////
/// GreaterThan ///
//////////////////
///
template< typename T, typename U >
struct GreaterThan;

template< >
struct IsCompoundOperation< GreaterThan >: std::true_type { };

/// @brief greater than expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct GreaterThan: Compound< GreaterThan< T, U >>
{ 
    static constexpr auto
    value( T const& left, U const& right )
    { return left > right; }
    
    using Compound< GreaterThan< T, U >>::Compound;
};

/////////////////
/// LessThan ///
///////////////
///
template< typename T, typename U >
struct LessThan;

template< >
struct IsCompoundOperation< LessThan >: std::true_type { };

/// @brief less than expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct LessThan: Compound< LessThan< T, U >>
{ 
    static constexpr auto
    value( T const& left, U const& right )
    { return left < right; }
    
    using Compound< LessThan< T, U >>::Compound;
};

////////////////////////////
/// GreaterThanOrEquals ///
//////////////////////////
///
template< typename T, typename U >
struct GreaterThanOrEquals;

template< >
struct IsCompoundOperation< GreaterThanOrEquals >: std::true_type { };

/// @brief greater than or equal to expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct GreaterThanOrEquals: Compound< GreaterThanOrEquals< T, U >>
{ 
    static constexpr auto
    value( T const& left, U const& right )
    { return left >= right; }
    
    using Compound< GreaterThanOrEquals< T, U >>::Compound;
};

/////////////////////////
/// LessThanOrEquals ///
///////////////////////
///
template< typename T, typename U >
struct LessThanOrEquals;

template< >
struct IsCompoundOperation< LessThanOrEquals >: std::true_type { };

/// @brief less than or equal to expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct LessThanOrEquals: Compound< LessThanOrEquals< T, U >>
{ 
    static constexpr auto
    value( T const& left, U const& right )
    { return left <= right; }
    
    using Compound< LessThanOrEquals< T, U >>::Compound;
};

/////////////////////////
/// Operation Traits ///
///////////////////////
/// 
namespace detail {

template< typename ExprT >
struct IsEquals: integral_constant< bool, false > { };

template< typename A, typename B >
struct IsEquals< Equals< A, B >>: integral_constant< bool, true > { };

} // namespace detail

template< typename ExprT >
constexpr bool is_equals_v = detail::IsEquals< ExprT >::value;

//////////////////
/// Operators ///
////////////////
/// 
// equality
template< expression T, expression U >
constexpr auto operator ==( T const& left, U const& right )
{ return Equals< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator ==( T const& left, U const& right )
{ return Equals< T, StaticValue< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator ==( T const& left, U const& right )
{ return Equals< StaticValue< T >, U >{ static_expr( left ), right }; }

template< typename TupleT, typename TupleU, size_t... Is >
constexpr auto tuple_equals_helper( TupleT const& left, TupleU const& right,
    seq< Is... > )
{ return (( get< Is >( left ) == get< Is >( right )) and ... ); }

template< typename TensorT, typename TensorU, size_t... Is >
constexpr auto tensor_equals_helper( TensorT const& left, TensorU const& right,
    seq< Is... > )
{ return (( tensor_get< Is >( left ) == get< Is >( right )) and ... ); }

template< typename... Ts, typename... Us >
requires( sizeof...( Ts ) == sizeof...( Us ) and 
    (( expression< Ts > or ... ) or ( expression< Us > or ... )))
constexpr auto operator==( tuple< Ts... > const& left, 
    tuple< Us... > const& right )
{ return tuple_equals_helper( left, right, make_seq< sizeof...( Ts )>{} ); }

template< typename... Ts, typename... Us >
requires( sizeof...( Ts ) == sizeof...( Us ) and 
    (( expression< Ts > or ... ) or ( expression< Us > or ... )))
constexpr auto operator!=( tuple< Ts... > const& left, 
    tuple< Us... > const& right )
{ return not tuple_equals_helper( left, right, make_seq< sizeof...( Ts )>{} ); }

template< typename ShapeT, typename... Ts, typename... Us >
requires( (( expression< Ts > or ... ) or ( expression< Us > or ... )))
constexpr auto operator==( Tensor< ShapeT, Ts... > const& left,
    Tensor< ShapeT, Us... > const& right )
{ return tensor_equals_helper( left, right, make_seq< sizeof...( Ts )>{} ); }

template< typename ShapeT, typename... Ts, typename... Us >
requires( (( expression< Ts > or ... ) or ( expression< Us > or ... )))
constexpr auto operator!=( Tensor< ShapeT, Ts... > const& left,
    Tensor< ShapeT, Us... > const& right )
{ return not tensor_equals_helper( left, right, make_seq< sizeof...( Ts )>{} ); }

// greater than
template< expression T, expression U >
constexpr auto operator >( T const& left, U const& right )
{ return GreaterThan< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator >( T const& left, U const& right )
{ return GreaterThan< T, StaticValue< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator >( T const& left, U const& right )
{ return GreaterThan< StaticValue< T >, U >{ static_expr( left ), right }; }

// less than
template< expression T, expression U >
constexpr auto operator <( T const& left, U const& right )
{ return LessThan< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator <( T const& left, U const& right )
{ return LessThan< T, StaticValue< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator <( T const& left, U const& right )
{ return LessThan< StaticValue< T >, U >{ static_expr( left ), right }; }

// greater than or equals
template< expression T, expression U >
constexpr auto operator >=( T const& left, U const& right )
{ return GreaterThanOrEquals< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator >=( T const& left, U const& right )
{ return GreaterThanOrEquals< T, StaticValue< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator >=( T const& left, U const& right )
{ return GreaterThanOrEquals< StaticValue< T >, U >{ static_expr( left ), right }; }

// less than or equals
template< expression T, expression U >
constexpr auto operator <=( T const& left, U const& right )
{ return LessThanOrEquals< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator <=( T const& left, U const& right )
{ return LessThanOrEquals< T, StaticValue< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator <=( T const& left, U const& right )
{ return LessThanOrEquals< StaticValue< T >, U >{ static_expr( left ), right }; }


} // namespace expressions

#endif

