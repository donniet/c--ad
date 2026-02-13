#ifndef __EXPRESSION_BASE_HPP__
#define __EXPRESSION_BASE_HPP__

#include "utility.hpp"

#include <any>
#include <concepts>
#include <tuple>
#include <type_traits>
#include <vector>
#include <cmath>

namespace expressions {

using std::tuple, std::get, std::tuple_element_t, std::tuple_size_v, 
    std::make_tuple;
using std::is_base_of_v, std::is_same_v, std::conditional_t;
using std::isgreater, std::isless;
using std::index_sequence, std::make_index_sequence;
using std::any;
using std::vector;

/**
 * All non-staic expressions inherit from this class
 */
struct Expression
{ };

template< typename T >
struct StaticExpression
{
    static constexpr bool is_static_expression = true;
    using result_type = T;

    operator result_type() const { return value; };
    constexpr StaticExpression() = default;
    constexpr StaticExpression( result_type value ) : value{ value } { }

    result_type value;
};

template< typename T >
StaticExpression< T > make_static( T value )
{ return { value }; }

/**
 * traits of expressions
 */
template< typename T >
struct expression_traits;

template< typename T >
concept expression = requires
{ typename expression_traits< T >; };

template< typename T >
struct expression_traits< StaticExpression< T >>
{
    using result_type = T;
    // by default the expression is a static one
    static constexpr bool is_static_expression = true;
    static constexpr bool is_compound_expression = false;
    static constexpr bool is_variable = false;
    using dependent_types = tuple<>;
};

template< typename... Ts >
struct DependsOn : Expression
{ 
    using dependent_types = 
        tuple_cat_t< typename expression_traits< Ts >::dependent_types... >;

    static constexpr bool is_static_expression = 
        ( ... and expression_traits< Ts >::is_static_expression );

    tuple< Ts... > exprs;

    DependsOn() = default;
    DependsOn( DependsOn const& ) = default;
    DependsOn( Ts... ts ) : exprs{ ts... } { }
};

template< typename DepTuple >
struct DependsFromTuple;

template< typename... Ts >
struct DependsFromTuple< tuple< Ts... >>
{ using type = DependsOn< Ts... >; };

template< typename T >
requires ( is_base_of_v< Expression, T > )
struct expression_traits< T >
{ 
    using result_type = T::result_type;
    static constexpr bool is_static_expression = false;
    static constexpr bool is_compound_expression = true;
    static constexpr bool is_variable = false;
    using dependent_types = T::dependent_types;
};

template< expression... Ts >
struct expression_traits< tuple< Ts... >>
{ 
    using result_type = tuple< 
        typename expression_traits< Ts >::result_type... >;
    static constexpr bool is_static_expression = 
        ( ... and expression_traits< Ts >::is_static_expression );
    static constexpr bool is_compound_expression = not is_static_expression;
    static constexpr bool is_variable = false;
    using dependent_types = tuple< Ts... >;
};

template< typename T >
using result_t = expression_traits< T >::result_type;

/**
 * get an element from a type that inherits from DependsOn<...>
 */
template< size_t I, typename Op >
struct dependent
{
    using dependent_types = expression_traits< Op >::dependent_types;
    using type = tuple_element_t< I, dependent_types >;
    static constexpr type getter( Op x )
    { return get< I >( static_cast< DependsFromTuple< 
        dependent_types >::type >( x ).exprs ); }
};

template< size_t I, typename Op >
using dependent_t = dependent< I, Op >::type;

template< size_t I, typename Op >
static constexpr dependent_t< I, Op > get_dependent( Op x )
{ return dependent< I, Op >::getter( x ); }

/**
 * structure to hold values of variables
 */
struct variable_values 
{
    any operator[]( size_t I ) const { return values[I]; }
    vector< any > values;
};

/**
 * invokes an expression and returns the result_type
 */
template< typename T >
struct Invoker;

// forward decl
template< typename T, size_t I >
struct Variable;

/**
 * accessor for the value of variable I with type T
 */
template< typename T, size_t I >
struct VariableHarness
{
    friend Invoker< Variable< T, I >>;

    operator T() const
    { return any_cast< T >(( *m_values )[I]); }

    VariableHarness& operator=( T const& other )
    { ( *m_values )[I] = other; return *this; }

private:
    variable_values const* m_values;
};

/**
 * Expressions whose value may change
 * 
 * @tparam T is the type of this variable
 * @tparam I is an index to uniquely identify this variable
 * 
 * TODO: implement variables with more than a single index (and maybe no indices?)
 */
template< typename T, size_t I >
struct Variable : Expression
{ using result_type = VariableHarness< T, I >; };

template< typename T, size_t I >
struct expression_traits< Variable< T, I >>
{ 
    using result_type = T;
    static constexpr bool is_static_expression = false;
    static constexpr bool is_compound_expression = false;
    static constexpr bool is_variable = true;
    using dependent_expressions = tuple<>;
};


/**
 * trait to determine if a given type is a variable
 */
template< expression V >
constexpr bool is_variable_v = expression_traits< V >::is_variable;

template< typename V >
concept variable = is_variable_v< V >;

// template< typename V >
// constexpr bool is_static_expr_v = expression_traits< V >::is_static_expression;

// template< typename V >
// concept static_expr = is_static_expr_v< V >;

template< expression V >
constexpr bool is_compound_v = expression_traits< V >::is_compound_expression;

template< typename V >
concept compound = is_compound_v< V >;

/**
 * determines if an expression depends on the given variable
 * 
 * @tparam E expression to be searched
 * @tparam V variable to be searched for
 * @returns true if expression E uses variable V as a subtype
 */
template< variable V, expression E >
struct depends_on;

template< variable V, typename T >
struct depends_on< V, StaticExpression< T >>
{ static constexpr bool value = false; };

template< typename T, size_t I, typename U, size_t J >
struct depends_on< Variable< T, I >, Variable< U, J >>
{ static constexpr bool value = ( I == J ); };

template< variable V, typename TupleT >
struct depends_on_compound_helper;

template< variable V, expression... Ts >
struct depends_on_compound_helper< V, tuple< Ts... >>
{ static constexpr bool value = ( ... or depends_on< V, Ts >::value ); };

template< variable V, expression T >
requires ( compound< T > and not expression_traits< T >::is_static_expression )
struct depends_on< V, T >
{ 
    using dependent_types = expression_traits< T >::dependent_types;
    static constexpr bool value = 
        depends_on_compound_helper< V, dependent_types >::value; 
};

template< variable V, typename E >
constexpr bool depends_on_v = depends_on< V, E >::value;

#ifndef NDEBUG
static_assert( depends_on_v< Variable< int, 0 >, Variable< int, 0 >> );
static_assert( not depends_on_v< Variable< int, 1 >, Variable< int, 0 >> );
static_assert( depends_on_v< Variable< int, 0 >, 
    tuple< Variable< int, 0>, Variable< int, 0 >>> );
static_assert( depends_on_v< Variable< int, 0 >, 
    tuple< Variable< int, 1>, Variable< int, 0 >>> );
static_assert( not depends_on_v< Variable< int, 0 >, 
    tuple< Variable< int, 1>, Variable< int, 1 >>> );
#endif

template< size_t I, typename E >
constexpr bool depends_on_index_v = depends_on_v< Variable< any, I >, E >;

#ifndef NDEBUG
static_assert( depends_on_index_v<0,  Variable< int, 0 >> );
static_assert( not depends_on_index_v< 0, Variable< int, 1 >> );
static_assert( depends_on_index_v< 0, tuple< Variable< int, 0 >, Variable< int, 0 >>> );
static_assert( depends_on_index_v< 0, tuple< Variable< int, 1 >, Variable< int, 0 >>> );
static_assert( not depends_on_index_v< 0, tuple< Variable< int, 1 >, 
    Variable< int, 1 >>> );
#endif

/**
 * finds the type of variable with index I
 * 
 * @tparam I the index to the variable
 * @tparam E the expression to be searched
 */
template< size_t I, typename E >
struct variable_type
{ using type = void; };

template< size_t I >
struct variable_type< I, tuple<>>
{ using type = void; };

template< size_t I, typename T, typename... Ts >
struct variable_type< I, tuple< T, Ts... >>
{ using type = conditional_t< depends_on_index_v< I, T >,
    typename variable_type< I, T >::type, 
    typename variable_type< I, tuple< Ts... >>::type >; };

template< size_t I, typename T >
struct variable_type< I, Variable< T, I >>
{ using type = T; };

template< size_t I, compound E >
struct variable_type< I, E >
{ using type = variable_type< I, 
    typename expression_traits< E >::dependent_types >::type; };

template< size_t I, typename E >
using variable_type_t = variable_type< I, E >::type;

#ifndef NDEBUG
static_assert( is_same_v< variable_type_t< 0, Variable< int, 0 >>, int > );
static_assert( is_same_v< variable_type_t< 0, tuple< Variable< int, 0 >, 
    Variable< bool, 1 >>>, int > );
static_assert( is_same_v< variable_type_t< 1, tuple< Variable< int, 0 >, 
    Variable< bool, 1 >>>, bool > );
static_assert( is_same_v< variable_type_t< 0, tuple< Variable< int, 1 >, 
    Variable< bool, 0 >>>, bool > );
#endif

template< typename E >
struct max_variable_index;

template< typename T >
struct max_variable_index< StaticExpression< T >>
{ static constexpr size_t value = 0; };

template< typename T, size_t I >
struct max_variable_index< Variable< T, I >>
{ static constexpr size_t value = I; };

template< compound E >
requires ( isgreater( tuple_size_v< typename expression_traits< E >::dependent_types >, 0 ))
struct max_variable_index< E >
{ static constexpr size_t value = max_variable_index< 
    typename expression_traits< E >::dependent_types >::value; };

template< compound E >
requires ( tuple_size_v< typename expression_traits< E >::dependent_types > == 0 )
struct max_variable_index< E >
{ static constexpr size_t value = 0; };

template< typename... Ts >
struct max_variable_index< tuple< Ts... >>
{ static constexpr size_t value = 
    max_v< max_variable_index< Ts >::value... >; };

template< typename E >
constexpr size_t max_variable_index_v = max_variable_index< E >::value;

#ifndef NDEBUG
static_assert( 1 == max_variable_index_v< Variable< int, 1 >> );
static_assert( 0 == max_variable_index_v< Variable< int, 0 >> );
static_assert( 2 == max_variable_index_v< tuple< Variable< int, 0 >, 
    Variable< int, 2 >>> );
#endif

template< typename Seq, typename E >
struct dependent_variables_helper;

template< size_t... Is, typename E >
struct dependent_variables_helper< seq< Is... >, E >
{ using type = tuple_cat_t< conditional_t< depends_on_index_v< Is, E >, 
    tuple< Variable< variable_type_t< Is, E >, Is >>,
    tuple<> >... >; };

// NOTE: max_seq< N > == seq< 0, 1, ... N-1 >
template< typename E >
struct dependent_variables
{ using type = dependent_variables_helper< 
    make_seq< 1 + max_variable_index_v< E >>, E >::type; };

template< typename E >
using dependent_variables_t = dependent_variables< E >::type;


// by default, call the operator() on the expr to invoke it
template< typename T >
struct Invoker;

template< template< typename... > class Compound, typename... Ts >
requires( expression_traits< Compound< Ts... >>::is_compound )
struct Invoker< Compound< Ts... >>
{
    using expression_type = Compound< Ts... >;
    using depends = expression_traits< expression_type >::dependent_types;
    static constexpr size_t depends_size = tuple_size_v< depends >;
    using result_type = result_t< expression_type >;

    result_type operator()( expression_type expr, variable_values const& vals ) 
    { return compound_invoker( expr, vals, make_seq< depends_size >{} ); }

private:
    template< size_t I >
    using dep_invoker = Invoker< tuple_element_t< I, depends >>;

    template< size_t... Is >
    result_type compound_invoker( expression_type expr,
        variable_values const& vals, seq< Is... > )
    { return expr( dep_invoker< Is >{}( expr.template at< Is >(), vals )... ); }
};

template< typename T >
requires expression_traits< T >::is_static_expression
struct Invoker< T >
{
    T operator()( T value, variable_values const& ) 
    { return value; }
};

template< typename T >
result_t< T > invoke( T expr, variable_values const& values )
{ return Invoker< T >{}( expr, values ); }

template< typename T >
requires( tuple_size_v< dependent_variables_t< T >> == 0 )
result_t< T > invoke( T expr )
{ return Invoker< T >{}( expr, variable_values{} ); }

template< typename T, size_t I >
struct Invoker< Variable< T, I >>
{ 
    VariableHarness< T, I > operator()( Variable< T, I >, 
        variable_values const& values )
    { return { &values }; }
};

// /**
//  * Constant expression will never change it's value
//  * 
//  * @tparam T is the unit of this contstant
//  * @tparam Value is the value of this constant
//  * @returns the constant Value
//  */
// template< typename T, T Value >
// struct Constant : public Expression
// {
//     using result_type = T;
//     static constexpr T value = Value;
// };

// #ifndef NDEBUG
// static_assert( is_same_v< tuple<>, dependent_variables_t< Constant< int, 0 >>> );
// static_assert( is_same_v< tuple< Variable< int, 0 >>, 
//     dependent_variables_t< Variable< int, 0 >>> );
// static_assert( is_same_v< tuple< Variable< int, 0 >>, 
//     dependent_variables_t< tuple< Constant< int, 0 >, Variable< int, 0 >>>> );
// #endif

// template< typename T, T Value >
// struct Invoker< Constant< T, Value >>
// { 
//     T operator()( variable_values const& )
//     { return Value; }
// };

// /**
//  * trait to check if an expression is constant
//  */
// template< typename T >
// struct is_constant_expression 
// { static constexpr bool value = false; };

// template< typename T, T Value >
// struct is_constant_expression< Constant< T, Value >>
// { static constexpr bool value = true; };

} // namespace expression

#endif 