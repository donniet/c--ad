#ifndef __EXPRESSIONS_HPP__
#define __EXPRESSIONS_HPP__

#include "tensors.hpp"
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

using namespace tensors;

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

    // NOTE: this is for advanced use.  It re-wires the underlying
    // pointer to a new value.  Since an expression may have many instances
    // of the same variable this could make expressions inconsistent.
    void realize( T& storage )
    { _ptr = &storage; }

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

    constexpr Variable(): 
        _ptr{ nullptr }, _name{} { }

    T* _ptr;
    string _name;
};

namespace details {

} // namespace details



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
array< string, sizeof...( Ts )> default_variable_names( string base = "var" )
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

namespace details {

template< typename... Exprs >
struct DependentVariableIDs;

template< >
struct DependentVariableIDs< >
{ using type = seq< >; };

template< size_t I, typename T >
struct DependentVariableIDs< Variable< I, T >>
{ using type = seq< I >; };

template< typename T >
struct DependentVariableIDs< StaticValue< T >>
{ using type = seq< >; };

template< typename T, T Value >
struct DependentVariableIDs< Constant< T, Value >>
{ using type = seq< >; };

template< typename ExprT >
struct DependentVariableIDs< ExprT >
{ 
    using argument_types = ExprT::argument_types;

    template< typename Seq >
    struct helper;

    template< size_t... Is >
    struct helper< seq< Is... >>
    { using type = merge_unique_sorted_seq< typename DependentVariableIDs< 
        tuple_element_t< Is, argument_types >>::type... >; };

    using type = helper< make_seq< tuple_size_v< argument_types >>>::type; 
};

template< typename... Exprs >
requires( isgreater( sizeof...( Exprs ), 1 ))
struct DependentVariableIDs< Exprs... >
{ using type = merge_unique_sorted_seq< 
    typename DependentVariableIDs< Exprs >::type... >; };

template< size_t I, typename ExprT >
struct VariableType
{ 
protected:
    template< typename Seq >
    struct helper;

    template< >
    struct helper< seq< >>
    { using type = void; };

    template< size_t J >
    struct helper< seq< J >>
    { using type = VariableType< I, 
        tuple_element_t< J, typename ExprT::argument_types >>::type; };

    template< size_t J, size_t... Js >
    requires( isgreater( sizeof...( Js ), 0 ))
    struct helper< seq< J, Js... >>
    { 
        using jth = VariableType< I, tuple_element_t< J, 
            typename ExprT::argument_types >>::type;

        using type = std::conditional_t< not is_same_v< void, jth >, jth,
            typename helper< seq< Js...>>::type >;
    };

public:
    using type = helper< make_seq< tuple_size_v< 
        typename ExprT::argument_types >>>::type;
};

template< size_t I, size_t J, typename T >
requires( I == J )
struct VariableType< I, Variable< J, T >>
{ using type = T; };

template< size_t I, size_t J, typename T >
requires( I != J )
struct VariableType< I, Variable< J, T >>
{ using type = void; };

template< size_t I, typename T >
struct VariableType< I, StaticValue< T >>
{ using type = void; };

template< size_t I, typename T, T Value >
struct VariableType< I, Constant< T, Value >>
{ using type = void; };

template< typename ExprT >
struct DependentVariables
{
protected:
    using variable_ids = DependentVariableIDs< ExprT >::type;

    template< typename Seq >
    struct helper;

    template< size_t... Is >
    struct helper< seq< Is... >>
    { using type = tuple< Variable< Is,
        typename VariableType< Is, ExprT >::type >... >; };

public:
    using type = helper< variable_ids >::type;
};

} // namespace details

template< typename ExprT >
using dependent_variables_t = details::DependentVariables< ExprT >::type;

/// @brief negation expression
/// @tparam T the negated type
///
template< typename T >
struct Negation: ExpressionTag
{ 
    using argument_types = tuple< T >;
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
    using argument_types = tuple< T, U >;
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
    using argument_types = tuple< T, U >;
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
    using argument_types = tuple< T, U >;
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
    using argument_types = tuple< T, U >;
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
    using argument_types = tuple< T >;
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
    using argument_types = tuple< T >;
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
    using argument_types = tuple< T >;
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
    using argument_types = tuple< T >;
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
    using argument_types = tuple< T >;
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
    using argument_types = tuple< T >;
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
    using argument_types = tuple< T >;
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
    using argument_types = tuple< T >;
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
    using argument_types = tuple< T >;
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
    using argument_types = tuple< T, U >;
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
/// @tparam Ts... 
template< typename... Ts >
struct Conjunction: ExpressionTag
{
    using argument_types = tuple< Ts... >;
    static constexpr size_t arguments_size() { return sizeof...( Ts ); }
    using result_type = bool;

    template< size_t I >
    constexpr Ts...[ I ] arg() const
    { return std::get< I >( _args ); }

    template< size_t... Is >
    constexpr result_type eval_helper( seq< Is... > ) const
    { return ( expressions::eval( arg< Is >()) and ... ); }

    constexpr result_type eval( ) const
    { return eval_helper( make_seq< arguments_size() >{} ); }

    constexpr Conjunction( Ts... ts ): _args{ ts... } { } 
    constexpr Conjunction() = default;
    
    argument_types _args;
};

/// @brief logical or expression
/// @tparam Ts...
template< typename... Ts >
struct Disjunction: ExpressionTag
{
    using argument_types = tuple< Ts... >;
    static constexpr size_t arguments_size() { return sizeof...( Ts ); }
    using result_type = bool;

    template< size_t I >
    constexpr Ts...[ I ] arg() const
    { return std::get< I >( _args ); }

    template< size_t... Is >
    constexpr result_type eval_helper( seq< Is... > ) const
    { return ( expressions::eval( arg< Is >()) or ... ); }

    constexpr result_type eval( ) const
    { return eval_helper( make_seq< arguments_size() >{} ); }

    constexpr Disjunction( Ts... ts ): _args{ ts... } { } 
    constexpr Disjunction() = default;
    
    argument_types _args;
};

/// @brief logical not expression
/// @tparam T 
/// @tparam U 
template< typename T >
struct Compliment: ExpressionTag
{
    using argument_types = tuple< T >;
    using result_type = bool;

    constexpr T arg() const { return _arg; }

    constexpr result_type eval( ) const
    { return not expressions::eval( arg() ); }

    constexpr Compliment( T arg ): _arg{ arg } { } 
    constexpr Compliment() = default;
    
    T _arg;
};

template< typename ExprT >
struct IsBooleanExpression: 
    integral_constant< bool, is_same_v< result_t< ExprT >, bool >> { };

template< typename ExprT >
struct IsConjunction: integral_constant< bool, false > { };

template< typename... Exprs >
struct IsConjunction< Conjunction< Exprs... >>: 
    integral_constant< bool, true > { };

template< typename ExprT >
struct IsDisjunction: integral_constant< bool, false > { };

template< typename... Exprs >
struct IsDisjunction< Disjunction< Exprs... >>: 
    integral_constant< bool, true > { };

template< typename ExprT >
struct IsCompliment: integral_constant< bool, false > { };

template< typename ExprT >
struct IsCompliment< Compliment< ExprT >>: 
    integral_constant< bool, true > { };

template< typename ExprT >
struct IsCanonicalTerminus: integral_constant< bool, 
    not IsConjunction< ExprT >::value and
    not IsDisjunction< ExprT >::value and
    not IsCompliment< ExprT >::value > { };

template< typename ExprT >
struct IsCanonicalComplimentedTerminus: integral_constant< bool, false > { };

template< typename ExprT >
struct IsCanonicalComplimentedTerminus< Compliment< ExprT >>: 
    integral_constant< bool, IsCanonicalTerminus< ExprT >::value > { };

template< typename ExprT >
struct IsCanonicalConjunctiveTerminus: 
    integral_constant< bool, false > { };

template< typename... Exprs >
struct IsCanonicalConjunctiveTerminus< Conjunction< Exprs... >>:
    integral_constant< bool, (
        ( IsCanonicalTerminus< Exprs >::value or 
          IsCanonicalComplimentedTerminus< Exprs >::value ) and ... )> { };

template< typename ExprT >
struct IsCanonicalDisjunctiveTerminus: 
    integral_constant< bool, false > { };

template< typename... Exprs >
struct IsCanonicalDisjunctiveTerminus< Disjunction< Exprs... >>:
    integral_constant< bool, (
        ( IsCanonicalTerminus< Exprs >::value or 
          IsCanonicalComplimentedTerminus< Exprs >::value or 
          IsCanonicalConjunctiveTerminus< Exprs >::value ) and ... )> { };

template< typename ExprT >
struct IsCanonical: integral_constant< bool, 
    IsCanonicalTerminus< ExprT >::value or
    IsCanonicalComplimentedTerminus< ExprT >::value or
    IsCanonicalConjunctiveTerminus< ExprT >::value or
    IsCanonicalDisjunctiveTerminus< ExprT >::value > { };


template< typename ExprT >
constexpr bool is_boolean_expression_v = IsBooleanExpression< ExprT >::value;

template< typename ExprT >
constexpr bool is_conjunction_v = IsBooleanExpression< ExprT >::value;

template< typename ExprT >
constexpr bool is_disjunction_v = IsBooleanExpression< ExprT >::value;

template< typename ExprT >
constexpr bool is_compliment_v = IsBooleanExpression< ExprT >::value;

template< typename ExprT >
constexpr bool is_canonical_v = IsCanonical< ExprT >::value;

/// @brief restructures a boolean expression into 
/// disjunction-of-conjunctions-of-negations format
/// @tparam  
template< typename ExprT >
struct CanonicalExpression;

template< typename ExprT >
requires( IsCanonical< ExprT >::value )
struct CanonicalExpression< ExprT >
{ using type = ExprT; };

/// first we push the compliments down through disjunctions and conjunctions
/// using demorgan's laws.  Then distribute conjunctions through disjunctions
/// 

template< typename ExprT >
struct PushCompliments
{
protected:
    template< typename U >
    struct pusher
    { using type = U; };

    template< bool Value >
    struct pusher< Compliment< Constant< bool, Value >>>
    { using type = Constant< bool, not Value >; };

    template< typename U >
    struct pusher< Compliment< Compliment< U >>>
    { using type = pusher< U >::type; };

    template< typename... Us >
    struct pusher< Compliment< Disjunction< Us... >>>
    { using type = Conjunction< typename pusher< Compliment< Us >>::type... >; };

    template< typename... Us >
    struct pusher< Compliment< Conjunction< Us... >>>
    { using type = Disjunction< typename pusher< Compliment< Us >>::type... >; };

public:
    using type = pusher< ExprT >::type;
};

namespace test {

using compf = constant< bool, false >;
using compt = constant< bool, true >;
using comp0 = Compliment< constant< bool, false >>;
using comp1 = Compliment< comp0 >;
using comp2 = Compliment< Disjunction< compf, compt >>;
using comp3 = Compliment< Conjunction< compf, compt >>;

static_assert( is_same_v< typename PushCompliments< comp0 >::type, compt > );
static_assert( is_same_v< typename PushCompliments< comp1 >::type, compf > );
static_assert( is_same_v< typename PushCompliments< comp2 >::type, 
    Conjunction< compt, compf >> );
static_assert( is_same_v< typename PushCompliments< comp3 >::type, 
    Disjunction< compt, compf >> );
} // namespace test

// template< typename ExprT >
// struct DistributeConjunctions {
// protected:
//     template< typename ConjT, typename Seq >
//     struct helper;

//     template< typename... Os, typename... As, size_t... Is >
//     struct helper< Conjunction< Disjunction< Os... >, As... >, seq< Is... >>
//     { using type = Disjunction< typename distributor< 
//         Conjunction< Os...[Is], As... >>::type... >; };


//     template< typename U >
//     struct commutator
//     { using type = U; };

//     template< typename... As, typename... Bs >
//     struct commutator< Conjunction< Conjunction< As... >, Bs... >>
//     { using type = commutator< Conjunction< As..., Bs... >>::type; };

//     template< typename A, typename... As >
//     struct commutator< Conjunction< A, As... >>
//     { using type = commutator< Conjunction< A, 
//         commutator< Conjunction<  As..., Bs... >>::type; };

//     template< typename... As, typename... Bs >
//     struct commutator< Disjunction< Disjunction< As... >, Bs... >>
//     { using type = commutator< Disjunction< As..., Bs... >>::type; };

//     template< typename U >
//     struct distributor
//     { using type = U; };


//     /// (( O or Os... ) and As... )
//     /// (( O and As... ) or ( Os...[0] and As... ) or ( Os...[1] and As... ) or ... )
//     ///
//     template< typename... Os, typename... As >
//     struct distributor< Conjunction< Disjunction< Os... >, As... >>
//     { using type = helper< Conjunction< Disjunction< Os... >, As... >, 
//         make_seq< sizeof...( Os ) >>::type; };

//     template< typename A, typename... As >
//     struct distributor< Conjunction< A, As... >>
//     { using type = commutator< Conjunction< A, 
//         typename distributor< Conjunction< As... >::type >::type; };

// public:
//     using type = distributor< ExprT >::type;
// };

// namespace test {

// using stat = StaticValue< bool >;

// using conj0 = Conjunction< stat, stat >;
// using conj1 = Conjunction< Disjunction< stat, compt >, compf >;
// using conj2 = Conjunction< compf, Disjunction< stat, compt >>;

// static_assert( is_same_v< typename DistributeConjunctions< conj0 >::type, conj0 > );
// static_assert( is_same_v< typename DistributeConjunctions< conj1 >::type, 
//     Disjunction< Conjunction< stat, compf >, Conjunction< compt, compf >>> );
// // static_assert( is_same_v< typename DistributeConjunctions< conj2 >::type, 
// //     Disjunction< Conjunction< compf, stat >, Conjunction< compf, compt >>> );

// } // namespace test

namespace test {
    using stat = StaticValue< bool >;
} // namespace test

/// @brief converts the boolean logic expression into disjunctive normal form
/// @tparam ExprT 
namespace canonical {
    // replace parameter packs with binary operations
    template< typename A >
    struct preprocess
    { using type = A; };

    template< typename A >
    struct preprocess< Negation< A >>
    { using type = Negation< typename preprocess< A >::type >; };

    template< typename A, typename B >
    struct preprocess< Conjunction< A, B >>
    { using type = Conjunction< typename preprocess< A >::type, 
        typename preprocess< B >::type >; };

    template< typename A, typename B >
    struct preprocess< Disjunction< A, B >>
    { using type = Disjunction< typename preprocess< A >::type, 
        typename preprocess< B >::type >; };

    template< typename First, typename... Rest >
    requires( isgreater( sizeof...( Rest ), 1 ))
    struct preprocess< Conjunction< First, Rest... >>
    { using type = Conjunction< typename preprocess< First >::type, 
        typename preprocess< Conjunction< Rest... >>::type >; };

    template< typename First, typename... Rest >
    requires( isgreater( sizeof...( Rest ), 1 ))
    struct preprocess< Disjunction< First, Rest... >>
    { using type = Disjunction< typename preprocess< First >::type, 
        typename preprocess< Disjunction< Rest... >>::type >; };

    // tests to make sure preprocessor is working right
    static_assert( is_same_v< Conjunction< test::stat, Conjunction< test::stat, test::stat >>,
        typename preprocess< Conjunction< test::stat, test::stat, test::stat >>::type > );

    // using preprocessed_type = preprocess< ExprT >::type;

    // demorgans laws and double negatives
    template< typename A >
    struct dem
    { using type = A; };

    template< typename A >
    struct dem< Compliment< Compliment< A >>>
    { using type = dem< A >::type; };

    template< typename A, typename B >
    struct dem< Compliment< Disjunction< A, B >>>
    { using type = Conjunction< typename dem< Compliment< A >>::type, 
        typename dem< Compliment< B >>::type >; };

    template< typename A, typename B >
    struct dem< Compliment< Conjunction< A, B >>>
    { using type = Disjunction< typename dem< Compliment< A >>::type, 
        typename dem< Compliment< B >>::type >; };

    // using demorgans_type = dem< preprocessed_type >::type;

    // testing demorgans laws
    // ~(A & B) == ~A | ~B
    static_assert( is_same_v< Disjunction< Compliment< test::stat >, Compliment< test::stat >>,
        typename dem< Compliment< Conjunction< test::stat, test::stat >>>::type > );

    // ~(A | B) == ~A & ~B
    static_assert( is_same_v< Conjunction< Compliment< test::stat >, Compliment< test::stat >>,
        typename dem< Compliment< Disjunction< test::stat, test::stat >>>::type > );

    // ~(A & ~B) == ~A | B
    static_assert( is_same_v< Disjunction< Compliment< test::stat >, test::stat >,
        typename dem< Compliment< Conjunction< test::stat, Compliment< test::stat >>>>::type > );


    // distribution
    template< typename A >
    struct dist
    { using type = A; };

    template< typename A, typename B, typename C >
    struct dist< Conjunction< Disjunction< A, B >, C >>
    { using type = Disjunction< typename dist< Conjunction< A, C >>::type, 
        typename dist< Conjunction< B, C >>::type >; };

    template< typename A, typename B, typename C >
    struct dist< Conjunction< A, Disjunction< B, C >>>
    { using type = Disjunction< typename dist< Conjunction< A, B >>::type, 
        typename dist< Conjunction< A, C >>::type >; };

    template< typename A, typename B, typename C, typename D >
    struct dist< Conjunction< Disjunction< A, B >, Disjunction< C, D >>>
    { using type = Disjunction< 
        Disjunction< 
            typename dist< Conjunction< A, C >>::type, 
            typename dist< Conjunction< A, D >>::type>,
        Disjunction<
            typename dist< Conjunction< B, C >>::type, 
            typename dist< Conjunction< B, D >>::type>>; };

    // using distributed_type = dist< demorgans_type >::type;

    // postprocess by bringing all conjunctions and disjunctions back to 
    // parameter packs
    
    // our final structure will be 
    // Disjunction< Conjunction< Negation<A>... >... >
    template< typename T >
    struct rotate_right
    { using type = T; };
    
    template< typename A, typename B, typename C >
    struct rotate_right< Disjunction< Disjunction< A, B >, C >>
    { using type = rotate_right< Disjunction< 
        typename rotate_right< A >::type, 
        typename rotate_right< Disjunction< B, C >>::type >::type; };


} // namespace canonical

/// @brief compliment of a compliment is the original expression
/// @tparam ExprT 
template< typename ExprT >
requires( IsCanonicalTerminus< ExprT >::value )
struct CanonicalExpression< Compliment< Compliment< ExprT >>>
{ using type = CanonicalExpression< ExprT >::type; };


/////////////
/// Operators
///

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

template< expression T, expression U >
constexpr auto operator or( T const& left, U const& right )
{ return Disjunction< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator or( T const& left, U const& right )
{ return Disjunction< T, StaticValue< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator or( T const& left, U const& right )
{ return Disjunction< StaticValue< T >, U >{ static_expr( left ), right }; }

template< expression T >
constexpr auto operator not( T const& arg )
{ return Compliment< T >{ arg }; }

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

///////////
/// Solvers
///

/// @brief trait to list the combination of 
/// @tparam ExprT 
template< typename ExprT >
struct BooleanSatisfaction;

struct MaximumIterations: optional< size_t >
{ 
    constexpr MaximumIterations( value_type const& value ): 
        optional< size_t >{ value } { }
    constexpr MaximumIterations() = default;
};

// TODO: this requires a unit!
struct LearningRate: optional< long double >
{
    constexpr LearningRate( value_type const& value ): 
        optional< long double >{ value } { }
    constexpr LearningRate() = default;
};

// TODO: this requires a unit!
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
    void initialize_variables( seq< Is... > ) 
    { /* TODO: not sure what to do here yet */ }

    template< size_t I, typename ErrorT, typename GradElementT >
    auto descend_element( ErrorT err, GradElementT grad_n )
    {
        auto x = get_variable< I >();
        auto r = param( learning_rate, default_learning_rate );
        // TODO: check this math
        // gradient unit will be err unit / var unit
        // err * grad ~= err * err / var unit
        // rate unit must therefore be var unit * var unit / err unit / err unit
        // the learning rate must be in the same units

        using rate_type = result_t< decltype( x * x / err / err ) >;
        auto rate = static_cast< rate_type >( r );

        return eval( x ) - rate * err * grad_n;
    }

    template< typename ErrorT, typename GradT, size_t... Is >
    void descend( ErrorT err, GradT grad, seq< Is... > )
    { (( get_variable< Is >() = descend_element< Is >( err, tensor_get< Is >( grad ))), ... ); }

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
        auto str = std::format( "({}and{})", expr.template arg<0>(), expr.template arg<1>() );
        return formatter< std::string >::format( str, ctx );
    }
};


} // namespace std

#endif

#endif