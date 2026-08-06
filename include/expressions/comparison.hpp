#ifndef __EXPRESSIONS_COMPARISON_HPP__
#define __EXPRESSIONS_COMPARISON_HPP__

#include "expressions/expressions.hpp"

namespace expressions {

///////////////////
/// EqualsZero ///
/////////////////
///
template< typename T >
struct EqualsZero;

template< >
struct IsExpressionOperation< EqualsZero >: std::true_type { };

/// @brief equals zero expression
///
/// This is not intended to be used to construct expressions, but 
/// instead is used in the canonical form of all other comparisons
template< typename T >
struct EqualsZero: Arguments< EqualsZero, T >
{
    using result_type = bool;

    constexpr T arg() const { return get_argument< 0 >( *this ); }

    template< typename U >
    static constexpr auto value( U const& val )
    { return val == 0; }

    constexpr EqualsZero( T arg ): Arguments< EqualsZero, T >{ arg } { }
    constexpr EqualsZero() = default;
};

///////////////
/// Equals ///
/////////////
///
template< typename T, typename U >
struct Equals;

template< >
struct IsExpressionOperation< Equals >: std::true_type { };

/// @brief equality expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct Equals: Arguments< Equals, T, U >
{ 
    using result_type = bool;

    constexpr T left_arg() const { return get_argument< 0 >( *this ); }
    constexpr U right_arg() const { return get_argument< 1 >( *this ); }
    
    template< typename V, typename W >
    static constexpr auto value( V const& left, W const& right )
    { return ( left == right ); } 

    constexpr Equals( T left, U right ): 
        Arguments< Equals, T, U >{ left, right } { }
    constexpr Equals() = default;
};

//////////////////
/// NotEquals ///
////////////////
///
template< typename T, typename U >
struct NotEquals;

template< >
struct IsExpressionOperation< NotEquals >: std::true_type { };

/// @brief non-equality expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct NotEquals: Arguments< NotEquals, T, U >
{ 
    using result_type = bool;

    constexpr T left_arg() const { return get_argument< 0 >( *this ); }
    constexpr U right_arg() const { return get_argument< 1 >( *this ); }
    
    template< typename V, typename W >
    static constexpr auto value( V const& left, W const& right )
    { return ( left != right ); } 

    constexpr NotEquals( T left, U right ): 
        Arguments< NotEquals, T, U >{ left, right } { }
    constexpr NotEquals() = default;
};

////////////////////
/// GreaterThan ///
//////////////////
///
template< typename T, typename U >
struct GreaterThan;

template< >
struct IsExpressionOperation< GreaterThan >: std::true_type { };

/// @brief greater than expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct GreaterThan: Arguments< GreaterThan, T, U >
{ 
    using result_type = bool;

    constexpr T left_arg() const { return get_argument< 0 >( *this ); }
    constexpr U right_arg() const { return get_argument< 1 >( *this ); }
    
    template< typename V, typename W >
    static constexpr auto value( V const& left, W const& right )
    { return ( left > right ); } 

    constexpr GreaterThan( T left, U right ): 
        Arguments< GreaterThan, T, U >{ left, right } { }
    constexpr GreaterThan() = default;
};

/////////////////
/// LessThan ///
///////////////
///
template< typename T, typename U >
struct LessThan;

template< >
struct IsExpressionOperation< LessThan >: std::true_type { };

/// @brief less than expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct LessThan: Arguments< LessThan, T, U >
{ 
    using result_type = bool;

    constexpr T left_arg() const { return get_argument< 0 >( *this ); }
    constexpr U right_arg() const { return get_argument< 1 >( *this ); }
    
    template< typename V, typename W >
    static constexpr auto value( V const& left, W const& right )
    { return ( left < right ); } 

    constexpr LessThan( T left, U right ): 
        Arguments< LessThan, T, U >{ left, right } { }
    constexpr LessThan() = default;
};

////////////////////////////
/// GreaterThanOrEquals ///
//////////////////////////
///
template< typename T, typename U >
struct GreaterThanOrEquals;

template< >
struct IsExpressionOperation< GreaterThanOrEquals >: std::true_type { };

/// @brief greater than or equal to expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct GreaterThanOrEquals: Arguments< GreaterThanOrEquals, T, U >
{ 
    using result_type = bool;

    constexpr T left_arg() const { return get_argument< 0 >( *this ); }
    constexpr U right_arg() const { return get_argument< 1 >( *this ); }
    
    template< typename V, typename W >
    static constexpr auto value( V const& left, W const& right )
    { return ( left >= right ); } 

    constexpr GreaterThanOrEquals( T left, U right ): 
        Arguments< GreaterThanOrEquals, T, U >{ left, right } { }
    constexpr GreaterThanOrEquals() = default;
};

/////////////////////////
/// LessThanOrEquals ///
///////////////////////
///
template< typename T, typename U >
struct LessThanOrEquals;

template< >
struct IsExpressionOperation< LessThanOrEquals >: std::true_type { };

/// @brief less than or equal to expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct LessThanOrEquals: Arguments< LessThanOrEquals, T, U >
{ 
    using result_type = bool;

    constexpr T left_arg() const { return get_argument< 0 >( *this ); }
    constexpr U right_arg() const { return get_argument< 1 >( *this ); }
    
    template< typename V, typename W >
    static constexpr auto value( V const& left, W const& right )
    { return ( left <= right ); } 

    constexpr LessThanOrEquals( T left, U right ): 
        Arguments< LessThanOrEquals, T, U >{ left, right } { }
    constexpr LessThanOrEquals() = default;
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

