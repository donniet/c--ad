#ifndef __EXPRESSION_BASE_HPP__
#define __EXPRESSION_BASE_HPP__

#include "utility.hpp"
#include "units.hpp"

#include <any>
#include <concepts>
#include <tuple>
#include <type_traits>
#include <vector>
#include <cmath>
#include <map>

namespace expressions {

using namespace units;

/**
 * Traits for the units
 */
using std::tuple, std::get, std::tuple_element_t, std::tuple_size_v, 
    std::make_tuple;
using std::is_base_of_v, std::is_same_v, std::conditional_t;
using std::isgreater, std::isless;
using std::index_sequence, std::make_index_sequence;
using std::any;
using std::vector;
using std::map;


template< typename T >
concept tuple_like = requires
{ tuple_size_v< T >; };

template< size_t I, tuple_like T >
struct has_element
{ static constexpr bool value = isless( I, tuple_size_v< T > ); };

template< size_t I, tuple_like T >
using has_element_v = has_element< I, T >::value;

template< typename T >
struct Expression;

template< typename T >
result_t< T > eval( T const& expr, variable_values const& )
{ return expr; }


/**
 * 
 */
template< typename T >
struct Expression
{ 
    using value_type = T;
    constexpr value_type const& get_value() const
    { return _value; }

    constexpr Expression( value_type const& expr ): _value{ expr } { }

    value_type _value;
};

template< typename T >
result_t< Expression< T >> eval( Expresison< T > const& expr, 
    variable_values const& vars )
{ return eval( expr.get_value(), vars ); }

template< typename T, typename U >
struct Sum
{
    
};

/**
 * trait to mark a type as an expression
 */
template< typename T >
struct is_expression
{ static constexpr bool value = is_base_of_v< Expression, T >; };

// a tuple of expressions is also an expression
template< typename... Ts >
struct is_expression< tuple< Ts... >>
{ static constexpr bool value = ( is_expression< Ts >::value and ... ); };

template< typename T >
constexpr bool is_expression_v = is_expression< T >::value;

template< typename T >
concept expression = is_expression_v< T >;

/**
 * traits of expressions
 */
template< typename T >
struct expression_traits;

/**
 * an expression generally inherits from Expression
 */
template< typename T >
requires ( is_base_of_v< Expression, T > )
struct expression_traits< T >
{ 
    using result_type = T::result_type;
    using dependent_types = T::dependent_types;
};

/**
 * a tuple of expressions is also an expression
 */
template< expression... Ts >
struct expression_traits< tuple< Ts... >>
{ 
    using result_type = tuple< 
        typename expression_traits< Ts >::result_type... >;
    using dependent_types = tuple< Ts... >;
};

/**
 * get dependent
 */
template< size_t I, typename ExprT >
struct dependent;

template< size_t I, typename... Ts >
struct dependent< I, tuple< Ts... >>
{
    using type = tuple_element_t< I, tuple< Ts... >>;

    static constexpr type value( tuple< Ts... > const& expr ) 
    { return get< I >( expr ); }
};

template< size_t I, typename ExprT >
requires is_base_of_v< Expression, ExprT >
struct dependent< I, ExprT >
{
    using dependent_types = expression_traits< ExprT >::dependent_types;
    using type = tuple_element_t< I, dependent_types >;

    static constexpr type value( ExprT const& expr ) 
    { return expr.template get_dependent< I >(); }
};

template< size_t I, typename ExprT >
using dependent_t = dependent< I, ExprT >::type;

template< size_t I, typename ExprT >
static constexpr dependent_t< I, ExprT > get_dependent( ExprT x )
{ return dependent< I, ExprT >::value( x ); }


/**
 * Trait to determine the result of an expression.
 */
template< typename T, typename... Ts >
struct Result
{ using type = T; };

template< typename T, typename... Ts >
requires( expression< T > )
struct Result< T, Ts... >
{ using type = expression_traits< T >::result_type; };

/**
 * the result_type of the eval of the first of Ts
 */
template< typename... Ts >
using result_t = Result< Ts... >::type;

/**
 * trait to determine if a given expression has dependents
 */
template< expression ExprT >
struct DependentsSize
{ static constexpr size_t value = 
    tuple_size_v< typename expression_traits< ExprT >::dependent_types >; };

template< expression ExprT >
using dependents_size_v = DependentsSize< ExprT >::value;

/**
 * Expressions whose value may change
 * 
 * @tparam U is the type of this variable
 * @tparam I is an index to uniquely identify this variable
 * 
 * TODO: implement variables with more than a single index (and maybe no indices?)
 */
template< typename U, size_t I >
struct Variable : Expression
{ 
    using result_type = U; 
    using dependent_types = tuple<>;
};

/**
 * structure to hold values of variables
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

/**
 * prototype for evaluating expressions
 */
template< typename ExprT >
result_t< ExprT > eval( ExprT const& expr, variable_values const& values );

template< typename TupleT, size_t... Is >
auto eval_tuple_helper( TupleT const& tup, variable_values const& values,
    seq< Is... > )
{ return make_tuple( eval( get< Is >( tup ), values )... ); }

template< typename... Ts >
tuple< result_t< Ts >... > eval( tuple< Ts... > const& expr, 
    variable_values const& values )
{ return eval_tuple_helper( expr, values, make_seq< sizeof...( Ts )>{} ); }

template< typename U, size_t I >
U eval( Variable< U, I > const& expr, variable_values const& values )
{ return any_cast< U >( values[I] ); }


/**
 * indexables implement the subscript operator
 */
template< typename ExprT >
concept indexable = requires( ExprT expr, size_t I )
{ expr[I]; };

/**
 * An expression with a unchanging value known at run time
 */
template< unit U >
struct StaticExpression : Expression
{  
    using result_type = U;
    using dependent_types = tuple<>;

    constexpr result_type get_value() const
    { return value; }

    constexpr operator result_type() const
    { return value; }

    constexpr result_type eval( variable_values const& ) const
    { return get_value(); }

    constexpr StaticExpression() = default;
    constexpr StaticExpression( result_type value ) : value{ value } { }

    result_type value;
};

template< unit U >
U eval( StaticExpression< U > const& expr, variable_values const& )
{ return expr.get_value(); }

template< unit U >
StaticExpression< U > static_expr( U value )
{ return { value }; }

StaticExpression< Scalar > static_expr( long double value )
{ return { scalar( value ) }; }

StaticExpression< Cardinal > static_expr( unsigned long long value )
{ return { cardinal( value ) }; }

/**
 * an expression with a constant value known at compile time
 */
template< unit U, U Value >
struct ConstantExpression : Expression
{ 
    using result_type = U;
    using dependent_types = tuple<>;

    static constexpr result_type get_value()
    { return value; }
    constexpr operator result_type() const
    { return value; }

    constexpr result_type eval( variable_values const& ) const
    { return get_value(); }

    static constexpr result_type value = Value;
};

template< unit U, U Value >
U eval( ConstantExpression< U, Value > const& expr, variable_values const& )
{ return Value; }

template< unit U >
using zero_expr = ConstantExpression< U, 0 >;

template< unit U >
using one_expr = ConstantExpression< U, 1 >;

template< unit U, U Value >
using constant_expr = ConstantExpression< U, Value >;



/**
 * unary expression
 */
template< typename UnaryT, typename T >
struct Unary : Expression
{
    using result_type = decltype( UnaryT{}( result_t< T >{} ));
    using dependent_types = tuple< T >;
    
    template< size_t I >
    requires( I == 0 )
    constexpr T get_dependent() { return first; }

    constexpr result_type eval( variable_values const& values ) const
    { return func( eval( first, values )); }

    constexpr Unary() = default;
    constexpr Unary( T const& t ) : first{ t } { }

    T first;
    static constexpr UnaryT func;
};

/**
 * Associative binary expression
 */
template< typename BinaryT, typename T, typename... Ts >
struct Associative : Associative< BinaryT, Ts... >
{
    using dependent_types = tuple< T, Ts... >;
    using first_type = T;
    using rest = Associative< BinaryT, Ts... >;
    
    template< size_t I >
    constexpr tuple_element_t< I, dependent_types > get_dependent()
    {
        if( I == 0 ) 
            return first;
        return rest::template get_dependent< I - 1 >();
    }

    using rest::func;

    using result_type = decltype( 
        func( expressions::eval( T{}, {} ), typename rest::result_type{} ));

    constexpr auto eval( variable_values const& values ) const
    { return func( eval( first, values ), rest::eval( values )); }

    constexpr Associative() = default;
    constexpr Associative( T const& t, Ts const&... ts ) : first{ t }, rest{ ts... } { }

    first_type first;
};

/**
 * tail of an associative binary expression
 */
template< typename BinaryT, typename T >
struct Associative< BinaryT, T > : Expression
{
    using result_type = result_t< T >;
    using dependent_types = tuple< T >;
    using first_type = T;

    template< size_t I >
    requires( I == 0 )
    constexpr first_type get_dependent() { return first; }

    constexpr result_type eval( variable_values const& values ) const
    { return eval( first, values ); }

    constexpr Associative() = default;
    constexpr Associative( T const& t ) : first{ t } { }

    first_type first;
    static constexpr BinaryT func;
};

template< size_t I, typename TupleT >
tuple_element_t< I, TupleT > get_element( TupleT const& tup )
{ return get< I >( tup ); }



template< typename T, size_t I >
struct expression_traits< Variable< T, I >>
{ 
    using result_type = T;
    using dependent_types = tuple<>;
};

/**
 * trait to determine if a given type is a variable
 */
template< expression V >
struct is_variable
{ static constexpr bool value = false; };

template< typename T, size_t I >
struct is_variable< Variable< T, I >>
{ static constexpr bool value = true; };

template< expression V >
constexpr bool is_variable_v = is_variable< V >::value;

template< typename V >
concept variable = is_variable_v< V >;

template< expression V >
constexpr bool is_compound_v = 
    isgreater( tuple_size_v< 
        typename expression_traits< V >::dependent_types >, 0 );

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

// template< variable V, typename T >
// struct depends_on< V, StaticExpression< T >>
// { static constexpr bool value = false; };

template< typename T, size_t I, typename U, size_t J >
struct depends_on< Variable< T, I >, Variable< U, J >>
{ static constexpr bool value = ( I == J ); };

template< variable V, typename TupleT >
struct depends_on_compound_helper;

template< variable V, expression... Ts >
struct depends_on_compound_helper< V, tuple< Ts... >>
{ static constexpr bool value = ( ... or depends_on< V, Ts >::value ); };

template< variable V, expression T >
requires ( compound< T > )
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

// template< typename T >
// struct max_variable_index< StaticExpression< T >>
// { static constexpr size_t value = 0; };

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


} // namespace expression

#endif 