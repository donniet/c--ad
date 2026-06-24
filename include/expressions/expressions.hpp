////////////////////////////
/// Expressions Library ///
//////////////////////////
///
/// Lazily evaluated arithmetic expressions with autodifferentiation.
///
/// # Concepts
/// - expression< T >: must be true for types that can be composed into
///   expressions in this library.
/// - compound_expression< T >: is an expression with arguments
/// - variable< V >: is a Variable< I, T > expression placeholder
/// - derivation< D >: a manipulator that obeys the product and chain rules
///   of differentiation.
///
/// # Base Types
/// - Constant< value > is a template-aware constant expression
/// - StaticValue< T > is a typed expression with an unchanging value
/// - Variable< I, T > is a placeholder uniquely identified by the size_t 
///   template argument I exposed as Variable< I, T >::index
/// - Scope< Vars... > is a tuple-like object that stores names and values
///   of Variable< I, T > placeholder types and acts as a manipulator
///   such that the application of a Scope<> to an expression evaluates the 
///   placeholders against the scoped values.
/// - Arguments< Op, Args... > is the base class for compound expressions
/// - Scope< Vars... > is an example of a scope-type class, having 
///   get_value< VarT >() and set_value< VarT >( ValueT ) methods and an
///   invocation operator `ScopeT::operator ()( Exprs... )` that evaluates
///   the expressions against the scoped values.
/// - Solver: `Solver< ExprT >{ expr }( ScopeT, Params... )` is the template
///   for all solver classes. A solver class must accept an ExprT instance as a 
///   construction parameter, and overload the invocation operator() to accept
///   a scope-type object and may accept an aribitrary number of ...Params.  
///
/// # Key Operations
/// - Substitution: `new_expr = expr( args... ); // ExprU ExprT::operator ()( Subs... )`
///   Overloaded by Arguments< Op, Args... > which is inherited by all expressions. 
///   ...Subs are substituted for the dependent variables and a new expression is 
///   returned
/// - Manipulation: `expr | manipulator; // auto operator |( ExprT, ManipulatorT )`
///   applies a Manipulator to an expression. A ScopeT is a type of manipulator.
///   Manipulation by a scope is a dual-recursion between this operator and 
///   `ScopeT::operator ()( Exprs... )` until the expression is parsed.
/// - Evaluation: `scope( exprs... ); // ScopeT::operator ()( Exprs... )` 
///   evaluates the ...Exprs expressions against the scope_type instance.  The 
///   default implementation uses the expressions::operator |( ExprT, ScopeT ) 
///   operators, creating a dual recursion that parses the expression.  The 
///   ScopeT class must, therefore, only directly handle the leaves of the 
///   expression (Constant, StaticValue, and Variable types).  
/// - Solving: `expr | solve_for( vars... )[ Params... ];` returns a scope-type object
///   which, when invoked against `expr` results in a true value. 
///
/// # Expression Type Requirements
///   (i) Expressions MUST be literal types.
///  (ii) Compound expressions MUST be a tuple-like object of their arguments
/// (iii) Compound expressions MUST be constructible from a parameter pack of it's
///       arguments.
///  (iv) Expressions MUST declare a result_type
///   (v) Expressions MUST implement a static, generically typed, constexpr value
///       method that calculates the result of the expression given the arguments
///   
/// # Example of an Expression Class
/// ```
/// template< typename... Args >
/// requires( sizeof...( Args ) >= 2 )
/// class Sum: Arguments< Sum, Args... > {          // requirement (ii) 
/// public:
///     using result_type = Args...[ 0 ];           // requirement (iv)
///
///     static constexpr auto value( auto... args ) // requirement (v)
///     { return ( args + ... ); }                  // 
///
///     constexpr Sum() = default;                  // requirement (i)
///     constexpr Sum( Sum const& ) = default;      // requirement (i)
///     constexpr Sum( Args&&... args ):            // requirement (iii)
///         Arguments< Sum, Args... >{ args } { }   //
/// };
/// ```
/// - The Arguments base template is tuple-like object of the ...Args
/// - Arguments imbues parent classes with substitution 
///   { operator()( subs&&... ); } and manipulator application 
///   { apply( manipulator_type& ); } methods.
///
///
#ifndef __EXPRESSIONS_EXPRESSIONS_HPP__
#define __EXPRESSIONS_EXPRESSIONS_HPP__

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

using namespace tensors;

///////////////////////////////////
/// Bootstrapping: Expressions ///
/////////////////////////////////
///
/// Forward declaration, concept, and traits to identify an expression class
///
namespace detail {

/// @brief base class used to identify an expression
///
struct ExpressionTag { };
} // namespace detail

/// @brief trait to identify a type as an expression type
/// @tparam T is the type to be checked
template< typename T >
struct IsExpression: integral_constant< bool, 
    std::is_base_of_v< detail::ExpressionTag, T >> { };

template< typename... Ts >
struct IsExpression< tuple< Ts... >>: integral_constant< bool,
    true and ( IsExpression< Ts >::value and ... )> { };

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
concept compound_expression = expression< T > and requires( T )
{ typename T::arguments_tuple; };

/// @brief type erased expression container
/// TODO: build a stack representation of the expression
template< typename ResultT >
struct Expression: detail::ExpressionTag
{
    using result_type = ResultT;

    template< typename ExprT >
    requires( std::is_same_v< ResultT, typename ExprT::result_type > )
    constexpr Expression( ExprT const& expr );

    constexpr Expression() = default;

private:
};

////////////////////
/// StaticValue ///
//////////////////
/// 
/// Class to hold a value considered to be unchanging in an expression
///
/// @brief wrapper to turn any type into an expression
/// @tparam T the wrapped type
///
template< typename T >
requires( not is_expression_v< T > ) // not sure if we need this.. meta expressions?
struct StaticValue: detail::ExpressionTag
{ 
    using value_type = T;
    using result_type = value_type;

    // casting to and from an expression should be explicit
    explicit constexpr operator value_type() const
    { return _value; } 

    constexpr value_type get_value() const
    { return _value; }

    constexpr StaticValue(): _value{} { }
    constexpr StaticValue( value_type const& other ): _value{ other } { }
    constexpr StaticValue( StaticValue const& ) = default;
    constexpr StaticValue( StaticValue&& ) = default;

private:
    value_type _value;
};

/// @brief helper to create expressions with unchanging values
/// @tparam T the type of this expression
/// @param value the value of this expression
/// @return returns an expression that will always evaluate to value
///
template< typename T >
requires( not is_expression_v< T > )
constexpr StaticValue< T > static_expr( T const& value )
{ return StaticValue< T >{ value }; }

/////////////////
/// Constant ///
///////////////
///
/// Wraps a compile-time constant as an expression. This allows
/// for expression sipmlification at compile-time.
///
/// @brief a compile-time constant
/// @tparam T the type of the constant
/// @tparam Value of the constant
///
template< auto Value >
struct Constant: detail::ExpressionTag
{
    using value_type = std::remove_cv_t< decltype( Value )>;
    using result_type = value_type;

    static constexpr value_type value = Value;

    constexpr operator value_type() const
    { return value; }

    constexpr Constant() = default;
};

template< auto Value >
using constant = Constant< Value >;

// NOTE: not sure about the constants
constexpr constant< 0 > constant_zero = constant< 0 >{};
constexpr constant< 1 > constant_one = constant< 1 >{};
constexpr constant< true > constant_true = constant< true >{};
constexpr constant< false > constant_false = constant< false >{};

/////////////////////////////////////////////
/// Variable and Substitution Declaraion ///
///////////////////////////////////////////
/// 
/// A typed placeholder in an expression
///
/// @brief Placeholder for a value of type T.  Two variables are 
/// equivalent if their identifiers I are the same
template< size_t I, typename T >
struct Variable;

/// a trait to extract the variable ID during bootstrapping
template< typename Var >
struct VariableId;

template< size_t I, typename T >
struct VariableId< Variable< I, T >>: integral_constant< size_t, I > { };

template< typename Var >
static constexpr size_t variable_id_v = VariableId< Var >::id;

/// @brief forward declaration of Substitution expression
template< typename ExprT, typename... Subs >
struct Substitution;

// forward declaration required for Variable<...>::operator ()
// Worker class for Substitution Expressions
template< typename ExprT, typename... Subs >
struct Substituter;

// we have to bootstrap Substitutions as expressions
template< typename ExprT, typename... Subs >
struct IsExpression< Substitution< ExprT, Subs... >>: std::true_type { };

// traits to handle substitutions in our bootstraping
template< typename Sub >
struct SubstitutionFormula;

template< typename ExprT, typename... Subs >
struct SubstitutionFormula< Substitution< ExprT, Subs... >>
{ 
    using type = ExprT;
    static constexpr type value( Substitution< ExprT, Subs... > const& sub )
    { return std::get< 0 >( sub ); }
};

template< size_t I, typename Sub >
struct SubstitutionSub;

template< size_t I, typename ExprT, typename... Subs >
struct SubstitutionSub< I, Substitution< ExprT, Subs... >>
{
    using type = Subs...[ I ];
    static constexpr type value( Substitution< ExprT, Subs... > const& sub )
    { return std::get< I + 1 >( sub ); }
};
// represents the Ith variable/substitution match 
template< size_t I, typename SubT >
struct SubstitutionMatch;

/// Variable Order
template< typename >
struct VariableOrder: integral_constant< size_t, 0 > { };

template< size_t I, typename T >
struct VariableOrder< Variable< I, T >>: integral_constant< size_t, 
    1 + VariableOrder< T >::value > { };

template< typename... Ts >
struct VariableOrder< tuple< Ts... >> {
private:
    static constexpr size_t tuple_size = sizeof...( Ts );
    typedef make_seq< tuple_size > for_elements;

    template< typename Seq >
    struct Helper;

    template< size_t I, size_t... Is >
    struct Helper< seq< I, Is... >> {
    private:
        static constexpr size_t first_order = 
            VariableOrder< Ts...[ I ]>::value;
        static constexpr size_t rest_order = Helper< seq< Is... >>::value;
    public:
        static constexpr size_t value = 
            ( first_order >= rest_order )? first_order : rest_order;
    };

    template< >
    struct Helper< seq< >>
    { static constexpr size_t value = 0; };

public:
    static constexpr size_t value = Helper< for_elements >::value;
};

template< compound_expression ExprT >
struct VariableOrder< ExprT >: 
    VariableOrder< typename ExprT::arguments_tuple > { };

template< typename V >
constexpr size_t variable_order_v = VariableOrder< V >::value;

// we need trait for substitution since it requires a bespoke means of 
// calculating the dependent variables
template< typename T >
struct IsSubstitutionExpression: std::false_type { };

template< typename ExprT, typename... Subs >
struct IsSubstitutionExpression< Substitution< ExprT, Subs... >>:
    std::integral_constant< bool, true > { };

template< typename T >
constexpr bool is_substitution_expression_v = 
    IsSubstitutionExpression< T >::value;

/// forward delaration
template< typename ExprT, typename... Args >
constexpr typename Substituter< ExprT, Args... >::type 
substitute( ExprT expr, Args... args );


/// @brief trait to identify variables
/// @tparam T the type to be tested
///
template< typename T >
struct IsVariable: std::false_type { };

template< size_t I, typename T >
struct IsVariable< Variable< I, T >>: std::integral_constant< bool, true > { };

template< typename T >
constexpr bool is_variable_v = IsVariable< T >::value;

template< typename T >
concept variable = is_variable_v< T >;

template< typename Var, typename T >
concept variable_of = is_variable_v< Var > and 
    is_same_v< typename Var::value_type, T >;

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
struct ScopeContainsVariable;

template< size_t I, typename ScopeT >
requires( not is_scope_v< ScopeT >)
struct ScopeContainsVariable< I, ScopeT >: integral_constant< bool, false > { };

template< size_t I, typename ScopeT >
requires( is_scope_v< ScopeT >)
struct ScopeContainsVariable< I, ScopeT >: integral_constant< bool,
    ScopeT::template has_value_v< Variable< I, any >>> { };
} // namespace detail

template< size_t I, typename ScopeT >
constexpr bool scope_contains_variable_v = 
    detail::ScopeContainsVariable< I, ScopeT >::value;

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
struct SetVariableValue: detail::ExpressionTag
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

    constexpr SetVariableValue( Var const& var, value_type const& value ):
        _var{ var }, _value{ value }
    { }

private:
    Var _var;
    value_type _value;
};

/// @brief helper function to construct SetVariableValue expressions
template< variable Var >
constexpr SetVariableValue< Var > 
set_variable( typename Var::value_type const& value, Var var = {} )
{ return { var, value }; }

/////////////////
/// Variable ///
///////////////
///
/// @brief a placeholder in an expression whose value can change
/// @tparam I is the id in the declared variables to this variable
/// @tparam T is the type of this variable
///
/// Two variables with the same identifier I are considered equal and MUST
/// have the same value_type T. The identifier I is also used to sort a
/// list of free variables when substituting.
///
/// The value_type T must a literal type and may be an expression type, in 
/// which case the result_type of the variable will be the result_t< T >. 
/// If the value_type is not an expression, then any substitution into the
/// variable must only match result_types with the type T.  If the value_type
/// is an expression itself then the value being substituted must also be
/// substitutable into the value_type.
///
/// is_valid_substitution< Variable< I, T >, ExprT >:
/// - is_convertible_v< result_t< ExprT >, result_t< T >> and
/// - 
///
template< size_t I, typename T >
struct Variable: detail::ExpressionTag
{ 
    using value_type = T;
    using result_type = value_type;
    static constexpr size_t id = I;

private:
    using this_type = Variable< id, value_type >;

public:
    /// @brief operator= is overriden to construct a SetVariableValue 
    /// expression
    /// HACK: this is not typical of C++ classes and may cause problems
    constexpr SetVariableValue< Variable > 
    operator=( value_type const& other ) const
    { return { *this, other }; }

    constexpr string const& name() const 
    { return _name; }

    constexpr void set_name( string const& new_name )
    { _name = new_name; }

    /// @brief substitution operator. Substitution into a variable creates a
    /// Substitution expression
    ///
    /// // x is a second-order variable
    /// Variable< 0, Variable< 0, double >> x;
    /// Variable< 1, unsigned long > n;
    ///
    /// auto fibbonacci = (
    ///     x(0) == 0 and x(1) == 1 and 
    ///     x(n) == x(n-1) + x(n-2) );
    ///
    /// assert( fibbonacci | solve_for( x(5) ) == 5 );
    /// // x(5) == x(4)                             + x(3)
    /// //      == x(3)               + x(2)        + x(2) + x(1)
    /// //      == x(2)        + x(1) + x(1) + x(0) + x(1) + x(0) + 1
    /// //      == x(1) + x(0) + 1    + 1    + 0    + 1    + 0    + 1
    /// //      == 1    + 0    + 1    + 1    + 0    + 1    + 0    + 1
    /// //      == 5
    ///
    template< typename... Subs >
    //constexpr Substitution< Variable< I, 
    //    Substitution< Variable< I, T >, Subs... >>,
    //        Subs... >
    //constexpr Substitution< Variable< I, Variable< I, T >>, Subs... >
    //constexpr Substitution< Variable< I, T >, Subs... >
    constexpr Variable< I, Substitution< Variable< I, T >, Subs... >>
    operator ()( Subs... subs ) const;
    //{ return {{ _name }, subs... }; }

    constexpr Variable( string const& name = "var" ): _name{ name } { };
    constexpr Variable( Variable const& ) = default; 
    constexpr Variable( Variable&& ) = default;

private:
    string _name;
};

///////////////////////
/// Variable Order ///
/////////////////////
/// 
/// Variables that represent other variables are higher-order:
///
///     Variable< 0, Variable< 0, float >> x; // second order variable that results in float
///
/// NOTE: I don't know if the IDs should match or not.  Right now I'm allowing them not to match
///
/// A non-variable is a variable of order 0.
/// A variable of a non-variable is order 1
///

static_assert( variable_order_v< int > == 0 );
static_assert( variable_order_v< Variable< 0, int >> == 1 );
static_assert( variable_order_v< Variable< 0, Variable< 0, int >>> == 2 );
static_assert( variable_order_v< tuple< Variable< 0, int >, Variable< 0, Variable< 0, int >>>> == 2 );

////////////////////
/// Result Type ///
//////////////////
/// 
/// Trait to determine the result_type of an expression type
///
namespace detail {

/// @brief Trait for the result type of an expression
/// @tparam T the type to be evaluated
///
template< typename T >
struct Result;
//{ using type = T; };

/// @brief Trait for the result type of an expression
/// @tparam T the type of the expression
///
template< typename T >
requires( is_expression_v< T > and not is_variable_v< T > )
struct Result< T >
{ using type = T::result_type; };

// handle higher order variables
template< typename T >
requires( is_variable_v< T > )
struct Result< T >: Result< typename T::value_type > { };

template< typename... Ts >
struct Result< tuple< Ts... >>
{ using type = tuple< typename Result< Ts >::type... >; };

template< shape S, typename... Ts >
struct Result< Tensor< S, Ts... >>
{ using type = Tensor< S, typename Result< Ts >::type... >; };

/// @brief bootstrap substitutions as expressions
template< typename ExprT, typename... Subs >
struct Result< Substitution< ExprT, Subs... >>
{ using type = Result< ExprT >::type; };

/// @brief evaluator for non-expression types
/// @tparam T the type of the non-expression
///
template< typename T >
requires( not is_expression_v< T > and not is_tensor_v< T > and not is_tuple_v< T > )
struct Result< T >
{ using type = T; };

} // namespace detail

/// @brief trait for the result_type of an expression
/// @tparam T the tested type
///
template< typename T >
using result_t = detail::Result< T >::type;

////////////////////////
/// Variable Traits ///
//////////////////////
///
template< typename Var >
struct variable_traits: integral_constant< bool, false > { };

template< size_t I, typename T >
struct variable_traits< Variable< I, T >>: integral_constant< bool, true > 
{
    using value_type = T;
    using result_type = result_t< value_type >;
    static constexpr size_t id = I;
    static constexpr size_t order = variable_order_v< Variable< I, T >>; 
    using variable_type = Variable< id, value_type >;
    static constexpr variable_type variable() { return {}; }
};

////////////////////////////
/// Dependent Variables ///
//////////////////////////
///
/// A specialized tuple-like type-set class for uniquely referencing the 
/// dependent variables in an expression.  The variables must be unique and 
/// sorted by their ID in the parameter pack of this class. 
///
///
///
/// Merging two unique_variables sets together keeps the resulting class'
/// template parameters in unique sorted order and is the only way new classes
/// of this template should be defined.
template< variable... >
class unique_variables;
/// 
/// Case: Empty Set
///
template< >
class unique_variables< > {
public:
    static constexpr size_t size = 0;
    using scope_type = Scope< >;
    using variables_tuple = tuple< >;

    static constexpr variables_tuple as_tuple() 
    { return { }; }

    constexpr operator variables_tuple() const
    { return as_tuple(); }

    template< size_t I >
    using element_t = std::tuple_element_t< I, variables_tuple >;

    constexpr scope_type make_scope() const;

    template< variable V >
    static consteval bool contains( V = {} )
    { return false; }

    constexpr unique_variables() = default;
    explicit constexpr unique_variables( tuple< >&& )
    { }
};
///
/// Case: One or More Dependent Variables
///
template< variable First, variable... Rest >
requires( is_sorted_unique_seq_v< seq< variable_traits< First >::id, 
    variable_traits< Rest >::id... >> )
class unique_variables< First, Rest... >: unique_variables< Rest... > {
public:
    static constexpr size_t size = 1 + sizeof...( Rest );
    using scope_type = Scope< First, Rest... >;
    using values_tuple = tuple< typename variable_traits< First >::value_type,
        typename variable_traits< Rest >::value_type... >;
    using first_type = First;
    using last_type = std::tuple_element_t< sizeof...( Rest ), 
        tuple< First, Rest... >>;
    using variables_tuple = tuple< first_type, Rest... >;

    // returns this unique, sorted set of variables as a tuple
    constexpr variables_tuple as_tuple() const
    { return std::tuple_cat( tuple< first_type >{ first() },
        unique_variables< Rest... >::as_tuple() ); }

    // implicit conversion of this class into an std::tuple
    constexpr operator variables_tuple() const
    { return as_tuple(); }

    // Ith dependent variable type
    template< size_t I >
    using element_t = std::tuple_element_t< I, variables_tuple >;

    // Ith variable in this collection
    template< size_t I >
    constexpr element_t< I >
    at() { return std::get< I >( as_tuple() ); }

    // Create the minimal scope that an expression with these dependent variables
    // can be executed against.
    constexpr scope_type make_scope() const;

    // Checks if a scope contains the values necessary to evaluate an expression
    // with these dependent variables
    template< variable... Vars >
    static consteval bool is_valid_scope( Scope< Vars... > scope = {} )
    { return unique_variables< Rest... >::is_valid_scope( scope ) and
        (( variable_traits< First >::id == variable_traits< Vars >::id ) or ... ); }

    // Does this set contain a particular variable?
    template< variable V >
    static consteval bool contains( V v = {} )
    { 
        static constexpr size_t vid = variable_traits< V >::id;

        if( variable_traits< first_type >::id == vid )
            return true;

        return unique_variables< Rest... >::contains( v );
    }

    // The first variable in this set
    constexpr first_type const& first() const
    { return _first; } 

    // The last variable in this set
    constexpr last_type const& last() const
    { return std::get< sizeof...( Rest )>( 
        operator tuple< first_type, Rest... >() ); }

    // The set of dependent variables except the first
    constexpr unique_variables< Rest... > rest() const
    { return *this; }

    constexpr unique_variables() = default;
    constexpr unique_variables( unique_variables const& ) = default;
    constexpr unique_variables( first_type&& first, Rest&&... rest ):
        unique_variables< Rest... >{ std::forward( rest )... }, 
            _first{ first } { }

private:
    // storage for the variable name
    first_type _first;
};
} // namespace expressions

namespace std {
/// @brief specialization of std::tuple_size
template< expressions::variable... Vars >
struct tuple_size< expressions::unique_variables< Vars... >>:
    integral_constant< size_t, sizeof...( Vars )> { };

/// @brief specialization of std::tuple_element_t for unique_variables
template< size_t I, expressions::variable... Vars >
struct tuple_element< I, expressions::unique_variables< Vars... >>
{ using type = Vars...[ I ]; };

template< size_t I, expressions::variable... Vars >
struct tuple_element< I, const expressions::unique_variables< Vars... >>
{ using type = add_const< Vars...[ I ]>::type; };

template< size_t I, expressions::variable... Vars >
struct tuple_element< I, volatile expressions::unique_variables< Vars... >>
{ using type = add_volatile< Vars...[ I ]>::type; };

template< size_t I, expressions::variable... Vars >
struct tuple_element< I, const volatile expressions::unique_variables< Vars... >>
{ using type = add_cv< Vars...[ I ]>::type; };

/// @brief sepcialization of std::get for unique_variables
//template< size_t I, expressions::variable... Vars >
//constexpr tuple_element_t< I, unique_variables< Vars... >> 
//get( unique_variables< Vars... >& vars )
//{ return vars.template at< I >(); }

/// @brief sepcialization of std::get for unique_variables
template< size_t I, expressions::variable... Vars >
constexpr tuple_element_t< I, expressions::unique_variables< Vars... >> 
get( expressions::unique_variables< Vars... > const& vars )
{ return vars.template at< I >(); }

} // namespace std
namespace expressions {

/////////////////////////////////////////
/// Merging Sets of Unique Variables ///
///////////////////////////////////////
///
namespace detail {
template< typename VarsT, typename VarsU, variable... Merged >
struct MergeUniqueVars;

// Base case directly inherits from unique_variables
template< variable... Merged >
struct MergeUniqueVars< unique_variables< >, unique_variables< >,
    Merged... >: unique_variables< Merged... >
{ 
    using type = unique_variables< Merged... >;

    constexpr MergeUniqueVars( unique_variables< >, unique_variables< >,
        Merged... merged ): type{ merged... } { } 
};

// Case: T is Ith variable
template< variable T, variable... Ts, variable U, variable... Us, 
    variable... Merged >
requires( isless( variable_traits< T >::id, variable_traits< U >::id ))
struct MergeUniqueVars< unique_variables< T, Ts... >,
    unique_variables< U, Us... >, Merged... >:
        MergeUniqueVars< unique_variables< Ts... >, 
            unique_variables< U, Us... >, Merged..., T >
{
    using base_type = MergeUniqueVars< unique_variables< Ts... >, 
        unique_variables< U, Us... >, Merged..., T >;

    constexpr MergeUniqueVars( unique_variables< T, Ts... > left,
        unique_variables< U, Us... > right, Merged... merged ):
            base_type{ left.rest(), right, merged..., left.first() } { }
};

// Case: U is the Ith variable
template< variable T, variable... Ts, variable U, variable... Us,
    variable... Merged >
requires( isgreater( variable_traits< T >::id, variable_traits< U >::id ))
struct MergeUniqueVars< unique_variables< T, Ts... >,
    unique_variables< U, Us... >, Merged... >:
        MergeUniqueVars< unique_variables< T, Ts... >, 
            unique_variables< Us... >, Merged..., U >
{
    using base_type = MergeUniqueVars< unique_variables< T, Ts... >, 
        unique_variables< Us... >, Merged..., U >;

    constexpr MergeUniqueVars( unique_variables< T, Ts... > left,
        unique_variables< U, Us... > right, Merged... merged ):
            base_type{ left, right.rest(), merged..., right.first() } { }
};

// Case: T and U are the same
template< variable T, variable... Ts, variable U, variable... Us,
    variable... Merged >
requires( variable_traits< T >::id == variable_traits< U >::id )
struct MergeUniqueVars< unique_variables< T, Ts... >,
    unique_variables< U, Us... >, Merged... >:
        MergeUniqueVars< unique_variables< Ts... >, 
            unique_variables< Us... >, Merged..., T >
{ 
    static_assert( is_same_v< typename variable_traits< T >::value_type,
        typename variable_traits< U >::value_type >, 
            "variables with the same id must have the same value_type" );

    using base_type = MergeUniqueVars< unique_variables< Ts... >, 
        unique_variables< Us... >, Merged..., T >;

    constexpr MergeUniqueVars( unique_variables< T, Ts... > left,
        unique_variables< U, Us... > right, Merged... merged ):
            base_type{ left.rest(), right.rest(), merged..., left.first() } { }
};

// Case: left is empty
template< variable U, variable... Us, variable... Merged >
struct MergeUniqueVars< unique_variables< >,
    unique_variables< U, Us... >, Merged... >:
        MergeUniqueVars< unique_variables< >, 
            unique_variables< Us... >, Merged..., U >
{
    using base_type = MergeUniqueVars< unique_variables< >, 
        unique_variables< Us... >, Merged..., U >;

    constexpr MergeUniqueVars( unique_variables< > left, 
        unique_variables< U, Us... > right, Merged... merged ):
            base_type{ left, right.rest(), merged..., right.first() } { }
};

// Case: right is empty
template< variable T, variable... Ts, variable... Merged >
struct MergeUniqueVars< unique_variables< T, Ts... >,
    unique_variables< >, Merged... >:
        MergeUniqueVars< unique_variables< Ts... >, 
            unique_variables< >, Merged..., T >
{
    using base_type = MergeUniqueVars< unique_variables< Ts... >, 
        unique_variables< >, Merged..., T >;

    constexpr MergeUniqueVars( unique_variables< T, Ts... > left, 
        unique_variables< > right, Merged... merged ):
            base_type{ left.rest(), right, merged..., left.first() } { }
};

template< typename... Ts >
struct MergeManyUniqueVars;

template< >
struct MergeManyUniqueVars< >
{ using type = unique_variables< >; };

template< typename... Vars >
struct MergeManyUniqueVars< unique_variables< Vars... >>
{ using type = unique_variables< Vars... >; };

template< typename First, typename... Rest >
requires( isgreater( sizeof...( Rest ), 0 ))
struct MergeManyUniqueVars< First, Rest... >
{ using type = MergeUniqueVars<
    First, typename MergeManyUniqueVars< Rest... >::type >::type; };

} // namespace detail

template< typename... Ts >
using merge_unique_variables_t = detail::MergeManyUniqueVars< Ts... >::type;

template< variable... Ts, variable... Us >
constexpr merge_unique_variables_t< unique_variables< Ts... >, 
    unique_variables< Us... >>
merge_unique_variables( unique_variables< Ts... > const& left, 
    unique_variables< Us... > const& right )
{ return detail::MergeUniqueVars< unique_variables< Ts... >, 
    unique_variables< Us... >>{ left, right }; }

namespace detail {
/// forward declaration of Substitution expression
/// Expression class for Substitution Expressions
///
/// NOTES: Substitutions are complex because checking for free variables inside
///        of a substitution expression requires a checking for free variables.
///
///
///
/// @brief Forward declaration of free_variables_t expression trait
template< typename ExprT >
struct FreeVariables;
///
template< typename ExprT >
using free_variables_t = FreeVariables< ExprT >::type;

/// @brief Forward Declaration of is_compatible_variable_substitutions_v
template< typename UniqueVariables, typename... Subs >
struct IsCompatibleVariableSubstitutions;
///
template< typename UniqueVariables, typename... Subs >
constexpr bool
is_compatible_variable_substitutions_v = 
    IsCompatibleVariableSubstitutions< UniqueVariables, Subs... >::value;

///
template< typename ExprT, typename... Subs >
constexpr bool
is_compatible_substitution_v = 
    is_compatible_variable_substitutions_v< 
        free_variables_t< ExprT >, Subs... >;
///
/// Case: Single Variable Expression
///
///       A single variable expression, regardless of variable order, is 
///       its own free variable
///
template< size_t I, typename ExprT >
struct FreeVariables< Variable< I, ExprT >>
{ using type = unique_variables< Variable< I, ExprT >>; };
///
/// Case: Non-Variable, Non-Compound Expression
///
///       No variables can be in this type of expression, so our result
///       is empty.
///
template< typename ExprT >
requires( not requires { typename ExprT::arguments_tuple; } and
    not variable< ExprT > )
struct FreeVariables< ExprT >
{ using type = unique_variables< >; };
///
/// Case: Tuple of Expressions
///
///       The free variables in a tuple of expressions is the merger of 
///       free variables from each expression
///
template< typename... Ts >
struct FreeVariables< tuple< Ts... >>
{ using type = merge_unique_variables_t< typename
    FreeVariables< Ts >::type... >; };
///
/// Case: Compound Expression (that's not a substitution)
///      
///       Free variables in a compound expression that is not a substitution
///       are the free variables in the arguments tuple of the compound
///       expression
///
template< typename ExprT >
requires( requires { typename ExprT::arguments_tuple; } and
    not is_substitution_expression_v< ExprT >)
struct FreeVariables< ExprT >:
    FreeVariables< typename ExprT::arguments_tuple > { };
///
/// Case: Second Order Substitution Expression
///
///       A second-order substitution expression is the result of substituting
///       into a free variable.  We stash the substitution expression, downgraded,
///       in the expression_type of variable I, and merge it with any free
///       free variables in the substitutions.
///
/// NOTE: substitution cannot be deemed compatible since second order variable I
///       remains free
///
template< size_t I, typename ExprT, typename... Subs >
struct FreeVariables< Substitution< Variable< I, Variable< I, ExprT >>, Subs... >>
{ 
    using substitution_expression = Substitution< Variable< I, ExprT >, Subs... >;
    using type = merge_unique_variables_t< 
        unique_variables< Variable< I, substitution_expression >>,
        typename FreeVariables< tuple< Subs... >>::type >;
};
///
/// Case: Empty First-order Substitution into a Single Variable
///
///       The single variable remains free since an empty substitution is 
///       idempotent
///
template< size_t I, typename ExprT >
requires( not variable< ExprT > ) // first order variable
struct FreeVariables< Substitution< Variable< I, ExprT >>>
{ using type = unique_variables< Variable< I, ExprT >>; };
///
/// Case: Substitution of a Single Expression into a Single, First-Order 
///       Variable
///
///       Any substitution expressions beyond the first are not instantiated,
///       and so the only free variables are those in the first substitution
///       expression.
///
template< size_t I, typename ExprT, typename FirstSub, typename... RestSubs >
requires( not variable< ExprT > )
struct FreeVariables< Substitution< Variable< I, ExprT >, FirstSub, RestSubs... >>:
    FreeVariables< FirstSub > { };
///
/// Case: Substitution into a non-variable, non-compound expression (ie:
///       static or constant)
///
///       non-variable, non-compound expressions have no free variables, 
///       and so substituting into them will not instantiate any of the
///       substitution expressions, and so there will be no free variables.
///
template< typename ExprT, typename... Subs >
requires( not requires { typename ExprT::arguments_tuple; } and 
    not variable< ExprT > )
struct FreeVariables< Substitution< ExprT, Subs... >>
{ using type = unique_variables< >; };
///
/// Case: Substituting into a Compound Expression
///
///       We must fetch the free variables in the compound expression 
///
template< typename ExprT, typename... Subs >
requires( requires { typename ExprT::arguments_tuple; } )
struct FreeVariables< Substitution< ExprT, Subs... >> {
private:
    using expression_type = ExprT;
    using expression_free_variables = FreeVariables< expression_type >::type;

    template< typename UniqueVars, typename... RemainingSubs >
    struct Helper;

    /// Case: One or more variables are matched by one or more substitutions
    ///
    template< typename FirstVar, typename... RestVars, 
        typename FirstSub, typename... RestSubs >
    struct Helper< unique_variables< FirstVar, RestVars... >, 
        FirstSub, RestSubs... >
    { 
        // NOTE: we include ...RestSubs to centralize logic for over-substituted 
        //       expressions in the FreeVariables specialization for variable 
        //       expressions
        using first_substitution_expression_type = 
            Substitution< FirstVar, FirstSub, RestSubs... >;

        using type = merge_unique_variables_t< 
            typename FreeVariables< first_substitution_expression_type >::type,
            typename Helper< unique_variables< RestVars... >, RestSubs... >::type >;
    };

    /// Case: One or more variables are unmatched
    ///
    template< typename First, typename... Rest >
    struct Helper< unique_variables< First, Rest... >>
    { using type = unique_variables< First, Rest... >; };

    /// Case: One or more substitution expressions do not match any variables
    ///       and so are never instantiated
    ///
    template< typename First, typename... Rest >
    struct Helper< unique_variables< >, First, Rest... >
    { using type = unique_variables< >; };

public:
    using type = Helper< expression_free_variables, Subs... >::type;
};

static_assert( std::is_same_v< typename 
    FreeVariables< Variable< 0, int >>::type, 
        unique_variables< Variable< 0, int >> >);
static_assert( std::is_same_v< typename FreeVariables< 
    Substitution< Variable< 0, int >, Variable< 1, int >>>::type, 
        unique_variables< Variable< 1, int >>> );
static_assert( std::is_same_v< typename FreeVariables<
    Substitution< Variable< 0, int >, Variable< 1, int >>>::type,
        unique_variables< Variable< 1, int >>> );

template< typename ExprT >
struct NextVariableId;

template< typename ExprT >
requires( not requires { typename FreeVariables< ExprT >; })
struct NextVariableId< ExprT >: integral_constant< size_t, 0 > { };

template< typename ExprT >
requires( FreeVariables< ExprT >::type::size == 0 )
struct NextVariableId< ExprT >: integral_constant< size_t, 0 > { };

template< typename ExprT >
requires( FreeVariables< ExprT >::type::size != 0 )
struct NextVariableId< ExprT >: integral_constant< size_t, 
    variable_traits< typename FreeVariables< ExprT >::type::last_type >::
        id + 1 > { };
} // namespace detail

/// @brief helper alias to identify free variables in an expression
///
/// We remove const/volatile because Variable<...> only store the variable name
template< typename ExprT >
using free_variables_t = detail::FreeVariables< std::remove_cv_t< ExprT >>::type;

template< typename ExprT >
constexpr free_variables_t< ExprT >
get_free_variables( ExprT const& expr )
{ return detail::FreeVariables< ExprT >::value( expr ); }

template< variable Var, typename ExprT >
constexpr bool depends_on_variable_v = free_variables_t< ExprT >::template 
    contains< Var >(); 

template< typename ExprT >
constexpr size_t next_variable_id_v = detail::NextVariableId< ExprT >::value;

namespace test {

static_assert( is_same_v< unique_variables< Variable< 0, int >>, 
    free_variables_t< Variable< 0, int >>> );
static_assert( is_same_v< unique_variables< Variable< 0, int >, Variable< 1, int >>, 
    free_variables_t< tuple< Variable< 0, int >, Variable< 1, int >>>> );
static_assert( is_same_v< unique_variables< Variable< 0, int >, Variable< 1, int >>, 
    free_variables_t< tuple< Variable< 1, int >, Variable< 0, int >>>> );
static_assert( is_same_v< unique_variables< Variable< 0, int >>, 
    free_variables_t< tuple< Variable< 0, int >, Variable< 0, int >>>> );
static_assert( is_same_v< unique_variables< Variable< 1, int >>, 
    free_variables_t< tuple< Variable< 1, int >, Variable< 1, int >>>> );

// tests for higher-order variables
//
// 1 free second order variable of type int
static_assert( is_same_v< unique_variables< Variable< 0, Variable< 0, int >>>,
    free_variables_t< Variable< 0, Variable< 0, int >>>> );
//
// 1 free second order variable from a tuple of variables
static_assert( is_same_v< unique_variables< Variable< 0, Variable< 0, int >>>,
    free_variables_t< tuple< Variable< 0, Variable< 0, int >>>>> );
//
// A constant will be substituted into an expression substituted into variable 0
static_assert( is_same_v< 
    unique_variables< Variable< 0, Substitution< Variable< 0, int >, Constant< 0 >>>>,
    free_variables_t< Substitution< Variable< 0, Variable< 0, int >>, Constant< 0 >>>> );
static_assert( is_same_v< 
    unique_variables< Variable< 0, Substitution< Variable< 0, int >, Variable< 1, int >>>, 
        Variable< 1, int >>,
    free_variables_t< Substitution< Variable< 0, Variable< 0, int >>, Variable< 1, int >>>> );

static_assert( next_variable_id_v< Variable< 0, int >> == 1 );
static_assert( next_variable_id_v< Variable< 1, int >> == 2 );
static_assert( next_variable_id_v< Variable< 2, Variable< 2, int >>> == 3 );

static_assert( next_variable_id_v< tuple< Variable< 0, int >, Variable< 1, int >>> == 2 );

} // namespace test

//////////////////////////
/// Predicate Matches ///
////////////////////////
///
/// @brief trait to reference a matched variable from an expression predicate
template< typename Var, typename ExprT >
struct match: std::tuple< Var, ExprT >
{
    using variable_type = Var;
    using expression_type = ExprT;

    constexpr variable_type const& var() const
    { return std::get< 0 >( *this ); }

    constexpr expression_type const& expression() const
    { return std::get< 1 >( *this ); }

    constexpr match() = default;
    constexpr match( match const& ) = default;
    constexpr match( variable_type const& var, expression_type const& expr ):
        std::tuple< Var, ExprT >{ var, expr } { }
};

struct no_match;

template< typename MatchT >
using match_variable_t = MatchT::variable_type;

template< typename MatchT >
using match_expression_t = MatchT::expression_type;

namespace detail {

template< typename MatchT >
struct IsMatch: integral_constant< bool, false > { };

template< typename Var, typename ExprT >
struct IsMatch< match< Var, ExprT >>: integral_constant< bool, true > { };

template< typename MatchT >
concept match = IsMatch< MatchT >::value;

// represents a Substitution<...> that has been matched against an expression
//
template< typename ExprT, match... Matches >
struct SubMatches
{
    using expression_type = ExprT;
    using matches_tuple = tuple< Matches... >;

    typedef make_seq< sizeof...( Matches )> for_matches;

    constexpr expression_type const& expression() const
    { return _expr; }

    constexpr matches_tuple const& matches() const
    { return _matches; }

private:

    template< typename Var, typename Seq >
    struct MatchHelper;

    template< typename Var >
    struct MatchHelper< Var, seq< >>
    { static_assert( false, "variable was not matched" ); };

    template< variable Var, size_t J, size_t... Js >
    requires( variable_id_v< Var > == 
        variable_id_v< typename std::tuple_element_t< J, matches_tuple >::variable_type >)
    struct MatchHelper< Var, seq< J, Js... >>
    { 
        using type = std::tuple_element_t< J, matches_tuple >::expression_type;
        static constexpr type value( matches_tuple const& matches )
        { return std::get< J >( matches ).expression(); }
    };

    template< variable Var, size_t J, size_t... Js >
    requires( Var::id != std::tuple_element_t< J, matches_tuple >::variable_type::id )
    struct MatchHelper< Var, seq< J, Js... >>:
        MatchHelper< Var, seq< Js... >> { };
 

public:
    template< typename Var >
    constexpr typename MatchHelper< Var, for_matches >::type const&
    sub_for( Var var = {} ) const
    { return MatchHelper< Var, for_matches >::value( var ); }

    constexpr SubMatches() = default;
    constexpr SubMatches( SubMatches const& ) = default;
    constexpr SubMatches( expression_type const& expr, Matches const&... matches ):
        _expr{ expr }, _matches{ matches... } { }

private:
    expression_type _expr;
    matches_tuple _matches;
};

} // namespace detail

/////////////////////////////
/// Variable Declaration ///
///////////////////////////
///
/// @brief the declaration of a variable in a delcare_variables function
/// @tparam T the type of the variable
///
template< typename T >
struct VariableDeclaration
{ 
    using value_type = T;

    constexpr string const& name() const
    { return _name; }

    string _name = "var"; 
};

/// @brief primary way to declare a variable inside a declare_variables expression
/// @tparam T the type of this variable
/// @param name the name of this variable
/// @return a declaration of a variable
template< typename T >
constexpr VariableDeclaration< T > var( string name = "var" )
{ return { name }; }

template< size_t Start, typename... Ts >
struct SequentialVariables
{ 
    template< typename Seq >
    struct Helper;

    template< size_t... Is >
    struct Helper< seq< Is... >>
    { using type = tuple< Variable< Start + Is, Ts...[ Is ]>... >; };

    using type = Helper< make_seq< sizeof...( Ts )>>::type; 
};

template< size_t Start, typename... Ts >
using sequential_variables_t = SequentialVariables< Start, Ts... >::type;

/////////////////////////////
/// Scope Implementation ///
///////////////////////////
///
/// @brief container and factory for Variables.  Scope is a manipulator and
/// application of scope via operator| evaluates dependent variables against
/// scoped values.
///
template< variable... Vars >
struct Scope: tuple< Vars... > 
{
    using values_tuple_type = 
        tuple< typename variable_traits< Vars >::value_type... >;
    using dirty_tuple_type = std::array< bool, sizeof...( Vars )>;
    using variables_tuple_type = tuple< Vars... >;
    static constexpr size_t size = sizeof...( Vars );

protected:
    template< size_t I, typename Seq >
    struct Helper;

    template< size_t I, size_t J, size_t... Js >
    requires( I == variable_traits< Vars...[ J ]>::id )
    struct Helper< I, seq< J, Js... >>
    { 
        static constexpr tuple_element_t< J, values_tuple_type > 
        get( values_tuple_type const& vals )
        { return std::get< J >( vals ); }

        static constexpr void 
        set( values_tuple_type& vals, dirty_tuple_type& flags, 
            tuple_element_t< J, values_tuple_type > const& val )
        { 
            std::get< J >( vals ) = val; 
            std::get< J >( flags ) = true;
        }

        static constexpr bool
        is_dirty( dirty_tuple_type const& flags )
        { return std::get< J >( flags ); }

        static constexpr void
        wash( dirty_tuple_type& flags )
        { std::get< J >( flags ) = false; }

        static constexpr bool has_value = true; 
    };

    template< size_t I, size_t J, size_t... Js >
    requires( I != variable_traits< Vars...[ J ]>::id )
    struct Helper< I, seq< J, Js... >>:
        Helper< I, seq< Js... >>
    { };

    template< size_t I >
    struct Helper< I, seq<>>
    { static constexpr bool has_value = false; };

    template< variable Var >
    using helper_for = Helper< variable_traits< Var >::id, make_seq< size >>;

    template< size_t... Js >
    constexpr void initialize_flags( seq< Js... > )
    {(( std::get< Js >( _flags ) = false ), ... ); }

public:
    /// @brief determines if the scope has a value for variable I
    template< variable Var >
    static constexpr bool 
    has_value_v = Helper< variable_traits< Var >::id, 
        make_seq< size >>::has_value;

    template< variable Var >
    static consteval bool 
    has_value( Var ) 
    { return has_value_v< Var >; }

    /// @brief retrieves the value of variable Var in this scope
    template< variable Var >
    constexpr typename Var::value_type 
    get_value( Var = {} ) const 
    { return helper_for< Var >::get( _values ); }

    /// @brief assigns other to the scoped value of Var
    template< variable Var >
    constexpr void 
    set_value( typename Var::value_type const& other, Var var = {} ) 
    { helper_for< Var >::set( _values, _flags, other ); }

    /// @brief has a variable's value been assigned by set_value?
    template< variable Var >
    constexpr bool
    is_dirty( Var = {} ) const
    { return helper_for< Var >::is_dirty( _flags ); }

    /// @brief force our flag to false for Var
    template< variable Var >
    constexpr void
    wash( Var = {} ) const
    { helper_for< Var >::wash( _flags ); }

    /// @brief take values and flags from another 
    template< typename ScopeU >
    constexpr void
    take_from( ScopeU const& other )
    {
        auto take_value_if_dirty = [&]< variable Var >( Var var ) constexpr 
        { if( other.is_dirty( var ))
                set_value( other.get_value( var ), var ); };

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
        {( take_value_if_dirty( Vars...[ Is ]{} ), ... ); }; 

        helper( make_seq< sizeof...( Vars )>{} );
    }

    /// @brief returns a tuple of scoped variables
    constexpr tuple< Vars... > variables() const
    { return { Vars{}... }; }

    /// @brief invocation against a constant will return the constant's value
    template< auto Value > 
    constexpr decltype( Value ) operator ()( Constant< Value > ) const
    { return Value; }

    /// @brief invocation against a static will return the static's value
    template< typename T >
    constexpr T operator ()( StaticValue< T > const& static_value ) const
    { return static_cast< T >( static_value ); }

    template< typename T >
    requires( not expression< T > )
    constexpr T 
    operator ()( T const& value ) const
    { return value; }

    // @brief sets the value of a variable in this scope
    template< variable Var >
    requires( has_value_v< Var >)
    constexpr void 
    operator ()( SetVariableValue< Var > const& setter )
    { set_value< Var >( setter.value() ); }

    // @brief invoking with a parameter list will evaluate
    // the comma operator on the invocation of each argument
    template< typename... Args >
    requires( is_greater( sizeof...( Args ), 1 ))
    constexpr auto 
    operator ()( Args const&... args )
    { return ( operator ()( args ), ... ); }

    // @brief invoking on a compound expression
    template< compound_expression ExprT >
    //requires( scope_contains_unique_variables_v< ExprT, Scope< Vars... >> )
    constexpr result_t< ExprT > 
    operator ()( ExprT const& expr ) const;

    /// @brief invocation against a scoped variable will return the
    /// scoped value
    template< variable Var >
    requires( scope_contains_variable_v< variable_traits< Var >::id, 
        Scope< Vars... >> )
    constexpr variable_traits< Var >::value_type 
    operator ()( Var const& var ) const 
    { return helper_for< Var >::get( _values ); }

    constexpr Scope& operator =( Scope const& other )
    {
        take_from( other );
        return *this;
    }

    explicit constexpr Scope( tuple< Vars... > const& vars ):
        tuple< Vars... >{ vars } 
    { initialize_flags( make_seq< size >{} ); }

    constexpr Scope( Vars&&... vars ) requires( is_greater( sizeof...( Vars ), 0 )): 
        tuple< Vars... >{ vars... } 
    { initialize_flags( make_seq< size >{} ); }

    constexpr Scope( Scope const& other ) = default;

    constexpr Scope(): tuple< Vars... >{}, _values{}
    { initialize_flags( make_seq< size >{} ); }

private:
    values_tuple_type _values;
    dirty_tuple_type _flags;
};

namespace detail {

template< expression ExprT, typename ScopeT >
requires( is_scope_v< ScopeT >)
struct ScopeContainsFreeVariables {
private:
    template< typename Deps >
    struct Helper;

    template< variable... Vars >
    struct Helper< unique_variables< Vars... >>: integral_constant< bool,
        ( ScopeContainsVariable< variable_traits< Vars >::id, ScopeT >::value 
            and ... )> { };

public:
    static constexpr bool value = 
        Helper< typename FreeVariables< ExprT >::type >::value;
};

} // namespace detail

template< expression ExprT, typename ScopeT >
constexpr bool scope_contains_free_variables_v = 
    detail::ScopeContainsFreeVariables< ExprT, ScopeT >::value;

/////////////////////////
/// Scope Comparison ///
///////////////////////
///
/// Utilities to compare a scope's breadth, values and flags
template< variable... VarsA, typename ScopeB >
consteval bool 
is_sub_scope( Scope< VarsA... > const&, ScopeB const& )
{ return ( ScopeB::template has_value_v< VarsA > and ... ); }

#ifndef NDEBUG
template< variable... Vars >
constexpr std::string ScopeString( Scope< Vars... > const& scope )
{
    std::string ret = "";
    ret += (( std::to_string( Vars::id ) + "==" + std::to_string( scope.get_value( Vars{} )) + "(" + std::to_string( scope.is_dirty( Vars{} )) + "), " ) + ... );
    return ret;
}
#endif // DEBUG

template< variable... VarsA, typename ScopeB >
constexpr bool 
compatible_scopes( Scope< VarsA... > const& left, 
    ScopeB const& right )
{
#ifndef NDEBUG
    std::string avars = ScopeString( left );
    std::string bvars = ScopeString( right );

    std::println( "checking scope compatibility:\n{}\n{}", avars, bvars ); 
    std::println( "subscopes: {} and {}", is_sub_scope( left, right ), is_sub_scope( right, left ));
#endif // DEBUG
    // scopes must store the same variables...
    if( not is_sub_scope( left, right ) or not is_sub_scope( right, left ))
        return false;

    // ... and if they are both flagged they must have the same value.
    return (( not ( left.is_dirty( VarsA{} ) and right.is_dirty( VarsA{} )) or 
        left.get_value( VarsA{} ) == right.get_value( VarsA{} )) and ... );
}

template< variable... VarsA, typename... OtherScopes >
requires( is_greater( sizeof...( OtherScopes ), 1 ))
constexpr bool 
compatible_scopes( Scope< VarsA... > const& left, 
    OtherScopes const&... others )
{ return ( compatible_scopes( left, others ) and ... ); }

template< variable... VarsA, typename... OtherScopes >
constexpr bool compatible_scopes( 
    tuple< Scope< VarsA... >, OtherScopes... > const& tup )
{ 
    auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr
    { return compatible_scopes( std::get< 0 >( tup ), 
        std::get< 1 + Is >( tup )... ); };

    return helper( make_seq< sizeof...( OtherScopes )>{} );
}

template< variable... VarsA, size_t Size >
constexpr bool compatible_scopes( 
    std::array< Scope< VarsA... >, Size > const& tup )
{
    auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr
    { return compatible_scopes( std::make_tuple( std::get< Is >( tup )... )); };

    return helper( make_seq< Size >{} );
}

/// @brief Constructs a new scope by adopting values of un-flaged variables
/// from the flagged values in other scopes
///
/// @pre assumes scopes are compatible
/// @returns A scope compatible with left and right whose variables are dirty
/// if and only if left or right's variable was dirty
///
template< variable... VarsA, typename ScopeB >
constexpr Scope< VarsA... > 
merge_compatible_scopes( Scope< VarsA... > const& left, ScopeB const& right )
{
    Scope< VarsA... > scope;
#ifndef NDEBUG
    if( not compatible_scopes( left, right ))
        throw std::logic_error( "incompatible scopes cannot be merged." );
#endif
    auto set_variable_value = [&]< variable Var >( Var var ) constexpr 
    {
        // if the right scope is dirty then it's value must be equal
        // to left by the compatibility assumption, so we use it.
        if( right.is_dirty( var ))
            scope.set_value( right.get_value( var ), var );
        
        // otherwise if the left scope is dirty we use it's value
        else if( left.is_dirty( var ))
            scope.set_value( left.get_value( var ), var );

        // neither scope contains a dirty Var so we do nothing
        // so that the returned scope also has a clean Var and is
        // therefore compatible with left and right.
    };

    auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
    {( set_variable_value( VarsA...[ Is ]{} ), ... ); }; 

    helper( make_seq< sizeof...( VarsA )>{} );
    return scope;
}

template< variable... VarsA >
constexpr Scope< VarsA... > const&
merge_compatible_scopes( Scope< VarsA... > const& only )
{ return only; }

template< variable... VarsA, typename... OtherScopes >
requires( is_greater( sizeof...( OtherScopes ), 1 ))
constexpr Scope< VarsA... >
merge_compatible_scopes( Scope< VarsA... > const& left, 
    OtherScopes const&... others )
{
    Scope< VarsA... > scope = left;
    return (( scope = merge_compatible_scopes( scope, others )), ... );
}

template< variable... VarsA, typename... OtherScopes >
constexpr Scope< VarsA... >
merge_compatible_scopes( 
    std::tuple< Scope< VarsA... >, OtherScopes... > const& tup )
{
    auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
    { return merge_compatible_scopes( std::get< 0 >( tup ),
        std::get< 1 + Is >( tup )... ); };

    return helper( make_seq< sizeof...( OtherScopes )>{} );
}

template< variable... VarsA, size_t Size >
constexpr Scope< VarsA... >
merge_compatible_scopes(
    std::array< Scope< VarsA... >, Size > const& scopes )
{
    auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr
    { return merge_compatible_scopes( std::make_tuple( 
        std::get< Is >( scopes )... )); };

    return helper( make_seq< Size >{} );
};

////////////////////
/// SimpleScope ///
//////////////////
///
template< typename Seq, typename... Values >
struct SimpleScopeHelper;

template< size_t... Is, typename... Values >
struct SimpleScopeHelper< seq< Is... >, Values... >
{ 
    using type = Scope< Variable< Is, Values...[ Is ]>... >;

    template< typename... Names >
    requires( sizeof...( Names ) == sizeof...( Is ))
    static constexpr type value( Names const&... names )
    { return { Variable< Is, Values...[ Is ]>{ names...[ Is ] }... }; }
};

template< typename... Values >
constexpr typename SimpleScopeHelper< make_seq< sizeof...( Values )>, Values... >::type
simple_scope( Values const&... values )
{ return SimpleScopeHelper< make_seq< sizeof...( Values )>, Values... >::value( values... ); }

/// @brief method to declare a set of variables to be used in expressions
/// @tparam ...Decls 
/// @param ...decls 
/// @return 
template< typename... Decls >
typename SimpleScopeHelper< make_seq< sizeof...( Decls )>, 
    typename Decls::value_type... >::type
constexpr declare_variables( Decls... decls )
{ return SimpleScopeHelper< make_seq< sizeof...( Decls )>, typename
    Decls::value_type... >::value( decls.name()... ); }


////////////////////////////
/// Dependent Variables ///
//////////////////////////
///
/// A specialized tuple-like type-set class for uniquely referencing the 
/// dependent variables in an expression.  The variables must be unique and 
/// sorted by their ID in the parameter pack of this class. 
///
/// Merging two dependent_variables sets together keeps the resulting class'
/// template parameters in unique sorted order and is the only way new classes
/// of this template should be defined.
template< variable... >
class dependent_variables;
/// 
/// Case: Empty Set
///
template< >
class dependent_variables< > {
public:
    static constexpr size_t size = 0;
    using scope_type = Scope< >;
    using variables_tuple = tuple< >;

    static constexpr variables_tuple as_tuple() 
    { return {}; }

    constexpr operator variables_tuple() const
    { return as_tuple(); }

    constexpr scope_type make_scope() const
    { return { }; }

    template< variable V >
    static consteval bool contains( V = {} )
    { return false; }

    constexpr dependent_variables() = default;
    explicit constexpr dependent_variables( tuple< >&& )
    { }
};
///
/// Case: One or More Dependent Variables
///
template< variable First, variable... Rest >
requires( is_sorted_unique_seq_v< seq< variable_traits< First >::id, 
    variable_traits< Rest >::id... >> )
class dependent_variables< First, Rest... >: dependent_variables< Rest... > {
public:
    static constexpr size_t size = 1 + sizeof...( Rest );
    using scope_type = Scope< First, Rest... >;
    using values_tuple = tuple< typename variable_traits< First >::value_type,
        typename variable_traits< Rest >::value_type... >;
    using first_type = First;
    using last_type = std::tuple_element_t< sizeof...( Rest ), 
        tuple< First, Rest... >>;
    using variables_tuple = tuple< first_type, Rest... >;

    // returns this unique, sorted set of variables as a tuple
    constexpr variables_tuple as_tuple() const
    { return std::tuple_cat( tuple< first_type >{ first() },
        dependent_variables< Rest... >::as_tuple() ); }

    // implicit conversion of this class into an std::tuple
    constexpr operator variables_tuple() const
    { return as_tuple(); }

    // Ith dependent variable type
    template< size_t I >
    using element_t = std::tuple_element_t< I, variables_tuple >;

    // Create the minimal scope that an expression with these dependent variables
    // can be executed against.
    constexpr scope_type make_scope() const
    { return scope_type::from_tuple( 
        operator tuple< first_type, Rest... >() ); }

    // Checks if a scope contains the values necessary to evaluate an expression
    // with these dependent variables
    template< variable... Vars >
    static consteval bool is_valid_scope( Scope< Vars... > scope = {} )
    { return dependent_variables< Rest... >::is_valid_scope( scope ) and
        (( variable_traits< First >::id == variable_traits< Vars >::id ) or ... ); }

    // Does this set contain a particular variable?
    template< variable V >
    static consteval bool contains( V v = {} )
    { 
        static constexpr size_t vid = variable_traits< V >::id;

        if( variable_traits< first_type >::id == vid )
            return true;

        return dependent_variables< Rest... >::contains( v );
    }

    // The first variable in this set
    constexpr first_type const& first() const
    { return _first; } 

    // The last variable in this set
    constexpr last_type const& last() const
    { return std::get< sizeof...( Rest )>( 
        operator tuple< first_type, Rest... >() ); }

    // The set of dependent variables except the first
    constexpr dependent_variables< Rest... > rest() const
    { return *this; }

    constexpr dependent_variables() = default;
    constexpr dependent_variables( dependent_variables const& ) = default;
    constexpr dependent_variables( first_type&& first, Rest&&... rest ):
        dependent_variables< Rest... >{ std::forward( rest )... }, 
            _first{ first } { }

private:
    // storage for the variable name
    first_type _first;
};

////////////////////////////////////////////
/// Merging Sets of Dependent Variables ///
//////////////////////////////////////////
///
namespace detail {
template< typename VarsT, typename VarsU, variable... Merged >
struct MergeDependentVars;

// Base case directly inherits from dependent_variables
template< variable... Merged >
struct MergeDependentVars< dependent_variables< >, dependent_variables< >,
    Merged... >: dependent_variables< Merged... >
{ 
    using type = dependent_variables< Merged... >;

    constexpr MergeDependentVars( dependent_variables< >, dependent_variables< >,
        Merged... merged ): type{ merged... } { } 
};

// Case: T is Ith variable
template< variable T, variable... Ts, variable U, variable... Us, 
    variable... Merged >
requires( is_less( variable_traits< T >::id, variable_traits< U >::id ))
struct MergeDependentVars< dependent_variables< T, Ts... >,
    dependent_variables< U, Us... >, Merged... >:
        MergeDependentVars< dependent_variables< Ts... >, 
            dependent_variables< U, Us... >, Merged..., T >
{
    using base_type = MergeDependentVars< dependent_variables< Ts... >, 
        dependent_variables< U, Us... >, Merged..., T >;

    constexpr MergeDependentVars( dependent_variables< T, Ts... > left,
        dependent_variables< U, Us... > right, Merged... merged ):
            base_type{ left.rest(), right, merged..., left.first() } { }
};

// Case: U is the next variable
template< variable T, variable... Ts, variable U, variable... Us,
    variable... Merged >
requires( is_greater( variable_traits< T >::id, variable_traits< U >::id ))
struct MergeDependentVars< dependent_variables< T, Ts... >,
    dependent_variables< U, Us... >, Merged... >:
        MergeDependentVars< dependent_variables< T, Ts... >, 
            dependent_variables< Us... >, Merged..., U >
{
    using base_type = MergeDependentVars< dependent_variables< T, Ts... >, 
        dependent_variables< Us... >, Merged..., U >;

    constexpr MergeDependentVars( dependent_variables< T, Ts... > left,
        dependent_variables< U, Us... > right, Merged... merged ):
            base_type{ left, right.rest(), merged..., right.first() } { }
};

// Case: T and U are the same, we pick T
template< variable T, variable... Ts, variable U, variable... Us,
    variable... Merged >
requires( variable_traits< T >::id == variable_traits< U >::id )
struct MergeDependentVars< dependent_variables< T, Ts... >,
    dependent_variables< U, Us... >, Merged... >:
        MergeDependentVars< dependent_variables< Ts... >, 
            dependent_variables< Us... >, Merged..., T >
{
    using base_type = MergeDependentVars< dependent_variables< Ts... >, 
        dependent_variables< Us... >, Merged..., T >;

    constexpr MergeDependentVars( dependent_variables< T, Ts... > left,
        dependent_variables< U, Us... > right, Merged... merged ):
            base_type{ left.rest(), right.rest(), merged..., left.first() } { }
};

// Case: left is empty
template< variable U, variable... Us, variable... Merged >
struct MergeDependentVars< dependent_variables< >,
    dependent_variables< U, Us... >, Merged... >:
        MergeDependentVars< dependent_variables< >, 
            dependent_variables< Us... >, Merged..., U >
{
    using base_type = MergeDependentVars< dependent_variables< >, 
        dependent_variables< Us... >, Merged..., U >;

    constexpr MergeDependentVars( dependent_variables< > left, 
        dependent_variables< U, Us... > right, Merged... merged ):
            base_type{ left, right.rest(), merged..., right.first() } { }
};

// Case: right is empty
template< variable T, variable... Ts, variable... Merged >
struct MergeDependentVars< dependent_variables< T, Ts... >,
    dependent_variables< >, Merged... >:
        MergeDependentVars< dependent_variables< Ts... >, 
            dependent_variables< >, Merged..., T >
{
    using base_type = MergeDependentVars< dependent_variables< Ts... >, 
        dependent_variables< >, Merged..., T >;

    constexpr MergeDependentVars( dependent_variables< T, Ts... > left, 
        dependent_variables< > right, Merged... merged ):
            base_type{ left.rest(), right, merged..., left.first() } { }
};
} // namespace detail

template< typename T, typename U >
using merge_dependent_variables_t = detail::MergeDependentVars< T, U >::type;

template< variable... Ts, variable... Us >
constexpr merge_dependent_variables_t< dependent_variables< Ts... >, 
    dependent_variables< Us... >>
merge_dependent_variables( dependent_variables< Ts... > const& left, 
    dependent_variables< Us... > const& right )
{ return detail::MergeDependentVars< dependent_variables< Ts... >, 
    dependent_variables< Us... >>{ left, right }; }


////////////////////////////////////////////////////////////////
/// Construct a Depedendent Variable Set from an Expression ///
//////////////////////////////////////////////////////////////
///
namespace detail {

template< typename ExprT >
struct GetFreeVariables;

/// Express a variable as it's result type
template< typename ExprT >
struct ExpressVariables 
{ 
    using type = ExprT; 
    static constexpr type value( ExprT const& expr )
    { return expr; }
};

// Case: Express a single variable as it's value_type
template< size_t I, typename T >
struct ExpressVariables< Variable< I, T >>
{ 
    using type = T; 
    static constexpr type value( Variable< I, T > const& )
    { return T{}; }
};

/// Case: Expressing variables inside a substitution expression
///
/// We express the variables in the substitution arguments directly. We 
/// assume that the substitution is valid: 
/// - sizeof...( Subs ) == free_variables_t< ExprT >::size
/// - ( is_valid_substitution_v< free_variable_t< Is, ExprT >, Subs...[ Is ]> 
///       and ... )
/// 
/// For that to be valid ExprT must have only variables to be substituted with
/// ...Subs thus we only need express the variables in ...Subs
/// 
template< typename ExprT, typename... Subs >
struct ExpressVariables< Substitution< ExprT, Subs... >>
{
    using substitution_type = Substitution< ExprT, Subs... >;
    static constexpr make_seq< sizeof...( Subs )> for_subs;

    using type = Substitution< ExprT, typename ExpressVariables< Subs >::type
        ... >;

    static constexpr type value( substitution_type const& sub )
    { 
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
        { return { SubstitutionFormula< substitution_type >::value( sub ), 
            ExpressVariables< Subs...[ Is ]>::value(
                SubstitutionSub< Is, substitution_type >::value( sub ))... }; };

        return helper( for_subs );
    }
};

// Case: Express variables in a compound expression, recurse
template< template< typename... > class Op, typename... Args >
requires( requires { typename Op< Args... >::arguments_tuple; } and
    not std::is_same_template_v< Substitution, Op > )
struct ExpressVariables< ExprT >
{
    static constexpr make_seq< sizeof...( Args )> for_arguments;

    using type = Op< typename ExpressVariables< Args >::type... >;
    static constexpr type value( Op< Args... > const& expr )
    { 
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
        { return { ExpressVariables< Args...[ Is ]>::value(
            std::get< Is >( expr ))... }; };
        
        return helper( for_arguments );
    }
};

/// Identify free variables in an expression
///
/// Case: Single Variable must be free
template< size_t I, typename T >
struct GetFreeVariables< Variable< I, T >>
{
    using type = dependent_variables< Variable< I, T >>;
    static constexpr size_t size = 1;
    static constexpr type value( Variable< I, T > const& var )
    { return { var }; }
};

/// Case: Free Variables in a Substitution must be free variables in the 
///       substituted expressions or be higher order variables in the 
///       
template< typename ExprT, typename... Subs >
struct GetFreeVariables< Substitution< ExprT, Subs... >>
{ 
    using type = merge_dependent_variables_t< 
        typename GetFreeVariables< tuple< Subs... >>::type,
        typename GetFreeVariables< typename ExpressVariables< ExprT >::type >
            ::type >;

    using merged_variables = type;
    static constexpr size_t size = merged_variables::size;

    static constexpr type value( Substitution< ExprT, Subs... > const& sub )
    { return merge_dependent_variables( 
        GetFreeVariables< tuple< Subs... >>::value( sub.substitutions() ),
        GetFreeVariables< ExpressVariables< ExprT >>::value({ sub.formula() )); }
};

/// Case: Compound Expressions can be treated like a tuple of their arguments
template< template< typename... > class Op, typename... Args >
requires( requires { typename Op< Args... >::arguments_tuple; } and
    not std::is_same_template_v< Substitution, Op > )
struct GetFreeVariables< Op< Args... >>:
    GetFreeVariables< tuple< Args... >> { };

/// Case: Tuple of expressions
template< >
struct GetFreeVariables< tuple< >>
{
    using type = dependent_variables< >;
    static constexpr type value( tuple< > const& )
    { return { }; }
};

template< typename ExprT, typename... SubArgTuples >
struct GetDependentVariables;

///
/// Case: We defeault to no dependent variables found
template< typename ExprT >
struct GetDependentVariables< ExprT >
{ 
    using type = dependent_variables< >;
    static constexpr size_t size = 0;
    static constexpr type value( ExprT const& )
    { return { }; }
};
///
/// Case: single variable at base substitution level
template< size_t I, typename ExprT, typename... SubArgTuples >
struct GetDependentVariables< Variable< I, ExprT >, SubArgTuples... >
{
    using variable_type = Variable< I, ExprT >;
    using type = dependent_variables< variable_type >;
    static constexpr size_t size = 1;
    static constexpr type value( variable_type const& var )
    { return { var }; }
};
///
/// Case: single variable at higher substitution levels.  We recurse on the 
///       variable's value type, and decrease our substitution level by one
///       until the base case above handles the variable.
template< size_t I, typename ExprT, typename EnclosingSubstitutionT >
requires( not std::is_same_v< EnclosingSubstitutionT, void >)
struct GetDependentVariables< Varable< I, ExprT >, EnclosingSubstitutionT >:
    GetDependentVariables<  , 
        SubstitutionLevel - 1 >
{ };
///
/// Case: Non-empty tuple of expressions
/// NOTE: empty case handled by default implementation
template< typename T, typename... Ts, size_t SubstitutionLevel >
struct GetDependentVariables< tuple< T, Ts... >, SubstitutionLevel >
{
    using type = merge_dependent_variables_t< 
        typename GetDependentVariables< T, SubstitutionLevel >::type,
        typename GetDependentVariables< tuple< Ts... >, SubstitutionLevel >::type >;

    static constexpr size_t size = type::size;

    static constexpr type value( tuple< T, Ts... > const& tup )
    { 
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr
        { return GetDependentVariables< tuple< Ts... >, SubstitutionLevel >::
            value( std::get< 1 + Is >( tup )... ); };

        return merge_dependent_variables(
            GetDependentVariables< T, SubstitutionLevel >::value( std::get< 0 >( tup )),
            helper( make_seq< sizeof...( Ts )>{} ));
    }
};
///
/// Case: Tensor of expressions
template< shape S, typename... Ts, size_t SubstitutionLevel >
struct GetDependentVariables< Tensor< S, Ts... >, SubstitutionLevel >:
    GetDependentVariables< tuple< Ts... >, SubstitutionLevel > 
{ };
///
/// Case: Substitution for an expression with variable order 1 ( the minimum ).
///       In this case any variable in the original expression will be replaced
///       and so we need only check the ...Subs for dependent variables
template< typename ExprT, typename... TSubs, typename ExprU, typename... USubs >
struct GetDependentVariables< Substitution< ExprT, TSubs... >, 
    Substitution< ExprU, USubs... >>
{
    using type = merge_dependent_variables_t<
        typename GetDependentVariables< ExprT, 
            Substitution< Substitution< ExprT, TSubs... >, USubs... >>
};
///
/// Case: Substitution for an expression with higher order than 1
///       In this case we need to search the value_type of higher order variables
///       tracking our substitution level using the size_t template parameter
template< typename ExprT, typename... Subs, typename... SubArgTuples >
struct GetDependentVariables< Substitution< ExprT, Subs... >, SubArgTuples... ArgTuples >
{
    using type = merge_dependent_variables_t<
        typename GetDependentVariables< ExprT, ArgTuples..., tuple< Subs... >>::type,
        typename GetDependentVariables< tuple< Subs... >, ArgTuples... >::type >;

    static constexpr make_seq< sizeof...( Subs )> for_subs;

    static constexpr size_t size = type::size;

    static constexpr type value( Substitution< ExprT, Subs... > const& sub )
    { 
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
        { return GetDependentVariables< tuple< Subs... >, SubstitutionLevel >::
            value({ sub.template arg< Is >()... }); };

        return merge_dependent_variables(
            GetDependentVariables< ExprT, 1 + SubstitutionLevel >::value( 
                sub.expression()),
            helper( for_subs ));
    }
};
///
/// Case: Dependent Variables of compound expressions that are not substitutions
/// NOTE: We cannot use the compound_expression concept here because it requires
///       that the expression type be concrete and we are still in the bootstrap
///       stage of expressions.
template< typename ExprT, size_t SubstitutionLevel >
requires( requires( ExprT ) { typename ExprT::arguments_tuple; } and 
    not is_substitution_expression_v< ExprT >)
struct GetDependentVariables< ExprT, SubstitutionLevel >:
    GetDependentVariables< typename ExprT::arguments_tuple, SubstitutionLevel > { };

static_assert( is_same_v< 
    typename GetDependentVariables< Variable< 0, int >>::type,
        dependent_variables< Variable< 0, int >>> );

template< typename ExprT >
struct NextVariableId;

template< typename ExprT >
requires( GetDependentVariables< ExprT >::size == 0 )
struct NextVariableId< ExprT >: integral_constant< size_t, 0 > { };

template< typename ExprT >
requires( GetDependentVariables< ExprT >::size != 0 )
struct NextVariableId< ExprT >: integral_constant< size_t, 
    variable_traits< typename GetDependentVariables< ExprT >::type::last_type >::
        id + 1 > { };

template< expression ExprT, typename ScopeT >
requires( is_scope_v< ScopeT >)
struct ScopeContainsDependentVariables {
private:
    template< typename Deps >
    struct Helper;

    template< variable... Vars >
    struct Helper< dependent_variables< Vars... >>: integral_constant< bool,
        ( ScopeContainsVariable< variable_traits< Vars >::id, ScopeT >::value 
            and ... )> { };

public:
    static constexpr bool value = 
        Helper< typename GetDependentVariables< ExprT >::type >::value;
};

} // namespace detail

template< typename ExprT >
using dependent_variables_t = detail::GetDependentVariables< ExprT >::type;

template< typename ExprT >
constexpr dependent_variables_t< ExprT >
get_dependent_variables( ExprT const& expr )
{ return detail::GetDependentVariables< ExprT >::value( expr ); }

template< variable Var, typename ExprT >
constexpr bool depends_on_variable_v = dependent_variables_t< ExprT >::template 
    contains< Var >(); 

template< typename ExprT >
constexpr size_t next_variable_id_v = detail::NextVariableId< ExprT >::value;

template< expression ExprT, typename ScopeT >
constexpr bool scope_contains_dependent_variables_v = 
    detail::ScopeContainsDependentVariables< ExprT, ScopeT >::value;

namespace test {

static_assert( is_same_v< dependent_variables< Variable< 0, int >>, 
    dependent_variables_t< Variable< 0, int >>> );
static_assert( is_same_v< dependent_variables< Variable< 0, int >, Variable< 1, int >>, 
    dependent_variables_t< tuple< Variable< 0, int >, Variable< 1, int >>>> );
static_assert( is_same_v< dependent_variables< Variable< 0, int >, Variable< 1, int >>, 
    dependent_variables_t< tuple< Variable< 1, int >, Variable< 0, int >>>> );
static_assert( is_same_v< dependent_variables< Variable< 0, int >>, 
    dependent_variables_t< tuple< Variable< 0, int >, Variable< 0, int >>>> );
static_assert( is_same_v< dependent_variables< Variable< 1, int >>, 
    dependent_variables_t< tuple< Variable< 1, int >, Variable< 1, int >>>> );

// tests for higher-order variables
static_assert( is_same_v< dependent_variables< Variable< 0, Variable< 0, int >>>,
    dependent_variables_t< Variable< 0, Variable< 0, int >>>> );
static_assert( is_same_v< dependent_variables< Variable< 0, Variable< 0, int >>>,
    dependent_variables_t< tuple< Variable< 0, Variable< 0, int >>>>> );
static_assert( is_same_v< dependent_variables< Variable< 0, int >>,
    typename detail::GetDependentVariables< Variable< 0, Variable< 0, int >>, 1 >::type > );
static_assert( is_same_v< dependent_variables< Variable< 0, int >>,
    dependent_variables_t< Substitution< Variable< 0, Variable< 0, int >>, Constant<0>>>> );

} // namespace test

//////////////////////////
/// Expression Traits ///
////////////////////////
///
template< typename ExprT >
struct expression_traits;

template< typename ExprT >
requires( expression< ExprT > and not compound_expression< ExprT >)
struct expression_traits< ExprT >
{
    using variables = free_variables_t< ExprT >;
    static constexpr size_t variables_size = variables::size;
    using result_type = result_t< ExprT >;
    using variable_values_tuple = variables::values_tuple;
    using scope_type = variables::scope_type;
    using arguments_tuple = tuple< >;

    template< variable... Vars >
    static constexpr bool is_valid_scope( Scope< Vars... > const& scope )
    { return variables::is_valid_scope( scope ); }
};

template< typename ExprT >
requires( compound_expression< ExprT >)
struct expression_traits< ExprT >
{
    using variables = free_variables_t< ExprT >;
    static constexpr size_t variables_size = variables::size;
    using result_type = result_t< ExprT >;
    using variable_values_tuple = variables::values_tuple;
    using scope_type = variables::scope_type;
    using arguments_tuple = ExprT::arguments_tuple;

    template< variable... Vars >
    static constexpr bool is_valid_scope( Scope< Vars... > const& scope )
    { return variables::is_valid_scope( scope ); }
};

template< typename ScopeT, expression ExprU >
struct IsScopeFor;

template< variable... Vars, expression ExprU >
struct IsScopeFor< Scope< Vars... >, ExprU >: 
    integral_constant< bool, expression_traits< ExprU >::
        is_valid_scope( Scope< Vars... >{} )>
{ };

template< typename ScopeT, expression ExprU >
constexpr bool is_scope_for_v = IsScopeFor< ScopeT, ExprU >::value;

template< expression ExprT >
using expression_scope_t = expression_traits< ExprT >::scope_type;

/// @brief constructs a scope from the dependent variables of ExprT and init-
/// ializes the values to ...ts in variable id order
///
template< expression ExprT, typename... Ts >
constexpr typename expression_traits< ExprT >::scope_type
make_scope( Ts const&... ts )
{ 
    using scope_type = expression_traits< ExprT >::scope_type;
    using variables_tuple = scope_type::variables_tuple_type;

    scope_type scope{ variables_tuple{} };

    auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr
    { ( scope.template set_value< std::tuple_element_t< Is, variables_tuple >>( 
        ts...[ Is ] ), ...); };

    helper( make_seq< sizeof...( Ts )>{} );

    return scope;
}

/////////////////////////////////
/// Compatible Substitutions ///
///////////////////////////////
///
/// Determines if the dependent variables of an expression support substitution
/// by a given list of expression types ...Subs ( or tuple< Subs... > )
///
///
template< typename ExprT, typename... Subs >
struct CompatibleSubstitution;

///
/// @brief Validates a single variable against a substitution argument
template< typename Var, typename Sub >
struct IsCompatibleVariableSubstitution;
///
/// Case: First-order compatibility
///
/// If the result type of the variable and substitution do not match
/// then this is not a compatible variable substitution.  We have a separate
/// logical path for higher-order variables.
template< size_t I, typename ExprT, typename Sub >
requires( not is_substitution_expression_v< ExprT > )
struct IsCompatibleVariableSubstitution< Variable< I, ExprT >, Sub >: 
    integral_constant< bool, 
        std::is_convertible_v< result_t< ExprT >, result_t< Sub >>> { }; 
///
/// Case: Higher-order compatibility
///
/// This variable has been substituted into by ...SubSubs.  We must check the
/// result, but also we must ensure that the substitution into the variable
/// will still be valid after Sub is substituted for Variable< I, ExprT >.
template< size_t I, typename ExprT, typename... SubSubs, typename Sub >
requires( is_substitution_expression_v< ExprT > ) // redundant but kept for clarity
struct IsCompatibleVariableSubstitution< Variable< I, 
    Substitution< Variable< I, ExprT >, SubSubs... >>, Sub >: 
        integral_constant< bool, 
            std::is_convertible_v< result_t< ExprT >, result_t< Sub >> and
                CompatibleSubstitution< Sub, SubSubs... >::value > { };

///
/// @brief Helper decides whether substitution into tuple< Vars... > by 
/// tuple< Subs... > is allowed
namespace detail {
template< typename VarsTule, typename SubTuple >
struct CompatibleSubstitutionHelper;
///
/// Case: ...Vars and ...Subs are not the same size so this substitution
///       is incompatible.
template< typename... Vars, typename... Subs >
requires( sizeof...( Vars ) != sizeof...( Subs ))
struct CompatibleSubstitutionHelper< unique_variables< Vars... >, 
    tuple< Subs... >>: integral_constant< bool, false > { };
///
/// Case: The result of the first substitution argument is convertible to
///       the result of the first variable.  Move on to the next variable and
///       substitution pair.
template< typename Var, typename... Vars, typename Sub, typename... Subs >
requires( sizeof...( Vars ) == sizeof...( Subs ) and 
    IsCompatibleVariableSubstitution< Var, Sub >::value )
struct CompatibleSubstitutionHelper< 
    unique_variables< Var, Vars... >, tuple< Sub, Subs... >>: 
        CompatibleSubstitutionHelper< unique_variables< Vars... >, tuple< Subs... >> 
{ };
///
/// Case: The first variable's and substitution's results are not convertible
///       so this substitution is incompatible.
template< typename Var, typename... Vars, typename Sub, typename... Subs >
requires( sizeof...( Vars ) == sizeof...( Subs ) and 
    not IsCompatibleVariableSubstitution< Var, Sub >::value )
struct CompatibleSubstitutionHelper< 
    unique_variables< Var, Vars... >, tuple< Sub, Subs... >>: integral_constant< bool,
        false > 
{ };
///
/// Case: Terminal case for helper recursion represents an empty substitution
///       which is valid.
template< >
struct CompatibleSubstitutionHelper< unique_variables<>, tuple<> >: 
    integral_constant< bool, true > { };


/// Finally we have the primary class to verify substitutions
///
/// Case: Empty substitutions are always valid 
///
/// TODO: This violates the idea that the number of free variables must equal the 
///       number of arguments being substituted.  We either need to keep unsubstituted
///       variables free, or require that this substitution fail if ExprT has any free
///       variables itself. 
///
///       DT: I'm inclined to go the "keep unsub'd variables free" route since higher-
///       order variables also can sneak through a substitution remaining free.
// substituting nothing is always ok
template< typename ExprT, typename... Subs >
struct CompatibleSubstitution: 
    std::integral_constant< bool, sizeof...( Subs ) == 0 > { };

///
template< variable Var, typename U >
requires( variable_traits< Var >::order == 1 )
struct CompatibleSubstitution< Var, U >: std::is_convertible< result_t< U >, 
    typename variable_traits< Var >::value_type > { }; 

// NOTE: let's do these higher-order compatibilities one by one so my head 
//       doesn't hurt too much
//
/// Case: Substituting into a substitution expression's variable.
///
/// f(x)(n) [ it's sneakier than that 'cause it is substituting into the
///           variable x itself, but maybe that's the same? ]
///
/// using  v0 = Variable< 0, float >;
/// using vv0 = Variable< 0, v0 >;
/// using  v1 = Variable< 1, float >;
/// using vv1 = Variable< 1, v1 >;
/// using  v2 = Variable< 2, float >;
/// using vv2 = Variable< 2, v2 >;
/// v0 f;
/// v1 x;
/// v2 y;
///
/// Substitution< vv0, v1 > fx = f(x);
/// Substitution< Substitution< vv0, v1 >, v2 > fxy = fx(y);
/// Substitution< vv0, v2 > fy = f(y);
/// Substitution< Substitution< vv0, v2 >, v1 > fyx = fy(x);
///
//template< variable Var, typename... Subs >
//requires( variable_traits< Var >::order == 2 )
//struct CompatibleSubstitution< Var, Subs... >: std::true_type { };

template< typename... Ts, typename... Subs >
struct CompatibleSubstitution< tuple< Ts... >, Subs... >:
    CompatibleSubstitutionHelper< free_variables_t< tuple< Ts... >>,
        tuple< Subs... >>
{ };

template< shape ShapeT, typename... Ts, typename... Subs >
struct CompatibleSubstitution< Tensor< ShapeT, Ts... >, Subs... >:
    CompatibleSubstitutionHelper< free_variables_t< Tensor< ShapeT, Ts... >>,
        tuple< Subs... >>
{ };

// we haven't defined Substitution as an expression yet, but we can 
// still decide whether or not substituting into a substitution would work
// or not by pretending we are substituting into a tuple of the substitution
// expressions substitutions.
template< typename ExprT, typename... Subs, typename... SubSubs >
struct CompatibleSubstitution< Substitution< ExprT, Subs... >, SubSubs... >:
    CompatibleSubstitution< tuple< Subs... >, SubSubs... > { };

template< typename ExprT, typename... Subs >
requires( compound_expression< ExprT > )
struct CompatibleSubstitution< ExprT, Subs... >:
    CompatibleSubstitutionHelper< 
        free_variables_t< ExprT >, tuple< Subs... >>
{ };
} // namespace detail

template< typename ExprT, typename... Subs >
constexpr bool is_compatible_substitution_v = 
    detail::CompatibleSubstitution< ExprT, Subs... >::value;

static_assert( is_compatible_substitution_v< Variable< 0, float >, float >);
static_assert( is_compatible_substitution_v< tuple< float, Variable< 0, float >>, float >);

//////////////////////////////////////////////////////
/// Arguments Base Class for Compound Expressions ///
////////////////////////////////////////////////////
///
/// Base class for compound expressions implements substitution and manipulator
/// application methods
///
/// @brief Base class for compound operations. 
///
/// Provides a base implementation of substitution for dependent variables via
/// operator() and application of a manipulator via operator|.
///
template< template< typename... > class Op, typename... Args >
struct Arguments: tuple< Args... >, detail::ExpressionTag
{
    using arguments_tuple = tuple< Args... >;
    static constexpr size_t arguments_size = sizeof...( Args );
    static constexpr make_seq< arguments_size > for_arguments;

    constexpr arguments_tuple const& arguments() const
    { return *this; }

    template< typename... Subs >
    using make_expression_t = Op< Subs... >;

    template< typename... Subs >
    static constexpr make_expression_t< Subs... >
    make_expression( Subs&&... subs )
    { return { subs... }; }

    using expression_type = Op< Args... >;

    /// @brief reconstructs the expression from arguments using the curiously
    /// recurring template pattern
    constexpr expression_type 
    expression() const
    {
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr ->
            expression_type
        { return { std::get< Is >( static_cast< arguments_tuple >( *this ))... }; };

        return helper( for_arguments ); 
    }

    /// @brief Substitution via invocation operator
    ///
    /// For each dependent variable in this expression, ordered by I by it's 
    /// variable id, substitute subs...[I] and return the new expression.
    ///
    /// @tparam Op is the root operation of the expression
    /// @tparam ...Args are the types of the arguments of the expression
    /// @tparam ...Subs are the types of the substitutions made into the dependent
    /// variables of the expression
    /// @returns a new expression with ...Subs substituted into the dependent
    /// variables of Op< Args... > in order
    template< typename... Subs >
    requires( is_compatible_substitution_v< Op< Args... >, Subs... > )
    constexpr Substitution< Op< Args... >, Subs... >
    operator ()( Subs... subs ) const;
    //{ return { expression(), subs... }; }

    /// Apply a manipulator
    ///
    /// Manipulators are applied to the arguments of this expression and 
    /// recombined into a new expression by the operation.
    ///
    /// example: 
    ///
    /// assert( static_expr( 1 ) + static_expr( 2 )         | 
    ///         manipulate( [&]( auto n ){ return n + 1; }) | 
    ///         evalulate() == 5 );
    ///
    /// @brief Helper class for apply method
    template< typename ManipulatorT >
    struct Applier;
    /// 
    /// @brief Applies a manipulator to the parent expression
    template< typename ManipulatorT >
    constexpr auto apply( ManipulatorT& manipulator ) const;

protected:
    constexpr Arguments( Args... args ): 
        arguments_tuple{ args... } { }

    constexpr Arguments() = default;
};

///////////////////////////////
/// Second-order Variables ///
/////////////////////////////
///
/// @brief variable specialization for an expression with dependent
/// variables
template< size_t I, expression ExprT >
requires( not expression< result_t< ExprT >> and 
    is_greater( std::tuple_size_v< dependent_variables_t< ExprT >>, 0 ))
struct Variable< I, ExprT >: detail::ExpressionTag
{
    using value_type = ExprT;
    using result_type = value_type;
    static constexpr size_t id = I;
    using this_type = Variable< id, value_type >;

    constexpr SetVariableValue< Variable >
    operator =( value_type const& other ) const
    { return { *this, other }; }

    constexpr string const& name() const
    { return _name; }

    constexpr void set_name( string const& new_name )
    { _name = new_name; }
    
    /// @brief substitution operator for second-order variables.  
    ///
    /// We do not have a value for the expression_type this variable is
    /// a placeholder for.  
    template< typename... Subs >
    requires( is_compatible_substitution_v< result_type, Subs... > )
    constexpr Substitution< result_type, Subs... >
    operator ()( Subs... subs ) const;
//    { return substitute( expression(), subs... ); }


    constexpr Variable( string const& name = "expr" ): _name{ name } { };
    constexpr Variable( Variable const& ) = default;
    constexpr Variable( Variable&& ) = default;

private:
    string _name;
};

/////////////////
/// Sequence ///
///////////////
///

//////////////////////////////////
/// Evaluation of Expressions ///
////////////////////////////////
///
///
template< typename ScopeT = void >
struct Evaluator;

namespace detail {
template< typename FuncT >
struct ManipulatorFunctor 
{
    using functor_type = FuncT;

    template< typename ExprT >
    constexpr auto 
    operator ()( ExprT const& expr ) const
    { return _func( expr ); }

    functor_type _func;
};
} // namespace detail

template< typename ScopeT >
struct Evaluator
{
    using scope_type = ScopeT;

    template< variable Var >
    constexpr typename Var::value_type
    operator ()( Var const& var ) const
    { return _scope.get_value( var ); }

    template< auto Value >
    constexpr typename Constant< Value >::value_type
    operator ()( Constant< Value > const& constant ) const
    { return Value; }

    template< typename T >
    constexpr T
    operator ()( StaticValue< T > const& static_value ) const
    { return static_value.get_value(); }

    template< typename T >
    requires( not expression< T > )
    constexpr T
    operator ()( T const& value ) const
    { return value; }

    template< typename ExprT, typename... Subs >
    constexpr auto
    operator ()( Substitution< ExprT, Subs... > const& sub_expr ) const;

    constexpr Evaluator() = delete;
    constexpr Evaluator( Evaluator const& ) = default;
    constexpr Evaluator( scope_type const& scope ): _scope{ scope } { }

    scope_type const& _scope;
};

template< >
struct Evaluator< void >
{
    template< typename T >
    requires( not expression< T > )
    constexpr T
    operator ()( T const& val ) const
    { return val; }

    template< auto Value >
    constexpr typename Constant< Value >::value_type
    operator ()( Constant< Value > const& constant ) const
    { return Value; }

    template< typename T >
    constexpr T
    operator ()( StaticValue< T > const& static_value ) const
    { return static_value.get_value(); }

    template< typename ExprT, typename... Subs >
    constexpr auto
    operator ()( Substitution< ExprT, Subs... > const& sub_expr ) const;
};

template< typename FuncT >
constexpr detail::ManipulatorFunctor< FuncT > manipulate( FuncT&& func )
{ return { func }; }

template< typename ScopeT >
constexpr Evaluator< ScopeT > eval( ScopeT const& scope )
{ return { scope }; }

constexpr Evaluator< void > eval()
{ return {}; }


////////////////////////////////
/// Manipulation: operator| ///
//////////////////////////////
/// 
/// We commandeer operator| on expression types as the "manipulation" operator
///
/// @brief Application of a temporary manipulator
///
/// @tparam ExprT is the type of the expression to be manipulated
/// @tparam ManipulatorT is the type of the manipulator
/// @param expr is the expression value
/// @param manipulator is the manipulator value
/// @returns the result of calling apply on expr with the manipulator parameter

// TODO: Manipulators may need a rigourous definition as a type manipulator

/// @brief application of a non-expression value against an evaluator yields the 
/// value
template< typename T, typename ScopeT >
requires( not expression< T >)
constexpr T 
operator |( T const& value, Evaluator< ScopeT > const& eval )
{ return value; }

/// @brief application of a non-expression value against a scope that cannot be
/// invoked on the value yields the value
template< typename T, typename ScopeT >
requires( not expression< T > and is_scope_v< ScopeT > and 
    not std::invocable< const ScopeT, T > )
constexpr T
operator |( T const& value, ScopeT const& scope )
{ return value; }

/// @brief application of a value or expression against a manipulator reference
/// that has an overloaded invocation operator yields the inovocation of the 
/// manipulator on the value or expression
template< typename T, typename ManipulatorT >
requires std::invocable< ManipulatorT, T >
constexpr auto operator |( T const& value_or_expression, ManipulatorT& manipulator )
{ return manipulator( value_or_expression ); }

/// @brief application of a compound expression against a manipulator reference
/// that does not have an overloaded invocation operator starts a dual recursion
/// between the compound expression's apply method and the application operator|
template< compound_expression ExprT, typename ManipulatorT >
requires( not std::invocable< ManipulatorT, ExprT >)
constexpr auto operator |( ExprT const& expr, ManipulatorT& manipulator )
{ return expr.apply( manipulator ); }

/// @brief application of a value or expression against a const manipulator
/// reference that has an overloaded invocation operator yields the invocation of
/// the manipulator on the value or expression
template< typename T, typename ManipulatorT >
requires std::invocable< const ManipulatorT, T >
constexpr auto operator |( T const& value_or_expression, ManipulatorT const& manipulator )
{ return manipulator( value_or_expression ); }

/// @brief application of a compound expression against a const manipulator
/// reference that does not have an overloaded inovcation operator starts a dual
/// recursion between the compound expression's apply method and the application
/// operator|
///
/// This is where the rubber meets the road: (3*x | eval()) fails because product 
/// doesn't know evaluator yet, so neither can determine the return type
template< compound_expression ExprT, typename ManipulatorT >
requires( not std::invocable< const ManipulatorT, ExprT >)
constexpr auto operator |( ExprT const& expr, ManipulatorT const& manipulator )
{ return expr.apply( manipulator ); }

/// @brief application of a tuple of at least one expression and a manipulator
/// that is not invocable on the tuple yields a tuple of the result of applying
/// the manipulator against each element of the tuple.
template< typename... Ts, typename ManipulatorT >
requires( expression< tuple< Ts... >> and 
    not std::invocable< ManipulatorT, tuple< Ts... >> )
constexpr auto operator |( tuple< Ts... > const& expr_tup, 
    ManipulatorT& manipulator )
{
    make_seq< sizeof...( Ts )> for_tuple_elements;

    auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
    { return std::make_tuple(
        ( std::get< Is >( expr_tup ) | manipulator )... ); };

    return helper( for_tuple_elements );
}

/// @brief application of a tensor of at least one expression and a manipulator
/// that is not invocable on the tensor yields a tensor of the result of applying
/// the manipulator against each element of the tensor.
template< typename ShapeT, typename... Exprs, typename ManipulatorT >
requires( expression< Tensor< ShapeT, Exprs... >> and
    not std::invocable< ManipulatorT, Tensor< ShapeT, Exprs... >> )
constexpr auto operator |( Tensor< ShapeT, Exprs... > const& expr_ten, 
    ManipulatorT& manipulator )
{
    auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
    { return make_tensor< ShapeT >(
        ( tensor_get< Is >( expr_ten ) | manipulator )... ); };

    return helper( make_seq< sizeof...( Exprs )>{} );
}

//////////////////////////////////////////
/// Arguments::Applier implementation ///
////////////////////////////////////////
///
/// There are three things going on:
/// - Application of a Manipulator to an Expression or Expression Argument
/// - Termination of a Manipulator through Invocation
/// - Calling the Expression's value static method on possibly manipulated
///   arguments.
///
/// If an expression or one of it's arguments has a defined operator| for 
/// the given manipulator, it is called, otherwise if the manipulator 
/// has a defined operator() for the given argument then that is called.
///
/// In either case the result is summarized with expression_type::value. If
/// no arguments were manipulated then this result is applied against the
/// manipulator and returned.
///
/// 
/// Case: Some Expression's args recognize this manipulator, some are
///       recognized by the manipulator
template< template< typename... > class Op, typename... Args >
template< typename ManipulatorT >
struct Arguments< Op, Args... >::Applier
{
    // recurse down until the manipulator can accept the argument
    template< typename Arg >
    static constexpr bool terminal = 
        requires( ManipulatorT manip, Arg arg ) { manip( arg ); };

    template< typename Arg >
    struct Helper;

    template< typename Arg >
    requires( terminal< Arg > )
    struct Helper< Arg >
    {
        using type = std::remove_cv_t< decltype( ManipulatorT{}( Arg{} ))>;

        static constexpr type value( Arg const& arg, ManipulatorT& manip )
        { return manip( arg ); }
    };

    template< template< typename... > class ArgOp, typename... ArgArgs >
    requires( not terminal< ArgOp< ArgArgs... >> and
        compound_expression< ArgOp< ArgArgs... >> )
    struct Helper< ArgOp< ArgArgs... >>
    {
        using type = std::remove_cv_t< decltype( ArgOp< ArgArgs... >{} | ManipulatorT{} )>;

        static constexpr type value( ArgOp< ArgArgs... > const& arg, ManipulatorT& manip )
        { return arg | manip; }
    };

    using type = std::remove_cv_t< decltype( 
        expression_type::value(( typename Helper< Args >::type{})... ))>;

    static constexpr type value( Arguments< Op, Args... > const& args,
        ManipulatorT& manip )
    {
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr
        { return expression_type::value( 
            Helper< Args...[ Is ]>::value( std::get< Is >( args ), manip )... ); };

        return helper( for_arguments );
    }
};

//////////////////////////////////////////////////
/// Arguments<...>::apply(...) Implementation ///
////////////////////////////////////////////////
///
template< template< typename... > class Op, typename... Args >
template< typename ManipulatorT >
constexpr auto Arguments< Op, Args... >::apply( ManipulatorT& manip ) const
{ return Applier< ManipulatorT >::value( *this, manip ); }

/////////////////////////////////////////////////
/// Substitution: Arguments<...>::operator() ///
///////////////////////////////////////////////
/// 
/// @brief compound expressions MUST implement an apply method for manipulator
/// types which is primarily accomplished by inheriting from Arguments< Op, Args... >
template< variable... Vars >
template< compound_expression ExprT >
//requires( scope_contains_unique_variables_v< ExprT, Scope< Vars... >> )
constexpr result_t< ExprT > 
Scope< Vars... >::operator ()( ExprT const& expr ) const
{ return expr.apply( *this ); }

//////////////////////////////////////////////////////////////////
/// Exppression Arguments: get_argument< I >,                 ///
///                        expression_argument_t< I, ExprT > ///
///////////////////////////////////////////////////////////////
///
/// @brief type trait for extracting an argument from a compound expression
/// @tparam I is the id of the argument
/// @tparam ExprT is the type of the compound expression
template< size_t I, typename ExprT >
struct GetArgument
{
    using type = tuple_element_t< I, typename ExprT::arguments_tuple >;
    static constexpr type const& value( ExprT const& expr )
    { return std::get< I >( expr ); }
};

template< size_t I, typename ExprT >
using expression_argument_t = GetArgument< I, ExprT >::type;

template< size_t I, typename ExprT >
constexpr expression_argument_t< I, ExprT > const& 
get_argument( ExprT const& expr )
{ return GetArgument< I, ExprT >::value( expr ); }

//////////////////////////////////
/// Substitution / Predicates ///
////////////////////////////////
///
/// Predicates used to identify parts of expressions to substitute.  
/// A predicate is a boolean valued trait consistent with 
/// std::integral_constant< bool, value >.  Typically they are nested inside
/// another templated class to allow the erasure of parameters to the
/// predicate.  For example a predicate that matches a variable of a given type
/// would be:
///
/// template< typename T >
/// struct ForVariableOf
/// {
///     template< typename ExprT >
///     struct Is: integral_constant< bool, false > { };
///
///     template< size_t I >
///     struct Is< Variable< I, T >>: integral_constant< bool, true > { };
/// };
///

////////////////////////////////////
/// Predicate: ForVariable< I > ///
//////////////////////////////////
///
/// @brief predicate class for the Ith variable id 
template< size_t I >
struct ForVariable
{
    // default case
    template< typename TestT >
    struct Is: integral_constant< bool, false > { };

    // true case
    template< typename T >
    struct Is< Variable< I, T >>: integral_constant< bool, true > { };
};

//////////////////////////////////////////
/// Predicate: ForExpression< ExprT > ///
////////////////////////////////////////
/// 
/// Matches an expression of type ExprT including matching dependent variables
/// against sub-expressions and yielding them as match< Var, ExprU > in the
/// matches_type typedef.
///
template< typename ExprT >
struct ForExpression;
/// 
/// Case: No Dependent Variables in Expression to be matched
///
/// @brief predicate for matching an expression which contains no dependent
/// variables
template< typename ExprT >
requires( free_variables_t< ExprT >::size == 0 )
struct ForExpression< ExprT >
{
    // default case does not match
    template< typename TestT >
    struct Is: integral_constant< bool, false > 
    {  using matches_type = tuple<>; };

    // expression matches verbatim, so no variable matches need to be tracked
    template< >
    struct Is< ExprT >: integral_constant< bool, true > 
    { using matches_type = tuple<>; };
};
/// 
/// Case: Match a variable against any part of the expression whose result_type 
///       is the same as the variable's result_type
///
/// @brief specialization for individual variables
///
/// TODO: I believe this should be thee same logic as is_
template< variable Var > 
requires( variable< Var >)
struct ForExpression< Var >
{
    // variables match anything with the appropriate result type 
    template< typename TestT >
    struct Is: integral_constant< bool, 
        is_same_v< typename variable_traits< Var >::result_type, result_t< TestT >>> 
    { using matches_type = tuple< match< Var, TestT >>; };
};

/// @brief specialization for compound expressions with dependent variables
///
template< template< typename... > class Op, typename... Args >
requires( compound_expression< Op< Args... >> and 
    is_greater( dependent_variables_t< Op< Args... >>::size, 0 ))
struct ForExpression< Op< Args... >> {
private:
    using expression_type = Op< Args... >;
    using arguments_tuple = expression_traits< expression_type >::arguments_tuple;

    // yields the first match from ...Matches with the variable id equal
    // to VariableId
    template< size_t VariableId, typename... Matches >
    struct ChooseMatchByVariableId;

    // null case should never happen
    template< size_t VariableId >
    struct ChooseMatchByVariableId< VariableId >
    { 
#ifndef NDEBUG
        static_assert( false, 
            "no expression found while searching for variable" ); 
#endif
        using type = no_match;
    };

    // if the first id matches yield the corresponding expression type
    template< size_t I, size_t J, typename U, typename ExprU, typename... Rest >
    requires( I == J )
    struct ChooseMatchByVariableId< I, match< Variable< J, U >, ExprU >, 
        Rest... >
    { using type = match< Variable< J, U >, ExprU >; };

    // if the first id does not match test the remaining matches
    template< size_t I, size_t J, typename U, typename ExprU, typename... Rest >
    requires( I != J )
    struct ChooseMatchByVariableId< I, match< Variable< J, U >, ExprU >, 
        Rest... >: ChooseMatchByVariableId< I, Rest... >
    { };

    // helpers to extract a tuple of unique variable matches from a list
    template< typename Seq, typename... Matches >
    struct UniqueMatchesHelper;

    template< size_t... Is, typename... Matches >
    struct UniqueMatchesHelper< seq< Is... >, Matches... >
    {
        // collect the unique variable indices from all the matches
        using unique_variable_id_seq = sort_unique_seq< seq< 
            match_variable_t< Matches...[ Is ]>::id... >>;

        // helper to find matched expression for each unique variable
        // indexed by Seq
        template< typename Seq >
        struct Helper;

        template< size_t... Js >
        struct Helper< seq< Js... >>
        { using matches_type = tuple< typename ChooseMatchByVariableId< Js, 
            Matches... >::type... >; };

        using matches_type = Helper< unique_variable_id_seq >::matches_type;
    };
    
    template< typename... Matches >
    struct UniqueMatches: UniqueMatchesHelper< make_seq< sizeof...( Matches )>,
        Matches... >
    { };

    // helper to test the predicate matches from our arguments
    template< typename... >
    struct AreArgumentMatchesCompatible;

    template< >
    struct AreArgumentMatchesCompatible<>: integral_constant< bool, true >
    { using matches_type = tuple<>; };

    template< typename MatchesT >
    struct AreArgumentMatchesCompatible< MatchesT >: integral_constant< bool, MatchesT::value >
    { using matches_type = MatchesT::matches_type; };

    // helper class for comparing two tuples of matched variables
    template< typename LeftTuple, typename RightTuple, typename Seq >
    struct CompatibilityHelper;

    // if one of the variable matches is empty we automatically succeed
    // and use the other list of matches
    template< typename... LeftMatches >
    struct CompatibilityHelper< tuple< LeftMatches... >, tuple< >, seq< >>:
        integral_constant< bool, true >
    { using matches_type = tuple< LeftMatches... >; };

    template< typename... RightMatches >
    requires( is_greater( sizeof...( RightMatches ), 0 )) // remove ambiguity
    struct CompatibilityHelper< tuple< >, tuple< RightMatches... >, seq< >>:
        integral_constant< bool, true >
    { using matches_type = tuple< RightMatches... >; };

    // neither side is empty
    template< typename LeftMatches, typename RightMatches, size_t... Is >
    requires( 
        is_greater( tuple_size_v< typename LeftMatches::matches_type >, 0 ) and
        is_greater( tuple_size_v< typename RightMatches::matches_type >, 0 ))
    struct CompatibilityHelper< LeftMatches, RightMatches, seq< Is... >>
    {
        static constexpr size_t left_matches_size = tuple_size_v< typename
            LeftMatches::matches_type >;

        static constexpr size_t right_matches_size = tuple_size_v< typename 
            RightMatches::matches_type >;

        template< size_t I >
        using left_match_t = tuple_element_t< I / right_matches_size, typename 
            LeftMatches::matches_type >;

        template< size_t I >
        using right_match_t = tuple_element_t< I % right_matches_size, typename
            RightMatches::matches_type >;

        // if the variable indices match the corresponding expression types must be equal
        static constexpr bool value = ((
            match_variable_t< left_match_t< Is >>::id != 
                match_variable_t< right_match_t< Is >>::id or is_same_v<
                    match_expression_t< left_match_t< Is >>,
                        match_expression_t< right_match_t< Is >>> ) and ... );

        template< typename SeqLeft, typename SeqRight >
        struct UniqueMatchesHelper;

        template< size_t... Js, size_t... Ks >
        struct UniqueMatchesHelper< seq< Js... >, seq< Ks... >>:
            UniqueMatches< left_match_t< Js >..., right_match_t< Ks >... >
        { };

        // our matches are the unique set of matches from the combined left and right matches
        using matches_type = UniqueMatchesHelper< make_seq< left_matches_size >, 
            make_seq< right_matches_size >>::matches_type;
    };

    template< typename LeftMatches, typename RightMatches >
    requires( not LeftMatches::value or not RightMatches::value )
    struct AreArgumentMatchesCompatible< LeftMatches, RightMatches >:
        integral_constant< bool, false >
    { using matches_type = tuple<>; };

    template< typename LeftMatches, typename RightMatches >
    requires( LeftMatches::value and RightMatches::value )
    struct AreArgumentMatchesCompatible< LeftMatches, RightMatches >:
        CompatibilityHelper< LeftMatches, RightMatches,
            make_seq< tuple_size_v< typename LeftMatches::matches_type > * 
                tuple_size_v< typename RightMatches::matches_type >>>
    { };

    // when we have more than two variable matching tuples to compare, check
    // the tail.  if that is compatible, compare those matches to the first
    template< typename First, typename... Rest >
    requires( is_greater( sizeof...( Rest ), 1 ) and 
        AreArgumentMatchesCompatible< Rest... >::value )
    struct AreArgumentMatchesCompatible< First, Rest... >:
        AreArgumentMatchesCompatible< First, AreArgumentMatchesCompatible< Rest... >>
    { };

    // when the tail is incompatible simply evaluate to false with an empty
    // tuple of matches
    template< typename First, typename... Rest >
    requires( is_greater( sizeof...( Rest ), 1 ) and not
        AreArgumentMatchesCompatible< Rest... >::value )
    struct AreArgumentMatchesCompatible< First, Rest... >:
        integral_constant< bool, false >
    { using matches_type = tuple<>; };
        
    // helper allows us to compare the arguments of this operation pairwise
    template< typename Seq, typename... TestArgs >
    struct Helper;

    // helper recurses when our operation matches the operation of the
    // expression being tested.  
    template< size_t... Js, typename... TestArgs >
    struct Helper< seq< Js... >, TestArgs... >: 
        AreArgumentMatchesCompatible< typename 
            ForExpression< std::tuple_element_t< Js, arguments_tuple >>::
                template Is< TestArgs...[ Js ]>... >
    { };

public:
    // by default we will not match TestT
    template< typename TestT >
    struct Is: integral_constant< bool, false > 
    { using matches_type = tuple<>; };

    // this predicate requires the operation of the compound expression to match
    // the helper will determine if the arguments match.
    template< typename... TestArgs >
    requires( std::tuple_size_v< arguments_tuple > == sizeof...( TestArgs ))
    struct Is< Op< TestArgs... >>:
        Helper< make_seq< sizeof...( Args )>, TestArgs... > 
    { };
};


/// @brief expression manipulator that replaces any argument in an expression
/// with the given WithT argument.
///
/// @tparam Predicate resolves to an integral_constant< bool, ... > like
/// like object-type (ie: has a constexpr static bool value member) which
/// flags the expression passed as the template parameter as substitutable
///
///
/// Case: Default is idempotent
///
template< template< typename > class Predicate, typename ExprT, typename WithT >
struct PredicateSubstitution
{
    using type = ExprT;
    static constexpr type value( ExprT const& expr, WithT const& )
    { return expr; }
};
/// 
/// Case: Predicate matched expression.  We resolve to our replacement
///
template< template< typename > class Predicate, typename ExprT, typename WithT >
requires( Predicate< ExprT >::value )
struct PredicateSubstitution< Predicate, ExprT, WithT >
{
    using type = WithT;
    static constexpr type value( ExprT const& expr, WithT const& with )
    { return with; } 
};
///
/// Case: Unmatched non-compound expression is idempotent
///
template< template< typename > class Predicate, typename NonCompoundExprT, 
    typename WithT >
requires( not Predicate< NonCompoundExprT >::value and 
    not requires { typename NonCompoundExprT::arguments_tuple; })
struct PredicateSubstitution< Predicate, NonCompoundExprT, WithT >
{
    using type = NonCompoundExprT;
    static constexpr type value( NonCompoundExprT const& expr, WithT const& with )
    { return expr; }
};
///
/// Case: Unmatched compound expression recurses
///
template< template< typename > class Predicate, 
    template< typename... > class Op, typename... Args, typename WithT >
requires( compound_expression< Op< Args... >> and 
    not Predicate< Op< Args... >>::value )
struct PredicateSubstitution< Predicate, Op< Args... >, WithT > {
private:
    template< typename ArgT >
    using ArgumentSub = PredicateSubstitution< Predicate, ArgT, WithT >;

    template< typename ArgT >
    static constexpr typename ArgumentSub< ArgT >::type sub_argument( 
        ArgT const& arg, WithT const& with )
    { return ArgumentSub< ArgT >::value( arg, with ); }

public:
    using type = Op< typename ArgumentSub< Args >::type... >;

private:
    template< size_t... Is >
    static constexpr type value_helper( 
        Op< Args... > const& op, WithT const& with, seq< Is... > )
    { return { sub_argument( get_argument< Is >( op ), with )... }; }
public:

    static constexpr type value( Op< Args... > const& op, WithT const& with )
    { return value_helper( op, with, make_seq< sizeof...( Args )>{} ); }
};

template< typename ExprT, variable Var, expression SubU >
struct SubstitutionFor: PredicateSubstitution< 
    ForVariable< Var::id >::template Is, ExprT, SubU >
{ };

/////////////////////
/// Substitution ///
///////////////////
/// 
template< typename ExprT, typename... Args >
struct Substituter;

/// @brief expression manipulator for substitution of first order variables
///
template< typename ExprT, typename... Args >
//requires( sizeof...( Args ) == free_variables_t< ExprT >::size )
requires( is_compatible_substitution_v< ExprT, Args... > ) //and
//    ( not is_variable_v< ExprT > or variable_order_v< ExprT > == 1 ))
struct Substituter< ExprT, Args... > {
private:
    using expression_type = ExprT;
    using variables = free_variables_t< expression_type >;

    template< typename ExprU, typename Seq >
    struct Helper;

    // null case for substitution of no arguments into an expression
    template< typename ExprU >
    struct Helper< ExprU, seq< >>
    {
        using type = ExprU;
        static constexpr type value( ExprU const& expr, Args const&... args )
        { return expr; }
    };

    // Ith argument substitution case is recursively defined
    template< typename ExprU, size_t I, size_t... Is >
    struct Helper< ExprU, seq< I, Is... >>
    {
    private:
        // fetch the id of the Ith variable
        static constexpr size_t Ith = variables::template element_t< I >::id;

        // alias for the Ith argument substitutor
        using ith_substituter = PredicateSubstitution< 
            ForVariable< Ith >::template Is, ExprU, Args...[ I ]>;

    public:
        // recursively define our resultant type
        using type = Helper< typename ith_substituter::type, seq< Is... >>::
            type;

        // recursively define our final substitution value
        static constexpr type value( ExprU expr, Args const&... args )
        { return Helper< typename ith_substituter::type, seq< Is... >>::value( 
            ith_substituter::value( expr, args...[ I ]), args... ); }
    };

    using helper_type = Helper< expression_type, make_seq< sizeof...( Args )>>;

public:
    // Type Manipulator Requirements //
    using type = helper_type::type;

    static constexpr type value( expression_type const& expr, 
        Args const&... args )
    { return helper_type::value( expr, args... ); }
    // //
};

/// @brief substitution for higher-order variables results in an expression 
template< typename ExprT, typename... Subs >
//requires( is_compatible_substitution_v< ExprT, Subs... > )
struct Substitution: Arguments< Substitution, ExprT, Subs... >
{
    using expression_type = ExprT;
    static constexpr make_seq< sizeof...( Subs )> for_arguments;

    // Compound Expression Requirements //
    using result_type = result_t< expression_type >;

    constexpr expression_type 
    expression() const
    { return get_argument< 0 >( *this ); }

    template< size_t I >
    constexpr Subs...[ I ]
    arg() const { return get_argument< I + 1 >( *this ); }

    template< typename ExprU, typename... OtherSubs >
    static constexpr typename Substituter< ExprU, OtherSubs... >::type 
    value( ExprU expr, OtherSubs... other_subs )
    { return substitute( expr, other_subs... ); } 

    constexpr typename Substituter< expression_type, Subs... >::type 
    value() const
    { 
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr
        { return substitute( expression(), arg< Is >()... ); };

        return helper( for_arguments );
    };
    
    constexpr Substitution() = default;
    constexpr Substitution( expression_type v, Subs... subs ):
        Arguments< Substitution, ExprT, Subs... >{ v, subs... } { }
    // //
    
};

template< template< typename... > class Op, typename... Args >
template< typename... Subs >
requires( is_compatible_substitution_v< Op< Args... >, Subs... > )
constexpr Substitution< Op< Args... >, Subs... >
Arguments< Op, Args... >::operator ()( Subs... subs ) const
{ return { expression(), subs... }; }

// when evaluating a substitution, apply the substitution then continue evaluating
template< typename ExprT, typename... Subs >
constexpr auto Evaluator< void >::
operator ()( Substitution< ExprT, Subs... > const& sub_expr ) const
{ return sub_expr.value() | *this; }

template< typename ScopeT >
template< typename ExprT, typename... Subs >
constexpr auto Evaluator< ScopeT >::
operator ()( Substitution< ExprT, Subs... > const& sub_expr ) const
{ return sub_expr().value() | *this; }

template< variable Var, expression ExprT, expression SubU >
constexpr typename SubstitutionFor< ExprT, Var, SubU >::type
substitute_for( ExprT const& expr, Var, SubU const& sub )
{ return SubstitutionFor< ExprT, Var, SubU >::value( expr, sub ); }

template< variable Var, expression ExprT, expression SubU >
constexpr typename SubstitutionFor< ExprT, Var, SubU >::type
substitute_for( ExprT const& expr, SubU const& sub )
{ return SubstitutionFor< ExprT, Var, SubU >::value( expr, sub ); }

/// @brief substitutes arguments for the dependent variables of an expression
///
/// @tparam ExprT type of the expression to be substituted into
/// @tparam Args... types of the args being substituted
/// @param expr is the instance of the original expression
/// @param args... are the 
template< typename ExprT, typename... Args >
constexpr typename Substituter< ExprT, Args... >::type 
substitute( ExprT expr, Args... args )
{ return Substituter< ExprT, Args... >::value( expr, args... ); }

namespace test {
} // namespace test

//////////////////////////////
/// Canonical Expressions ///
////////////////////////////
/// 
/// Expressions must have a canonical format which users associativity, commutativity
/// and distribution rules to put expressions into a form that can be manipulated 
/// easily

/// @brief Canonicalizer is a type-manipulator.  The unspecialized implementation is
/// idempotent
//template< typename ExprT >
//struct Canonicalizer
//{ 
//    using type = ExprT;
//    static constexpr type value( ExprT const& expr )
//    { return expr; }
//};
//
///// @brief method to invoke the Canonicalizer type-manipulator
//template< typename ExprT >
//constexpr typename Canonicalizer< ExprT >::type
//canonicalize( ExprT const& expr )
//{ return Canonicalizer< ExprT >::value( expr ); }

/////////////////
/// Visitors ///
///////////////
///
/// Reconstructs an expression of type ExprT using the Visitor template-template
/// argument as a manipulator.  Visitor must meet the following criteria
/// - typename Visitor< I, ExprT >::type is the type of the expression replacing 
///   ExprT AND
/// - static type Visitor< I, ExprT >::value( ExprT const& ) returns the value 
///   of the replacing expression 
///
/// I represents an index of expression visited.  This parameter allows for 
/// unique variables to be declared during the parsing of an expression.  This 
/// is needed for canonicalization of comparison operations which must be 
/// transformed to EqualsZero expressions.
/// 
template< template< size_t, expression > class Visitor, 
          expression ExprT, 
          size_t Start = 0 >
class DepthFirst;

/// Leaf case for the parser is a standard type-manipulator that also includes
/// a static size_t size member representing the expressions visited in this
/// subtree, which is 1 since this is a terminal expression.
template< template< size_t, expression > class Visitor, 
          expression ExprT,
          size_t Start >
requires( not compound_expression< ExprT >)
class DepthFirst< Visitor, ExprT, Start > {
public:
    // we visit exactly one expression at this level, this one
    static constexpr size_t size = 1;

    // our new type is given by the visitor's type member 
    using type = Visitor< Start, ExprT >::type;

    // and the value is given by the visitor's value(ExprT const&) static
    // member function.
    static constexpr type value( ExprT const& expr )
    { return Visitor< Start, ExprT >::value( expr ); }
};

/// Compoound case for the parser must calculate the size_t by summing
/// the sizes of the argument expressions
template< template< size_t, expression > class Visitor, 
          template< expression... > class Op, expression... Args,
          size_t Start >
requires compound_expression< Op< Args... >> // TODO: this could be removed?
class DepthFirst< Visitor, Op< Args... >, Start > {
public:
    // the count of expressions visited at this stage.
    // NOTE: this must be independent of the Start template parameter. We
    //       pass zero for this parameter to signal it's required independence.
    // NOTE: we add 1 to represent the eventual visitation of this, 
    //       Op< Args... > typed, expression after visiting the arguments.
    static constexpr size_t size = 
        ( DepthFirst< Visitor, Args, 0 >::size + ... + 1 );

private:
    // type manipulator which calculates the start index for our DepthFirst
    // parse of ExprT and recurses our DepthFirst visitation.
    template< size_t I >
    class Argument 
    {
        template< typename Seq >
        struct Helper;

        // we sum the expressions visited by each Args...[ Is < I ] to 
        // determine the offset to Start for each Args...[ I ]
        // NOTE: we do not add 1 here since this, Op< Args... > typed
        //       expression will not be visited until after it's arguments. In
        //       this way when we finally do visit Op< Args... > it will also
        //       have a unique, increasing, index
        template< size_t... Is >
        struct Helper< seq< Is... >>
        { static constexpr size_t start = 
            ( Start + ... + DepthFirst< Visitor, Args...[ Is ], 0 >::size ); };

    public:
        // our value is the helper value for the sequence [ 0...I )
        static constexpr size_t start = Helper< make_seq< I >>::start;

        using type = DepthFirst< Visitor, Args...[ I ], start >::type;

        static constexpr type value( Args...[ I ] const& arg )
        { return DepthFirst< Visitor, Args...[ I ], start >::value( arg ); } 
    };

    template< typename Seq >
    struct Helper;

    // helper to visit each argument and recurse our visitor
    template< size_t... Is >
    struct Helper< seq< Is... >>
    {
        // type of our visitor, post argument visitation
        using op_visitor = Visitor< Start + size - 1,
            Op< typename Argument< Is >::type... >>;

        // our final type will be determined by the visitor, assuming we 
        // have already visited the arguments
        using type = op_visitor::type;

        // the value is similarly calculated by first visiting the arguments
        // then combining 
        static constexpr type value( Op< Args... > const& expr )
        { 
            // we construct a temporary operation by visiting each argument
            Op< typename Argument< Is >::type... > arguments_visited = 
                { Argument< Is >::value( get_argument< Is >( expr ))... };
            
            // finally we visit our re-constructed operation, post visitation
            return op_visitor::value( arguments_visited );
        }
    };

public:
    // we leverage our helper and an index sequence for our arguments
    using type = Helper< make_seq< sizeof...( Args )>>::type;

    static constexpr type value( Op< Args... > const& expr )
    { return Helper< make_seq< sizeof...( Args )>>::value( expr ); }
};

/// @brief method for indexed visiting of expressions
template< template< template< size_t, typename > class, typename, size_t > 
          class                                Route, 
          size_t                               Start,
          template< size_t, expression > class Visitor,
          expression                           ExprT >
constexpr typename Route< Visitor, ExprT, Start >::type
visit( ExprT const& expr )
{ return Route< Visitor, ExprT, Start >::value( expr ); }

////////////////////////////////
/// Bootstrapping Completed ///
//////////////////////////////
/// 
/// Our bootstrapping is complete.  From now on variables and expressions
/// should be tested for by their concepts, and inspected via their _traits
///

///////////////
/// Derivations
///
/// Mathematical operators like derivations which transform expressions are
/// implemented as manipulators

struct Derivation
{ };

namespace detail {
template< typename D >
struct IsDerivation: integral_constant< bool,
    std::is_base_of_v< Derivation, D >> { };
};

template< typename D >
concept derivation = detail::IsDerivation< D >::value;

///////////////////
/// Operations ///
/////////////////
///
///////////////////////////////
/// Conditional Expression ///
/////////////////////////////
/// 
template< typename T >
constexpr T if_( bool cond, T true_value, T false_value = {} )
{ return cond ? true_value : false_value; }

template< typename ConditionT, typename TrueResultT,
    typename FalseResultT >
requires( is_same_v< result_t< ConditionT >, bool > and
    is_same_v< result_t< TrueResultT >, result_t< FalseResultT >> )
struct Conditional;

template< expression ConditionT, typename TrueResultT, typename FalseResultT >
constexpr Conditional< ConditionT, TrueResultT, FalseResultT >
if_( ConditionT condition, TrueResultT true_result, FalseResultT false_result );

template< typename ConditionT, typename TrueResultT,
    typename FalseResultT >
requires( is_same_v< result_t< ConditionT >, bool > and
    is_same_v< result_t< TrueResultT >, result_t< FalseResultT >> )
struct Conditional: Arguments< Conditional, ConditionT, 
    TrueResultT, FalseResultT >
{
    using condition_type = ConditionT;
    using true_result_type = TrueResultT;
    using false_result_type = FalseResultT;

    constexpr condition_type condition() const 
    { return get_argument< 0 >( *this ); }

    constexpr true_result_type true_result() const
    { return get_argument< 1 >( *this ); }

    constexpr false_result_type false_result() const
    { return get_argument< 2 >( *this ); }

    template< typename C, typename T, typename F >
    static constexpr auto value( C cond, T true_case, F false_case )
    { return if_( cond, true_case, false_case ); }

    constexpr Conditional() = default;
    constexpr Conditional( condition_type condition,
        true_result_type true_result, false_result_type false_result ): 
        Arguments< Conditional, ConditionT, TrueResultT, FalseResultT >{ 
            condition, true_result, false_result }
    { }
};

template< expression ConditionT, typename TrueResultT, typename FalseResultT >
constexpr Conditional< ConditionT, TrueResultT, FalseResultT >
if_( ConditionT condition, TrueResultT true_result, FalseResultT false_result )
{ return { condition, true_result, false_result }; }

///////////////////
/// Element Of ///
/////////////////
///
/// @brief Element Of operation
template< size_t I >
struct Element
{
    template< typename ArrayT >
    struct Of;
};

template< size_t I >
template< typename ArrayT >
requires( not tensor< result_t< ArrayT >> )
struct Element< I >::Of< ArrayT >: Arguments< Of, ArrayT >
{
    using result_type = tuple_element_t< I, result_t< ArrayT >>;

    template< typename ArrayU >
    static constexpr auto value( ArrayU const& arr )
    { return std::get< I >( arr ); }
    
    constexpr ArrayT arg() const { return get_argument< 0 >( *this ); }

    constexpr Of( ArrayT const& arr ): Arguments< Of, ArrayT >{ arr } { };
    constexpr Of() = default;
};

template< size_t I >
template< typename ArrayT >
requires( tensor< result_t< ArrayT >> )
struct Element< I >::Of< ArrayT >: Arguments< Of, ArrayT >
{
    using result_type = tensor_element_t< I, result_t< ArrayT >>;

    template< typename ArrayU >
    static constexpr auto value( ArrayU const& arr )
    { return std::get< I >( arr ); } 

    constexpr ArrayT arg() const { return std::get< 0 >( *this ); }
    
    constexpr Of( ArrayT const& arr ): Arguments< Of, ArrayT >{ arr } { };
    constexpr Of() = default;
};

template< size_t I, typename T >
using element_of = Element< I >::template Of< T >;

template< size_t I, typename T >
constexpr element_of< I, T > element( T const& arr )
{ return { arr }; }

/////////////////
/// Negation ///
///////////////
///
/// @brief negation expression
/// @tparam T the negated type
///
template< typename T >
struct Negation: Arguments< Negation, T >
{ 
    using result_type = decltype( -result_t< T >{} );
    
    template< typename U >
    static constexpr auto value( U const& arg )
    { return -arg; }

    constexpr T arg() const 
    { return get_argument< 0 >( *this ); }

    // derivative of a negation is the negation of the derivative
    template< derivation D >
    constexpr auto operator |( D const& d ) const
    { return -( arg() | d ); }

    constexpr Negation( T arg ): Arguments< Negation, T >{ arg } { } 
    constexpr Negation() = default;
};

//static_assert(( Negation< Variable< 0, int >>{} | simple_scope( 5 )) == -5 );

////////////
/// Sum ///
//////////
///
template< typename... Ts >
struct Sum;

/// @brief sum expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct Sum< T, U >: Arguments< Sum, T, U >
{ 
    using result_type = decltype( result_t< T >{} + result_t< U >{} );

    constexpr T left_arg() const { return get_argument< 0 >( *this ); }
    constexpr U right_arg() const { return get_argument< 1 >( *this ); }

    template< typename V, typename W >
    static constexpr auto value( V const& left, W const& right )
    { return left + right; }

    // derivative of a sum is the sum of the derivative
    template< derivation D >
    constexpr auto operator |( D const& d ) const
    { return ( left_arg() | d ) + ( right_arg() | d ); } 

    constexpr Sum( T left, U right ): Arguments< Sum, T, U >{ left, right } { } 
    constexpr Sum() = default;
};

static_assert( compound_expression< Sum< Variable< 0, int >, Variable< 1, int >>> );
static_assert( is_same_v< free_variables_t< Sum< Variable< 0, int >, Variable< 1, int >>>,
    unique_variables< Variable< 0, int >, Variable< 1, int >>> ); 
static_assert( free_variables_t< Sum< Variable< 0, int >, Variable< 1, int >>>::size == 2 );
static_assert( requires { typename ForExpression< Sum< Variable< 0, int >, Variable< 1, int >>>; } );

template< typename... Ts >
requires( is_greater( sizeof...( Ts ), 2 ))
struct Sum< Ts... >: Arguments< Sum, Ts... >
{
    using result_type = decltype(( result_t< Ts >{} + ... ));

    template< size_t I >
    constexpr Ts...[ I ] arg() const { return get_argument< I >( *this ); }

    template< typename... Us >
    static constexpr auto value( Us const&... us )
    { return ( us + ... ); }

    // derivative of a sum is the sum of the derivative
    template< derivation D >
    constexpr auto operator |( D const& d ) const
    {
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
        { return (( arg< Is >() | d ) + ... ); };

        return helper( make_seq< sizeof...( Ts )>{} );
    }

    constexpr Sum( Ts const&... ts ): Arguments< Sum, Ts... >{ ts... } { }
    constexpr Sum() = default;
};

///////////////////
/// Difference ///
/////////////////
///
template< typename... Ts >
struct Difference;

/// @brief difference expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct Difference< T, U >: Arguments< Difference, T, U >
{ 
    using result_type = decltype( result_t< T >{} - result_t< U >{} );

    constexpr T left_arg() const { return get_argument< 0 >( *this ); }
    constexpr U right_arg() const { return get_argument< 1 >( *this ); }

    template< typename V, typename W >
    static constexpr auto value( V const& left, W const& right )
    { return left - right; }

    // derivative of a sum is the sum of the derivative
    template< derivation D >
    constexpr auto operator |( D const& d ) const
    { return ( left_arg() | d ) - ( right_arg() | d ); } 

    constexpr Difference( T left, U right ): 
        Arguments< Difference, T, U >{ left, right } { } 
    constexpr Difference() = default;
};

template< typename... Ts >
requires( is_greater( sizeof...( Ts ), 2 ))
struct Difference< Ts... >: Arguments< Difference, Ts... >
{
    using result_type = decltype(( result_t< Ts >{} - ... ));

    template< size_t I >
    constexpr Ts...[ I ] arg() const { return get_argument< I >( *this ); }

    template< typename... Us >
    static constexpr auto value( Us const&... us )
    { return ( us - ... ); }

    // derivative of a difference is the difference of the derivative
    template< derivation D >
    constexpr auto operator |( D const& d ) const
    {
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
        { return (( arg< Is >() | d ) - ... ); };

        return helper( make_seq< sizeof...( Ts )>{} );
    }

    constexpr Difference( Ts const&... ts ): 
        Arguments< Difference, Ts... >{ ts... } { }
    constexpr Difference() = default;
};

template< typename... >
struct Product;

/// @brief product expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct Product< T, U >: Arguments< Product, T, U > 
{ 
    using result_type = decltype( result_t< T >{} * result_t< U >{} );

    constexpr T left_arg() const { return get_argument< 0 >( *this ); }
    constexpr U right_arg() const { return get_argument< 1 >( *this );; }

    template< typename V, typename W >
    static constexpr auto value( V const& left, W const& right )
    { return ( left * right ); }

    // product rule
    template< derivation D >
    constexpr auto operator |( D const& d ) const
    { return ( left_arg() | d ) * right_arg() + left_arg() * ( right_arg() | d ); } 

    constexpr Product( T left, U right ): 
        Arguments< Product, T, U >{ left, right } { }
    constexpr Product() = default;
};

template< typename T, typename... Ts >
requires( is_greater( sizeof...( Ts ), 1 ))
struct Product< T, Ts... >: Arguments< Product, T, Ts... >
{
    using result_type = decltype( result_t< T >{} * ( result_t< Ts >{} * ... ));

    template< size_t I >
    constexpr Ts...[ I ] arg() const 
    { return get_argument< I >( *this ); }

    constexpr Ts...[ 0 ] first() const 
    { return arg< 0 >(); }

    constexpr Product< Ts... > rest() const
    { 
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr ->
            Product< Ts... >
        { return { arg< Is + 1 >()... }; };

        return helper( make_seq< sizeof...( Ts )>{} );
    }

    template< typename... Us >
    static constexpr auto value( Us const&... us )
    { return ( us * ... ); }

    template< derivation D >
    constexpr auto operator |( D const& d ) const
    { return ( first() | d ) * rest() + first() * ( rest() | d ); }

    constexpr Product( Ts const&... ts ): 
        Arguments< Product, Ts... >{ ts... } { }
    constexpr Product() = default;
};

static_assert( is_compatible_substitution_v< Product< StaticValue< int >, Variable< 0, float >>, float >,
    "FAILURE: substitution into product" );

/// @brief quotient expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct Quotient: Arguments< Quotient, T, U >
{ 
    using result_type = decltype( result_t< T >{} / result_t< U >{} );

    constexpr T numerator_arg() const { return get_argument< 0 >( *this ); }
    constexpr U denominator_arg() const { return get_argument< 1 >( *this ); }

    template< typename V, typename W >
    static constexpr auto value( V const& left, W const& right )
    { return ( left / right ); }

    // quotient rule
    template< derivation D >
    constexpr auto operator |( D const& d ) const
    { return ( numerator_arg() * ( denominator_arg() | d ) - 
        ( numerator_arg() | d ) * denominator_arg() ) / 
            ( denominator_arg() * denominator_arg() ); }

    constexpr Quotient( T numerator, U denominator ):
        Arguments< Quotient, T, U >{ numerator, denominator } { }
    constexpr Quotient() = default;
};

/// @brief square root expression
/// @tparam T 
template< typename T >
struct SquareRoot: Arguments< SquareRoot, T >
{
    using result_type = decltype( std::sqrt( result_t< T >{} ));

    constexpr T arg() const { return get_argument< 0 >( *this ); }

    template< typename U >
    static constexpr auto value( U const& arg )
    { return std::sqrt( arg ); }

    template< derivation D >
    constexpr auto operator |( D const& d ) const
    { return 0.5l / sqrt( arg() ) * ( arg() | d ); }

    constexpr SquareRoot( T arg ):  
        Arguments< SquareRoot, T >{ arg } { }
    constexpr SquareRoot() = default;
};

/// @brief integral power expression
/// @tparam T 
/// @tparam Exp 
template< int Exp >
struct Power
{
    static constexpr int exponent = Exp;

    template< typename T >
    struct Of: Arguments< Of, T >
    {
        using result_type = decltype( std::pow< Exp >( result_t< T >{} ));

        constexpr T arg() const { return get_argument< 0 >( *this ); }

        template< typename U >
        static constexpr auto value( U const& arg )
        { return std::pow< Exp >( arg ); }

        template< derivation D >
        constexpr auto operator |( D const& d ) const
        { return exponent * pow< Exp - 1 >( arg() ) * ( arg() | d ); }

        constexpr Of( T arg ): Arguments< Of, T >{ arg } {} 
        constexpr Of() = default;
    };
};

template< size_t Exp, typename T >
using power_of = Power< Exp >::template Of< T >;

/// @brief sine expression
/// @tparam T 
template< typename T >
struct Sine: Arguments< Sine, T >
{
    using result_type = decltype( std::sin( result_t< T >{} ));

    constexpr T arg() const { return get_argument< 0 >( *this ); }

    template< typename U >
    static constexpr auto value( U const& arg )
    { return std::sin( arg ); }

    template< derivation D >
    constexpr auto operator |( D const& d ) const
    { return cos( arg() ) * ( arg() | d ); }

    constexpr Sine( T arg ): Arguments< Sine, T >{ arg } { } 
    constexpr Sine() = default;
};

/// @brief cosine expression
/// @tparam T 
template< typename T >
struct Cosine: Arguments< Cosine, T >
{
    using result_type = decltype( std::cos( result_t< T >{} ));

    constexpr T arg() const { return get_argument< 0 >( *this ); }

    template< typename U >
    static constexpr auto value( U const& arg )
    { return std::cos( arg ); }

    template< derivation D >
    constexpr auto operator |( D const& d ) const
    { return -sin( arg() ) * ( arg() | d ); }

    constexpr Cosine( T arg ): Arguments< Cosine, T >{ arg } { } 
    constexpr Cosine() = default;

    T _arg;
};

/// @brief tangent expression
/// @tparam T 
template< typename T >
struct Tangent: Arguments< Tangent, T >
{
    using result_type = decltype( std::tan( result_t< T >{} ));

    constexpr T arg() const { return get_argument< 0 >( *this ); }

    template< typename U >
    static constexpr auto value( U const& arg )
    { return std::tan( arg ); }

    template< derivation D >
    constexpr auto operator |( D const& d ) const
    { return ( arg() | d ) / ( cos( arg() ) * cos( arg() )); } 

    constexpr Tangent( T arg ): Arguments< Tangent, T >{ arg } { } 
    constexpr Tangent() = default;
};

/// @brief arcsine expression
/// @tparam T 
template< typename T >
struct Arcsine: Arguments< Arcsine, T >
{
    using result_type = decltype( std::asin( result_t< T >{} ));

    constexpr T arg() const { return get_argument< 0 >( *this ); }

    template< typename U >
    static constexpr auto value( U const& arg )
    { return std::asin( arg ); }

    template< derivation D >
    constexpr auto operator |( D const& d ) const
    { return ( arg() | d ) / sqrt( 1l - pow< 2 >( arg() )); }

    constexpr Arcsine( T arg ): Arguments< Arcsine, T >{ arg } { } 
    constexpr Arcsine() = default;
};

/// @brief arccosine expression
/// @tparam T 
template< typename T >
struct Arccosine: Arguments< Arccosine, T >
{
    using result_type = decltype( std::acos( result_t< T >{} ));

    constexpr T arg() const { return get_argument< 0 >( *this ); }

    template< typename U >
    static constexpr auto value( U const& arg )
    { return std::acos( arg ); }

    template< derivation D >
    constexpr auto operator |( D const& d ) const 
    { return -( arg() | d ) / sqrt( 1l - pow< 2 >( arg() )); }

    constexpr Arccosine( T arg ): Arguments< Arccosine, T >{ arg } { } 
    constexpr Arccosine() = default;
};

/// @brief sine expression
/// @tparam T 
template< typename T >
struct Arctangent: Arguments< Arctangent, T >
{
    using result_type = decltype( std::atan( result_t< T >{} ));

    constexpr T arg() const { return get_argument< 0 >( *this ); }

    template< typename U >
    static constexpr auto value( U const& arg )
    { return std::atan( arg ); }

    template< derivation D >
    constexpr auto operator |( D const& d ) const
    { return ( arg() | d ) / ( 1l + pow< 2 >( arg() )); }

    constexpr Arctangent( T arg ): Arguments< Arctangent, T >{ arg } { } 
    constexpr Arctangent() = default;
};

/// @brief arctangent of a slope expression
/// @tparam T rise type
/// @tparam U run type
template< typename T, typename U >
struct Arctangent2: Arguments< Arctangent2, T, U >
{ 
    // we use the result_type of a fraction here to factor units properly
    // this assumes that std::atan2 doesn't change the unit. hopefully it stays
    // true that trig functions operate only on scalars and this won't be an 
    // issue.  
    using result_type = decltype( result_t< T >{} / result_t< U >{} );

    constexpr T numerator_arg() const { return get_argument< 0 >( *this ); }
    constexpr U denominator_arg() const { return get_argument< 1 >( *this ); }

    // TODO: write an eval for std::atan2 that handles units properly
    template< typename V, typename W >
    static constexpr auto value( V const& num, W const& den );

    template< derivation D >
    constexpr auto operator |( D const& d ) const;

    constexpr Arctangent2( T numerator, U denominator ):
        Arguments< Arctangent2, T, U >{ numerator, denominator } { }
    constexpr Arctangent2() = default;
};

/// @brief extract the element from a tuple-like array
/// @tparam ArrayT 
/// @tparam I 
// template< size_t I, typename ArrayT >
// struct Element: tuple_element_t< I, ArrayT >
// {  
//     constexpr Element( ArrayT arr ): 
//         tuple_element_t< I, ArrayT >{ std::get< I >( arr ) }
//     { }
// };
//

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

/// @brief logical and expression
/// @tparam Ts... 
template< typename... Ts >
struct Conjunction: Arguments< Conjunction, Ts... >
{
    static constexpr size_t arguments_size() { return sizeof...( Ts ); }
    using result_type = bool;

    template< size_t I >
    constexpr Ts...[ I ] arg() const
    { return get_argument< I >( *this ); }

    template< typename... Us >
    static constexpr auto value( Us const&... us )
    { return ( us and ... ); }

    constexpr Conjunction( Ts... ts ): Arguments< Conjunction, Ts... >{ ts... } { } 
    constexpr Conjunction() = default;
};

/// @brief logical or expression
/// @tparam Ts...
template< typename... Ts >
struct Disjunction: Arguments< Disjunction, Ts... >
{
    static constexpr size_t arguments_size() { return sizeof...( Ts ); }
    using result_type = bool;

    template< size_t I >
    constexpr Ts...[ I ] arg() const
    { return get_argument< I >( *this ); }

    template< typename... Us >
    static constexpr auto value( Us const&... us )
    { return ( us or ... ); }

    constexpr Disjunction( Ts... ts ): Arguments< Disjunction, Ts... >{ ts... } { } 
    constexpr Disjunction() = default;
};

/// @brief logical not expression
/// @tparam T 
/// @tparam U 
template< typename T >
struct Compliment: Arguments< Compliment, T >
{
    using result_type = bool;

    constexpr T arg() const { return get_argument< 0 >( *this ); }

    template< typename U >
    static constexpr auto value( U const& arg )
    { return not arg; }

    constexpr Compliment( T arg ): Arguments< Compliment, T >{ arg } { } 
    constexpr Compliment() = default;
};

// predicate expression tests
//#ifndef NDEBUG
namespace test {

static_assert( ForExpression< Variable< 0, int >>::template 
    Is< Constant< 5 >>::value );
static_assert( not ForExpression< Variable< 0, int >>::template 
    Is< Constant< units::Length{ 5 }>>::value );

static_assert( ForExpression< Sum< Variable< 0, int >, Variable< 1, int >>>::
    template Is< Sum< Constant< 5 >, Constant< 6 >>>::value, 
        "FAILED: <int> + <int> =matches=> 5 + 6" );

static_assert( not ForExpression< Sum< Variable< 0, int >, Variable< 1, int >>>::template Is<
    Sum< Constant< units::Length{ 5 } >, Constant< units::Length{ 6 } >>>::value, 
        "FAILED: <int> + <int> =not-matches=> 5m + 6m" );

static_assert( not ForExpression< Sum< Variable< 0, int >, Variable< 1, int >>>::template Is<
    Difference< Constant< 5 >, Constant< 6 >>>::value,
        "FAILED: <int> + <int> =not-matches=> 5 - 6" );

static_assert( ForExpression< Sum< Variable< 0, int >, Variable< 0, int >>>::template Is<
    Sum< Constant< 5 >, Constant< 5 >>>::value, 
        "FAILED: <int[0]> + <int[0]> =matches=> 5 + 5" );

static_assert( not ForExpression< Sum< Variable< 0, int >, Variable< 0, int >>>::template Is<
    Sum< Constant< 5 >, Constant< 7 >>>::value, 
        "FAILED: <int[0]> + <int[0]> =not-matches=> 5 + 7" );

static_assert( is_same_v< tuple_element_t< 0, typename ForExpression< 
    Sum< Variable< 0, int >, Variable< 0, int >>>::template Is<
        Sum< Constant< 5 >, Constant< 5 >>>::matches_type >, 
            match< Variable< 0, int >, Constant< 5 >>> );

} // namespace test
//#endif // DEBUG

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

///////////////
/// Aggregates
///
// 
// template< typename ExprT >
// struct Minimum: Iterative 
// {
//     using expression_type = ExprT;
//     using arguments_tuple = tuple< expression_type >;
//     using result_type = result_t< ExprT >;
// 
//     constexpr expression_type expr() const { return _expr; }
// 
//     constexpr Minimum( expression_type expr ): _expr{ expr } { }
//     constexpr Minimum() = default;
//     
//     expression_type _expr;
// };
// 
// template< typename ExprT >
// struct ArgumentMinimum: Iterative
// {
//     using expression_type = ExprT;
//     using arguments_tuple = tuple< expression_type >;
//     using variable_types = free_variables_t< ExprT >;
// 
//     template< typename TupleT >
//     struct ResultHelper;
// 
//     template< typename... Vars >
//     struct ResultHelper< tuple< Vars... >>
//     { using type = tuple< result_t< Vars >... >; };
// 
//     using result_type = ResultHelper< variable_types >::type;
// 
//     constexpr expression_type expr() const { return _expr; }
// 
//     constexpr ArgumentMinimum( expression_type expr ): _expr{ expr } { }
//     constexpr ArgumentMinimum() = default;
// 
//     expression_type _expr;
// };
// 
// #ifndef NDEBUG
// 
// static_assert( ForExpression< Sum< Variable< 0, int >, Variable< 1, int >>>::
//     Is< Sum< int, int >>::value );
// 
// #endif // DEBUG
// 

/////////////////////////////
/// Experiment: Calculus ///
///////////////////////////
///
/// This is an experimental thought about how derivations themselves could be brought into the
/// expression algebra. The non-gramatical, non-lazy mechanism for derivatives is below.

template< typename T >
struct Infinitesimal
{ 
    template< typename U >
    friend struct Infinitesimal;

    using value_type = T;

    // behaves as non-zero
    constexpr operator bool() const
    { return true; }

    constexpr operator value_type() const
    { return _negative ? -std::numeric_limits< value_type >::denorm_min() :
        std::numeric_limits< value_type >::denorm_min(); }

    constexpr Infinitesimal< T > operator -() const
    { return Infinitesimal< T >( true ); }

    template< typename U >
    requires( std::is_convertible_v< U, T > )
    constexpr Infinitesimal& operator =( Infinitesimal< U > const& other )
    { 
        _negative = other._negative;
        return *this;
    }

    constexpr Infinitesimal( bool negative = false ): _negative{ negative } { }
    constexpr Infinitesimal( Infinitesimal const& other ): 
        _negative{ other._negative } { }

private:
    bool _negative;
};

template< typename T >
struct IsInfinitesimal: integral_constant< bool, false > { };

template< typename T >
struct IsInfinitesimal< Infinitesimal< T >>:
    integral_constant< bool, true > { };

template< typename T >
constexpr bool is_infinitesimal_v = IsInfinitesimal< T >::value;

// infinitesimals are never equal
template< typename T, typename U >
requires( is_convertible_v< U, T > )
constexpr bool operator ==( Infinitesimal< T > const& left, 
    Infinitesimal< U > const& right )
{ return false; }

template< typename T, typename U >
requires( is_convertible_v< U, T > )
constexpr bool operator !=( Infinitesimal< T > const& left, 
    Infinitesimal< U > const& right )
{ return true; }

template< typename T, typename U >
requires( is_convertible_v< U, T > and not is_infinitesimal_v< U > )
constexpr bool operator ==( Infinitesimal< T > const& left, U const& right )
{ return false; }

template< typename T, typename U >
requires( is_convertible_v< U, T > and not is_infinitesimal_v< U > )
constexpr bool operator !=( Infinitesimal< T > const& left, U const& right )
{ return true; }

template< typename T, typename U >
requires( is_convertible_v< U, T > and not is_infinitesimal_v< U > )
constexpr bool operator ==( T const& left, Infinitesimal< U > const& right )
{ return false; }

template< typename T, typename U >
requires( is_convertible_v< U, T > and not is_infinitesimal_v< U > )
constexpr bool operator !=( T const& left, Infinitesimal< U > const& right )
{ return true; }

// infinitesimal arithmetic
template< typename T, typename U >
requires( is_convertible_v< U, T > and not is_infinitesimal_v< U > )
constexpr U operator +( Infinitesimal< T > const& left, U const& right )
{ return right; }

template< typename T, typename U >
requires( is_convertible_v< U, T > and not is_infinitesimal_v< T > )
constexpr T operator +( T const& left, Infinitesimal< U > const& right )
{ return left; }

template< typename T, typename U >
requires( is_convertible_v< U, T > and not is_infinitesimal_v< U > )
constexpr U operator -( Infinitesimal< T > const& left, U const& right )
{ return -right; }

template< typename T, typename U >
requires( is_convertible_v< U, T > and not is_infinitesimal_v< T > )
constexpr T operator -( T const& left, Infinitesimal< U > const& right )
{ return left; }

template< typename T, typename U >
requires( is_convertible_v< U, T > and not is_infinitesimal_v< T > )
constexpr Infinitesimal< T > operator *( T const& left, 
    Infinitesimal< U > const& right )
// return a negative infinitesimal if left xor right is negative
{ return { left < static_cast< T >( 0 ) xor right < static_cast< U >( 0 ) }; }

template< typename T, typename U >
requires( is_convertible_v< U, T > and not is_infinitesimal_v< U > )
constexpr Infinitesimal< T > operator *( Infinitesimal< T > const& left, 
    U const& right )
// return a negative infinitesimal if left xor right is negative
{ return { left < static_cast< T >( 0 ) xor right < static_cast< U >( 0 ) }; }

template< typename T, typename U >
requires( is_convertible_v< U, T > )
constexpr Infinitesimal< T > operator *( Infinitesimal< T > const& left, 
    Infinitesimal< U > const& right )
// we will take the physicists approach and return zero if multiplying two 
// infinitesimals
{ return static_cast< T >( 0 ); }

template< typename T, typename U >
requires( is_convertible_v< U, T > and std::numeric_limits< T >::has_quiet_NaN() )
constexpr Infinitesimal< T > operator /( Infinitesimal< T > const& left, 
    Infinitesimal< U > const& right )
{ return std::numeric_limits< T >::quiet_NaN(); }

template< typename T, typename U >
requires( is_convertible_v< U, T > and not is_infinitesimal_v< T > and 
    std::numeric_limits< T >::has_infinity() )
constexpr T operator /( T const& left, Infinitesimal< U > const& right )
{
    if( left == 0 )
        return 0;

    if( left < 0 xor right < 0 )
        return std::numeric_limits< T >::infinity();

    return -std::numeric_limits< T >::infinity();
}


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
{ return power_of< Exp, T >{ arg }; }

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

// template< typename ExprT >
// constexpr auto min( ExprT const& expr )
// { return Minimum< ExprT >{ expr }; }
// 
// template< typename ExprT >
// constexpr auto argmin( ExprT const& expr )
// { return ArgumentMinimum< ExprT >{ expr }; }
// 
//


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

template< typename ExprT >
struct Delta: Derivation
{
    
};

template< size_t I, typename T >
struct Differential
{
    using variable_type = Variable< I, T >;

    // derivative of a static value is 0
    template< typename U >
    auto operator()( StaticValue< U > const& expr )
    { 
        using result_type = decltype( U{} / T{} );
        return Constant< static_cast< result_type >( 0 ) >{}; 
    }

    // derivative of a constant is 0
    template< auto Value >
    auto operator()( Constant< Value > const& expr )
    { 
        using result_type = decltype( typename Constant< Value >::result_type{} / T{} );
        return Constant< static_cast< result_type >( 0 )>{}; 
    }

    template< size_t J, typename U >
    auto operator()( Variable< J, U > const& expr )
    { return Constant< static_cast< decltype( U{} / T{} ) >( I == J ? 1 : 0 )>{ }; }

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
    { 
        using result_type = decltype( result_t< U >{} / result_t< U >{} );
        return Constant< static_cast< result_type >( 0.5 ) >{} / 
            expr * (*this)( expr.arg() ); 
    }

    template< int Exp, typename U >
    auto operator()( power_of< Exp, U > const& expr )
    { 
        using result_type = decltype( result_t< U >{} / result_t< U >{} );
        return Constant<  static_cast< result_type >( Exp ) >{} * 
            pow< Exp-1 >( expr.arg() ) * (*this)( expr.arg() ); 
    }

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
    //template< typename U, typename V >
    //auto operator()( Arctangent2< U, V > const& expr )
    //{ return (*this)( expr.numerator_arg() / expr.denominator_arg() ) / 
    //    ( constant_one - pow< 2 >( expr.numerator_arg() / expr.denominator_arg() )); }
    
};

template< typename X >
struct DifferentialFor;

template< size_t I, typename T >
struct DifferentialFor< Variable< I, T >>
{ using type = Differential< I, T >; };

template< typename X >
using differential_for_t = DifferentialFor< X >::type;

template< size_t I, typename T >
Differential< I, T > 
differential( Variable< I, T > const& )
{ return {}; }

template< variable Var, expression Expr >
auto differential_for( Expr const& expr )
{ return differential_for_t< Var >{}( expr ); }

template< typename ExprT >
struct GradientOperator
{
    using expression_type = ExprT;
    using free_variables_tuple = free_variables_t< expression_type >;
    static constexpr size_t size = tuple_size_v< free_variables_tuple >;

private:
    template< size_t I >
    struct Element
    {
//        using differential_type = differential_for_t< tuple_element_t< I, 
//            free_variables_tuple >, expression_type >;
//
//        static constexpr auto value( expression_type const& expr )
//        { return differential_type{}( expr ); }
    };

    template< typename Seq >
    struct Helper;

    template< size_t... Is >
    struct Helper< seq< Is... >>
    { 
        using differential_type = Tensor< Shape< sizeof...( Is )>, 
            Element< Is >... >;

        static constexpr auto value( ExprT const& expr ) ;
//        { make_tensor< Shape< sizeof...( Is )>>( Element< Is >::value( expr )
//            ... ); }
    };

public:
    using type = Helper< make_seq< size >>::type;
    static constexpr auto value( ExprT const& expr )
    { return Helper< make_seq< size >>::value( expr ); }
};

template< expression ExprT >
constexpr auto gradient( ExprT const& expr )
{ return GradientOperator< ExprT >::value( expr ); }

template< expression ExprT > 
struct JacobianOperator
{
    template< size_t I, size_t J >
    struct Element
    {
//        using differential_type = differential_for_t< tuple_element_t< I,
//            free_variables_tuple >, tensor_element_t< J, expression_type >>;
    };
};




} // namespace expressions

namespace std {

template< expressions::expression ExprT >
constexpr auto sqrt( ExprT const& expr )
{ return expressions::sqrt( expr ); }


} // namespace std 


#endif // __EXPRESSIONS_EXPRESSIONS_HPP__
