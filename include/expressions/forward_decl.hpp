#ifndef __EXPRESSIONS_FORWARD_DECL_HPP__
#define __EXPRESSIONS_FORWARD_DECL_HPP__


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
#include <limits>

// #include <string_view>
//

//#ifndef NDEBUG
#include <print>
//#endif


namespace expressions {

//////////////////////
/// Bootstrapping ///
////////////////////
/// 
/// We need to bootstrap variables, variable dependencies, expressions and
/// substitutions with forward declarations and traits prior to our official
/// expression_traits and variable_traits classes since there are
/// interdependencies that must be carefully declared and implemented to avoid
/// circular logic.
///
using std::size_t;
using std::tuple, std::make_tuple, std::tuple_element_t, std::get;
using std::map, std::set;
using std::any, std::any_cast;
using std::optional;
using std::integral_constant, std::true_type, std::false_type;

using namespace tensors;

///////////////////////////////////
/// Bootstrapping: Expressions ///
/////////////////////////////////
///
/// Forward declaration, concept, and traits to identify an expression class
///
/// expression< Op<...>>
/// - CompoundExpression< Op<... >>
///     - ExpressionOperation< Op > // deprecated
///     - CompoundOperation< Op >
///     - DiscriminatedOperation< Op >

/// @brief trait to identify a type as an expression type
/// @tparam T is the type to be checked
template< typename T >
struct IsExpression: std::false_type { };

template< template< typename... > class Op >
struct IsCompoundOperation: std::false_type { };

template< template< auto, typename... > class Op >
struct IsDiscriminatedOperation: std::false_type { };

template< typename T >
struct IsCompoundExpression: std::false_type { };

// all compound expressions are expressions
template< typename T >
requires( IsCompoundExpression< T >::value )
struct IsExpression< T >: true_type { };

template< typename... Ts >
struct IsExpression< tuple< Ts... >>: integral_constant< bool,
    ( IsExpression< Ts >::value or ... )> { };

template< shape S, typename... Ts >
struct IsExpression< Tensor< S, Ts... >>: integral_constant< bool,
    ( IsExpression< Ts >::value or ... )> { };

// Op< typename... > is a compound expression if Op is an compound operation
// or an expression operation (deprecated)
template< template< typename... > class Op, typename... Args >
struct IsCompoundExpression< Op< Args... >>: integral_constant< bool,
    IsCompoundOperation< Op >::value > 
{ };

// Op< auto, typename... > is a compound operation if Op is a discriminated
// operation
template< template< auto, typename... > class Op, auto Discriminator, 
    typename... Args >
struct IsCompoundExpression< Op< Discriminator, Args... >>:
    IsDiscriminatedOperation< Op > { };

template< typename T >
constexpr bool is_compound_expression_v = IsCompoundExpression< T >::value;

/// @brief is true if T is an expression type
/// @tparam T is the type to be checked
///
template< typename T >
static constexpr bool is_expression_v = IsExpression< T >::value;

/// @brief concept to identify an expression
///
template< typename T >
concept expression = is_expression_v< T >;

template< typename T >
concept non_expression = not expression< T >;

template< typename T >
concept compound_expression = is_compound_expression_v< T >;

/// @brief type erased expression container
/// TODO: build a stack representation of the expression
template< typename ResultT >
struct Expression
{
    using result_type = ResultT;

    template< typename ExprT >
    requires( std::is_same_v< ResultT, typename ExprT::result_type > )
    constexpr Expression( ExprT const& expr );

    constexpr Expression() = default;

private:
};

//////////////////////////////////////////////////////////////////
/// Exppression Arguments: get_argument< I >,                 ///
///                        expression_argument_t< I, ExprT > ///
///////////////////////////////////////////////////////////////
///
/// @brief type trait for extracting an argument from a compound expression
/// @tparam I is the id of the argument
/// @tparam ExprT is the type of the compound expression
template< size_t I, typename ExprT >
struct GetArgument;

template< size_t I, template< typename... > class Op, typename... Args >
struct GetArgument< I, Op< Args... >>
{
    using type = Args...[ I ];
    static constexpr type const&
    value( Op< Args... > const& expr )
    { return std::get< I >( expr ); }
};

template< size_t I, typename ExprT >
using expression_argument_t = GetArgument< I, ExprT >::type;

template< size_t I, typename ExprT >
constexpr expression_argument_t< I, ExprT > const& 
get_argument( ExprT const& expr )
{ return GetArgument< I, ExprT >::value( expr ); }


////////////////////
/// Result Type ///
//////////////////
/// 
/// Trait to determine the result_type of an expression type
///
/// @brief Trait for the result type of an expression
/// @tparam T the type to be evaluated
///
template< typename T >
struct Result
{ using type = std::remove_cvref_t< T >; };

/// @brief result of a tuple is a tuple of results
template< typename... Ts >
requires( expression< tuple< Ts... >> )
struct Result< tuple< Ts... >>
{ using type = tuple< typename Result< Ts >::type... >; };

/// @brief result of a tensor is a tensor of results
template< shape S, typename... Ts >
requires( expression< Tensor< S, Ts... >> )
struct Result< Tensor< S, Ts... >>
{ using type = Tensor< S, typename Result< Ts >::type... >; };

/// @brief results of a compound expression uses the required static value
///        method
template< template< typename... > class Op, typename... Args >
requires( compound_expression< Op< Args... >> )
struct Result< Op< Args... >>
{ using type = std::remove_cvref_t< decltype(
    Op< typename Result< Args >::type... >::value( 
        typename Result< Args >::type{}... ))>; };

template< template< auto, typename... > class Op, auto Discriminator, 
    typename... Args >
requires( compound_expression< Op< Discriminator, Args... >> )
struct Result< Op< Discriminator, Args... >>
{ using type = std::remove_cvref_t< decltype(
    Op< Discriminator, typename Result< Args >::type... >::value(
        typename Result< Args >::type{}... ))>; };

/// @brief trait to resolve the result type of an expression
template< typename T >
using result_t = Result< T >::type;

//////////////////////
/// Reconstituter ///
////////////////////
///
/// Creates a new compound expression using provided subs as arguments in place
/// of the original argument types.
///
namespace detail {

template< typename ExprT, typename... Subs >
struct Reconstituter;

template< template< typename... > class Op, typename... Args, 
    typename... Subs >
requires( sizeof...( Args ) == sizeof...( Subs ))
struct Reconstituter< Op< Args... >, Subs... >
{
    using type = Op< Subs... >;
    static constexpr type
    value( Op< Args... > const& expr, Subs const&... subs )
    { return { subs... }; }
};

template< template< auto, typename... > class Op, auto Discriminator,
    typename... Args, typename... Subs >
requires( sizeof...( Args ) == sizeof...( Subs ))
struct Reconstituter< Op< Discriminator, Args... >, Subs... >
{
    using type = Op< Discriminator, Subs... >;
    static constexpr type
    value( Op< Discriminator, Args... > const& expr, Subs const&... subs )
    { return { subs... }; } 
};

} // namespace detail
  
template< typename ExprT, typename... Subs >
using reconstitute_t = detail::Reconstituter< ExprT, Subs... >::type;

template< typename ExprT, typename... Subs>
constexpr reconstitute_t< ExprT, Subs... >
reconstitute( ExprT const& expr, Subs const&... subs )
{ return detail::Reconstituter< ExprT, Subs... >::value( expr, subs... ); }

////////////
/// Var ///
//////////
/// 
/// A typed placeholder in an expression
///
/// @brief Placeholder for a value of type T. Two variables are equivalent if 
/// their identifiers I are the same. Variables with the same identifier in an
/// expression must have the same value_type T.
template< size_t I, typename T >
struct Var;

/// @brief limit the id of variables created by the user to half 
///        those available
//#ifdef NDEBUG
//constexpr size_t maximum_user_variable_id = 
//    std::numeric_limits< size_t >::max() >> 1;
//#else
//constexpr size_t maximum_user_variable_id = 10;
//#endif

template< size_t I, typename T >
struct IsExpression< Var< I, T >>: std::true_type { };

/// @brief result of a variable is the result of it's value_type
template< size_t I, typename T >
struct Result< Var< I, T >>
{ using type = Result< T >::type; };

//////////////
/// VarId ///
////////////
///
/// a trait to extract the variable ID during bootstrapping
template< typename Var >
struct VarId;

template< size_t I, typename T >
struct VarId< Var< I, T >>: integral_constant< size_t, I > { };

template< typename Var >
constexpr size_t var_id_v = VarId< Var >::value;

template< typename Var >
struct VarValue;

template< size_t I, typename T >
struct VarValue< Var< I, T >>
{ using type = T; };

template< typename Var >
using var_value_t = VarValue< Var >::type;

///////////////////////////
/// Static Expressions ///
/////////////////////////
///
/// @brief trait to identify expressions that do not contain variables, free
/// nor bound.
template< typename ExprT >
struct IsStaticExpression: IsExpression< ExprT > { }; 

template< size_t I, typename T >
struct IsStaticExpression< Var< I, T >>: 
    std::integral_constant< bool, false > { };

template< template< typename... > class Op, typename... Args >
requires( compound_expression< Op< Args... >> )
struct IsStaticExpression< Op< Args... >>: std::integral_constant< bool, 
    ( IsStaticExpression< Args >::value and ... )> { };

template< typename T >
concept static_expression = IsStaticExpression< T >::value;

///////////////////////
/// Terminal trait ///
/////////////////////
///
template< typename T >
struct IsTerminal: std::true_type {};

template< expression ExprT >
struct IsTerminal< ExprT >: integral_constant< bool,
    std::is_same_v< std::remove_cvref_t< decltype( ExprT{}() )>, ExprT >>
{ };

////////////
/// Sub ///
//////////
///
/// @brief forward declaration of substitution expression
template< typename ExprT, typename... Subs >
struct Sub;

// NOTE: we aren't treating Sub<...> as a compound expression, but
//       instead are treating any specialization as an expression
template< >
struct IsCompoundOperation< Sub >: std::true_type { };

/// @brief any specializaiton of a Sub</*stitution*/> is an 
///        expression, but it is not a compound expression.  This 
///        prevents the default parsing of compound expressions.
//template< typename ExprT, typename... Subs >
//struct IsExpression< Sub< ExprT, Subs... >>: std::true_type { };

/// @brief result of a substitution is the result of it's expression formula
template< typename FormulaT, typename... Args >
struct Result< Sub< FormulaT, Args... >>: Result< FormulaT > { };

// we need trait for substitution since it requires a bespoke means of 
// calculating the dependent variables
template< typename T >
struct IsSubExpression: std::false_type { };

template< typename ExprT, typename... Subs >
struct IsSubExpression< Sub< ExprT, Subs... >>:
    std::integral_constant< bool, true > { };

template< typename T >
constexpr bool is_substitution_expression_v = 
    IsSubExpression< T >::value;

//////////////////
/// Var Order ///
////////////////
/// 
/// Vars that represent other variables are higher-order:
///
///     Var< 0, Var< 0, float >> x; // second order variable that results in float
///
/// NOTE: I don't know if the IDs should match or not.  Right now I'm allowing them not to match
///
/// A non-variable is a variable of order 0.
/// A variable of a non-variable is order 1
///
//
/// Var Order
template< typename >
struct VarOrder: integral_constant< size_t, 0 > { };

template< size_t I, typename T >
struct VarOrder< Var< I, T >>: integral_constant< size_t, 
    1 + VarOrder< T >::value > { };

template< typename... Ts >
struct VarOrder< tuple< Ts... >> {
private:
    static constexpr size_t tuple_size = sizeof...( Ts );
    typedef make_seq< tuple_size > for_elements;

    template< typename Seq >
    struct Helper;

    template< size_t I, size_t... Is >
    struct Helper< seq< I, Is... >> {
    private:
        static constexpr size_t first_order = 
            VarOrder< Ts...[ I ]>::value;
        static constexpr size_t rest_order = Helper< seq< Is... >>::value;
    public:
        static constexpr size_t value = std::max( first_order, rest_order );
    };

    template< >
    struct Helper< seq< >>
    { static constexpr size_t value = 0; };

public:
    static constexpr size_t value = Helper< for_elements >::value;
};

template< compound_expression ExprT >
struct VarOrder< ExprT >: 
    VarOrder< typename ExprT::arguments_tuple > { };

template< typename V >
constexpr size_t var_order_v = VarOrder< V >::value;

//static_assert( var_order_v< int > == 0 );
//static_assert( var_order_v< Var< 0, int >> == 1 );
//static_assert( var_order_v< Var< 0, Var< 0, int >>> == 2 );
//static_assert( var_order_v< tuple< Var< 0, int >, Var< 0, Var< 0, int >>>> == 2 );

/// @brief trait to identify variables
/// @tparam T the type to be tested
///
template< typename T >
struct IsVar: std::false_type { };

template< size_t I, typename T >
struct IsVar< Var< I, T >>: std::integral_constant< bool, true > { };

template< typename T >
constexpr bool is_variable_v = IsVar< T >::value;

template< typename T >
concept variable = is_variable_v< T >;

template< typename Var, typename T >
concept variable_of = is_variable_v< Var > and 
    is_same_v< typename Var::value_type, T >;

////////////////////////////////////////////
/// Increasing and Decreasing Var Order ///
//////////////////////////////////////////
/// 
/// @brief raising variable order
template< typename Var >
struct IncreaseVarOrder;

template< size_t I, typename T >
struct IncreaseVarOrder< Var< I, T >>
{
    using type = Var< I, Var< I, T >>;
    static constexpr type
    value( Var< I, T > const& var )
    { return { var.name() }; }
};

template< typename Var >
using increase_var_order_t = IncreaseVarOrder< Var >::type;

template< typename Var >
constexpr increase_var_order_t< Var >
increase_var_order( Var const& var )
{ return IncreaseVarOrder< Var >::value( var ); }

/// @brief lowering variable order
template< typename Var >
struct DecreaseVarOrder;

template< size_t I, typename T >
struct DecreaseVarOrder< Var< I, Var< I, T >>>
{
    using type = Var< I, T >;
    static constexpr type
    value( Var< I, Var< I, T >> const& var )
    { return { var.name() }; }
};

template< typename Var >
using decrease_var_order_t = DecreaseVarOrder< Var >::type;

template< typename Var >
constexpr decrease_var_order_t< Var >
decrease_var_order( Var const& var )
{ return DecreaseVarOrder< Var >::value( var ); }

template< typename T >
struct IsSecondOrderVar: std::false_type { };

template< size_t I, typename T >
struct IsSecondOrderVar< Var< I, Var< I, T >>>: std::true_type 
{ };

template< typename T >
concept second_order_variable = IsSecondOrderVar< T >::value;

/////////////
/// Func ///
///////////
///
/// @brief representation of a mathematical function
template< typename FormulaT, variable... Vars >
struct Func;

template< typename T >
struct IsFunc: std::false_type { };

template< typename FormulaT, variable... Vars >
struct IsFunc< Func< FormulaT, Vars... >>: std::true_type { };

template< typename T >
constexpr bool is_function_v = IsFunc< T >::value;

/// @brief a function is an expression, but not a compound one
template< typename FormulaT, variable... Vars >
struct IsExpression< Func< FormulaT, Vars... >>: std::true_type { };
//template< >
//struct IsExpressionOperation< Func >: std::true_type { };

/// @brief the result of a function is the result of the formula
template< typename T, typename... Vars >
struct Result< Func< T, Vars... >>: Result< T > { };

///////////////////
/// Element Of ///
/////////////////
///
/// @brief Element Of operation
template< size_t I, typename ArrayT >
struct Element;

template< size_t I, typename ArrayT >
struct IsExpression< Element< I, ArrayT >>: std::true_type { };

template< typename T >
struct IsElementExpression: std::false_type { };

template< size_t I, typename ArrayT >
struct IsElementExpression< Element< I, ArrayT >>: std::true_type { };

template< typename T >
constexpr bool is_element_expression_v = IsElementExpression< T >::value;

template< size_t I, typename ArrayT >
struct Result< Element< I, ArrayT >>
{ using type = tuple_element_t< I, typename Result< ArrayT >::type >; }; 

//////////////
/// Scope ///
////////////
///
/// Container for values of variables
///
/// forward decl
template< variable... Vars >
struct Scope;

template< typename T >
struct IsScope: integral_constant< bool, false > { };

template< typename... Vars >
struct IsScope< Scope< Vars... >>: integral_constant< bool, true > { };

template< typename T >
constexpr bool is_scope_v = IsScope< T >::value;

// scope contains details
namespace detail {
template< size_t I, typename ScopeT >
struct ScopeContainsVar;

template< size_t I, typename ScopeT >
requires( not is_scope_v< ScopeT >)
struct ScopeContainsVar< I, ScopeT >: integral_constant< bool, false > { };

template< size_t I, typename ScopeT >
requires( is_scope_v< ScopeT >)
struct ScopeContainsVar< I, ScopeT >: integral_constant< bool,
    ScopeT::template has_value_v< Var< I, any >>> { };
} // namespace detail

template< size_t I, typename ScopeT >
constexpr bool scope_contains_variable_v = 
    detail::ScopeContainsVar< I, ScopeT >::value;

template< typename T, size_t I >
concept scope_containing = scope_contains_variable_v< I, T >;

/// @brief standard mechanism to set a scoped variable value
template< variable Var, typename ScopeT, typename U >
constexpr void 
set_value( ScopeT& scope, U const& value )
{ scope.template set_value< Var >( value ); }

/// @brief standard mechanism to get a scoped variable value
template< variable Var, typename ScopeT >
constexpr typename Var::value_type 
get_value( ScopeT const& scope )
{ scope.template get_value< Var >(); }

/// @brief standard mechanism to get a scoped variable name
//template< variable Var, typename ScopeT >
//constexpr string get_name( ScopeT const& scope )
//{ scope.template get_name< Var >(); }

///////////////////////
/// Set Expression ///
/////////////////////
///
/// @brief an operation which sets the value of a variable
/// in a given scope
template< variable Var >
struct SetVarValue
{
    using variable_type = Var;
    using value_type = variable_type::value_type;
    using arguments_tuple = tuple< variable_type >;

    // setting a variable does not result in a value, unlike in C.
    // this effectively forbids composing a set expression in any other 
    // expressions
    using result_type = void;

    constexpr value_type const& value() const
    { return _value; }

    constexpr SetVarValue( Var const& var, value_type const& value ):
        _var{ var }, _value{ value }
    { }

private:
    Var _var;
    value_type _value;
};

/// @brief helper function to construct SetVarValue expressions
template< variable Var >
constexpr SetVarValue< Var > 
set_variable( typename Var::value_type const& value, Var var = {} )
{ return { var, value }; }

/// @brief type of a variable given the index, order, and base type
template< size_t I, typename T, size_t Order >
struct VarType
{ using type = Var< I, typename VarType< I, T, Order - 1 >::type >; };

template< size_t I, typename T >
struct VarType< I, T, 0 >
{ using type = T; };

////////////////////////
/// Var Declaration ///
//////////////////////
///
/// @brief the declaration of a variable in a delcare_variables function
/// @tparam T the type of the variable
///
template< typename T, size_t Order = 1 >
struct VarDeclaration
{ 
    using value_type = T;
    static constexpr size_t order = Order;

    template< size_t I >
    using variable_type = VarType< I, value_type, order >::type;

    constexpr string const& name() const
    { return _name; }

    string _name = "var"; 
};

/// @brief primary way to declare a variable inside a declare_variables expression
/// @tparam T the type of this variable
/// @param name the name of this variable
/// @return a declaration of a variable
template< typename T, size_t Order = 1 >
constexpr VarDeclaration< T, Order > var( string name = "var" )
{ return { name }; }

template< size_t Start, typename... Ts >
struct SequentialVars
{ 
    template< typename Seq >
    struct Helper;

    template< size_t... Is >
    struct Helper< seq< Is... >>
    { using type = tuple< Var< Start + Is, Ts...[ Is ]>... >; };

    using type = Helper< make_seq< sizeof...( Ts )>>::type; 
};

template< size_t Start, typename... Ts >
using sequential_variables_t = SequentialVars< Start, Ts... >::type;


} // namespace expressions

#endif

