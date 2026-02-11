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

#include "expression_base.hpp"

#include <any>
#include <vector>
#include <unordered_map>
#include <tuple>
#include <utility>
#include <functional>
#include <cmath>
// TODO: handle floating point environments
// #include <cfenv>


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
 * Base class for all expressions
 * 
 * @tparam ResultType is the return type of this expression
 * @tparam Exprs are the types of the sub-expressions that make up this one
 */
template< typename ResultType, typename... Exprs >
struct Expression : ExpressionBase
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

// private:
    // executes the fold operation using an index_sequence 
    template< typename Op, size_t... Is >
    ResultType fold_left_helper( Op op, ResultType result, 
        index_sequence< Is... > ) const
    { return (( result = op( result, eval< Is >() )), ... ); }

// private:
    tuple< Exprs... > exprs;
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
    using result_type = T;

    // DT: weird having a const setter, isn't it? see set_variable_value...
    constexpr void set( T value ) const
    { set_variable_value< T, I, Is... >( value ); }

    constexpr T get() const
    { return get_variable_value< T, I, Is... >(); }

    // TODO: getters and setters that can be connected to dimensions
    constexpr expression_type::result_type operator()() const
    { return get(); }

    Variable() = default;
    Variable( T value ) { set(value); }
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

template< boolean_or_numeric T, size_t... Is, size_t... Js >
struct DependsOn< Variable< T, Is... >, Variable< T, Js... >>
{ static constexpr bool value = false; };

template< template< typename... > class Op, typename... Exprs, 
    boolean_or_numeric T, size_t... Is >
struct DependsOn< Op< Exprs... >, Variable< T, Is... >>
{ static constexpr bool value = 
    ( ... or DependsOn< Exprs, Variable< T, Is... > >::value ); };


/**
 * Visits the Expr at compile time and returns the maximum address size
 */
template< typename Expr >
struct MaximumVariableAddressSizeHelper
{ static constexpr size_t value = 0; /* default to no variables found */ };

template< typename T, size_t... Is >
struct MaximumVariableAddressSizeHelper< Variable< T, Is... >>
{ static constexpr size_t value = sizeof...( Is ); /* default to no variables found */ };

template< typename ResultType, typename... Exprs >
struct MaximumVariableAddressSizeHelper< Expression< ResultType, Exprs... >>
{ static constexpr size_t value = 
    max_v< MaximumVariableAddressSizeHelper< Exprs >::value... >; };

#ifndef NDEBUG
namespace test {
static_assert( MaximumVariableAddressSizeHelper< 
    constant_zero_expr< double >>::value == 0 );
static_assert( MaximumVariableAddressSizeHelper< 
    Variable< double, 0 >>::value == 1 );
static_assert( MaximumVariableAddressSizeHelper< 
    Variable< double, 0, 0 >>::value == 2 );
static_assert( MaximumVariableAddressSizeHelper< Expression< double, 
    Variable< double, 0 >, Variable< double, 0, 0 >, 
    Variable< double, 0, 0, 0 >>>::value == 3 );
} // namespace test
#endif

template< typename Expr, size_t address_component >
struct MaximumVariableAddressAtComponent
{ static constexpr size_t value = 0; };

template< typename T, size_t... Is, size_t address_component >
requires ( address_component < sizeof...( Is ))
struct MaximumVariableAddressAtComponent< Variable< T, Is... >, 
    address_component >
{ static constexpr size_t value = 
    sequence_at< address_component, seq< Is... >>; };

template< typename T, size_t... Is, size_t address_component >
requires ( address_component >= sizeof...( Is ))
struct MaximumVariableAddressAtComponent< Variable< T, Is... >, 
    address_component >
{ static constexpr size_t value = 0; };

template< typename ReturnType, typename... Exprs, size_t address_component >
struct MaximumVariableAddressAtComponent< Expression< ReturnType, Exprs... >, 
    address_component >
{ static constexpr size_t value = 
    max_v< MaximumVariableAddressAtComponent< Exprs, address_component >::value... >; };

#ifndef NDEBUG
namespace test {
static_assert( MaximumVariableAddressAtComponent< 
    constant_zero_expr< double >, 0 >::value == 0 );
static_assert( MaximumVariableAddressAtComponent< 
    Variable< double, 0 >, 0 >::value == 0 );
static_assert( MaximumVariableAddressAtComponent< 
    Variable< double, 1 >, 0 >::value == 1 );
static_assert( MaximumVariableAddressAtComponent< Expression< double, 
    Variable< double, 0 >, Variable< double, 1, 0 >, 
    Variable< double, 2, 3, 0 >>, 0 >::value == 2 );
static_assert( MaximumVariableAddressAtComponent< Expression< double, 
    Variable< double, 0 >, Variable< double, 1, 0 >, 
    Variable< double, 2, 3, 0 >>, 1 >::value == 3 );
} // namespace test
#endif

template< typename Expr, typename IndexSeq >
struct MaximumVariableIndicesSeqHelper;

template< typename Expr, size_t... Is >
struct MaximumVariableIndicesSeqHelper< Expr, seq< Is... >>
{ using type = seq< MaximumVariableAddressAtComponent< Expr, Is >::value... >; };

template< typename T, size_t... Js, size_t... Is >
struct MaximumVariableIndicesSeqHelper< Variable< T, Js... >, seq< Is... >>
{ using type = seq< sequence_at< Is, seq< Js...>>... >; };

#ifndef NDEBUG
namespace test {
static_assert( std::is_same_v< MaximumVariableIndicesSeqHelper< 
    Variable< double, 6, 1, 2 >, seq< 0, 1, 2 >>::type, seq< 6, 1, 2 >> );
} // namespace test
#endif

template< typename Expr >
struct DependentVariablesTupleTypeHelper
{ using type = tuple< >; };

template< template< typename... > class Op, typename... Exprs >
// requires( is_expression< Op< Exprs... >> )
struct DependentVariablesTupleTypeHelper< Op< Exprs... >>
{ using type = TupleUnique< tuple_cat_t<
    typename DependentVariablesTupleTypeHelper< Exprs >::type... >>::type; };

// template< typename ResultType, typename... Exprs >
// struct DependentVariablesTupleTypeHelper< Expression< ResultType, Exprs... >>
// { using type = TupleUnique< tuple_cat_t< 
//     typename DependentVariablesTupleTypeHelper< Exprs >::type... >>::type; };

template< typename T, size_t... Is >
struct DependentVariablesTupleTypeHelper< Variable< T, Is... >>
{ using type = tuple< Variable< T, Is... >>; };

} // namespace detail

/**
 * Returns a tuple of variable types the expression depends on
 * 
 * @tparam Expr is the expression type
 * @returns a tuple of unique variable types
 */
template< typename Expr >
using dependent_variable_tuple = 
    detail::DependentVariablesTupleTypeHelper< Expr >::type;

#ifndef NDEBUG
namespace test {
static_assert( is_same_v< tuple<>, dependent_variable_tuple<
    Expression< int, constant_one_expr< int >>>> );
static_assert( is_same_v< tuple< Variable< int, 1 >>, dependent_variable_tuple<
    Expression< int, Variable< int, 1 >>>> );
static_assert( is_same_v< tuple< Variable< int, 1 >>, dependent_variable_tuple<
    Expression< int, Variable< int, 1 >, Variable< int, 1 >>>> );
static_assert( is_same_v< tuple< Variable< int, 1 >>, dependent_variable_tuple<
    Expression< int, Variable< int, 1 >, 
        Expression< int, Variable< int, 1 >>>>> );
static_assert( is_same_v< tuple< Variable< int, 1 >, Variable< int, 2 >>, 
    dependent_variable_tuple<
        Expression< int, Variable< int, 1 >, 
            Expression< int, Variable< int, 2 >>>>> );
} // namespace test
#endif

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
    static constexpr size_t maximum_variable_address_size = 
        detail::MaximumVariableAddressSizeHelper< Expr >::value;
    
    using maximum_variable_indices_seq = 
        detail::MaximumVariableIndicesSeqHelper< Expr, 
            make_seq< maximum_variable_address_size >>::type;
  
    // tuple type referencing all types and addresses of variables found in Expr
    using dependent_variables_tuple_type = 
        detail::DependentVariablesTupleTypeHelper< Expr >::type;
};

/**
 * Represents a lazily evaluated conjunction (logical AND)
 */
template< boolean_expression... Exprs >
struct Conjunction : public Expression< bool, Exprs... >
{
    using expression_type = Expression< bool, Exprs... >;
    static constexpr size_t size = sizeof...( Exprs );

    // similar to ( ... and exprs() )
    expression_type::result_type operator()() const
    { expression_type::fold_left( std::logical_and<>{}, true ); }

    template< size_t I >
    auto conjunct() { return get< I >( expression_type::exprs ); }

    Conjunction() = default;
    Conjunction( Exprs... exprs ) : expression_type{ exprs... } { }
};

template< size_t I, boolean_expression... Exprs >
auto conjunct( Conjunction< Exprs... > conj )
{ return conj.template conjunct< I >(); }

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


namespace detail {
/**
 * Conjunction details
 */
template< typename... Exprs >
struct ConjunctionHelper
{ using type = Conjunction< Exprs... >; };

// if any expression is false the conjunction is false (left)
template< typename... Exprs >
struct ConjunctionHelper< Constant< bool, false >, Exprs... >
{ using type = Constant< bool, false >; };

// if the first expression is true, the result is the conjunction of the 
// remaining expressions
template< typename... Exprs >
struct ConjunctionHelper< Constant< bool, true >, Exprs... >
{ using type = ConjunctionHelper< Exprs... >::type; };

// if any expression is false, the conjunction is false (right)
template< typename Expr >
struct ConjunctionHelper< Expr, Constant< bool, false >>
{ using type = Constant< bool, false >; };

// if it's a single constant value, then the conjunction is the value
template< bool Value >
struct ConjunctionHelper< Constant< bool, Value >>
{ using type = Constant< bool, Value >; };

/**
 * Disjunction details
 */
template< typename... Exprs >
struct DisjunctionHelper
{ using type = Disjunction< Exprs... >; };

// if any expression is true the disjunction is true (left)
template< typename... Exprs >
struct DisjunctionHelper< Constant< bool, true >, Exprs... >
{ using type = Constant< bool, true >; };

// if any expression is true the disjunction is true (right)
template< typename Expr >
struct DisjunctionHelper< Expr, Constant< bool, true >>
{ using type = Constant< bool, true >; };

// if the first expression is false, the value is the disjunction of the 
// remaining expressions
template< typename... Exprs >
struct DisjunctionHelper< Constant< bool, false >, Exprs... >
{ using type = DisjunctionHelper< Exprs... >::type; };

// a disjunction of a constant is the constant itself
template< bool Value >
struct DisjunctionHelper< Constant< bool, Value >>
{ using type = Constant< bool, Value >; };

template< typename Expr >
struct ComplimentHelper
{ using type = Compliment< Expr >; };

template< >
struct ComplimentHelper< Constant< bool, true >>
{ using type = Constant< bool, false >; };

template< >
struct ComplimentHelper< Constant< bool, false >>
{ using type = Constant< bool, true >; };
} // namespace detail

template< typename... Exprs >
using conjunction_t = detail::ConjunctionHelper< Exprs... >::type;

template< typename... Exprs >
using disjunction_t = detail::DisjunctionHelper< Exprs... >::type;

template< typename Expr >
using compliment_t = detail::ComplimentHelper< Expr >::type;

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
 * Represents a lazily evaluated A == 0
 */
template< typename Expr >
struct EqualsZero : Expression< bool, Expr >
{
    using expression_type = Expression< bool, Expr >;

    expression_type::result_type operator()() const
    { return expression_type::template eval<0>() == 
        constant_zero< result_of< Expr >>; }

    Expr quantity() { return get< 0 >( expression_type::exprs ); }
    
    EqualsZero() = default;
    EqualsZero( Expr expr ) : expression_type{ expr } { }
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

namespace detail {
template< typename LeftExpr, typename RightExpr >
struct EqualHelper
{ using type = Equal< LeftExpr, RightExpr >; };

template< typename T, T LeftValue, T RightValue >
struct EqualHelper< Constant< T, LeftValue >, Constant< T, RightValue >>
{ using type = Constant< bool, ( LeftValue == RightValue )>; };

template< typename Expr, typename T >
requires ( not is_constant< Expr > )
struct EqualHelper< Expr, constant_zero_expr< T >>
{ using type = EqualsZero< Expr >; };

template< typename T, typename Expr >
requires ( not is_constant< Expr > )
struct EqualHelper< constant_zero_expr< T >, Expr >
{ using type = EqualsZero< Expr >; };

template< typename Expr >
struct EqualsZeroHelper
{ using type = EqualsZero< Expr >; };

template< typename T, T Value >
struct EqualsZeroHelper< Constant< T, Value >>
{ using type = Constant< bool, ( Value == constant_zero< T > )>; };

template< typename LeftExpr, typename RightExpr >
struct NotEqualHelper
{ using type = NotEqual< LeftExpr, RightExpr >; };

template< typename T, T LeftValue, T RightValue >
struct NotEqualHelper< Constant< T, LeftValue >, Constant< T, RightValue >>
{ using type = Constant< bool, ( LeftValue != RightValue )>; };

template< typename LeftExpr, typename RightExpr >
struct LessHelper
{ using type = Less< LeftExpr, RightExpr >; };

template< typename T, T LeftValue, T RightValue >
struct LessHelper< Constant< T, LeftValue >, Constant< T, RightValue >>
{ using type = Constant< bool, ( LeftValue < RightValue )>; };

template< typename LeftExpr, typename RightExpr >
struct LessOrEqualHelper
{ using type = LessOrEqual< LeftExpr, RightExpr >; };

template< typename T, T LeftValue, T RightValue >
struct LessOrEqualHelper< Constant< T, LeftValue >, Constant< T, RightValue >>
{ using type = Constant< bool, ( LeftValue <= RightValue )>; };

template< typename LeftExpr, typename RightExpr >
struct GreaterHelper
{ using type = Greater< LeftExpr, RightExpr >; };

template< typename T, T LeftValue, T RightValue >
struct GreaterHelper< Constant< T, LeftValue >, Constant< T, RightValue >>
{ using type = Constant< bool, ( LeftValue > RightValue )>; };

template< typename LeftExpr, typename RightExpr >
struct GreaterOrEqualHelper
{ using type = GreaterOrEqual< LeftExpr, RightExpr >; };

template< typename T, T LeftValue, T RightValue >
struct GreaterOrEqualHelper< Constant< T, LeftValue >, Constant< T, RightValue >>
{ using type = Constant< bool, ( LeftValue >= RightValue )>; };
} // namespace detail

template< typename LeftExpr, typename RightExpr >
using equal_t = detail::EqualHelper< LeftExpr, RightExpr >::type;

template< typename LeftExpr, typename RightExpr >
using not_equal_t = detail::NotEqualHelper< LeftExpr, RightExpr >::type;

template< typename Expr >
using equals_zero_t = detail::EqualsZeroHelper< Expr >::type;

template< typename LeftExpr, typename RightExpr >
using less_t = detail::LessHelper< LeftExpr, RightExpr >::type;

template< typename LeftExpr, typename RightExpr >
using less_or_equal_t = detail::LessOrEqualHelper< LeftExpr, RightExpr >::type;

template< typename LeftExpr, typename RightExpr >
using greater_t = detail::GreaterHelper< LeftExpr, RightExpr >::type;

template< typename LeftExpr, typename RightExpr >
using greater_or_equal_t = detail::GreaterOrEqualHelper< LeftExpr, RightExpr >::type;

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
        constant_zero< typename expression_type::result_type > ); }

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

namespace detail {
template< typename... Exprs >
struct SumHelper
{ using type = Sum< Exprs... >; };

template< typename Expr >
struct SumHelper< Expr >
{ using type = Expr; };

template< typename T, T... Values >
struct SumHelper< Constant< T, Values >... >
{ using type = Constant< T, ( ... + Values )>; };

template< typename Expr, typename T >
requires( not is_constant< Expr >)
struct SumHelper< Expr, constant_zero_expr< T >>
{ using type = Expr; };

template< typename T, typename Expr, typename... Exprs >
requires( not is_constant< Expr >)
struct SumHelper< constant_zero_expr< T >, Expr, Exprs... >
{ using type = SumHelper< Exprs... >::type; };
} // namespace detail

template< typename... Exprs >
using sum_t = detail::SumHelper< Exprs... >::type;

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

namespace detail {
template< typename Expr >
struct NegationHelper
{ using type = Negation< Expr >; };

template< typename T, T Value >
struct NegationHelper< Constant< T, Value >>
{ using type = Constant< T, -Value >; };
} // namespace detail

template< typename Expr >
using negation_t = detail::NegationHelper< Expr >::type;

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
 * 
 * DT: we're doing it. It's the pattern of alternating terms in a determinant
 */
template< typename Expr, typename... Exprs >
// TODO: fix the requires expression
requires same_expression_result_types< Expr, Exprs... >
struct Difference : Expression< result_of< Expr >, Expr, Exprs... >
{
    using expression_type = Expression< result_of< Expr >, Expr, Exprs... >;

    expression_type::result_type operator()() const
    { return expression_type::template eval<0>() - 
        expression_type::template eval<1>(); }

    template< size_t I >
    auto term() { return get< I >( expression_type::exprs ); }

    Difference() = default;
    Difference( Expr expr, Exprs... exprs ) : expression_type{ expr, exprs... } { }
};

namespace detail {
template< typename... Exprs >
struct DifferenceHelper
{ using type = Difference< Exprs... >; };

template< typename MinuendExpr, typename T >
requires( not is_constant< MinuendExpr >)
struct DifferenceHelper< MinuendExpr, constant_zero_expr< T >>
{ using type = MinuendExpr; };

template< typename T, typename... Exprs >
struct DifferenceHelper< constant_zero_expr< T >, Exprs... >
{ using type = negation_t< typename DifferenceHelper< Exprs... >::type >; };

template< typename T, T MinuendValue, T SubtractendValue >
struct DifferenceHelper< Constant< T, MinuendValue >, 
    Constant< T, SubtractendValue >>
{ using type = Constant< T, MinuendValue - SubtractendValue >; };

template< typename Expr >
struct DifferenceHelper< Expr, Expr >
{ using type = constant_zero_expr< result_of< Expr >>; };
} // namespace detail

template< typename... Exprs >
using difference_t = detail::DifferenceHelper< Exprs... >::type;

/**
 * Represents a lazy product
 */
template< typename Expr1, typename Expr2, typename... Exprs >
requires same_expression_result_types< Expr1, Expr2, Exprs... >
struct Product : Expression< result_of< Expr1 >, Expr1, Expr2, Exprs... >
{
    using expression_type = Expression< result_of< Expr1 >, Expr1, Expr2, 
        Exprs... >;

    expression_type::result_type operator()() const
    { return expression_type::fold_left( std::multiplies<>{}, 
        constant_one< typename expression_type::result_type > ); }

    Product() = default;
    constexpr Product( tuple< Expr1, Expr2, Exprs... > expr_tuple ) :
        expression_type{ expr_tuple } 
    { }
    constexpr Product( Expr1 expr1, Expr2 expr2, Exprs... exprs ) : 
        expression_type{ expr1, expr2, exprs... } 
    { }
};

namespace detail {
template< typename... Exprs >
struct ProductHelper
{ using type = Product< Exprs... >; };

template< typename Expr >
struct ProductHelper< Expr >
{ using type = Expr; };

template< typename T, T... Values >
struct ProductHelper< Constant< T, Values >... >
{ using type = Constant< T, ( ... * Values )>; };

template< typename T, typename... Exprs >
struct ProductHelper< constant_zero_expr< T >, Exprs... >
{ using type = constant_zero_expr< T >; };

template< typename T, typename... Exprs >
struct ProductHelper< constant_one_expr< T >, Exprs... >
{ using type = ProductHelper< Exprs... >::type; };

template< typename Expr, typename T >
struct ProductHelper< Expr, constant_zero_expr< T >>
{ using type = constant_zero_expr< T >; };

template< typename Expr, typename T >
struct ProductHelper< Expr, constant_one_expr< T >>
{ using type = Expr; };
} // namespace detail

template< typename... Exprs >
using product_t = detail::ProductHelper< Exprs... >::type;

template< typename Expr >
struct Inversion : Expression< result_of< Expr >, Expr >
{
    using expression_type = Expression< result_of< Expr >, Expr >;
    expression_type::result_type operator()() const
    { return constant_one< typename expression_type::result_type > / 
        expression_type::template eval<0>(); }
    
    Inversion() = default;
    Inversion( Expr expr ) : expression_type{ expr } {}
};

namespace detail {
template< typename Expr >
struct InversionHelper
{ using type = Inversion< Expr >; };

template< typename T, T Value >
struct InversionHelper< Constant< T, Value >>
{ using type = Constant< T, 1. / Value >; };

} // namespace detail

template< typename Expr >
using inversion_t = detail::InversionHelper< Expr >::type;

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

namespace detail {
template< typename NumeratorExpr, typename DenominatorExpr >
struct QuotientHelper
{ using type = Quotient< NumeratorExpr, DenominatorExpr >; };

template< typename NumeratorExpr, typename T >
struct QuotientHelper< NumeratorExpr, constant_one_expr< T >>
{ using type = NumeratorExpr; };

template< typename T, typename DenominatorExpr >
struct QuotientHelper< constant_zero_expr< T >, DenominatorExpr >
{ using type = constant_zero_expr< T >; };

// TODO: handle infinities and div-by-zero
} // namespace detail

template< typename NumeratorExpr, typename DenominatorExpr >
using quotient_t = detail::QuotientHelper< NumeratorExpr, DenominatorExpr >::type;

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

namespace detail {
template< typename ExponentExpr >
struct ExpHelper
{ using type = Exp< ExponentExpr >; };

template< typename T, T Value >
struct ExpHelper< Constant< T, Value >>
{ using type = Constant< T, std::exp( Value ) >; };
} //namespace detail

template< typename ExponentExpr >
using exp_t = detail::ExpHelper< ExponentExpr >::type;

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

namespace detail {
template< typename ArgumentExpr >
struct LogHelper
{ using type = Log< ArgumentExpr >; };

template< typename T, T Value >
struct LogHelper< Constant< T, Value >>
{ using type = Constant< T, std::log( Value ) >; };
} //namespace detail

template< typename ArgumentExpr >
using log_t = detail::LogHelper< ArgumentExpr >::type;

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

namespace detail {
template< typename BaseExpr, typename ExponentExpr >
struct PowerHelper
{ using type = Power< BaseExpr, ExponentExpr >; };

template< typename T, typename ExponentExpr >
requires( not is_constant< ExponentExpr >)
struct PowerHelper< constant_zero_expr< T >, ExponentExpr >
{ using type = constant_zero_expr< T >; };

template< typename T, typename ExponentExpr >
requires( not is_constant< ExponentExpr >)
struct PowerHelper< constant_one_expr< T >, ExponentExpr >
{ using type = constant_one_expr< T >; };

template< typename BaseExpr, typename T >
requires( not is_constant< BaseExpr >)
struct PowerHelper< BaseExpr, constant_zero_expr< T >>
{ using type = constant_one_expr< T >; };

template< typename BaseExpr, typename T >
requires( not is_constant< BaseExpr >)
struct PowerHelper< BaseExpr, constant_one_expr< T >>
{ using type = BaseExpr; };

template< typename T, T BaseValue, T ExponentValue >
struct PowerHelper< Constant< T, BaseValue >, Constant< T, ExponentValue >>
{ using type = Constant< T, std::pow( BaseValue, ExponentValue )>; };

} // namespace detail

template< typename BaseExpr, typename ExponentExpr >
using power_t = detail::PowerHelper< BaseExpr, ExponentExpr >::type;

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

namespace detail {
template< typename ArgumentExpr >
struct SineHelper 
{ using type = Sine< ArgumentExpr >; };

template< typename T >
struct SineHelper< constant_zero_expr< T >>
{ using type = constant_zero_expr< T >; };
} // namespace detail

template< typename ArgumentExpr >
using sine_t = detail::SineHelper< ArgumentExpr >::type;

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

namespace detail {
template< typename ArgumentExpr >
struct CosineHelper 
{ using type = Cosine< ArgumentExpr >; };

template< typename T >
struct CosineHelper< constant_zero_expr< T >>
{ using type = constant_one_expr< T >; };
} // namespace detail

template< typename ArgumentExpr >
using cosine_t = detail::CosineHelper< ArgumentExpr >::type;

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

namespace detail {
template< typename ArgumentExpr >
struct TangentHelper 
{ using type = Tangent< ArgumentExpr >; };

template< typename T >
struct TangentHelper< constant_zero_expr< T >>
{ using type = constant_zero_expr< T >; };
} // namespace detail

template< typename ArgumentExpr >
using tangent_t = detail::TangentHelper< ArgumentExpr >::type;

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

namespace detail {
template< typename ArgumentExpr >
struct ArcsineHelper
{ using type = Arcsine< ArgumentExpr >; };

template< typename T >
struct ArcsineHelper< constant_zero_expr< T >>
{ using type = constant_zero_expr< T >; };
} // namespace detail

template< typename ArgumentExpr >
using arcsine_t = detail::ArcsineHelper< ArgumentExpr >::type;

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

namespace detail {
template< typename ArgumentExpr >
struct ArccosineHelper
{ using type = Arccosine< ArgumentExpr >; };

// TODO: arccos(0) == pi/2

} // namespace detail

template< typename ArgumentExpr >
using arccosine_t = detail::ArccosineHelper< ArgumentExpr >::type;

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

namespace detail {
template< typename ArgumentExpr >
struct ArctangentHelper
{ using type = Arctangent< ArgumentExpr >; };

template< typename T >
struct ArctangentHelper< constant_zero_expr< T >>
{ using type = constant_zero_expr< T >; };
} // namespace detail

template< typename ArgumentExpr >
using arctangent_t = detail::ArctangentHelper< ArgumentExpr >::type;

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


// helper functions

/**
 * Logical Operations
 */
// TODO: the bodies of these functions will probably have to change when the 
// return type is a constant because constants take no parameters.
// we could have constants take multiple constructor parameters, but that seems
// confusing.  Having multiple versions similar to the conjunction_t
// specializations is likely the best option, but a lot of typing...
template< boolean_expression... Exprs >
conjunction_t< Exprs... > conjunction_of( Exprs... exprs )
{ return { exprs... }; }

template< boolean_expression... Exprs >
disjunction_t< Exprs... > disjunction_of( Exprs... exprs )
{ return { exprs... }; }

template< boolean_expression Expr >
compliment_t< Expr > compliment_of( Expr expr )
{ return { expr }; }

/**
 * Numeric comparisons
 */
template<typename LeftExpr, typename RightExpr >
equal_t< LeftExpr, RightExpr > equal( 
    LeftExpr left_expr, RightExpr right_expr )
{ return { left_expr, right_expr }; }

template<typename Expr >
equals_zero_t< Expr > equals_zero( Expr expr )
{ return { expr }; }

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

template< typename T, T... Values >
Constant< T, ( ... + Values )> sum_of( Constant< T, Values >... ) { }

template< typename... Exprs >
Difference< Exprs... > difference_of( Exprs... exprs )
{ return { exprs... }; }

template< typename T, T MinuendValue, T SubtractendValue >
Constant< T, ( MinuendValue - SubtractendValue )> difference_of(
    Constant< T, MinuendValue >, Constant< T, SubtractendValue > ) { }

template< typename Expr >
auto negation_of( Expr expr )
{ return negation_t< Expr >{ expr }; }

template< typename T, T Value >
auto negation_of( Constant< T, Value > )
{ return Constant< T, -Value >{}; }

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


/**
 * Applies the derivation D to the continuous_expression Expr using Leibniz 
 * rules
 * 
 * @tparam D is a template that takes an expression as it's singular argument
 * and exposes a type as the return type
 * @tparam Expr is the cntinuous expression to be differentiated
 * @returns a type corresponding to the derivative of Expr
 */
template< template< typename > class D, continuous_expression Expr >
struct Derivation
{ using type = D< Expr >::type; };

/**
 * Derivations of functions
 */
template< template< typename > class D, typename Unit, Unit Value >
struct Derivation< D, Constant< Unit, Value >> 
{ using type = constant_zero_expr< Unit >; };

template< template< typename > class D, typename... Exprs >
struct Derivation< D, Sum< Exprs... >>
{ using type = sum_t< typename Derivation< D, Exprs >::type... >; };

template< template< typename > class D, 
    typename MinuendType, typename SubtractendType >
struct Derivation< D, Difference< MinuendType, SubtractendType >>
{ using type = difference_t< typename Derivation< D, MinuendType >::type, 
    typename Derivation< D, SubtractendType >::type>; };

template< template< typename > class D, typename Expr >
struct Derivation< D, Negation< Expr >>
{ using type = negation_t< typename Derivation< D, Expr >::type >; };

template< template< typename > class D, typename Expr, typename... Exprs >
struct Derivation< D, Product< Expr, Exprs... >>
{
    // product rule: d(f*g) = df*g + f*dg
    using type = sum_t< 
        product_t< typename Derivation< D, Expr >::type, product_t< Exprs... >>,
        product_t< Expr, typename Derivation< D, product_t< Exprs... >>::type >>;
};

template< template< typename > class D, 
    typename NumeratorExpr, typename DenominatorExpr >
struct Derivation< D, Quotient< NumeratorExpr, DenominatorExpr >>
{
    // quotient rule: d(f/g) = ( g*df - f*dg ) / ( g * g );
    using type = quotient_t<
        difference_t<
            product_t< typename Derivation< D, NumeratorExpr >::type, DenominatorExpr >,
            product_t< NumeratorExpr, typename Derivation< D, DenominatorExpr >::type >>,
        product_t< DenominatorExpr, DenominatorExpr >>;
};

template< template< typename > class D, typename Expr >
struct Derivation< D, Exp< Expr >>
{ using type = product_t< Exp< Expr >, typename Derivation< D, Expr >::type >; };

template< template< typename > class D, typename Expr >
struct Derivation< D, Log< Expr >>
{ using type = quotient_t< typename Derivation< D, Expr >::type, Expr >; };

// NOTE: Derivations only defined for constant powers
// TODO: define derivations for non-constant power expressions
template< template< typename > class D, typename BaseExpr, numeric T, T Value >
struct Derivation< D, Power< BaseExpr, Constant< T, Value >>>
{
    // power rule: d(f^n) == nf^{n-1}df
    using type = product_t< 
        Constant< T, Value >, 
        power_t< 
            BaseExpr, 
            difference_t< Constant< T, Value >, constant_one_expr< T >>>,
        typename Derivation< D, BaseExpr >::type >;
    // TODO: ensure the power rule is 
};

template< template< typename > class D, numeric T, T Value, typename ExponentExpr >
struct Derivation< D, Power< Constant< T, Value >, ExponentExpr >>
{
    // logarithmic differentiation: v^f(x) f'(x) ln v 
    using type = product_t< 
        power_t< Constant< T, Value >, ExponentExpr >,
        typename Derivation< D, ExponentExpr >::type,
        log_t< Constant< T, Value >>>;
    // TODO: ensure the power rule is 
};

template< template< typename > class D, typename BaseExpr, typename ExponentExpr >
requires( not is_constant< BaseExpr > and not is_constant< ExponentExpr >)
struct Derivation< D, Power< BaseExpr, ExponentExpr >>
{
    // logarithmic differentiation: f(x)^g(x) ( g'(x) ln f(x) + g(x) f'(x) / f(x) )
    using type = product_t< 
        power_t< BaseExpr, ExponentExpr >,
        sum_t<
            product_t< 
                typename Derivation< D, ExponentExpr >::type,
                log_t< BaseExpr >>,
            product_t<
                ExponentExpr,
                quotient_t<
                    typename Derivation< D, BaseExpr >::type,
                    BaseExpr >>>>;
    // TODO: ensure the power rule is 
};

template< template< typename > class D, typename Expr >
struct Derivation< D, Sine< Expr >>
{
    using type = product_t<
        cosine_t< Expr >,
        typename Derivation< D, Expr >::type >;
};

template< template< typename > class D, typename Expr >
struct Derivation< D, Cosine< Expr >>
{
    using type = negation_t<
        product_t<
            sine_t< Expr >,
            typename Derivation< D, Expr >::type >>;
};

template< template< typename > class D, typename Expr >
struct Derivation< D, Tangent< Expr >>
{
    using type = quotient_t<
        typename Derivation< D, Expr >::type,
        product_t< cosine_t< Expr >, cosine_t< Expr >>>;
};

// TODO: arc-trig functions


template< template< typename > class D, continuous_expression Expr >
using derive = Derivation< D, Expr >::type;

// template< typename Var, typename Expr >
// struct PartialDerivative;

template< typename Var >
struct PartialDerivative;

template< typename T, size_t... Is >
struct PartialDerivative< Variable< T, Is... >>
{
    template< typename Expr >
    struct Of;

    template< typename Expr >
    requires ( not depends_on< Expr, Variable< T, Is... >> )
    struct Of< Expr >
    { using type = constant_zero_expr< result_of< Expr >>; };

    template<>
    struct Of< Variable< T, Is... >>
    { using type = constant_one_expr< T >; };

    template< template< typename... > class Op, typename... Exprs >
    struct Of< Op< Exprs... >>
    { using type = derive< PartialDerivative< Variable< T, Is... >>::Of, Op< Exprs... >>; };
};

template< typename Expr, typename Var >
using partial = PartialDerivative< Var >::template Of< Expr >::type;

#ifndef NDEBUG
namespace test {
using cf0 = Constant< float, 0.f >;
using cf1 = Constant< float, 1.f >;

using vf0 = Variable< float, 0 >;
using vf1 = Variable< float, 1 >;

static_assert( is_same_v< partial< vf0, vf0 >, cf1 >);
static_assert( is_same_v< partial< vf0, vf1 >, cf0 >);
static_assert( is_same_v< partial< sum_t< vf0, vf0 >, vf1 >, cf0 >);
static_assert( is_same_v< partial< difference_t< vf0, vf0 >, vf1 >, cf0 >);
static_assert( is_same_v< partial< difference_t< vf1, vf0 >, vf1 >, cf1 >);
static_assert( is_same_v< partial< negation_t< vf1 >, vf1 >, 
    Constant< float, -1.f >> );
static_assert( is_same_v< partial< product_t< vf0, cf1 >, vf1 >, cf0 >);
static_assert( is_same_v< partial< product_t< vf0, cf1 >, vf0 >, cf1 >);
static_assert( is_same_v< partial< product_t< vf0, vf1 >, vf0 >, product_t< cf1, vf1 >>);
static_assert( is_same_v< partial< quotient_t< vf1, vf0 >, vf1 >, quotient_t< vf0, product_t< vf0, vf0 >> >);
// DT: isn't this cool? d/dx sin(x) == cos(x)
static_assert( is_same_v< partial< sine_t< vf0 >, vf0 >, cosine_t< vf0 >>);
} // namespace test
#endif


// TODO: change these to use the functions instead of returning the op types
// directly
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
constexpr equal_t< LeftExpr, RightExpr > operator ==( 
    LeftExpr left_expr, RightExpr right_expr )
{ return { left_expr, right_expr }; }

template< numeric T, typename Expr >
auto constexpr operator ==( 
    const T value, Expr expr )
{ return equals( Constant< T, value >{}, expr ); }

template< typename Expr, numeric T >
auto constexpr operator ==( 
    Expr expr, T const value )
{ return equals( expr, Constant< T, value >{} ); }

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
auto operator - ( Expr expr )
{ return negation_of( expr ); }

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

// namespace std {

// template< typename Expr >
// requires ::expressions::is_expression< Expr >
// struct negate< Expr >
// {
//     constexpr auto operator()( Expr expr ) const
//     { return negation_of( expr ); }
// };

// } // namespace std

#endif