#ifndef __EXPRESSIONS_HPP__
#define __EXPRESSIONS_HPP__

#include "tensor.hpp"
#include "utility.hpp"

#include <cmath>
#include <type_traits>
#include <tuple>
#include <map>
#include <any>
#include <string>
#include <typeinfo>
#include <set>
#include <optional>

#ifndef NO_EXPRESSION_PRINTING
#include <format>
#endif

// #include <string_view>

namespace expressions {

using std::size_t;
using std::tuple, std::make_tuple, std::tuple_element_t, std::get;
using std::map, std::set;
using std::any, std::any_cast;
using std::optional;

using namespace tensor;

/// @brief base class of any expression
///
struct ExpressionTag 
{
    // using result_type = ...
    // result_type eval() const ...
};

/// @brief trait to identify a type as an expression type
/// @tparam T is the type to be checked
template< typename T >
struct is_expression
{ static constexpr bool value = std::is_base_of_v< ExpressionTag, T >; };

/// @brief is true if T is an expression type
/// @tparam T is the type to be checked
///
template< typename T >
static constexpr bool is_expression_v = is_expression< T >::value;

template< typename T >
concept expression = is_expression_v< T >;

/// @brief a placeholder in an expression whose value can change
/// @tparam I is the index in the declared variables to this variable
/// @tparam T is the type of this variable
///
template< size_t I, typename T >
struct Variable: ExpressionTag
{ 
    using value_type = T;
    using result_type = value_type;
    static constexpr size_t index = I;

    string name() const
    { return _name; }

    T eval() const
    { return *_ptr; }

    Variable& operator=( Variable const& ) = delete;
    template< typename U >
    requires( std::is_convertible_v< T, U > )
    Variable& operator =( U const& other )
    { 
        *_ptr = static_cast< T >( other ); 
        return *this;
    }
    Variable( T* ptr, string name ): 
        _ptr{ ptr }, _name{ name } {}
    constexpr Variable() = default;

    T* _ptr;
    string _name;
};

template< size_t... Is >
array< string, sizeof...( Is ) > default_variable_names_helper( seq< Is... > )
{ return { ( string( "var ") + std::to_string( Is ))... }; }

/// @brief the declaration of a variable in a delcare_variables function
/// @tparam T the type of the variable
///
template< typename T >
struct VariableDeclaration
{ 
    using value_type = T;

    string const& name() const
    { return _name; }

    string _name = ""; 
};

/// @brief primary way to declare a variable inside a declare_variables expression
/// @tparam T the type of this variable
/// @param name the name of this variable
/// @return a declaration of a variable
template< typename T >
VariableDeclaration< T > var( string name = "" )
{ return { name }; }

template< typename T >
struct IsVariableDeclaration: std::integral_constant< bool, false > {};

template< typename T >
struct IsVariableDeclaration< VariableDeclaration< T >>: 
    std::integral_constant< bool, true > {};

template< typename T >
constexpr bool is_variable_declaration_v = IsVariableDeclaration< T >::value;

template< typename T >
concept variable_declaration = is_variable_declaration_v< T >;

template< typename TupleT, typename Seq >
struct VariablesTupleHelper;

template< typename TupleT, size_t... Is >
struct VariablesTupleHelper< TupleT, seq< Is... >>
{ using type = tuple< Variable< Is, tuple_element_t< Is, TupleT >>... >; };

template< typename... Ts >
struct VariablesTuple
{ using type = VariablesTupleHelper< tuple< Ts... >, make_seq< sizeof...( Ts )>>::type; };

template< typename... Ts >
using variables_tuple_t = VariablesTuple< Ts... >::type;

template< typename TupleT, typename NamesT, size_t... Is >
tuple< Variable< Is, tuple_element_t< Is, TupleT >>... >  
variables_tuple_helper( TupleT& vals, NamesT const& names, seq< Is... > )
{ return {{ &get< Is >( vals ), 
    get< Is >( names ) == "" ? "var" + to_string( Is ) : get< Is >( names ) }... }; }

template< typename... Ts >
variables_tuple_t< Ts... > variables_tuple( tuple< Ts... >& tup, 
    array< string, sizeof...( Ts )> const& names )
{ return variables_tuple_helper( tup, names, make_seq< sizeof...( Ts )>{} ); }

template< typename... Ts >
array< string, sizeof...( Ts ) > default_variable_names( string base = "var" )
{
    array< string, sizeof...( Ts )> ret;
    for( size_t i = 0; i < sizeof...( Ts ); ++i )
        ret[ i ] = "var" + to_string( i );
    return ret;
}

/// @brief container and factory for Variables. 
///
template< typename... Ts >
struct VariableList: variables_tuple_t< Ts... >
{
    using values_tuple_type = tuple< Ts... >;
    using variables_tuple_type = variables_tuple_t< Ts... >;
    static constexpr size_t size = sizeof...( Ts );

    variables_tuple_t< Ts... > all()
    { return *this; }

    template< typename Body >
    auto express( Body&& method )
    { return call_method_with_variables( method, make_seq< size >{} ); }
  
    template< typename... Names >
    requires( sizeof...( Names ) == sizeof...( Ts ) )
    VariableList( Names const&... names ): 
        variables_tuple_type{ variables_tuple( _values, 
            array< string, size >{ names... }) } 
    { }

    VariableList():
        variables_tuple_type{ variables_tuple( _values, 
            default_variable_names< Ts... >() ) }
    { }

private:
    template< typename Body, size_t... Is >
    auto call_method_with_variables( Body& method, seq< Is... > )
    { return method( get< Is >( *this )... ); }

    values_tuple_type _values;
};

/// @brief method to declare a set of variables to be used in expressions
/// @tparam ...Decls 
/// @param ...decls 
/// @return 
template< variable_declaration... Decls >
VariableList< typename Decls::value_type... >
declare_variables( Decls... decls )
{ return { decls.name()... }; }

namespace detail {

/// @brief evaluates a type T
/// @tparam T the type to be evaluated
///
template< typename T >
struct Evaluator;

/// @brief evaluator for expressions of type T
/// @tparam T the type of the expression
///
template< typename T >
requires( is_expression_v< T > )
struct Evaluator< T >
{ 
    using result_type = T::result_type;

    constexpr static result_type evaluate( T const& expr )
    { return expr.eval( ); }
};

template< shape S, typename... Ts >
struct Evaluator< Tensor< S, Ts... >>
{ 
    using result_type = Tensor< S, typename Evaluator< Ts >::result_type... >;

    template< size_t... Is >
    static constexpr result_type evaluate_helper( Tensor< S, Ts... > const& value, 
        seq< Is... > )
    { return { Evaluator< tensor_element_t< Is, Tensor< S, Ts... >>>::evaluate( 
        tensor_get< Is >( value ))... }; }

    // always return the value itselv
    static constexpr result_type evaluate( Tensor< S, Ts... > const& value )
    { return evaluate_helper( value, make_seq< Tensor< S, Ts... >::size() >{} ); }
};

/// @brief evaluator for non-expression types
/// @tparam T the type of the non-expression
///
template< typename T >
requires( not is_expression_v< T > and not is_tensor_v< T > )
struct Evaluator< T >
{ 
    using result_type = T;

    // always return the value itselv
    constexpr static T evaluate( T const& value )
    { return value; }; 
};

} // namespace detail

/// @brief trait for the result_type of an expression
/// @tparam T the tested type
///
template< typename T >
using result_t = detail::Evaluator< T >::result_type;

/// @brief evaluates an expression given a set of variable values
/// @tparam T the expression type
/// @param expr the expression to be evaluated
/// @param vars the values of variables in the expression
/// @return the result of evaluating expression expr using values stored in vars
///
template< typename T >
constexpr result_t< T > eval( T const& expr )
{ return detail::Evaluator< T >::evaluate( expr ); }

/// @brief trait to identify variables
/// @tparam T the type to be tested
///
template< typename T >
struct IsVariable
{ static constexpr bool value = false; };

template< size_t I, typename T >
struct IsVariable< Variable< I, T >>
{ static constexpr bool value = true; };

template< typename T >
constexpr bool is_variable_v = IsVariable< T >::value;

template< typename T >
concept variable = is_variable_v< T >;

/// @brief wrapper to turn any type into an expression
/// @tparam T the wrapped type
///
template< typename T >
requires( not is_expression_v< T > ) // not sure if we need this.. meta expressions?
struct StaticValue : ExpressionTag
{ 
    using value_type = T;
    using result_type = value_type;

    constexpr result_type eval( ) const
    { return _value; }

    // casting to and from an expression should be explicit
    explicit operator value_type() const
    { return _value; } 
    explicit constexpr StaticValue( value_type const& expr ): _value{ expr } { }

    constexpr StaticValue() = default;
    constexpr StaticValue( StaticValue const& ) = default;
    constexpr StaticValue( StaticValue&& ) = default;

    // template< typename Arg, typename... Args >
    // explicit constexpr Expression( Arg&& arg, Args&&... args ): _value{ arg, args... } {}

    value_type _value;
};

/// @brief helper to create expressions with unchanging values
/// @tparam T the type of this expression
/// @param value the value of this expression
/// @return returns an expression that will always evaluate to value
///
template< typename T >
requires( not is_expression_v< T > )
StaticValue< T > static_expr( T const& value )
{ return StaticValue< T >{ value }; }

/// @brief a compile-time constant
/// @tparam T the type of the constant
/// @tparam Value of the constant
///
template< typename T, T Value >
struct Constant : ExpressionTag
{
    using value_type = T;
    using result_type = value_type;

    static constexpr T value = Value;

    constexpr operator T() const
    { return value; }

    consteval result_type eval( ) const
    { return value; }
};

template< typename T, T Value >
using constant = Constant< T, Value >;

// NOTE: not sure about the constants
constexpr constant< long double, 0.l > constant_zero = constant< long double, 0.l >{};
constexpr constant< long double, 1.l > constant_one = constant< long double, 1.l >{};
constexpr constant< bool, true > constant_true = constant< bool, true >{};
constexpr constant< bool, false > constant_false = constant< bool, false >{};

/// @brief negation expression
/// @tparam T the negated type
///
template< typename T >
struct Negation: ExpressionTag
{ 
    using result_type = decltype( -result_t< T >{} );

    constexpr T arg() const { return _arg; }

    constexpr result_type eval( ) const
    { return -expressions::eval( arg() ); }

    constexpr Negation( T arg ): _arg{ arg } { } 
    constexpr Negation() = default;

    T _arg;
};

/// @brief sum expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct Sum: ExpressionTag
{ 
    using result_type = decltype( result_t< T >{} + result_t< U >{} );

    constexpr T left_arg() const { return _left; }
    constexpr U right_arg() const { return _right; }

    constexpr auto eval( ) const
    { return expressions::eval( left_arg() ) + expressions::eval( right_arg() ); }

    constexpr Sum( T left, U right ): 
        _left{ left }, _right{ right } { } 
    constexpr Sum() = default;
    
    T _left;
    U _right;
};

/// @brief difference expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct Difference: ExpressionTag
{ 
    using result_type = decltype( result_t< T >{} - result_t< U >{} );

    constexpr T left_arg() const { return _left; }
    constexpr U right_arg() const { return _right; }

    constexpr result_type eval( ) const
    { return expressions::eval( left_arg() ) - expressions::eval( right_arg() ); }

    constexpr Difference( T left, U right ): 
        _left{ left }, _right{ right } { } 
    constexpr Difference() = default;

    T _left;
    U _right;
};

/// @brief product expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct Product: ExpressionTag
{ 
    using result_type = decltype( result_t< T >{} * result_t< U >{} );

    constexpr T left_arg() const { return _left; }
    constexpr U right_arg() const { return _right; }

    constexpr result_type eval( ) const
    { return expressions::eval( left_arg() ) * expressions::eval( right_arg() ); }

    constexpr Product( T left, U right ): 
        _left{ left }, _right{ right } { } 
    constexpr Product() = default;

    T _left;
    U _right;
};

/// @brief quotient expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct Quotient: ExpressionTag
{ 
    using result_type = decltype( result_t< T >{} / result_t< U >{} );

    constexpr T numerator_arg() const { return _numerator; }
    constexpr U denominator_arg() const { return _denominator; }

    constexpr result_type eval( ) const
    { return expressions::eval( numerator_arg() ) / expressions::eval( denominator_arg() ); }

    constexpr Quotient( T numerator, U denominator ):
        _numerator{ numerator }, 
        _denominator{ denominator } { } 
    constexpr Quotient() = default;

    T _numerator;
    U _denominator;
};

/// @brief square root expression
/// @tparam T 
template< typename T >
struct SquareRoot: ExpressionTag
{
    using result_type = decltype( std::sqrt( result_t< T >{} ));

    constexpr T arg() const { return _arg; }

    constexpr result_type eval( ) const
    { return std::sqrt( expressions::eval( arg() )); }

    constexpr SquareRoot( T arg ): _arg{ arg } { } 
    constexpr SquareRoot() = default;

    T _arg;
};

/// @brief integral power expression
/// @tparam T 
/// @tparam Exp 
template< int Exp, typename T >
struct Power: ExpressionTag
{
    using result_type = decltype( std::pow< Exp >( result_t< T >{} ));

    constexpr T arg() const { return _arg; }

    constexpr result_type eval( ) const
    { return std::pow< Exp >( expressions::eval( arg() )); }

    constexpr Power( T arg ): _arg{ arg } {} 
    constexpr Power() = default;

    T _arg;
};

/// @brief sine expression
/// @tparam T 
template< typename T >
struct Sine: ExpressionTag
{
    using result_type = decltype( std::sin( result_t< T >{} ));

    constexpr T arg() const { return _arg; }

    constexpr result_type eval( ) const
    { return std::sin( expressions::eval( arg() )); }

    constexpr Sine( T arg ): _arg{ arg } { } 
    constexpr Sine() = default;

    T _arg;
};

/// @brief cosine expression
/// @tparam T 
template< typename T >
struct Cosine: ExpressionTag
{
    using result_type = decltype( std::cos( result_t< T >{} ));

    constexpr T arg() const { return _arg; }

    constexpr result_type eval( ) const
    { return std::cos( expressions::eval( arg() )); }

    constexpr Cosine( T arg ): _arg{ arg } { } 
    constexpr Cosine() = default;

    T _arg;
};

/// @brief tangent expression
/// @tparam T 
template< typename T >
struct Tangent: ExpressionTag
{
    using result_type = decltype( std::tan( result_t< T >{} ));

    constexpr T arg() const { return _arg; }

    constexpr result_type eval( ) const
    { return std::tan( expressions::eval( arg() )); }

    constexpr Tangent( T arg ): _arg{ arg } { } 
    constexpr Tangent() = default;

    T _arg;
};

/// @brief arcsine expression
/// @tparam T 
template< typename T >
struct Arcsine: ExpressionTag
{
    using result_type = decltype( std::asin( result_t< T >{} ));

    constexpr T arg() const { return _arg; }

    constexpr result_type eval( ) const
    { return std::asin( expressions::eval( arg() )); }

    constexpr Arcsine( T arg ): _arg{ arg } { } 
    constexpr Arcsine() = default;

    T _arg;
};

/// @brief arccosine expression
/// @tparam T 
template< typename T >
struct Arccosine: ExpressionTag
{
    using result_type = decltype( std::acos( result_t< T >{} ));

    constexpr T arg() const { return _arg; }

    constexpr result_type eval( ) const
    { return std::acos( expressions::eval( arg() )); }

    constexpr Arccosine( T arg ): _arg{ arg } { } 
    constexpr Arccosine() = default;

    T _arg;
};

/// @brief sine expression
/// @tparam T 
template< typename T >
struct Arctangent: ExpressionTag
{
    using result_type = decltype( std::atan( result_t< T >{} ));

    constexpr T arg() const { return _arg; }

    constexpr result_type eval( ) const
    { return std::atan( expressions::eval( arg() )); }

    constexpr Arctangent( T arg ): _arg{ arg } { } 
    constexpr Arctangent() = default;

    T _arg;
};

/// @brief arctangent of a slope expression
/// @tparam T rise type
/// @tparam U run type
template< typename T, typename U >
struct Arctangent2: ExpressionTag
{ 
    // we use the result_type of a fraction here to factor units properly
    // this assumes that std::atan2 doesn't change the unit. hopefully it stays
    // true that trig functions operate only on scalars and this won't be an 
    // issue.  
    using result_type = decltype( result_t< T >{} / result_t< U >{} );

    constexpr T numerator_arg() const { return _numerator; }
    constexpr U denominator_arg() const { return _denominator; }

    // TODO: write an eval for std::atan2 that handles units properly
    constexpr result_type eval( ) const;

    constexpr Arctangent2( T numerator, U denominator ):
        _numerator{ numerator }, 
        _denominator{ denominator } { } 
    constexpr Arctangent2() = default;

    T _numerator;
    U _denominator;
};

/// @brief extract the element from a tuple-like array
/// @tparam ArrayT 
/// @tparam I 
template< size_t I, typename ArrayT >
struct Element: tuple_element_t< I, ArrayT >
{  
    constexpr Element( ArrayT arr ): 
        tuple_element_t< I, ArrayT >{ std::get< I >( arr ) }
    { }
};

/// @brief equality expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct Equals: ExpressionTag
{ 
    using result_type = bool;

    constexpr T left_arg() const { return _left; }
    constexpr U right_arg() const { return _right; }

    constexpr result_type eval( ) const
    { return expressions::eval( left_arg() ) == expressions::eval( right_arg() ); }

    constexpr Equals( T left, U right ): 
        _left{ left }, _right{ right } { } 
    constexpr Equals() = default;
    
    T _left;
    U _right;
};

/// @brief logical and expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct Conjunction: ExpressionTag
{
    using result_type = bool;

    constexpr T left_arg() const { return _left; }
    constexpr U right_arg() const { return _right; }

    constexpr result_type eval( ) const
    { return expressions::eval( left_arg() ) and expressions::eval( right_arg() ); }

    constexpr Conjunction( T left, U right ): 
        _left{ left }, _right{ right } { } 
    constexpr Conjunction() = default;
    
    T _left;
    U _right;
};


/**
 * OPERATORS
 */

// negation
template< expression T >
constexpr auto operator -( T const& arg )
{ return Negation< T >{ arg }; }

// addition
template< expression T, expression U >
constexpr auto operator +( T const& left, U const& right )
{ return Sum< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator +( T const& left, U const& right )
{ return Sum< T, StaticValue< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator +( T const& left, U const& right )
{ return Sum< StaticValue< T >, U >{ static_expr( left ), right }; }

// subtraction
template< expression T, expression U >
constexpr auto operator -( T const& left, U const& right )
{ return Difference< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator -( T const& left, U const& right )
{ return Difference< T, StaticValue< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator -( T const& left, U const& right )
{ return Difference< StaticValue< T >, U >{ static_expr( left ), right }; }

// multiplication
template< expression T, expression U >
constexpr auto operator *( T const& left, U const& right )
{ return Product< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator *( T const& left, U const& right )
{ return Product< T, StaticValue< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator *( T const& left, U const& right )
{ return Product< StaticValue< T >, U >{ static_expr( left ), right }; }

// division
template< expression T, expression U >
constexpr auto operator /( T const& left, U const& right )
{ return Quotient< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator /( T const& left, U const& right )
{ return Quotient< T, StaticValue< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator /( T const& left, U const& right )
{ return Quotient< StaticValue< T >, U >{ static_expr( left ), right }; }

// trig functions
template< expression T >
constexpr auto sin( T const& arg )
{ return Sine< T >{ arg }; }

template< expression T >
constexpr auto cos( T const& arg )
{ return Cosine< T >{ arg }; }

template< expression T >
constexpr auto tan( T const& arg )
{ return Tangent< T >{ arg }; }

template< expression T >
constexpr auto asin( T const& arg )
{ return Arcsine< T >{ arg }; }

template< expression T >
constexpr auto acos( T const& arg )
{ return Arccosine< T >{ arg }; }

template< expression T >
constexpr auto atan( T const& arg )
{ return Arctangent< T >{ arg }; }

template< expression T, expression U >
constexpr auto atan2( T const& num, U const& den )
{ return Arctangent2< T, U >{ num, den }; }

// sqrt
template< expression T >
constexpr auto sqrt( T const& arg )
{ return SquareRoot< T >{ arg }; }

// pow
template< int Exp, expression T >
constexpr auto pow( T const& arg )
{ return Power< Exp, T >{ arg }; }

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

template< typename ShapeT, typename... Ts, typename... Us >
requires( (( expression< Ts > or ... ) or ( expression< Us > or ... )))
constexpr auto operator==( Tensor< ShapeT, Ts... > const& left,
    Tensor< ShapeT, Us... > const& right )
{ return tensor_equals_helper( left, right, make_seq< sizeof...( Ts )>{} ); }

// logical operations
template< expression T, expression U >
constexpr auto operator and( T const& left, U const& right )
{ return Conjunction< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator and( T const& left, U const& right )
{ return Conjunction< T, StaticValue< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator and( T const& left, U const& right )
{ return Conjunction< StaticValue< T >, U >{ static_expr( left ), right }; }


template< size_t I, typename T >
struct Differential
{
    using variable_type = Variable< I, T >;

    // derivative of a static value is 0
    template< typename U >
    auto operator()( StaticValue< U > const& expr )
    { return Constant< decltype( U{} / T{} ), 
        static_cast< decltype( U{} / T{} ) >( 0 ) >{}; }

    // derivative of a constant is 0
    template< typename U, U Value >
    auto operator()( Constant< U, Value > const& expr )
    { return Constant< decltype( U{} / T{} ), 
        static_cast< decltype( U{} / T{} ) >( 0 ) >{}; }

    template< size_t J, typename U >
    auto operator()( Variable< J, U > const& expr )
    { return Constant< decltype( U{} / T{} ), 
        static_cast< decltype( U{} / T{} ) >( I == J ? 1 : 0 )>{ }; }

    template< typename U >
    auto operator()( Negation< U > const& expr )
    { return -(*this)( expr.arg() ); }

    template< typename U, typename V >
    auto operator()( Sum< U, V > const& expr )
    { return (*this)( expr.left_arg() ) + (*this)( expr.right_arg() ); }

    template< typename U, typename V >
    auto operator()( Difference< U, V > const& expr )
    { return (*this)( expr.left_arg() ) - (*this)( expr.right_arg() ); }

    template< typename U, typename V >
    auto operator()( Product< U, V > const& expr )
    { return (*this)( expr.left_arg() ) * expr.right_arg() + 
        expr.left_arg() * (*this)( expr.right_arg() ); }

    template< typename U, typename V >
    auto operator()( Quotient< U, V > const& expr )
    { return ( expr.numerator_arg() * (*this)( expr.denominator_arg() ) - 
        (*this)( expr.numerator_arg() ) * expr.denominator_arg() ) /
        expr.denominator_arg() / expr.denominator_arg(); }

    template< typename U >
    auto operator()( SquareRoot< U > const& expr )
    { return Constant< decltype( result_t< U >{} / result_t< U >{} ), 
        static_cast< decltype( result_t< U >{} / result_t< U >{} ) >( 0.5 ) >{} / 
            expr * (*this)( expr.arg() ); }

    template< int Exp, typename U >
    auto operator()( Power< Exp, U > const& expr )
    { return Constant< decltype( result_t< U >{} / result_t< U >{} ), 
        static_cast< decltype( result_t< U >{} / result_t< U >{} ) >( Exp ) >{} * 
            pow< Exp-1 >( expr.arg() ) * (*this)( expr.arg() ); }

    template< typename U >
    auto operator()( Sine< U > const& expr )
    { return cos( expr.arg() ) * (*this)( expr.arg() ); }

    template< typename U >
    auto operator()( Cosine< U > const& expr )
    { return -sin( expr.arg() ) * (*this)( expr.arg() ); }

    template< typename U >
    auto operator()( Tangent< U > const& expr )
    { return (*this)( expr.arg() ) / pow< 2 >( cos( expr.arg() )); }

    template< typename U >
    auto operator()( Arcsine< U > const& expr )
    { return (*this)( expr.arg() ) / sqrt( constant_one - pow< 2 >( expr.arg() )); }

    template< typename U >
    auto operator()( Arccosine< U > const& expr )
    { return -(*this)( expr.arg() ) / sqrt( constant_one - pow< 2 >( expr.arg() )); }

    template< typename U >
    auto operator()( Arctangent< U > const& expr )
    { return (*this)( expr.arg() ) / ( constant_one - pow< 2 >( expr.arg() )); }

    // TODO: double check the math here
    // TODO: also could we have multiple options for these derivative expressions
    //       if one evaluates to infinity and we can tell at compile time?
    template< typename U, typename V >
    auto operator()( Arctangent2< U, V > const& expr )
    { return (*this)( expr.numerator_arg() / expr.denominator_arg() ) / 
        ( constant_one - pow< 2 >( expr.numerator_arg() / expr.denominator_arg() )); }
    
};

template< size_t I, typename T >
Differential< I, T > 
differential( Variable< I, T > const& )
{ return {}; }

template< typename... Diffs >
struct Gradient: 
    Tensor< Shape< sizeof...( Diffs )>, Diffs... >
{
    template< typename T >
    auto operator()( T const& expr )
    { return make_tensor< Shape< sizeof...( Diffs ) >>( Diffs{}( expr )... ); }
};

template< variable... Vars >
Gradient< Differential< Vars::index, typename Vars::value_type >... > 
gradient( Vars const&... )
{ return {}; }

template< typename... Diffs >
struct Jacobian
{
    template< size_t I >
    auto get_diff() { get< I >( diffs ); }

    template< typename T >
    auto operator ()( T const& expr )
    { 
    }

    tuple< Diffs... > diffs;
};


} // namespace expressions

namespace std {

template< expressions::expression ExprT >
constexpr auto sqrt( ExprT const& expr )
{ return expressions::sqrt( expr ); }

} // namespace std 

namespace expressions {

/**
 * Solvers
 */

struct MaximumIterations: optional< size_t >
{ 
    constexpr MaximumIterations( value_type const& value ): 
        optional< size_t >{ value } { }
    constexpr MaximumIterations() = default;
};

struct LearningRate: optional< long double >
{
    constexpr LearningRate( value_type const& value ): 
        optional< long double >{ value } { }
    constexpr LearningRate() = default;
};

struct MinimumError: optional< long double >
{
    constexpr MinimumError( value_type const& value ): 
        optional< long double >{ value } { }
    constexpr MinimumError() = default;
};

constexpr MaximumIterations maximum_iterations = { 0 };
constexpr LearningRate learning_rate = { 0.l };
constexpr MinimumError minimum_error = { 0.l };

template< typename... Parameters >
struct Solver: tuple< Parameters... >
{
    template< typename T >
    auto& operator []( T )
    { return get< std::remove_const_t< T >>( *this ); }

    template< typename T >
    auto param( T, T::value_type def )
    { 
        if( auto p = get< std::remove_const_t< T >>( *this ); p )
            return *p;
        return def;
    }
};

template< variable... Vars >
struct GradientDescent: 
    Solver< MaximumIterations, LearningRate, MinimumError >
{
    static constexpr size_t variables_size = sizeof...( Vars );
    static constexpr size_t default_max_iterations = 100;
    static constexpr long double default_learning_rate = 1e-3;
    static constexpr long double default_minimum_error = 0;

    template< size_t I >
    auto& get_variable()
    { return std::get< I >( _vars ); }

    template< size_t... Is >
    void initialize_variables( seq< Is... > ) { }

    template< typename ErrorT, typename GradT, size_t... Is >
    void descend( ErrorT err, GradT grad, seq< Is... > )
    {
        auto rate = param( learning_rate, default_learning_rate );

        (( get_variable< Is >() = eval( get_variable< Is >() ) - 
            rate * err * tensor_get< Is >( grad )), ... );
    }

    template< size_t... Is >
    auto get_gradient( seq< Is... > )
    { return gradient( get_variable< Is >()... ); }

    template< typename ExprT >
    void operator ()( ExprT const& expr )
    {
        using result_type = result_t< ExprT >;
        auto grad = get_gradient( make_seq< variables_size >{} );

        auto iterations = param( maximum_iterations, default_max_iterations );
        auto thresh = param( minimum_error, default_minimum_error );

        // initialize variables...
        initialize_variables( make_seq< variables_size >{} );

        // calculate the gradient
        auto g = grad( expr );

        for( size_t step = 0; step < iterations; ++step )
        {
            auto err_n = eval( expr );
            if( static_cast< long double >( err_n ) < thresh )
                break;

            auto g_n = eval( g );

            descend( err_n, g_n, make_seq< variables_size >{} );
        }
    };

    GradientDescent( Vars... vars ): _vars{ vars... } { }

    tuple< Vars... > _vars;
};

template< variable... Vars >
GradientDescent< Vars... > gradient_descent( Vars... vars )
{ return { vars... }; }

} // namespace expressions


#ifndef NO_EXPRESSION_PRINTING

namespace std {

// template< size_t I, typename... Ts >
// Ts...[I] get( expressions::Array< Ts... > const& arr )
// { return get< I >( arr.values() ); }


template< typename ExprT >
struct formatter< expressions::StaticValue< ExprT >, char >: 
    formatter< ExprT >
{
    template< typename FormatContext >
    FormatContext::iterator format( expressions::StaticValue< ExprT > expr, 
        FormatContext& ctx ) const
    { return formatter< ExprT >::format( (ExprT)expr, ctx ); }
};

template< typename T, T Value >
struct formatter< expressions::Constant< T, Value >, char >: 
    formatter< T >
{
    template< typename FormatContext >
    FormatContext::iterator format( expressions::Constant< T, Value > expr, 
        FormatContext& ctx ) const
    { return formatter< T >::format( Value, ctx ); }
};

template< size_t I, typename T >
struct formatter< expressions::Variable< I, T >, char >:
    formatter< std::string >
{
    string format_string;

    template< typename FormatContext >
    FormatContext::iterator format( expressions::Variable< I, T > expr, 
        FormatContext& ctx ) const
    { return formatter< std::string >::format( expr.name(), ctx ); }
};

template< typename LeftT, typename RightT >
struct formatter< expressions::Sum< LeftT, RightT >, char >:
    formatter< std::string >
{   
    template< typename FormatContext >
    FormatContext::iterator format( expressions::Sum< LeftT, RightT > expr, 
        FormatContext& ctx ) const
    { 
        auto str = std::format( "({}+{})", expr.left_arg(), expr.right_arg() );
        return formatter< std::string >::format( str, ctx );
    }
};

template< typename LeftT, typename RightT >
struct formatter< expressions::Product< LeftT, RightT >, char >:
    formatter< std::string >
{   
    template< typename FormatContext >
    FormatContext::iterator format( expressions::Product< LeftT, RightT > expr, 
        FormatContext& ctx ) const
    { 
        auto str = std::format( "({}*{})", expr.left_arg(), expr.right_arg() );
        return formatter< std::string >::format( str, ctx );
    }
};

template< typename LeftT, typename RightT >
struct formatter< expressions::Quotient< LeftT, RightT >, char >:
    formatter< std::string >
{   
    template< typename FormatContext >
    FormatContext::iterator format( expressions::Quotient< LeftT, RightT > expr, 
        FormatContext& ctx ) const
    { 
        auto str = std::format( "({}/{})", expr.numerator_arg(), expr.denominator_arg() );
        return formatter< std::string >::format( str, ctx );
    }
};

template< typename LeftT, typename RightT >
struct formatter< expressions::Difference< LeftT, RightT >, char >:
    formatter< std::string >
{   
    template< typename FormatContext >
    FormatContext::iterator format( expressions::Difference< LeftT, RightT > expr, 
        FormatContext& ctx ) const
    { 
        auto str = std::format( "({}-{})", expr.left_arg(), expr.right_arg() );
        return formatter< std::string >::format( str, ctx );
    }
};

template< typename ExprT >
struct formatter< expressions::Negation< ExprT >, char >:
    formatter< ExprT >
{
    template< typename FormatContext >
    FormatContext::iterator format( expressions::Negation< ExprT > expr, 
        FormatContext& ctx ) const
    { 
        auto i = ctx.out();
        *i++ = '-';
        ctx.advance_to(i);
        return formatter< ExprT >::format( expr.arg(), ctx );
    }
};

template< typename LeftT, typename RightT >
struct formatter< expressions::Equals< LeftT, RightT >, char >:
    formatter< std::string >
{   
    template< typename FormatContext >
    FormatContext::iterator format( expressions::Equals< LeftT, RightT > expr, 
        FormatContext& ctx ) const
    { 
        auto str = std::format( "({}=={})", expr.left_arg(), expr.right_arg() );
        return formatter< std::string >::format( str, ctx );
    }
};

template< typename LeftT, typename RightT >
struct formatter< expressions::Conjunction< LeftT, RightT >, char >:
    formatter< std::string >
{   
    template< typename FormatContext >
    FormatContext::iterator format( expressions::Conjunction< LeftT, RightT > expr, 
        FormatContext& ctx ) const
    { 
        auto str = std::format( "({}and{})", expr.left_arg(), expr.right_arg() );
        return formatter< std::string >::format( str, ctx );
    }
};


} // namespace std

#endif

#endif