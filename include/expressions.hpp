/**
 * Expressions are laizily evaluated mathematical functions.  The type system
 * is used to allow autodifferentiation of the expressions which in turn can be
 * leveraged for auto-solvers.  These are designed to be used in constraints on
 * the dimensions of our c++ad objects
 * 
 * EXAMPLE:
 * 
 * auto model = Universe{ "my_model" };
 * auto block = new ( model ) Box{ "maple" };
 * // constraints passed to add_constraint are Conjunction< Equal< ... >, ... >
 * model.add_constraint( block.width == 2 * block.height and 
 *                       block.height == 3 * block.depth and
 *                       block.depth == 4_in );
 * cout << STL{ model };
 * 
 */

#ifndef __EXPRESSIONS_HPP__
#define __EXPRESSIONS_HPP__

#include "utility.hpp"

#include <any>
#include <vector>
#include <unordered_map>
#include <tuple>
#include <utility>
#include <functional>
#include <cmath>
#include <type_traits>
#include <concepts>
// TODO: handle floating point environments
// #include <cfenv>

namespace expressions {

using std::tuple, std::get, std::tuple_element_t, std::tuple_size_v;
using std::index_sequence, std::make_index_sequence;

/**
 * Concepts
 */
// template< typename Expr, typename ResultType >
// concept result_is = requires( Expr expr, ResultType result )
// { result = expr(); };


// is the unit continuous or discrete?
template< typename Unit >
concept continuous = std::floating_point< Unit > or 
    std::floating_point< typename Unit::value_type >;

template< typename Unit >
concept discrete = 
    ( std::integral< Unit > and not std::is_same_v< Unit, bool > ) or
    ( std::integral< typename Unit::value_type > and not 
        std::is_same_v< typename Unit::value_type, bool > );

template< typename Unit >
concept numeric = continuous< Unit > or discrete< Unit >;

template< typename Expr, typename Unit >
concept expression_of = Expr::is_expression and 
    std::is_same_v< typename Expr::result_type, Unit >;

template< typename Expr, typename... Exprs >
struct SameExpressionResultTypes
{ static constexpr bool value = ( std::is_same_v< typename Expr::result_type, 
        typename Exprs::result_type > and ... ); };

template< typename Expr >
struct SameExpressionResultTypes< Expr >
{ static constexpr bool value = true; };

template< typename... Exprs >
static constexpr bool same_expression_result_types = 
    SameExpressionResultTypes< Exprs... >::value;

template< typename Expr, typename... Exprs >
requires same_expression_result_types< Expr, Exprs... >
struct ResultOf
{ using type = typename Expr::result_type; };

template< typename... Exprs >
using result_of = typename ResultOf< Exprs... >::type;

template< typename Expr >
concept continuous_expression = Expr::is_expression and 
    continuous< typename Expr::result_type >;

template< typename Expr >
concept discrete_expression = Expr::is_expression and 
    discrete< typename Expr::result_type >;

template< typename Expr >
concept boolean_expression = Expr::is_expression and 
    std::is_same_v< bool, typename Expr::result_type >;

template< typename Unit >
concept boolean_or_numeric = std::is_same_v< bool, Unit > or numeric< Unit >;

/**
 * identities
 * 
 * TODO: where should these go? here, units.hpp, someplace else?
 */
template< typename Unit >
struct AdditiveIdentity
{ static constexpr Unit value = 0; };

template< typename Unit >
struct MultiplicativeIdentity
{ static constexpr Unit value = 1; };

template< typename Unit >
static constexpr auto additive_identity = AdditiveIdentity< Unit >::value;

template< typename Unit >
static constexpr auto multiplicative_identity = 
    MultiplicativeIdentity< Unit >::value;

/**
 * Base class for all expressions
 */
template< typename ResultType, typename... Exprs >
struct Expression
{
    static constexpr bool is_expression = true;
    static constexpr size_t size = sizeof...( Exprs );
    using result_type = ResultType;
    using element_seq = make_index_sequence< size >;
    using reverse_element_seq = reverse_integer_sequence_t< element_seq >;

    Expression& operator=( Expression const& ) = default;

    constexpr Expression( Exprs... exprs ) : exprs{ exprs... } { }
    Expression( Expression const& ) = default;
    
protected:
    // similar to ( ... op exprs ) except op is a binary function object 
    template< typename Op >
    ResultType fold_left( Op op, ResultType initial_value = ResultType{} ) const
    { return fold_left_helper( op, initial_value, element_seq{} ); }

    // similar to ( exprs op ... )
    template< typename Op >
    ResultType fold_right( Op op, ResultType initial_value = ResultType{} ) const
    { return fold_left_helper( op, initial_value, reverse_element_seq{} ); }

    // evaluates the Ith sub-expression and returns the result
    template< size_t I >
    ResultType eval() const
    { return get< I >( exprs )(); }

private:
    // executes the fold operation using an index_sequence 
    template< typename Op, size_t... Is >
    ResultType fold_left_helper( Op op, ResultType result, 
        index_sequence< Is... > ) const
    { return (( result = op( result, eval< Is >() )), ... ); }

private:
    tuple< Exprs... > exprs;
};

/**
 * Constant expression will never change it's value
 * 
 * @tparam T is the unit of this contstant
 * @tparam Value is the value of this constant
 */
template< boolean_or_numeric T, T Value >
struct Constant : public Expression< T >
{
    static constexpr bool is_constant = true;
    using expression_type = Expression< T >;
    using value_type = T;
    static constexpr T value = Value;

    value_type operator()() { return value; }

    Constant() = default;
};

// constant aliases
template< numeric T >
using constant_zero_expr = Constant< T, T{ 0 } >;

template< numeric T >
static constexpr auto constant_zero = constant_zero_expr< T >{};

template< numeric T >
using constant_one_expr = Constant< T, T{ 1 } >;

template< numeric T >
static constexpr auto constant_one = constant_one_expr< T >{};

using constant_true_expr = Constant< bool, true >;
static constexpr auto constant_true = constant_true_expr{};

using constant_false_expr = Constant< bool, false >;
static constexpr auto constant_false = constant_false_expr{};

namespace detail {

template< typename Expr >
struct IsConstant
{ static constexpr bool value = false; };

template< boolean_or_numeric T, T Value >
struct IsConstant< Constant< T, Value >>
{ static constexpr bool value = true; };

} // namespace detail

template< typename Expr >
static constexpr bool is_constant = detail::IsConstant< Expr >::value;

namespace detail {


/**
 * stores and retrieves a variable value.  
 * 
 * @tparam T is the type of the variable
 * @tparam Is are a set of identifiers used to specify which variable we are
 * getting or setting
 * @param value_ptr is a pointer to the new value of this variable or null if
 * this is not a setter call.
 * @returns the static value nested in the function
 * 
 * HACK: we will use static variables to store the value of a given variable 
 * right in it's getter/setter function. This works because any difference in 
 * the template arguments will instantiate a new variable_value<...> method and
 * thus a new static value (I think...)
 * 
 * DT: don't judge me. I've been known to use a goto here and there as well
 * 
 * TODO: add a scope based on the Universe, otherwise variables will conflict
 * across universes.  This could be as simple as adding the universe's ID
 * as one of the template parameters since we should enforce Universe's as not
 * dynamically allocated anyway
 */
template< typename T, size_t... Is >
static constexpr T variable_value( T* value_ptr )
{  
    // this value will be unique per the template parameters to this function
    static volatile T value = T{};

    // setter case
    if( value_ptr != nullptr )
        value = *value_ptr;

    // getter case
    return value;
}

} // namespace detail

/**
 * retrieves the variable of type T and address Is...
 * 
 * @tparam T is the type of the variable
 * @tparam Is... is the address of the variable
 * @returns the current value of the variable: either T{} or the last value set
 * through set_variable_value< Is... >( T value )
 */
template< typename T, size_t... Is >
constexpr T get_variable_value()
{ return detail::variable_value< T, Is... >( static_cast< T* >( nullptr )); }

/**
 * sets the variable with address Is... and type T
 * 
 * @tparam T is the type of the variable
 * @tparam Is is the address of the variable
 */
template< typename T, size_t... Is >
constexpr void set_variable_value( T value )
{ detail::variable_value< T, Is... >( &value ); }

/**
 * Variable in an expression
 * 
 * @tparam T the type of value this variable may take
 * @tparam I the necessary part of the address to this variable
 * @tparam Is zero or more numbers that when concatenated with I form the 
 * unique address of this variable
 */
template< boolean_or_numeric T, size_t I, size_t... Is >
struct Variable : public Expression< T >
{ 
    static constexpr bool is_variable = true;
    using unique_identifier = seq< I, Is... >;
    using expression_type = Expression< T >;
    using value_type = T;

    // DT: weird having a const setter, isn't it? see set_variable_value...
    constexpr void set( T value ) const
    { set_variable_value< T, I, Is... >( value ); }

    constexpr T get() const
    { get_variable_value< T, I, Is... >(); }

    // TODO: getters and setters that can be connected to dimensions
    constexpr expression_type::result_type operator()() const
    { return get(); }
};

namespace detail {

template< typename Expr, typename Var >
struct DependsOn;

template< boolean_or_numeric T, T Value, typename Var >
struct DependsOn< Constant< T, Value >, Var >
{ static constexpr bool value = false; };

template< boolean_or_numeric T, size_t... Is >
struct DependsOn< Variable< T, Is... >, Variable< T, Is... >>
{ static constexpr bool value = true; };

template< typename ResultType, typename... Exprs, 
    boolean_or_numeric T, size_t... Is >
struct DependsOn< Expression< ResultType, Exprs... >, Variable< T, Is... >>
{ static constexpr bool value = 
    ( ... or DependsOn< Exprs, Variable< T, Is... > >::value ); };

template< typename Expr >
struct MaximumVariableIndexSizeHelper;

template< typename Expr, size_t maximum_index_size >
struct MaximumVariableIndicesSeqHelper;

template< typename Expr, typename MaximumIndicesSeq >
struct DependentVariablesTupleTypeHelper;

} // namespace detail

/**
 * Predicate to determine if an Expression<...> type is dependent on a 
 * Variable<...> type
 * 
 * @tparam Expr is the Expression<...> type to be searched
 * @tparam Var is the Variable<...> type 
 * @returns true if Expr depends on Var
 */
template< typename Expr, typename Var >
static constexpr bool depends_on = detail::DependsOn< Expr, Var >::value;

template< typename Expr >
struct ExpressionTraits
{
    static constexpr size_t maximum_variable_index_size = 
        detail::MaximumVariableIndexSizeHelper< Expr >::value;
    
    using maximum_variable_indices_seq = 
        detail::MaximumVariableIndicesSeqHelper< Expr, 
            maximum_variable_index_size >::type;
  
    // tuple type referencing all types and addresses of variables found in Expr
    using dependent_variables_tuple_type = 
        detail::DependentVariablesTupleTypeHelper< Expr, 
            maximum_variable_indices_seq >;
};

/**
 * Represents a lazily evaluated conjunction (logical AND)
 */
template< boolean_expression... Exprs >
struct Conjunction : public Expression< bool, Exprs... >
{
    using expression_type = Expression< bool, Exprs... >;

    // similar to ( ... and exprs() )
    expression_type::result_type operator()() const
    { expression_type::fold_left( std::logical_and<>{}, true ); }

    Conjunction() = default;
    Conjunction( Exprs... exprs ) : expression_type{ exprs... } { }
};

/**
 * Represents a lazily evaluated disjunction (logical OR)
 */
template< boolean_expression... Exprs >
struct Disjunction : Expression< bool, Exprs... >
{
    using expression_type = Expression< bool, Exprs... >;

    // similar to ( ... and exprs() )
    expression_type::result_type operator()() const
    { expression_type::fold_left( std::logical_or<>{}, true ); }
    
    Disjunction() = default;
    Disjunction( Exprs... exprs ) : expression_type{ exprs... } { }
};

/**
 * Represents a lazily evaluate compliment (logical NOT)
 */
template< boolean_expression Expr >
struct Compliment : Expression< bool, Expr >
{
    using expression_type = Expression< bool, Expr >;

    // not expr()
    expression_type::result_type operator()() const
    { return expression_type::template eval<0>(); }
    
    Compliment() = default;
    Compliment( Expr expr ) : expression_type{ expr } { }
};

/**
 * Represents a lazily evaluated operator==
 */
template< typename LeftExpr, typename RightExpr >
requires same_expression_result_types< LeftExpr, RightExpr >
struct Equal : Expression< bool, LeftExpr, RightExpr >
{
    using expression_type = Expression< bool, LeftExpr, RightExpr >;

    expression_type::result_type operator()() const
    { return expression_type::template eval<0>() == 
        expression_type::template eval<1>(); }

    Equal() = default;
    Equal( LeftExpr left_expr, RightExpr right_expr ) : 
        expression_type{ left_expr, right_expr }
    { }
};

/**
 * Represents a lazily evaluated operator!=
 */
template< typename LeftExpr, typename RightExpr >
requires same_expression_result_types< LeftExpr, RightExpr >
struct NotEqual : Expression< bool, LeftExpr, RightExpr >
{
    using expression_type = Expression< bool, LeftExpr, RightExpr >;

    expression_type::result_type operator()() const
    { return expression_type::template eval<0>() != 
        expression_type::template eval<1>(); }

    NotEqual() = default;
    NotEqual( LeftExpr left_expr, RightExpr right_expr ) : 
        expression_type{ left_expr, right_expr }
    { }
};

/**
 * Represents a lazily evaluated operator< (std::less)
 */
template< typename LeftExpr, typename RightExpr >
requires same_expression_result_types< LeftExpr, RightExpr >
struct Less : Expression< bool, LeftExpr, RightExpr >
{
    using expression_type = Expression< bool, LeftExpr, RightExpr >;

    expression_type::result_type operator()() const
    { return expression_type::template eval<0>() < 
        expression_type::template eval<1>(); }

    Less() = default;
    Less( LeftExpr left_expr, RightExpr right_expr ) : 
        expression_type{ left_expr, right_expr }
    { }
};

/**
 * Represents a lazily evaluated operator<=
 */
template< typename LeftExpr, typename RightExpr >
requires same_expression_result_types< LeftExpr, RightExpr >
struct LessOrEqual : Expression< bool, LeftExpr, RightExpr >
{
    using expression_type = Expression< bool, LeftExpr, RightExpr >;

    expression_type::result_type operator()() const
    { return expression_type::template eval<0>() <=
        expression_type::template eval<1>(); }

    LessOrEqual() = default;
    LessOrEqual( LeftExpr left_expr, RightExpr right_expr ) : 
        expression_type{ left_expr, right_expr }
    { }
};

/**
 * Represents a lazily evaluated operator>
 */
template< typename LeftExpr, typename RightExpr >
requires same_expression_result_types< LeftExpr, RightExpr >
struct Greater : Expression< bool, LeftExpr, RightExpr >
{
    using expression_type = Expression< bool, LeftExpr, RightExpr >;

    expression_type::result_type operator()() const
    { return expression_type::template eval<0>() >
        expression_type::template eval<1>(); }

    Greater() = default;
    Greater( LeftExpr left_expr, RightExpr right_expr ) : 
        expression_type{ left_expr, right_expr }
    { }
};

/**
 * Represents a lazily evaluated operator>=
 */
template< typename LeftExpr, typename RightExpr >
requires same_expression_result_types< LeftExpr, RightExpr >
struct GreaterOrEqual : Expression< bool, LeftExpr, RightExpr >
{
    using expression_type = Expression< bool, LeftExpr, RightExpr >;

    expression_type::result_type operator()() const
    { return expression_type::template eval<0>() >=
        expression_type::template eval<1>(); }

    GreaterOrEqual() = default;
    GreaterOrEqual( LeftExpr left_expr, RightExpr right_expr ) : 
        expression_type{ left_expr, right_expr }
    { }
};

/**
 * Represents a lazy sum
 */
template< typename Expr1, typename Expr2, typename... Exprs >
requires same_expression_result_types< Expr1, Expr2, Exprs... >
struct Sum : Expression< result_of< Expr1 >, Expr1, Expr2, Exprs... >
{
    using expression_type = Expression< result_of< Expr1 >, 
        Expr1, Expr2, Exprs... >;

    expression_type::result_type operator()() const
    { return expression_type::fold_left( std::plus<>{}, 
        additive_identity< typename expression_type::result_type > ); }

    template< size_t I >
    auto term() { return get< I >( expression_type::exprs ); }

    constexpr Sum() = default;
    constexpr Sum( tuple< Expr1, Expr2, Exprs... > expr_tuple ) :
        expression_type{ expr_tuple } 
    { }
    constexpr Sum( Expr1 expr1, Expr2 expr2, Exprs... exprs ) : 
        expression_type{ expr1, expr2, exprs... } 
    { }
};

/**
 * Represents the lazy binary operator-(a,b)
 * 
 * TODO: should we have a Difference expression which fold_right's an 
 * alternating negation? for exmaple:
 * 
 * difference( 5, 6, 7, 8, 9 )() == 5 - (6 - (7 - (8 - 9)))
 *                               == 5 - (6 - (7 - 8 + 9))
 *                               == 5 - (6 - 7 + 8 - 9)
 *                               == 5 - 6 + 7 - 8 + 9
 */
template< typename MinuendExpr, typename SubtractendExpr >
requires same_expression_result_types< MinuendExpr, SubtractendExpr >
struct Difference : Expression< result_of< MinuendExpr >, 
    MinuendExpr, SubtractendExpr >
{
    using expression_type = Expression< result_of< MinuendExpr >, 
        MinuendExpr, SubtractendExpr >;

    expression_type::result_type operator()() const
    { return expression_type::template eval<0>() - 
        expression_type::template eval<1>(); }

    MinuendExpr minuend() { return get< 0 >( expression_type::exprs ); }
    SubtractendExpr subtractend() { return get< 1 >( expression_type::exprs ); }

    Difference() = default;
    Difference( MinuendExpr minuend_expr, SubtractendExpr subtractend_expr ) :
        expression_type{ minuend_expr, subtractend_expr }
    { }
};

/**
 * Represents a lazy nullary operator-()
 */
template< typename Expr >
struct Negation : Expression< result_of< Expr >, Expr >
{
    using expression_type = Expression< result_of< Expr >, Expr >;

    expression_type::result_type operator()() const
    { return -expression_type::template eval<0>(); }

    Negation() = default;
    Negation( Expr expr ) : expression_type{ expr } { }
};

/**
 * Represents a lazy product
 */
template< typename Expr1, typename Expr2, typename... Exprs >
requires same_expression_result_types< result_of< Expr1 >, Expr1, Expr2, 
    Exprs... >
struct Product : Expression< result_of< Expr1 >, Expr1, Expr2, Exprs... >
{
    using expression_type = Expression< result_of< Expr1 >, Expr1, Expr2, 
        Exprs... >;

    expression_type::result_type operator()() const
    { return expression_type::fold_left( std::multiplies<>{}, 
        multiplicative_identity< typename expression_type::result_type > ); }

    Product() = default;
    constexpr Product( tuple< Expr1, Expr2, Exprs... > expr_tuple ) :
        expression_type{ expr_tuple } 
    { }
    constexpr Product( Expr1 expr1, Expr2 expr2, Exprs... exprs ) : 
        expression_type{ expr1, expr2, exprs... } 
    { }
};

/**
 * Represents a lazy quotient
 */
template< typename NumeratorExpr, typename DenominatorExpr >
requires same_expression_result_types< NumeratorExpr, DenominatorExpr >
struct Quotient : Expression< result_of< NumeratorExpr >, 
    NumeratorExpr, DenominatorExpr >
{
    using expression_type = Expression< result_of< NumeratorExpr >, 
        NumeratorExpr, DenominatorExpr >;

    expression_type::result_type operator()() const
    { return expression_type::template eval<0>() / 
        expression_type::template eval<1>(); }

    Quotient() = default;
    Quotient( NumeratorExpr numerator_expr, DenominatorExpr denominator_expr ) :
        expression_type{ numerator_expr, denominator_expr }
    { }
};

/**
 * Represents lazy exponentiation (std::exp)
 */
template< typename ExponentExpr >
struct Exp : Expression< result_of< ExponentExpr >, ExponentExpr >
{
    using expression_type = Expression< result_of< ExponentExpr >, 
        ExponentExpr >;

    expression_type::result_type operator()() const
    { return std::exp( expression_type::template eval<0>() ); }

    Exp() = default;
    Exp( ExponentExpr exponent_expr ) : expression_type{ exponent_expr } { }
};

/**
 * Represents lazy exponentiation (std::log)
 */
template< typename ArgumentExpr >
struct Log : Expression< result_of< ArgumentExpr >, ArgumentExpr >
{
    using expression_type = Expression< result_of< ArgumentExpr >, 
        ArgumentExpr >;

    expression_type::result_type operator()() const
    { return std::log( expression_type::template eval<0>() ); }

    Log() = default;
    Log( ArgumentExpr argument_expr ) : expression_type{ argument_expr } { }
};

/**
 * Represents lazy power function (std::pow)
 */
template< typename BaseExpr, typename ExponentExpr >
requires same_expression_result_types< BaseExpr, ExponentExpr >
struct Power : Expression< result_of< BaseExpr >, BaseExpr, ExponentExpr >
{
    using expression_type = Expression< result_of< BaseExpr >, 
        BaseExpr, ExponentExpr >;

    expression_type::result_type operator()() const
    { return std::pow( expression_type::template eval<0>(),
        expression_type::template eval<1>() ); }

    Power() = default;
    Power( BaseExpr base_expr, ExponentExpr exponent_expr ) :
        expression_type{ base_expr, exponent_expr }
    { }
};

/**
 * Represents lazy sine (std::sin)
 */
template< typename ArgumentExpr >
struct Sine : Expression< result_of< ArgumentExpr >, ArgumentExpr >
{
    using expression_type = Expression< result_of< ArgumentExpr >, 
        ArgumentExpr >;

    expression_type::result_type operator()() const
    { return std::sin( expression_type::template eval<0>() ); }

    Sine() = default;
    Sine( ArgumentExpr argument_expr ) : expression_type{ argument_expr } { }
};

/**
 * Represents lazy cosine (std::cos)
 */
template< typename ArgumentExpr >
struct Cosine : Expression< result_of< ArgumentExpr >, ArgumentExpr >
{
    using expression_type = Expression< result_of< ArgumentExpr >, 
        ArgumentExpr >;

    expression_type::result_type operator()() const
    { return std::cos( expression_type::template eval<0>() ); }

    Cosine() = default;
    Cosine( ArgumentExpr argument_expr ) : expression_type{ argument_expr } { }
};

/**
 * Represents lazy tangent (std::tan)
 */
template< typename ArgumentExpr >
struct Tangent : Expression< result_of< ArgumentExpr >, ArgumentExpr >
{
    using expression_type = Expression< result_of< ArgumentExpr >, 
        ArgumentExpr >;

    expression_type::result_type operator()() const
    { return std::tan( expression_type::template eval<0>() ); }

    Tangent() = default;
    Tangent( ArgumentExpr argument_expr ) : expression_type{ argument_expr } { }
};

/**
 * Represents lazy arcsine (std::asin)
 */
template< typename ArgumentExpr >
struct Arcsine : Expression< result_of< ArgumentExpr >, ArgumentExpr >
{
    using expression_type = Expression< result_of< ArgumentExpr >, 
        ArgumentExpr >;

    expression_type::result_type operator()() const
    { return std::asin( expression_type::template eval<0>() ); }

    Arcsine() = default;
    Arcsine( ArgumentExpr argument_expr ) : expression_type{ argument_expr } { }
};

/**
 * Represents lazy arccosine (std::acos)
 */
template< typename ArgumentExpr >
struct Arccosine : Expression< result_of< ArgumentExpr >, ArgumentExpr >
{
    using expression_type = Expression< result_of< ArgumentExpr >, 
        ArgumentExpr >;

    expression_type::result_type operator()() const
    { return std::acos( expression_type::template eval<0>() ); }

    Arccosine() = default;
    Arccosine( ArgumentExpr argument_expr ) : expression_type{ argument_expr } { }
};

/**
 * Represents lazy arctangent (std::atan)
 */
template< typename ArgumentExpr >
struct Arctangent : Expression< result_of< ArgumentExpr >, ArgumentExpr >
{
    using expression_type = Expression< result_of< ArgumentExpr >, 
        ArgumentExpr >;

    expression_type::result_type operator()() const
    { return std::atan( expression_type::template eval<0>() ); }

    Arctangent() = default;
    Arctangent( ArgumentExpr argument_expr ) : expression_type{ argument_expr } { }
};

/**
 * Represents lazy arctangent with two arguments (std::atan2)
 */
template< typename NumeratorExpr, typename DenominatorExpr >
requires same_expression_result_types< NumeratorExpr, DenominatorExpr >
struct Arctangent2 : Expression< result_of< NumeratorExpr >, 
    NumeratorExpr, DenominatorExpr >
{
    using expression_type = Expression< result_of< NumeratorExpr >, 
        NumeratorExpr, DenominatorExpr >;

    expression_type::result_type operator()() const
    { return std::atan2( expression_type::template eval<0>(), expression_type::template eval<1>() ); }

    Arctangent2() = default;
    Arctangent2( NumeratorExpr numerator_expr, DenominatorExpr denominator_expr ) :
        expression_type{ numerator_expr, denominator_expr }
    { }
};

/**
 * TODO: hyperbolics, error functions, gamma functions
 */

/**
 * TODO: integer functions
 * NOTE: integer functions would require implementing the simplex method in C++ meta-programming... ugh...
 */

/**
 * TODO: implement isinf, isnan, etc
 */

/**
 * Derivations
 */
template< typename >
struct Derivation;

template< continuous_expression Expr >
using derive = Derivation< Expr >::type;

template< typename DerivedExpr, typename Var >
struct PartialDerivation;

template< typename DerivedExpr, typename Var >
requires ( not depends_on< DerivedExpr, Var > )
struct PartialDerivation< DerivedExpr, Var >
{ using type = constant_zero_expr< result_of< DerivedExpr >>; };

template< typename Var >
struct PartialDerivation< Var, Var >
{ using type = constant_one_expr< result_of< Var >>; };

template< continuous ResultType, typename... Exprs, typename Var >
struct PartialDerivation< Expression< ResultType, Exprs... >, Var >
{ using type = Expression< ResultType, PartialDerivation< Exprs, Var >... >; };

namespace detail {

template< typename Expr >
struct DerivationOfHelper
{  
    using type = derive< Expr >;
    static constexpr type value( Expr expr )
    { return { expr }; }
};

template< typename TupleType, typename Seq >
struct DerivationOfTuple;

template< typename TupleType, size_t... Is >
struct DerivationOfTuple< TupleType, seq< Is... >>
{
    using type = tuple< derive< tuple_element_t< Is, TupleType >>... >;
    static constexpr type value( TupleType tup )
    { return { get< Is >( tup )... }; }
};

// specialization for tuples
template< typename... Exprs >
struct DerivationOfHelper< tuple< Exprs... >> : 
    public DerivationOfTuple< tuple< Exprs... >, make_seq< sizeof...( Exprs )>>
{ };

} // namespace detail

template< typename ExprOrTuple >
constexpr auto derivation_of( ExprOrTuple expr_or_tuple )
{ return detail::DerivationOfHelper< ExprOrTuple >::value( expr_or_tuple ); }

template< typename Unit, Unit Value >
struct Derivation< Constant< Unit, Value >> 
{ using type = constant_zero_expr< Unit >; };

template< typename... Exprs >
struct Derivation< Sum< Exprs... >> : 
    Expression< result_of< Sum< Exprs... >>, Sum< derive< Exprs >... >>
{
    using function_type = Sum< Exprs... >;
    using expression_type = 
        Expression< Sum< derive< Exprs >... >>;

    expression_type::result_type operator()() const
    { return expression_type::template eval<0>(); }

    Derivation() = default;
    // what is a clean way of doing the tuple stuff below?
    // I think this will happen a lot
    // f.exprs is a tuple and we want sum_of< derivation_of< Exprs >... >
    Derivation( function_type const& f ) : 
        expression_type{ sum_of( derivation_of( f.exprs )) }
    { }
};

template< typename MinuendType, typename SubtractendType >
struct Derivation< Difference< MinuendType, SubtractendType >> :
    Expression< Difference< derive< MinuendType >, 
        derive< SubtractendType >>>
{
    using function_type = Difference< MinuendType, SubtractendType >;
    using expression_type = Expression< Difference< 
        derive< MinuendType >, derive< SubtractendType >>>;

    expression_type::result_type operator()() const 
    { return expression_type::template eval<0>(); }

    Derivation() = default;
    Derivation( function_type const& f ) :
        expression_type{ difference_of( derivation_of( 
            f.minuend(), f.subtractend() )) }
    { }
};

template< typename Expr >
struct Derivation< Negation< Expr >> : 
    Expression< Negation< derive< Expr >>>
{
    using function_type = Negation< typename Derivation< Expr >::type >;

};

template< typename Expr, typename... Exprs >
struct Derivation< Product< Expr, Exprs... >>
{
    // product rule: d(f*g) = df*g + f*dg
    using type = Sum< 
        Product< typename Derivation< Expr >::type, Product< Exprs... >>,
        Product< Expr, typename Derivation< Product< Exprs... >>::type >>;
};

template< typename NumeratorExpr, typename DenominatorExpr >
struct Derivation< Quotient< NumeratorExpr, DenominatorExpr >>
{
    // quotient rule: d(f/g) = ( g*df - f*dg ) / ( g * g );
    using type = Quotient<
        Difference<
            Product< typename Derivation< NumeratorExpr >::type, DenominatorExpr >,
            Product< NumeratorExpr, typename Derivation< DenominatorExpr >::type >>,
        Product< DenominatorExpr, DenominatorExpr >>;
};

template< typename Expr >
struct Derivation< Exp< Expr >>
{
    using type = Product< Exp< Expr >, typename Derivation< Expr >::type >;
};

template< typename Expr >
struct Derivation< Log< Expr >>
{
    using type = Quotient< typename Derivation< Expr >::type, Expr >;
};

// NOTE: Derivations only defined for constant powers
// TODO: define derivations for non-constant power expressions
template< typename BaseExpr, numeric T, T Value >
struct Derivation< Power< BaseExpr, Constant< T, Value >>>
{
    // power rule: d(f^n) == nf^{n-1}df
    using type = Product< 
        Constant< T, Value >, 
        Power< 
            BaseExpr, 
            Constant< T, Value - 1 >>,
        typename Derivation< BaseExpr >::type >;
    // TODO: ensure the power rule is 
};

template< typename Expr >
struct Derivation< Sine< Expr >>
{
    using type = Product<
        Cosine< Expr >,
        typename Derivation< Expr >::type >;
};

template< typename Expr >
struct Derivation< Cosine< Expr >>
{
    using type = Negation<
        Product<
            Sine< Expr >,
            typename Derivation< Expr >::type >>;
};

template< typename Expr >
struct Derivation< Tangent< Expr >>
{
    using type = Quotient<
        typename Derivation< Expr >::type,
        Product< Cosine< Expr >, Cosine< Expr >>>;
};

// TODO: arc-trig functions

// helper functions

/**
 * Logical Operations
 */
template< boolean_expression... Exprs >
Conjunction< Exprs... > conjunction_of( Exprs... exprs )
{ return { exprs... }; }

template< boolean_expression... Exprs >
Disjunction< Exprs... > disjunction_of( Exprs... exprs )
{ return { exprs... }; }

template< boolean_expression Expr >
Compliment< Expr > compliment_of( Expr expr )
{ return { expr }; }

/**
 * Numeric comparisons
 */
template<typename LeftExpr, typename RightExpr >
Equal< LeftExpr, RightExpr > equal( 
    LeftExpr left_expr, RightExpr right_expr )
{ return { left_expr, right_expr }; }

template<typename LeftExpr, typename RightExpr >
NotEqual< LeftExpr, RightExpr > not_equal( 
    LeftExpr left_expr, RightExpr right_expr )
{ return { left_expr, right_expr }; }

template<typename LeftExpr, typename RightExpr >
Less< LeftExpr, RightExpr > less_than( 
    LeftExpr left_expr, RightExpr right_expr )
{ return { left_expr, right_expr }; }

template<typename LeftExpr, typename RightExpr >
LessOrEqual< LeftExpr, RightExpr > less_than_or_equal( 
    LeftExpr left_expr, RightExpr right_expr )
{ return { left_expr, right_expr }; }

template<typename LeftExpr, typename RightExpr >
Greater< LeftExpr, RightExpr > greater_than( 
    LeftExpr left_expr, RightExpr right_expr )
{ return { left_expr, right_expr }; }

template<typename LeftExpr, typename RightExpr >
GreaterOrEqual< LeftExpr, RightExpr > greater_than_or_equal( 
    LeftExpr left_expr, RightExpr right_expr )
{ return { left_expr, right_expr }; }

/**
 * Mathematical operations
 */
template< typename... Exprs >
Sum< Exprs... > sum_of( Exprs... exprs )
{ return { exprs... }; }

template< typename MinuendExpr, typename SubtractendExpr >
Difference< MinuendExpr, SubtractendExpr > difference_of( MinuendExpr minuend_expr, SubtractendExpr subtractend_expr )
{ return { minuend_expr, subtractend_expr }; }

template< typename Expr >
Negation< Expr > negation_of( Expr expr )
{ return { expr }; }

template< typename... Exprs >
Product< Exprs... > product_of( Exprs... exprs )
{ return { exprs... }; }

template< typename NumeratorExpr,
    typename DenominatorExpr >
Quotient< NumeratorExpr, DenominatorExpr > quotient_of( 
    NumeratorExpr numerator_expr, DenominatorExpr denominator_expr )
{ return { numerator_expr, denominator_expr }; }

template< typename ExponentExpr >
Exp< ExponentExpr > exp_of( ExponentExpr exponent_expr )
{ return { exponent_expr }; }

template< typename ArgumentExpr >
Log< ArgumentExpr > log_of( ArgumentExpr argument_expr )
{ return { argument_expr }; }

template< typename BaseExpr,
    typename ExponentExpr >
Power< BaseExpr, ExponentExpr > power_of( BaseExpr base_expr, 
    ExponentExpr exponent_expr )
{ return { base_expr, exponent_expr }; }

template< typename ArgumentExpr >
Sine< ArgumentExpr > sine_of( ArgumentExpr argument_expr )
{ return { argument_expr }; }

template< typename ArgumentExpr >
Cosine< ArgumentExpr > cosine_of( ArgumentExpr argument_expr )
{ return { argument_expr }; }

template< typename ArgumentExpr >
Tangent< ArgumentExpr > tangent_of( ArgumentExpr argument_expr )
{ return { argument_expr }; }

template< typename ArgumentExpr >
Arcsine< ArgumentExpr > arcsine_of( ArgumentExpr argument_expr )
{ return { argument_expr }; }

template< typename ArgumentExpr >
Arccosine< ArgumentExpr > arccosine_of( ArgumentExpr argument_expr )
{ return { argument_expr }; }

template< typename ArgumentExpr >
Arctangent< ArgumentExpr > arctangent_of( ArgumentExpr argument_expr )
{ return { argument_expr }; }

template< typename NumeratorExpr,
    typename DenominatorExpr >
Arctangent2< NumeratorExpr, DenominatorExpr > arctangent2_of( 
    NumeratorExpr numerator_expr, DenominatorExpr denominator_expr )
{ return { numerator_expr, denominator_expr }; }


namespace operators {

/**
 * Logical 
 */
template< typename LeftExpr, typename RightExpr >
Conjunction< LeftExpr, RightExpr > operator and( 
    LeftExpr left_expr, RightExpr right_expr )
{ return { left_expr, right_expr }; }

template< typename LeftExpr, typename RightExpr >
Disjunction< LeftExpr, RightExpr > operator or(
    LeftExpr left_expr, RightExpr right_expr )
{ return { left_expr, right_expr }; }

template< typename Expr >
Compliment< Expr > operator!( Expr expr )
{ return { expr }; }

/**
 * Numeric comparisons
 */
template<typename LeftExpr, typename RightExpr >
Equal< LeftExpr, RightExpr > operator ==( 
    LeftExpr left_expr, RightExpr right_expr )
{ return { left_expr, right_expr }; }

template<typename LeftExpr, typename RightExpr >
NotEqual< LeftExpr, RightExpr > operator !=( 
    LeftExpr left_expr, RightExpr right_expr )
{ return { left_expr, right_expr }; }

template<typename LeftExpr, typename RightExpr >
Less< LeftExpr, RightExpr > operator <( 
    LeftExpr left_expr, RightExpr right_expr )
{ return { left_expr, right_expr }; }

template<typename LeftExpr, typename RightExpr >
LessOrEqual< LeftExpr, RightExpr > operator <=( 
    LeftExpr left_expr, RightExpr right_expr )
{ return { left_expr, right_expr }; }

template<typename LeftExpr, typename RightExpr >
Greater< LeftExpr, RightExpr > operator >( 
    LeftExpr left_expr, RightExpr right_expr )
{ return { left_expr, right_expr }; }

template<typename LeftExpr, typename RightExpr >
GreaterOrEqual< LeftExpr, RightExpr > operator <=( 
    LeftExpr left_expr, RightExpr right_expr )
{ return { left_expr, right_expr }; }

/**
 * Mathematical operations
 */
template< typename LeftExpr, typename RightExpr >
Sum< LeftExpr, RightExpr > operator + ( 
    LeftExpr left_expr, RightExpr right_expr )
{ return { left_expr, right_expr }; }

template< typename MinuendExpr, typename SubtractendExpr >
Difference< MinuendExpr, SubtractendExpr > operator - ( MinuendExpr minuend_expr, SubtractendExpr subtractend_expr )
{ return { minuend_expr, subtractend_expr }; }

template< typename Expr >
Negation< Expr > operator - ( Expr expr )
{ return { expr }; }

template< typename LeftExpr, typename RightExpr >
Product< LeftExpr, RightExpr > operator * ( 
    LeftExpr left_expr, RightExpr right_expr )
{ return { left_expr, right_expr }; }

template< typename NumeratorExpr,
    typename DenominatorExpr >
Quotient< NumeratorExpr, DenominatorExpr > operator / ( 
    NumeratorExpr numerator_expr, DenominatorExpr denominator_expr )
{ return { numerator_expr, denominator_expr }; }

template< typename ExponentExpr >
Exp< ExponentExpr > exp( ExponentExpr exponent_expr )
{ return { exponent_expr }; }

template< typename ArgumentExpr >
Log< ArgumentExpr > log( ArgumentExpr argument_expr )
{ return { argument_expr }; }

template< typename BaseExpr,
    typename ExponentExpr >
Power< BaseExpr, ExponentExpr > pow( BaseExpr base_expr, 
    ExponentExpr exponent_expr )
{ return { base_expr, exponent_expr }; }

template< typename ArgumentExpr >
Sine< ArgumentExpr > sin( ArgumentExpr argument_expr )
{ return { argument_expr }; }

template< typename ArgumentExpr >
Cosine< ArgumentExpr > cos( ArgumentExpr argument_expr )
{ return { argument_expr }; }

template< typename ArgumentExpr >
Tangent< ArgumentExpr > tan( ArgumentExpr argument_expr )
{ return { argument_expr }; }

template< typename ArgumentExpr >
Arcsine< ArgumentExpr > asin( ArgumentExpr argument_expr )
{ return { argument_expr }; }

template< typename ArgumentExpr >
Arccosine< ArgumentExpr > acos( ArgumentExpr argument_expr )
{ return { argument_expr }; }

template< typename ArgumentExpr >
Arctangent< ArgumentExpr > atan( ArgumentExpr argument_expr )
{ return { argument_expr }; }

template< typename NumeratorExpr,
    typename DenominatorExpr >
Arctangent2< NumeratorExpr, DenominatorExpr > atan2( 
    NumeratorExpr numerator_expr, DenominatorExpr denominator_expr )
{ return { numerator_expr, denominator_expr }; }

} // namespace operators

} // namespace expressions

#endif