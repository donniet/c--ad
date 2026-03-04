#ifndef __EXPRESSIONS_HPP__
#define __EXPRESSIONS_HPP__

#include "utility.hpp"

#include <type_traits>
#include <tuple>
#include <map>
#include <any>

namespace expressions {

using std::size_t;
using std::tuple, std::make_tuple, std::tuple_element_t, std::get;
using std::map;
using std::any, std::any_cast;

// parent class must define result_type and eval members
struct ExpressionTag 
{ 
    // usint result_type...
    // constexpr result_type eval( variable_values const& ) const...
};

template< typename T >
struct is_expression
{ static constexpr bool value = std::is_base_of_v< ExpressionTag, T >; };

template< typename T >
static constexpr bool is_expression_v = is_expression< T >::value;

/**
 * storage for variable valuess
 */
struct variable_values 
{ 
    any operator[]( size_t I ) const 
    { 
        auto i = values.find( I );
        if( i == values.end() )
            return {};
        return i->second;
    }
    any& operator[]( size_t I ) { return values[I]; }
    map< size_t, any > values;
};

template< typename T >
struct Evaluator;

template< typename T >
requires( is_expression_v< T > )
struct Evaluator< T >
{ 
    using result_type = T::result_type;

    constexpr static result_type evaluate( T const& expr, 
        variable_values const& vars )
    { return expr.eval( vars ); }
};

template< typename T >
requires( not is_expression_v< T > )
struct Evaluator< T >
{ 
    using result_type = T;
    constexpr static T evaluate( T const& expr, variable_values const& )
    { return expr; }; 
};

template< typename T >
using result_t = Evaluator< T >::result_type;

/**
 * expressions can be evaluated, and by default just return the expression.
 */
template< typename T >
constexpr result_t< T > eval( T const& expr, 
    variable_values const& vars)
{ return Evaluator< T >::evaluate( expr, vars ); }

template< typename T >
constexpr result_t< T > eval( T const& expr )
{ return eval( expr, {} ); }

/**
 * expressions can have variables
 */
template< size_t I, typename T >
// DT: do we need this?
// requires( not is_expression_v< T > )
struct Variable : ExpressionTag
{
    using value_type = T;
    using result_type = value_type;
    static constexpr size_t index = I;

    constexpr T eval( variable_values const& vars ) const
    { return any_cast< T >( vars[ I ] ); }
};

/**
 * Wrapper to identify something as an expression
 */
template< typename T >
struct Expression : ExpressionTag
{ 
    using value_type = T;
    using result_type = result_t< T >;

    static constexpr bool contains_expression = true;

    constexpr value_type const& get() const
    { return _value; }

    constexpr result_type eval( variable_values const& vars ) const
    { return expressions::eval( get(), vars ); }

    explicit operator value_type() const
    { return get(); }
    constexpr Expression() = default;
    explicit constexpr Expression( value_type const& expr ): _value{ expr } { }

    value_type _value;
};

template< typename T >
consteval T strip( T const& t )
{ return t; }

template< typename T >
consteval T strip( Expression< T > const& t )
{ return t.get(); }

template< typename T >
struct strip_expression
{ using type = T; };

template< typename T >
struct strip_expression< Expression< T >>
{ using type = T; };

template< typename T >
using stripped_t = strip_expression< T >::type;

/**
 * A static expression with an unchanging value defined at run-time
 */
template< typename T >
Expression< T > static_expr( T const& value )
{ return { value }; }

template< size_t I, typename T >
using variable = Expression< Variable< I, T >>;

/**
 * Constant expression
 */
template< typename T, T Value >
struct Constant : ExpressionTag
{
    using value_type = T;
    using result_type = value_type;
    static constexpr T value = Value;
    constexpr value_type const& get() const { return value; };

    consteval result_type eval( variable_values const& ) const
    { return value; }
};

template< typename T, T Value >
using constant = Expression< Constant< T, Value >>;

static constexpr constant< long double, 0.l > constant_zero = constant< long double, 0.l >{{}};
static constexpr constant< long double, 1.l > constant_one = constant< long double, 1.l >{{}};
static constexpr constant< bool, true > constant_true = constant< bool, true >{{}};
static constexpr constant< bool, false > constant_false = constant< bool, false >{{}};

/**
 * a unary expression
 */
template< typename T >
struct Unary : ExpressionTag
{
    using value_type = T;
    constexpr value_type const& get() const { return _arg; }
    constexpr Unary( value_type const& arg ): _arg{ arg } { }
    value_type _arg;
};

/**
 * a binary expression
 */
template< typename T, typename U >
struct Binary : ExpressionTag
{
    using left_value_type = T;
    using right_value_type = U;

    constexpr left_value_type const& get_left() const { return _left; }
    constexpr right_value_type const& get_right() const { return _right; }

    constexpr Binary( T const& left, U const& right ) : _left{ left }, _right{ right }
    { }

    left_value_type _left;
    right_value_type _right;
};

/**
 * an nary expression
 */
template< typename... Ts >
struct Nary : ExpressionTag
{
    using value_tuple_type = tuple< Ts... >;

    template< size_t I >
    constexpr tuple_element_t< I, value_tuple_type > get() const
    { return std::get< I >( _values ); }

    constexpr Nary( Ts const&... ts ): _values{ ts... } {}

    value_tuple_type _values;
};


/**
 * a negation
 */
template< typename T >
struct Negation : Unary< T >
{ 
    using result_type = decltype( -result_t< T >{} );
    constexpr result_type eval( variable_values const& vars ) const
    { return -eval( Unary< T >::get(), vars ); }

    constexpr Negation( T const& arg ): Unary< T >{ arg } {} 
};

/**
 * a sum
 */
template< typename T, typename U >
struct Sum : Binary< T, U >
{ 
    using result_type = decltype( result_t< T >{} + result_t< U >{} );
    constexpr auto eval( variable_values const& vars ) const
    { return expressions::eval( Binary< T, U >::get_left(), vars ) + 
        expressions::eval( Binary< T, U >::get_right(), vars ); }

    constexpr Sum( T const& left, U const& right ): 
        Binary< T, U >{ left, right } { } 
};

/**
 * a difference
 */
template< typename T, typename U >
struct Difference : Binary< T, U >
{ 
    using result_type = decltype( result_t< T >{} - result_t< U >{} );
    constexpr result_type eval( variable_values const& vars ) const
    { return expressions::eval( Binary< T, U >::get_left(), vars ) - 
        expressions::eval( Binary< T, U >::get_right(), vars ); }

    constexpr Difference( T const& left, U const& right ): 
        Binary< T, U >{ left, right } { } 
};

/**
 * a product
 */
template< typename T, typename U >
struct Product : Binary< T, U >
{ 
    constexpr auto eval( variable_values const& vars ) const
    { return eval( Binary< T, U >::get_left(), vars ) *
        eval( Binary< T, U >::get_right(), vars ); }

    constexpr Product( T const& left, U const& right ): 
        Binary< T, U >{ left, right } { } 
};

/**
 * a quotent
 */
template< typename T, typename U >
struct Quotient : Binary< T, U >
{ 
    constexpr auto eval( variable_values const& vars ) const
    { return eval( Binary< T, U >::get_left(), vars ) / 
        eval( Binary< T, U >::get_right(), vars ); }

    constexpr Quotient( T const& numerator, U const& denominator ):
        Binary< T, U >{ numerator, denominator } { } 
};

/**
 * ElementOf expression
 */
template< size_t I, typename ArrayT >
struct ElementOf: Unary< ArrayT >
{ 
    using result_type = result_t< tuple_element_t< I, ArrayT >>;

    constexpr result_type eval( variable_values const& vars ) const
    { return eval( get< I >( Unary< ArrayT >::get() ), vars ); }

    constexpr ElementOf( ArrayT const& arr ): Unary< ArrayT >{ arr } {} 
};

/**
 * ArrayOf expression
 */
template< typename... Ts >
struct ArrayOf: Nary< Ts... >
{ 
    using result_type = tuple< result_t< Ts >... >;

    template< size_t... Is >
    constexpr result_type eval_helper( variable_values const& vars, seq< Is... > ) const
    { return make_tuple( expressions::eval( Nary< Ts... >::template get< Is >(), vars )... ); }
    constexpr auto eval( variable_values const& vars ) const
    { return eval_helper( vars, make_seq< sizeof...( Ts )>{} ); }

    constexpr ArrayOf( Ts const&... ts ): Nary< Ts... >{ ts... } {} 
};


template< size_t I, typename... Ts >
struct ElementOf< I, ArrayOf< Ts... >>: Unary< ArrayOf< Ts... >>
{ 
    using result_type = result_t< tuple_element_t< I, tuple< Ts... >>>;

    constexpr result_type eval( variable_values const& vars ) const
    { return expressions::eval( Unary< ArrayOf< Ts... >>::get().template get< I >(), vars ); }

    constexpr ElementOf( ArrayOf< Ts... > const& arr ): 
        Unary< ArrayOf< Ts... > >{ arr } {} 
};

/**
 * equality testing
 */
template< typename T, typename U >
struct Equals : Binary< T, U >
{ 
    using result_type = bool;

    constexpr result_type eval( variable_values const& vars ) const
    { return expressions::eval( Binary< T, U >::get_left(), vars ) == 
        expressions::eval( Binary< T, U >::get_right(), vars ); }

    constexpr Equals( T const& left, T const& right ):
        Binary< T, U >{ left, right } { } 
};


/**
 * OPERATORS
 */

// arrays
template< typename... Ts >
constexpr auto array_of( Ts const&... ts )
{ return Expression< ArrayOf< stripped_t< Ts >... >>{{ strip( ts )... }}; }

// accessor
template< size_t I, typename T >
constexpr auto element_of( Expression< T > const& arg )
{ return Expression< ElementOf< I, T >>{ arg.get() }; }

template< size_t I, typename T >
constexpr auto element_of( T const& arg )
{ return Expression< ElementOf< I, T >>{ arg }; }

// negation
template< typename T >
constexpr auto operator-( Expression< T > const& arg )
{ return Expression< Negation< T >>{ arg.get() }; }

// addition
template< typename T, typename U >
constexpr auto operator+( Expression< T > const& left, 
    Expression< U > const& right )
{ return Expression< Sum< T, U >>{{  left.get(), right.get() }}; }

template< typename T, typename U >
constexpr auto operator+( Expression< T > const& left, 
    U const& right )
{ return Expression< Sum< T, U >>{{ left.get(), right }}; }

template< typename T, typename U >
constexpr auto operator+( T const& left, 
    Expression< U > const& right )
{ return Expression< Sum< T, U >>{{ left, right.get() }}; }

// subtraction
template< typename T, typename U >
constexpr auto operator-( Expression< T > const& left, Expression< U > const& right )
{ return Expression< Difference< T, U >>{{ left.get(), right.get() }}; }

template< typename T, typename U >
constexpr auto operator-( Expression< T > const& left, 
    U const& right )
{ return Expression< Difference< T, U >>{{ left.get(), right }}; }

template< typename T, typename U >
constexpr auto operator-( T const& left, 
    Expression< U > const& right )
{ return Expression< Difference< T, U >>{{ left, right.get() }}; }

// multiplication
template< typename T, typename U >
constexpr auto operator*( Expression< T > const& left, 
    Expression< U > const& right )
{ return Expression< Product< T, U >>{ left.get(), right.get() }; }

// division
template< typename T, typename U >
constexpr auto operator/( Expression< T > const& left, 
    Expression< U > const& right )
{ return Expression< Quotient< T, U >>{ left.get(), right.get() }; }

// equality
template< typename T, typename U >
constexpr auto operator==( Expression< T > const& left, 
    Expression< U > const& right )
{ return Expression< Equals< T, U >>{ left.get(), right.get() }; }

} // namespace expressions


#endif