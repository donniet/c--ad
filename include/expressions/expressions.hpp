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
///  (iv) ...
///   (v) Expressions MUST implement a static, generically typed, constexpr value
///       method that calculates the result of the expression given the arguments
///  (vi) IsExpression< E< Ts... >> or IsExpressionOperation< E > must evaluate
///       to a true_type without the expression class being defined.
///   
/// # Example of an Expression Class
/// ```
/// template< typename... Args >
/// class Sum;
///
/// template< >
/// struct IsExpressionOperation< Sum >: 
///     std::true_type { };                         // requirement (vi)
///
/// template< typename... Args >
/// requires( sizeof...( Args ) >= 2 )
/// class Sum: Arguments< Sum, Args... > {          // requirement (ii) 
/// public:
///     static constexpr auto value( auto... args ) 
///     { return ( args + ... ); }                  // requirement (v) 
///
///     constexpr Sum() = default;                  // requirement (i)
///     constexpr Sum( Sum const& ) = default;      // requirement (i)
///     constexpr Sum( Args&&... args ):            
///         Arguments< Sum, Args... >{ args } { }   // requirement (iii)
/// };
/// ```
/// - The Arguments base template is tuple-like object of the ...Args
/// - Arguments imbues parent class ExprT with:
///     ExprT::operator()( subs&&... );  // substitution
///     ExprT::operator();               // evaluation
///   
///
/// # Expression Evaluation
///
/// Expressions are evaluated using operator|, referred to as the application
/// operator.  
///
/// assert(( constant< 5 > + constant< 6 > | eval()) == 11 );
/// 
/// Constant and StaticValue classes simply evaluate to their compile-time or
/// run time values respectively.  Compound expressions are evaluated using the
/// Arguments base class and the provided operator| in the following manner:
///
/// 1. Each argument of the compound expression is evaluated
/// 2. The evaluated arguments are passed to the static value method of the
///    expression's class, and the result is returned
///
/// # Expressions with Variables and Scopes
///
/// Expressions containing variables including variables themselves have an
/// undefined value and require a Scope to evaluate.  The `eval` function
/// accepts a scope as an arguement.  When given a scope a variable's evaluated
/// value is the result of `get_value< variable_type >( scope )`.  
///
///     auto in_scope = simple_scope( 2.f );            // create a simple scope
///     Variable< 0, float > x;                         // name the zeroeth variable x
///     assert(( x + 2.f | eval( in_scope )) == 4.f );  // evalute an expression against the scope
/// 
/// # Substitutions and Free Variables
/// 
/// Expressions can be substituted into the free variables of other expressions 
/// via operator(). The result is a Substitution expression. Variables are 
/// considered free if they are not part of a substitution expression.
///
///     auto in_scope = simple_scope( 3.f, 4.f );
///     Variable< 0, float > x;
///     Variable< 1, float > y;
///     auto f = x + y;                     // f has free variables x and y
///     auto g = f( 2.f, 2.f );             // g has no free variables
///     auto h = f( 5.f );                  // h has one free variable
///     assert(( g | eval()) == 4.f );
///     assert(( f | eval( in_scope )) == 7.f );
///     assert(( h | eval( in_scope )) == 9.f );
///     assert(( h( x ) | eval( in_scope )) == 8.f );
/// 
/// Substitutions must be compatible with the variables they are replacing:
///
/// 1. The result type of the substitution argument must be convertible to
///    the value type of the variable it is replacing. 
/// 2. If a substitution is being made into a variable which is part of a 
///    substitution expression itself then the expression being substituted
///    for that variable must be compatible with the original substitution.
/// 
///     Variable< 1, float > x;
///     Variable< 0, float > f;  
///     auto g = f(x);           // g has free variables f and x
///     assert(( g( x * x, 3.f ) | eval() ) == 9.f );
///     assert(( g( x * x )( 3.f ) | eval() ) == 9.f );
///     
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

/// @brief trait to identify a type as an expression type
/// @tparam T is the type to be checked
template< typename T >
struct IsExpression: std::false_type { };

template< template< typename... > class Op >
struct IsExpressionOperation: std::false_type { };

template< template< typename... > class Op, typename... Args >
struct IsExpression< Op< Args... >>: IsExpressionOperation< Op > { };

template< typename... Ts >
struct IsExpression< tuple< Ts... >>: integral_constant< bool,
    ( IsExpression< Ts >::value or ... )> { };

template< shape S, typename... Ts >
struct IsExpression< Tensor< S, Ts... >>: integral_constant< bool,
    ( IsExpression< Ts >::value or ... )> { };

template< typename T >
struct IsCompoundExpression: std::false_type { };

template< template< typename... > class Op, typename... Args >
struct IsCompoundExpression< Op< Args... >>: IsExpressionOperation< Op > { };

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
concept compound_expression = IsCompoundExpression< T >::value; 

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

/// @brief limit the id of variables created by the user to half 
///        those available
#ifdef NDEBUG
constexpr size_t maximum_user_variable_id = 
    std::numeric_limits< size_t >::max() >> 1;
#else
constexpr size_t maximum_user_variable_id = 10;
#endif


template< size_t I, typename T >
struct IsExpression< Variable< I, T >>: std::true_type { };
/// a trait to extract the variable ID during bootstrapping
template< typename Var >
struct VariableId;

template< size_t I, typename T >
struct VariableId< Variable< I, T >>: integral_constant< size_t, I > { };

template< typename Var >
constexpr size_t variable_id_v = VariableId< Var >::value;

template< typename Var >
struct VariableValue;

template< size_t I, typename T >
struct VariableValue< Variable< I, T >>
{ using type = T; };

template< typename Var >
using variable_value_t = VariableValue< Var >::type;

/// @brief temporary variables required to evaluate substitutions
///        start above the maximum
static constexpr size_t minimum_temporary_variable_id =
    maximum_user_variable_id + 1;

template< typename T >
struct IsTemporaryVariable: std::false_type { };

template< size_t I, typename T >
struct IsTemporaryVariable< Variable< I, T >>: std::integral_constant< bool,
    is_greater( I, maximum_user_variable_id )> { };

template< typename... Vars >
struct NextTemporaryVariableId: std::integral_constant< size_t,
    max_of( maximum_user_variable_id, VariableId< Vars >::value... ) + 1 > { };

template< typename... Vars >
constexpr size_t next_temporary_variable_id_v = 
    NextTemporaryVariableId< Vars... >::value;
         

/// @brief forward declaration of Substitution expression
template< typename ExprT, typename... Subs >
struct Substitution;

template< >
struct IsExpressionOperation< Substitution >: std::true_type { };

// DT: I'm not sure which to instantiate here?
// we have to bootstrap Substitutions as expressions
//template< typename ExprT, typename... Subs >
//struct IsExpression< Substitution< ExprT, Subs... >>: std::true_type { };

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
struct Result;

template< typename T >
requires( not expression< T > )
struct Result< T >
{ using type = T; };

template< typename... Ts >
requires( expression< tuple< Ts... >> )
struct Result< tuple< Ts... >>
{ using type = tuple< typename Result< Ts >::type... >; };

template< shape S, typename... Ts >
requires( expression< Tensor< S, Ts... >> )
struct Result< Tensor< S, Ts... >>
{ using type = Tensor< S, typename Result< Ts >::type... >; };

/// @brief result of a variable is the result of it's value_type
template< size_t I, typename T >
struct Result< Variable< I, T >>: Result< T > { };

/// @brief result of a substitution is the result of it's expression formula
/// DT: handled by compound expression result assumption
//template< typename FormulaT, typename... Args >
//struct Result< Substitution< ExprT, Args... >>: Result< FormulaT > { };

/// @brief results of a compound expression are assumed to be the result of 
///        their first argument
/// HACK:  Are there compound expressions that this would not be true for?
template< template< typename... > class Op, typename First, typename... Rest >
requires compound_expression< Op< First, Rest... >>
struct Result< Op< First, Rest... >>: Result< First > { };

/// @brief trait to resolve the result type of an expression
template< typename T >
using result_t = Result< T >::type;

// forward declaration required for Variable<...>::operator ()
// Worker class for Substitution Expressions
template< typename ExprT, typename... Subs >
struct Substituter;

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
//
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

//static_assert( variable_order_v< int > == 0 );
//static_assert( variable_order_v< Variable< 0, int >> == 1 );
//static_assert( variable_order_v< Variable< 0, Variable< 0, int >>> == 2 );
//static_assert( variable_order_v< tuple< Variable< 0, int >, Variable< 0, Variable< 0, int >>>> == 2 );

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

///////////////////////////////
/// Functional Expressions ///
/////////////////////////////
///
/// A functional expression is a higher order substitution
///
/// ```
/// Variable< 0, int > x;
/// Variable< 1, int > y;
/// Variable< 2, int > f;
///
/// auto g = f(x, y); // g is a functional expression
/// ```
template< typename T >
struct IsFunctionalExpression: std::false_type { };

template< size_t I, size_t J, typename ExprI, typename First, typename... Rest >
struct IsFunctionalExpression< Substitution< Variable< I, Variable< J, ExprI >>,
    First, Rest... >>: std::integral_constant< bool, (
        variable< First > and ( variable< Rest > and ... ) and 
            is_greater( variable_order_v< First >, 1 ) and
                ( is_greater( variable_order_v< Rest >, 1 ) and ... ))>
{ };

template< typename T >
constexpr bool is_functional_expression_v = IsFunctionalExpression< T >::value;

template< typename T >
concept functional_expression = is_functional_expression_v< T >;


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
struct SetVariableValue
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
template< variable... Vars >
class unique_variables
{ static_assert( "invalid sorted, unique list of variables"); };
 
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

protected:
    template< size_t I >
    struct Element
    { static_assert( I >= size, "index into unique_variables must be less than size" ); }; 

public:
    template< size_t I >
    using element_t = void; // should never be instantiated

    constexpr scope_type make_scope() const;

    template< variable V >
    static consteval bool contains( V = {} )
    { return false; }

    template< variable V >
    static consteval size_t index_of( V = {} )
    { return 1; } // 1 > size

    constexpr unique_variables() = default;
    explicit constexpr unique_variables( tuple< >&& )
    { }
};

/// Case: One or More Dependent variables
///
template< variable First, variable... Rest >
requires( is_sorted_unique_seq_v< seq< variable_id_v< First >, 
    variable_id_v< Rest >... >> )
class unique_variables< First, Rest... >: unique_variables< Rest... > {
public:
    static constexpr size_t size = 1 + sizeof...( Rest );

    using scope_type = Scope< First, Rest... >;
    using values_tuple = tuple< variable_value_t< First >,
        variable_value_t< Rest >... >;
    using last_type = std::tuple_element_t< sizeof...( Rest ), 
        tuple< First, Rest... >>;
    using variables_tuple = tuple< First, Rest... >;
    
    using first_type = First;

    // returns this unique, sorted set of variables as a tuple
    constexpr variables_tuple as_tuple() const
    { return std::tuple_cat( tuple< first_type >{ first() },
        unique_variables< Rest... >::as_tuple() ); }

    // implicit conversion of this class into an std::tuple
    constexpr operator variables_tuple() const
    { return as_tuple(); }

protected:
    template< size_t I >
    struct Element
    { static_assert( I >= size, "index into unique_variables must be less than size" ); }; 

    template< >
    struct Element< 0 >
    { 
        using type = First; 
        static constexpr type value( unique_variables const& vars )
        { return vars.first(); }
    };

    template< size_t I >
    requires( 0 < I and I < size )
    struct Element< I >
    { 
        using type = unique_variables< Rest... >::template Element< I - 1 >::type;
        static constexpr type value( unique_variables const& vars )
        { return unique_variables< Rest... >::template Element< I - 1 >::value( vars ); }
    };

public:
    // Ith dependent variable type
    template< size_t I >
    using element_t = Element< I >::type;

    // Ith variable in this collection
    template< size_t I >
    constexpr element_t< I >
    at() const 
    { return Element< I >::value( *this ); }

    // does this set contain a given variable?
    // TODO: this could be a binary search
    template< variable Var >
    static consteval bool contains()
    { 
        if constexpr( is_greater( variable_id_v< Var >, variable_id_v< First > ))
            return unique_variables< Rest... >::template contains< Var >();
        
        return variable_id_v< Var > == variable_id_v< First >;
    };

    template< variable Var >
    static consteval bool contains( Var&& var )
    { return contains< Var >(); }

    // index of the given variable in this set
    template< variable Var >
    static consteval size_t index_of()
    {
        if constexpr( is_greater( variable_id_v< Var >, variable_id_v< First > ))
            return 1 + unique_variables< Rest... >::template index_of< Var >();

        if constexpr( variable_id_v< Var > == variable_id_v< First > )
            return 0;

        return size;
    }

    template< variable Var >
    static consteval bool index_of( Var&& var )
    { return index_of< Var >(); }

    // Create the minimal scope that an expression with these dependent variables
    // can be executed against.
    constexpr scope_type make_scope() const;

    // Checks if a scope contains the values necessary to evaluate an expression
    // with these dependent variables
    template< variable... Vars >
    static consteval bool is_valid_scope( Scope< Vars... > scope = {} )
    { return unique_variables< Rest... >::is_valid_scope( scope ) and
        (( variable_id_v< First > == variable_id_v< Vars > ) or ... ); }

    // The first variable in this set
    constexpr first_type first() const
    { return { _first_name }; } 

    // The last variable in this set
    constexpr last_type last() const
    { return std::get< sizeof...( Rest )>( 
        operator tuple< first_type, Rest... >() ); }

    // The set of dependent variables except the first
    constexpr unique_variables< Rest... > rest() const
    { return *this; }

    constexpr unique_variables() = default;
    constexpr unique_variables( unique_variables const& ) = default;
//    constexpr unique_variables( first_type&& first, Rest&&... rest ):
//        unique_variables< Rest... >{ std::forward( rest )... }, 
//            _first_name{ first.name() } { }
    constexpr unique_variables( first_type const& first, Rest const&... rest ):
        unique_variables< Rest... >{ rest... }, _first_name{ first.name() }
    { }
    
    template< typename FirstName, typename... RestNames >
    requires( std::is_same_v< std::remove_cv_t< FirstName >, std::string > and 
        ( std::is_same_v< std::remove_cv_t< RestNames >, std::string > and ... ) and
            1 + sizeof...( RestNames ) == size )
    constexpr unique_variables( FirstName const& first, RestNames const&... rest ):
        unique_variables< RestNames... >( rest... ), _first_name{ first } { }

private:
    // storage for the variable metadata
    std::string _first_name;
};
//  
//  ///////////////////////////////////////
//  /// unique_variables is tuple-like ///
//  /////////////////////////////////////
//  ///
//  } // namespace expressions
//  
//  namespace std {
//  /// @brief specialization of std::tuple_size
//  template< expressions::variable... Vars >
//  struct tuple_size< expressions::unique_variables< Vars... >>:
//      integral_constant< size_t, sizeof...( Vars )> { };
//  
//  /// @brief specialization of std::tuple_element_t for unique_variables
//  template< size_t I, expressions::variable... Vars >
//  struct tuple_element< I, expressions::unique_variables< Vars... >>
//  { using type = Vars...[ I ]; };
//  
//  template< size_t I, expressions::variable... Vars >
//  struct tuple_element< I, const expressions::unique_variables< Vars... >>
//  { using type = add_const< Vars...[ I ]>::type; };
//  
//  template< size_t I, expressions::variable... Vars >
//  struct tuple_element< I, volatile expressions::unique_variables< Vars... >>
//  { using type = add_volatile< Vars...[ I ]>::type; };
//  
//  template< size_t I, expressions::variable... Vars >
//  struct tuple_element< I, const volatile expressions::unique_variables< Vars... >>
//  { using type = add_cv< Vars...[ I ]>::type; };
//  
//  /// @brief sepcialization of std::get for unique_variables
//  //template< size_t I, expressions::variable... Vars >
//  //constexpr tuple_element_t< I, unique_variables< Vars... >> 
//  //get( unique_variables< Vars... >& vars )
//  //{ return vars.template at< I >(); }
//  
//  /// @brief sepcialization of std::get for unique_variables
//  template< size_t I, expressions::variable... Vars >
//  constexpr tuple_element_t< I, expressions::unique_variables< Vars... >> 
//  get( expressions::unique_variables< Vars... > const& vars )
//  { return vars.template at< I >(); }
//  
//  } // namespace std
//  namespace expressions {
//  
/////////////////////////////////////////
/// Merging Sets of Unique Variables ///
///////////////////////////////////////
///
namespace detail {

/********************************************************************
 * DESIGN DECISION * Variable Id Corresponds to a Unique value_type *
 ********************************************************************/

// DT: we are going to require that a variable id uniquely determines
//     the variable value type in a given expression, and remove the 
//     UniqueTypes class below
//
//template< typename... Ts >
//struct UniqueTypes: std::tuple< Ts... > 
//{ static constexpr size_t size = sizeof...( Ts ); };
//
//template< typename T >
//struct IsUniqueTypes: integral_constant< bool, false > { };
//
//template< typename... Ts >
//struct IsUniqueTypes< UniqueTypes< Ts... >>: integral_constant< bool,
//    true > { };

template< typename First, typename... Rest >
struct MergeValueTypes;

template< typename First, typename... Rest >
requires(( std::is_same_v< First, Rest > and ... ))
struct MergeValueTypes< First, Rest... >
{ using type = First; };

template< typename First, typename... Rest >
requires( not ( std::is_same_v< First, Rest > or ... ))
struct MergeValueTypes< First, Rest... >
{ static_assert( "variables with the same id must have the same value_type" ); };

template< variable... Vars >
struct MakeUniqueVariables;

template< >
struct MakeUniqueVariables< >
{
    using type = unique_variables< >;
    static constexpr type value( )
    { return { }; }
};

template< variable First, variable... Rest >
struct MakeUniqueVariables< First, Rest... >
{

    using rest_type = MakeUniqueVariables< Rest... >::type;
    static constexpr rest_type rest_value( Rest const&... rest )
    { return MakeUniqueVariables< Rest... >::value( rest... ); }

    static constexpr size_t first_id = variable_id_v< First >;

    typedef make_seq< sizeof...( Rest )> for_rest;

    template< typename RestUnique >
    struct Inserter;

    template< >
    struct Inserter< unique_variables< >>
    {
        //static_assert( not is_substitution_expression_v< variable_value_t< First >> ); // and tuple_size_v< tuple< Vars... >> == 1 );
        using type = unique_variables< First >;
        static constexpr type 
        value( First const& first )
        { return { first }; }
    };

    template< typename... Vars >
    requires( unique_variables< Vars... >::template contains< First >())
    struct Inserter< unique_variables< Vars... >>
    {
        static constexpr size_t index = unique_variables< Vars... >::
            template index_of< First >();
        
        using rest_variable = unique_variables< Vars... >::template 
            element_t< index >;
        using merged_variable = Variable< variable_id_v< First >,
            typename MergeValueTypes< variable_value_t< First >, 
                variable_value_t< rest_variable >>::type >;

        template< typename Seq >
        struct Enumerator;

        template< size_t... Is >
        struct Enumerator< seq< Is... >>
        {
            using type = unique_variables< std::conditional_t< Is == index,
                merged_variable, Vars...[ Is ]>... >;
            static constexpr type value( First const& first, 
                rest_type const& rest )
            { return { ( Is == index ? first.name() : 
                rest.template at< Is >().name() )... }; }
        };

        using type = Enumerator< for_rest >::type;

        static constexpr type 
        value( First const& first, Rest const&... rest )
        { return Enumerator< for_rest >::value( first, rest_value( rest... )); }
    };

    template< typename... Vars >
    requires( not unique_variables< Vars... >::template contains< First >())
    struct Inserter< unique_variables< Vars... >>
    {
        using unique_variables_type = unique_variables< Vars... >;

        static constexpr size_t index = 
            (( is_less( variable_id_v< Vars >, first_id ) ? 1 : 0 ) + ... + 0 );

        typedef make_seq< 1 + sizeof...( Rest )> for_spliced;

        template< typename Seq >
        struct Enumerator;

        template< size_t... Is >
        struct Enumerator< seq< Is... >>
        {
            template< size_t I >
            struct Element;

            template< size_t I >
            requires( I < index )
            struct Element< I >
            {
                using type = unique_variables_type::template element_t< I >;
                static constexpr type value( First const&, rest_type const& rest )
                { return rest.template at< I >(); }
            };

            template< size_t I >
            requires( I == index )
            struct Element< I >
            {
                using type = First;
                static constexpr type value( First const& first, rest_type const& )
                { return first; }
            };

            template< size_t I >
            requires( I > index )
            struct Element< I >
            {
                using type = unique_variables_type::template element_t< I - 1 >;
                static constexpr type value( First const&, rest_type const& rest )
                { return rest.template at< I - 1 >(); }
            };

            using type = unique_variables< typename Element< Is >::type... >;
            static constexpr type value( First const& first, rest_type const& rest )
            { return { Element< Is >::value( first, rest )... }; }
        };

        using type = Enumerator< for_spliced >::type;
        static constexpr type
        value( First const& first, Rest const&... rest )
        { return Enumerator< for_spliced >::value( first, rest_value( rest... )); }
    };

    using type = Inserter< rest_type >::type;
    static constexpr type
    value( First const& first, Rest const&... rest )
    { return Inserter< rest_type >::value( first, rest... ); }
};

template< typename... Sets >
struct MergeUniqueVariables;

template< >
struct MergeUniqueVariables< >
{ 
    using type = unique_variables< >;
    static constexpr type value( )
    { return { }; }
};

template< typename... Vars >
struct MergeUniqueVariables< unique_variables< Vars... >>
{ 
    using type = unique_variables< Vars... >;
    static constexpr type value( unique_variables< Vars... > const& vars )
    { return vars; }
};

// for now just use the MakeUniqueVars implementation but
// TODO: this could be improved
template< typename... LeftVars, typename... RightVars >
struct MergeUniqueVariables< unique_variables< LeftVars... >, 
    unique_variables< RightVars... >> 
{
private:
    using left_variable_set = unique_variables< LeftVars... >;
    using right_variable_set = unique_variables< RightVars... >;

    template< size_t I >
    struct Selector;

    template< size_t I >
    requires( is_less( I, left_variable_set::size ))
    struct Selector< I >
    { 
        static constexpr size_t index = I;
        using type = left_variable_set::template element_t< index >;
        static constexpr type
        value( left_variable_set const& left, 
            right_variable_set const& right )
        { return left.template at< index >(); }
    };

    template< size_t I >
    requires( not is_less( I, left_variable_set::size ))
    struct Selector< I >
    {
        static constexpr size_t index = I - left_variable_set::size;
        using type = right_variable_set::template element_t< index >;
        static constexpr type
        value( left_variable_set const& left, 
            right_variable_set const& right )
        { return right.template at< index >(); }
    };

    static constexpr size_t unmerged_size = 
        left_variable_set::size + right_variable_set::size;

    typedef make_seq< unmerged_size > for_unmerged;

    template< typename Seq >
    struct Maker;

    template< size_t... Is >
    struct Maker< seq< Is... >>
    { 
        using type = MakeUniqueVariables< typename 
            Selector< Is >::type... >::type;
        static constexpr type
        value( left_variable_set const& left,
            right_variable_set const& right )
        { return MakeUniqueVariables< typename Selector< Is >::type... >::value( 
            Selector< Is >::value( left, right )... ); }
    };

public:
    using type = Maker< for_unmerged >::type;

    static constexpr type 
    value( unique_variables< LeftVars... > const& left,
        unique_variables< RightVars... > const& right )
    { return Maker< for_unmerged >::value( left, right ); } 
};

template< typename First, typename... Rest >
requires( is_greater( sizeof...( Rest ), 1 ))
struct MergeUniqueVariables< First, Rest... >
{
    using rest_variable_set = MergeUniqueVariables< Rest... >::type;

    using type = MergeUniqueVariables< First, rest_variable_set >::type;
    static constexpr type value( First const& first, Rest const&... rest )
    { 
        rest_variable_set rest_set = MergeUniqueVariables< Rest... >::value( rest... );
        return MergeUniqueVariables< First, rest_variable_set >::value( first, rest_set );
    }
};

template< typename UniqueVars, typename RemoveVars >
struct SubtractUniqueVariables;

template< typename... MinuendVars, typename... SubtrahendVars >
struct SubtractUniqueVariables< unique_variables< MinuendVars... >,
    unique_variables< SubtrahendVars... >>
{
private:
    using minuend_type = unique_variables< MinuendVars... >;
    using subtrahend_type = unique_variables< SubtrahendVars... >;

    typedef make_seq< sizeof...( MinuendVars )> for_minuend;

    // (1) what elements from the first should be in our final set?
    template< size_t I >
    struct Pred: std::integral_constant< size_t, 
        ( subtrahend_type::template contains< MinuendVars...[ I ]>() ? 0 : 1 )>
    { };

    template< typename Seq >
    struct Tester;

    template< size_t... Is >
    struct Tester< seq< Is... >>
    { 
        static constexpr size_t size = ( Pred< Is >::value + ... + 0 );
        using selection_seq = seq< Pred< Is >::value... >;
        using prefix_sum_seq = seq_prefix_sum_t< selection_seq >;
    };

    static constexpr size_t size = Tester< for_minuend >::size;
    typedef make_seq< size > for_elements;

    template< size_t I >
    struct Element
    {
        // find the output element index + 1 in the prefix sum
        static constexpr size_t index = index_of_seq_v< I + 1, 
            typename Tester< for_minuend >::prefix_sum_seq >;

        using type = minuend_type::template element_t< index >;
        static constexpr type value( minuend_type const& minuend, 
            subtrahend_type const& subtrahend )
        { return minuend.template at< index >(); }
    };

    template< typename Seq >
    struct Helper;

    template< size_t... Is >
    struct Helper< seq< Is... >>
    {
        using type = unique_variables< typename Element< Is >::type... >;
        static constexpr type value( minuend_type const& minuend,
            subtrahend_type const& subtrahend )
        { return { Element< Is >::value( minuend, subtrahend )... }; }
    };

public:
    using type = Helper< for_elements >::type;
    static constexpr type value( minuend_type const& minuend,
        subtrahend_type const& subtrahend )
    { return Helper< for_elements >::value( minuend, subtrahend ); }
};

static_assert( is_sorted_unique_seq_v< seq< 0 >> );
static_assert( requires{ typename unique_variables< >; } );
static_assert( requires{ typename unique_variables< Variable< 0, int >>; } );
static_assert( unique_variables< Variable< 0, int >>::template 
    contains< Variable< 0, int >>());
static_assert( std::is_same_v< unique_variables< Variable< 0, int >>,
    typename MakeUniqueVariables< Variable< 0, int >, Variable< 0, int >>::type > );

} // namespace detail

template< typename... Ts >
using merge_unique_variables_t = detail::MergeUniqueVariables< Ts... >::type;

template< typename... Ts >
using make_unique_variables_t = detail::MakeUniqueVariables< Ts... >::type;

template< typename... Sets >
constexpr detail::MergeUniqueVariables< Sets... >::type
merge_unique_variables( Sets const&... sets )
{ return detail::MergeUniqueVariables< Sets... >::value( sets... ); }

template< typename... Vars >
constexpr make_unique_variables_t< Vars... >
make_unique_variables( Vars const&... vars )
{ return detail::MakeUniqueVariables< Vars... >::value( vars... ); }

template< typename Vars, typename Removals >
using subtract_unique_variables_t = 
    detail::SubtractUniqueVariables< Vars, Removals >::type;

template< typename Vars, typename Removals >
constexpr subtract_unique_variables_t< Vars, Removals >
subtract_unique_variables( Vars const& vars, Removals const& removals )
{ return detail::SubtractUniqueVariables< Vars, Removals >::value( vars, removals ); }

template< typename UniqueA, typename UniqueB >
struct UniqueVariablesOverlap;

template< variable A, variable... As, variable... Bs >
struct UniqueVariablesOverlap< unique_variables< A, As... >,
    unique_variables< Bs... >>: std::integral_constant< bool,
        (( variable_id_v< A > == variable_id_v< Bs > ) or ... ) or 
            UniqueVariablesOverlap< unique_variables< As... >, 
                unique_variables< Bs... >>::value > 
{ };

template< variable... Bs >
struct UniqueVariablesOverlap< unique_variables< >, unique_variables< Bs... >>:
    std::false_type { };

template< typename UniqueA, typename UniqueB >
struct UniqueVariablesDisjoint;

template< variable A, variable... As, variable... Bs >
struct UniqueVariablesDisjoint< unique_variables< A, As... >,
    unique_variables< Bs... >>: std::integral_constant< bool,
        (( A::id != Bs::id ) and ... ) and UniqueVariablesDisjoint<
            unique_variables< As... >, unique_variables< Bs... >>::value >
{ };

template< variable... Bs >
struct UniqueVariablesDisjoint< unique_variables< >, unique_variables< Bs... >>:
    std::true_type { };

///////////////////////////////////////////////////////////////////////
/// Bootstrap Substitutions: Substitution Argument Dependency Sort ///
/////////////////////////////////////////////////////////////////////
/// 
/// second and higher order variables:
///
/// auto g = f(x,y);  // f,x,y free; no bound
/// auto h = g(z,z);  // g,z free; f <- g, x <- z, y <- z
/// auto l = h(3);    // h free; f <- g <- h, x <- z <- 3, y <- z <- 3
/// auto val = l(x*y) // no free; f <- g <- h <- (x*y), x <- z <- 3, y <- z <- 3
///                   // -> expression evaluated here
///
/// assert( 9 == val 
///     and 9 == l(x*y) 
///     and 9 == h(3)(x*y) 
///     and 9 == g(z,z)(3)(x*y) 
///     and 9 == f(x,y)(z,z)(3)(x*y) );
///
///
/// Applying a substitution is done variable-by-variable yet the substituted
/// arguments may themselves contain variables.
///
///     ````
///     using Var = Variable;
///     usinv Sub = Substitution;
///     using v1 = Var< 1, int >; 
///     using v2 = Var< 2, int >;
///     using v3 = Var< 3, int >;
///
///     v1 x; v2 y; v3 f;
///
///     auto g = f(x, y);
///     auto h = f(y, x);
///
///     assert( g( 4, 2 )( x / y ) == 2 and
///             g( 4, 2 )( x / y ) == h( 2, 4 )( x / y ) );
///     //...
///     ```
///
/// This creates additional problems since unique_variables are sorted by id
/// and that order is used to identify the matching substitution arguement. 
/// But were we to apply the substitutions in that naive order we would see
/// inconsistent results of expressions.
///
///     h( 2, 4 ); // implies 2 is substituted for y, yet we said we apply 
///                // substitutions in variable id order, and x is lower than
///                // y!
///
/// To resolve this we will abuse the notation. Substitution expressions will
/// be constructed in a way that identifies the variable being substituted, then
/// executes it in order of id. 


/***********************************************************
 * DESIGN DECISION * Tuples of Expressions are Expressions *
 ***********************************************************/

/************************************************************
 * DESIGN DECISION * Tensors of Expressions are Expressions *
 ************************************************************/

/************************************
 * DESIGN DECISION * Free Variables *
 ************************************
 * Variables may be "free" or "bound".
 * 
 * Free Variable:
 * - Variable< id, value_type > is free by default.
 * - Substitution< ExprT, Subs... > binds free variables in ExprT with
 *   ...Subs in FreeVariables::id order.
 * - No other operation may bind a variable.
 * - A variable that is not free is bound.
 **/

/********************************************************
 * DESIGN DECISION * Variable Substitutions Are Allowed *
 ********************************************************
 * Ids for second-order variables correspond to substitution argument order,
 * but are re-ordered in resultant substitution expression by the nested
 * variable Id 
 * 
 * Second Order Variable: Variable< I, Variable< J, T >>
 *          temporary variable id --^            ^
 *          original variable id  ---------------
 *
 * 
 *
 **/

/*******************************************************
 * DESIGN DECISION * Partial Substitutions Are Allowed *
 *******************************************************
 * Partial substitutions are allowed.  Any unbound variables are free in 
 * resultant expression.  Over-substitutions are also allowed.  Unmatched
 * substitution arguments are ignored.
 **/

/*****************************************************************
 * DESIGN DECISION * Application Re-Recurses Until Terminal Case *
 *****************************************************************/

/***************************************************************
 * DESIGN DECISION * Operator() Re-Reurses Until Terminal Case *
 ***************************************************************/


///
/// auto g = f( x, y ); // Sub< Var< 6, v3 >, Var< 4, v1 >, Var< 5, v2 >>
/// auto h = f( y, x ); // Sub< Var< 6, v3 >, Var< 5, v1 >, Var< 4, v2 >>
///
/// auto p = g( 4, 2 ); // Sub< Sub< Var< 6, v3 >, Var< 4, v1 >, Var< 5, v2 >>,
///                     //     int, int >{{ "f", "x", "y" }, 4, 2 };
///                         
/// auto q = h( 2, 4 ); // Sub< Sub< Var< 6, v3 >, Var< 5, v1 >, Var< 4, v2 >>,
///                     //     int, int >{{ "f", "y", "x" }, 2, 4 };
///
/// int vg = p( x / y ); 
/// int vq = q( x / y );
///
///  ┌─ Substitution is constructed and evaluated:
///  │
///  │  Sub< Sub< Sub< Var< 6, v3 >, Var< 4, v1 >, Var< 5, v2 >>, int, int >, 
///  │      Quotient< v1, v2 >>{{{ "f", "x", "y" }, 4, 2 }, { "x", "y" }}()
///  │  
///  ├─ Evaluation recurses and second-order variables are replaced by
///  │  substitutions.
///  │                        
///  │  Sub< Sub< Var< 6, v3 >, Sub< v1, int >, Sub< v2, int >>,
///  │      Quotient< v1, v2 >>{{ "f", { "x", 4 }, { "y", 2 }}, { "x", "y" }}()
///  │  Sub< Quotient< v1, v2 >, Sub< v1, int >, Sub< v2, int >>
///  │      {{ "x", "y" }, { "x", 4 }, { "y", 2 }}()
///  │  Sub< Quotient< v1, v2 >, int, int >
///  │      {{ "x", "y" }, 4, 2 }()
///  │  Quotient< int, int >{ 4, 2 }()
///  │  2
///  │ 
///  │  Sub< Sub< Sub< Var< 6, v3 >, Var< 5, v1 >, Var< 4, v2 >>, int, int >,
///  │      Quotient< v1, v2 >>{{{ "g", "x", "y" }, 2, 4 }, { "x", "y" }}()
///  │  Sub< Sub< Var< 6, v3 >, Sub< v1, int >, Sub< v2, int >>,
///  │      Quotient< v1, v2 >>{{ "g", { "x", 4 }, { "y", 2 }}, { "x", "y" }}()
///  │  Sub< Quotient< v1, v2 >, int, int >
///  │      {{ "x", "y" }, 4, 2 }()
///  │  Quotient< int, int >{ 4, 2 }()
///  │  2
///  │       
///  │  
///
///  *** SPECIAL CASES ***
///  We will forbid substituting expressions directly into variables:
///
///  auto _ = f( y + x, x ); // not allowed because l( 2, 4 ) is now unclearlly defined
///
///  auto _ = f( x, x );     // not allowed because l( 2, 4 ) is malformed 
       
/////////////////////////////////
/// Free and Bound Variables ///
///////////////////////////////
///
///
/// @brief trait to for free variables in an expression. The default 
///        implementation is empty to handle non-expressions and terminal
///        cases.
template< typename ExprT >
struct GetFreeVariables
{ 
    using type = unique_variables< >; 
    static constexpr type value( ExprT const& )
    { return {}; }
};

/// @brief container of bound variables and the substitution arguments
///        that will replace them (referents)
template< typename ExprT >
struct BoundVariables
{ 
    using expression_type = ExprT;

    // number of bound variables
    static constexpr size_t size = 0;

    // sorted list of variable types
    using variable_set = unique_variables< >; 

    // type of a tuple that will contain the values to-be-substituted
    // (referents)
    using referent_tuple = tuple< >;

    // sorted index sequence of binding order
    using binding_order_seq = seq< >;

    constexpr variable_set
    variables() const
    { return {}; }

    // method to find the values that will be substituted for the
    // variables in variable_set
    constexpr referent_tuple
    referents() const
    { return {}; }

    constexpr expression_type const&
    expression()
    { return _expr; }

    BoundVariables( expression_type const& expr ): _expr{ expr } { }
    BoundVariables( BoundVariables const& ) = default;
    BoundVariables( ) = default;

private:
    expression_type _expr;
};

/// @brief container for bound variables in a substitution expression
template< typename ExprT, typename... Subs >
struct BoundVariables< Substitution< ExprT, Subs... >> {
private:
    using expression_type = Substitution< ExprT, Subs... >;
    using formula_expression = ExprT;
    using formula_variable_set = GetFreeVariables< formula_expression >::type;

    static constexpr formula_variable_set
    formula_variables( expression_type const& expr )
    { return GetFreeVariables< formula_expression >::value( 
        std::get< 0 >( expr )); }

public:
    static constexpr size_t size = std::min( 
        formula_variable_set::size, sizeof...( Subs ));

private:
    // (1) identify variables and substitution expressions that are bound
    typedef make_seq< size > for_bindings;

    template< typename Seq >
    struct Trimmer;

    // trims the variables and substitutions to just those that were matched
    template< size_t... Is >
    struct Trimmer< seq< Is... >>
    { 
        using variable_set = unique_variables<
            typename formula_variable_set::template element_t< Is >... >;
        using referent_tuple = std::tuple< Subs...[ Is ]... >;
    };

public:
    using variable_set = Trimmer< for_bindings >::variable_set;
    using referent_tuple = Trimmer< for_bindings >::referent_tuple;

private:
    // (2) sort dependencies of bound expressions
    // 
    // Variables and Subs are paired.
    // 
    // logic: given two variable/expression pairs A and B 
    // - if expression(B) depends on variable(A) but expression(A) does not 
    //   depend on variable(B) we substitute B first
    // - otherwise we substitue in id order
    //
    // Truth Table:
    //                                              not(B dep A) 
    //  id(A) < id(B) | A dep B | B dep A | A < B | or (A dep B) | Undirected
    //  ---------------------------------------------------------------------
    //  True          | True    | True    | True  | True         | True 
    //  True          | True    | False   | True  | True         | False
    //  True          | False   | True    | False | False        | False
    //  True          | False   | False   | True  | True         | False
    //  False         | True    | True    | False | False        | True
    //  False         | True    | False   | True  | True         | False
    //  False         | False   | True    | False | False        | False
    //  False         | False   | False   | False | True**       | False
    //  
    // Note: Evaluation order should be the reverse of substitution order

    template< size_t I >
    using bound_variable_t = variable_set::template element_t< I >;

    template< size_t I >
    using referent_free_variables_t = GetFreeVariables< 
        std::tuple_element_t< I, referent_tuple >>::type;

    // I is dependent on J if the Ith referent's free variables contain the Jth
    // bound variable
    template< size_t I, size_t J >
    struct IsDependent: std::integral_constant< bool, 
        referent_free_variables_t< I >::template 
            contains< bound_variable_t< J >>() > 
    { };

    // handles all cases except the last exception (**)
    template< size_t I, size_t J >
    struct BindsBefore: std::integral_constant< bool,
        IsDependent< I, J >::value or not IsDependent< J, I >::value > 
    { 
        static constexpr bool is_circular = IsDependent< I, J >::value and
            IsDependent< J, I >::value; 
        static_assert( not is_circular, "circular dependency detected" );
    };

    // handles the exception case (**)
    template< size_t I, size_t J >
    requires( is_greater( I, J )) 
    struct BindsBefore< I, J >: std::integral_constant< bool,
        not BindsBefore< J, I >::value > 
    { };

    template< size_t I >
    struct PreceedingCount
    {
        template< typename Seq >
        struct Helper;

        template< size_t... Is >
        struct Helper< seq< Is... >>
        { static constexpr size_t value = 
            (( BindsBefore< Is, I >::value ? 1 : 0 ) + ... ); };

        static constexpr size_t value = Helper< for_bindings >::value;
    };

    template< typename Seq >
    struct SortedIndexSeq;

    template< size_t... Is >
    struct SortedIndexSeq< seq< Is... >>
    { 
        using position_seq = seq< PreceedingCount< Is >::value... >;
        using type = sort_index_seq< position_seq >;
    };

public:
    // this is the sequence that must be used when replacing variables with 
    // their referents when applying a substitution expression
    using binding_order_seq = SortedIndexSeq< for_bindings >::type;

    constexpr expression_type const&
    expression() const
    { return *_expr_ptr; }

    constexpr variable_set
    variables() const
    {
        formula_variable_set vars = formula_variables( expression() );

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr ->
            variable_set
        { return { vars.template at< Is >()... }; };

        return helper( for_bindings{} );
    }

    constexpr referent_tuple
    referents() const
    {
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr ->
            referent_tuple
        { return { std::get< 1 + Is >( expression() )... }; };

        return helper( for_bindings{} );
    }

    constexpr BoundVariables( expression_type const& expr ): _expr_ptr{ &expr } { }
    constexpr BoundVariables( BoundVariables const& ) = default;
    constexpr BoundVariables( ) = default;

private:
    expression_type const* _expr_ptr;
};

template< typename T >
using bound_variables_t = BoundVariables< T >;

template< typename ExprT >
constexpr bound_variables_t< ExprT >
bound_variables( ExprT const& expr )
{ return { expr }; }

template< typename ExprT >
struct IsStaticExpression: std::integral_constant< bool, true > { };

template< size_t I, typename T >
struct IsStaticExpression< Variable< I, T >>: 
    std::integral_constant< bool, false > { };

template< template< typename... > class Op, typename... Args >
requires( compound_expression< Op< Args... >> )
struct IsStaticExpression< Op< Args... >>: std::integral_constant< bool, 
    ( IsStaticExpression< Args >::value and ... )> { };

template< typename T >
concept static_expression = IsStaticExpression< T >::value;

////////////////////
/// StaticValue ///
//////////////////
/// 
/// Class to hold a value considered to be unchanging in an expression
///
/// @brief wrapper to turn any type into an expression
/// @tparam T the wrapped type
///
///
template< typename T >
struct StaticValue;

template< typename T >
struct IsExpression< StaticValue< T >>: std::true_type { };

/// @brief result of a static value is the result of it's value type
template< typename T >
struct Result< StaticValue< T >>: Result< T > { };

template< typename T >
requires( not is_expression_v< T > ) // not sure if we need this.. meta expressions?
struct StaticValue< T >
{ 
    using value_type = std::remove_cv_t< T >;

    // casting to and from an expression should be explicit
    explicit constexpr operator value_type() const
    { return _value; } 

    constexpr value_type get_value() const
    { return _value; }

    constexpr value_type operator ()() const
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

// results in a static expression unless T is already an expression type
template< typename T >
struct MakeExpression
{ 
    using type = StaticValue< T >;
    static constexpr type 
    value( T const& value )
    { return { value }; }
};

template< expression T >
struct MakeExpression< T >
{
    using type = T;
    static constexpr type
    value( T const& value )
    { return value; }
};

template< typename T >
constexpr typename MakeExpression< T >::type
make_expression( T const& value )
{ return MakeExpression< T >::value( value ); }

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
template< auto >
struct Constant;

template< auto Value >
struct IsExpression< Constant< Value >>: std::true_type { };

template< auto Value >
struct Result< Constant< Value >>
{ using type = std::remove_cv_t< decltype( Value )>; };

template< auto Value >
struct Constant
{
    using value_type = std::remove_cv_t< decltype( Value )>;

    static constexpr value_type value = Value;

    constexpr operator value_type() const
    { return value; }

    constexpr value_type operator ()() const
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

//template< typename ExprT >
//constexpr free_variables_t< ExprT >
//get_free_variables( ExprT const& expr )
//{ return detail::FreeVariables< ExprT >::value( expr ); }
//

/* TODO: bound variables
    template< typename UniqueVars, typename... UnmatchedSubs >
    struct Bound;

    // recursive case 
    template< typename FirstVar, typename... RestVars,
        typename FirstSub, typename... RestSubs >
    struct Bound< unique_variables< FirstVar, RestVars... >,
        FirstSub, RestSubs... >
    { using type = merge_unique_variables_t< 
        unique_variables< Variable< FirstVar::id, FirstSub >>,
        typename Bound< unique_variables< RestVars... >, RestSubs... >::type >; };

    // terminal case
    template< typename... UnmatchedSubs >
    struct Bound< unique_variables< >, UnmatchedSubs... >
    { using type = unique_variables< >; };

    // terminal case with unbound variables
    template< typename FirstVar, typename... RestVars >
    struct Bound< unique_variables< FirstVar, RestVars... >>
    { using type = unique_variables< >; };
*/

/// @brief trait to construct an object similar to a mathematical function
template< typename FormulaVar, variable... Vars >
struct FunctionalSubstitution;

/// @brief trait to construct a mathematical function-like substitution into
///        a variable
///
/// Increases the order of the formula and argument variables and sorts them 
/// according to DESIGN DECISION * Variable Substitutions.
///
template< size_t I, typename T, variable... Vars >
struct FunctionalSubstitution< Variable< I, T >, Vars... > {
private:
    typedef make_seq< sizeof...( Vars )> for_variables;
    static constexpr size_t formula_id = I;
    static constexpr size_t next_temporary_id = 
        next_temporary_variable_id_v< Variable< I, T >, Vars... >;

    // ...Vars indices sorted by Vars::id.  Used by struct SortArguments<...>
    using sorted_indices = sort_index_seq< seq<
        variable_id_v< Vars >... >>;

    /// the formula variable second-order and will be the last to be 
    /// substituted in the final substitution expression.
    using formula_variable = 
        Variable< next_temporary_id + sizeof...( Vars ), Variable< I, T >>;

    template< typename UnsortedSubExpr >
    struct SortArguments;
    
    // sorts the substitution arguments by the original variable id
    template< typename F, variable... SubVars >
    struct SortArguments< Substitution< F, SubVars... >>
    {
        template< typename SortedIndicesSeq >
        struct Helper;

        // re-arranges the arguments in the resultant substitution expression
        template< size_t... Ks >
        struct Helper< seq< Ks... >>
        {
            using type = Substitution< F, SubVars...[ Ks ]... >;

            /// ASSUMPTION: Substitutions are tuple-like 
            /// TODO: perhaps we should move the implementation until after substitutions
            ///       are defined to have clearer code?
            /// DT: This is required for bootstrapping so we gotta do what we gotta do
            static constexpr type value( Substitution< F, SubVars... > const& sub )
            { return { std::get< 0 >( sub ), std::get< 1 + Ks >( sub )... }; }
        };

        using type = Helper< sorted_indices >::type;
        static constexpr type value( Substitution< F, SubVars... > const& sub )
        { return Helper< sorted_indices >::value( sub ); }
    };

    template< typename Seq >
    struct Helper;

    // creates temporary second-order variables for resultant substitution
    // arguments and formula such that the formula has the largest id.
    template< size_t... Js >
    struct Helper< seq< Js... >>
    {
        using type = SortArguments< Substitution< formula_variable, 
            Variable< next_temporary_id + Js, Vars...[ Js ]>... >>::type;

        //static_assert( is_same_v< type, void >, "TEST" );

        static constexpr type value( Variable< I, T > const& formula,
            Vars const&... vars )
        { return SortArguments< Substitution< formula_variable,
            Variable< next_temporary_id + Js, Vars...[ Js ]>... >>::value({
                { formula.name() }, { vars.name() }... }); }
    };

public:
    using type = Helper< for_variables >::type;
    static constexpr type value( Variable< I, T > const& formula,
        Vars const&... vars )
    { return Helper< for_variables >::value( formula, vars... ); }
};

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
/// @brief variable implementation
template< size_t I, typename T >
struct Variable
{ 
    using value_type = T;
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
    /// Only unique variables are allowed as parameters to create such a 
    /// substitution
    template< variable First, variable... Rest > 
    requires( is_non_repeating_v< seq< variable_id_v< First >,
        variable_id_v< Rest >... >> )
    constexpr typename FunctionalSubstitution< this_type, 
        typename MakeExpression< First >::type, typename MakeExpression< Rest >::type... >::type
    operator ()( First first, Rest... rest ) const
    { return FunctionalSubstitution< this_type, 
        typename MakeExpression< First >::type, typename MakeExpression< Rest >::type... >::
            value( { name() }, first, rest... ); }

    // variables evaluate to themselves
    constexpr Variable const& 
    operator ()() const
    { return *this; }

    constexpr Variable( string const& name = "var" ): _name{ name } { };
    constexpr Variable( Variable const& ) = default; 
    constexpr Variable( Variable&& ) = default;

private:
    string _name;
};
//using t0 =Substitution< tuple< StaticValue< float >, Variable< 1, float >>, tuple< StaticValue< float >, Variable< 0, float >>>;
//using test_type0 = free_variables_t< t0 >;
//using test_type1 = BoundVariables< t0 >::variable_set; 
//static_assert( std::is_same_v< test_type0, void > );

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

/// @brief type of a variable given the index, order, and base type
template< size_t I, typename T, size_t Order >
struct VariableType
{ using type = Variable< I, typename VariableType< I, T, Order - 1 >::type >; };

template< size_t I, typename T >
struct VariableType< I, T, 0 >
{ using type = T; };

/////////////////////////////
/// Variable Declaration ///
///////////////////////////
///
/// @brief the declaration of a variable in a delcare_variables function
/// @tparam T the type of the variable
///
template< typename T, size_t Order = 1 >
struct VariableDeclaration
{ 
    using value_type = T;
    static constexpr size_t order = Order;

    template< size_t I >
    using variable_type = VariableType< I, value_type, order >::type;

    constexpr string const& name() const
    { return _name; }

    string _name = "var"; 
};

/// @brief primary way to declare a variable inside a declare_variables expression
/// @tparam T the type of this variable
/// @param name the name of this variable
/// @return a declaration of a variable
template< typename T, size_t Order = 1 >
constexpr VariableDeclaration< T, Order > var( string name = "var" )
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
        tuple< variable_value_t< Vars >... >;
    using dirty_tuple_type = std::array< bool, sizeof...( Vars )>;
    using variables_tuple_type = tuple< Vars... >;
    static constexpr size_t size = sizeof...( Vars );

protected:
    template< size_t I, typename Seq >
    struct Helper;

    template< size_t I, size_t J, size_t... Js >
    requires( I == variable_id_v< Vars...[ J ]> and 
        1 == variable_order_v< Vars...[ J ]> )
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
    requires( I == variable_id_v< Vars...[ J ]> and
        1 != variable_order_v< Vars...[ J ]> )
    struct Helper< I, seq< J, Js... >>
    { static constexpr bool has_value = false; };

    template< size_t I, size_t J, size_t... Js >
    requires( I != variable_id_v< Vars...[ J ]> )
    struct Helper< I, seq< J, Js... >>:
        Helper< I, seq< Js... >>
    { };

    template< size_t I >
    struct Helper< I, seq<>>
    { static constexpr bool has_value = false; };

    template< variable Var >
    using helper_for = Helper< variable_id_v< Var >, make_seq< size >>;

    template< size_t... Js >
    constexpr void initialize_flags( seq< Js... > )
    {(( std::get< Js >( _flags ) = false ), ... ); }

public:
    /// @brief determines if the scope has a value for variable I
    template< variable Var >
    static constexpr bool 
    has_value_v = Helper< variable_id_v< Var >, 
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
    requires( scope_contains_variable_v< variable_id_v< Var >, 
        Scope< Vars... >> )
    constexpr variable_value_t< Var >
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
        ( ScopeContainsVariable< variable_id_v< Vars >, ScopeT >::value 
            and ... )> { };

public:
    static constexpr bool value = 
        Helper< typename GetFreeVariables< ExprT >::type >::value;
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
    static constexpr type from_names( Names const&... names )
    { return { Variable< Is, Values...[ Is ]>{ names...[ Is ] }... }; }

    static constexpr type from_values( Values const&... values )
    { 
        type scope;
        ( set_value< Variable< Is, Values...[ Is ]>>( scope, values...[ Is ]), ... );
        return scope;
    }
};

template< typename... Values >
constexpr typename SimpleScopeHelper< make_seq< sizeof...( Values )>, Values... >::type
simple_scope( Values const&... values )
{ return SimpleScopeHelper< make_seq< sizeof...( Values )>, Values... >::
    from_values( values... ); }

/// @brief method to declare a set of variables to be used in expressions
/// @tparam ...Decls 
/// @param ...decls 
/// @return 
template< typename... Decls >
typename SimpleScopeHelper< make_seq< sizeof...( Decls )>, 
    typename Decls::value_type... >::type
constexpr declare_variables( Decls... decls )
{ return SimpleScopeHelper< make_seq< sizeof...( Decls )>, typename
    Decls::value_type... >::from_names( decls.name()... ); }

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

    // first order case
    template< variable Var >
    requires( not variable< variable_value_t< Var >> )
    struct Is< Var >: integral_constant< bool, variable_id_v< Var > == I > { };

    // higher order matches if the any of the nested variables match
    template< variable Var >
    requires( variable< variable_value_t< Var >> )
    struct Is< Var >: Is< variable_value_t< Var >> { };
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
    static constexpr typename ArgumentSub< ArgT >::type 
    sub_argument( ArgT const& arg, WithT const& with )
    { return ArgumentSub< ArgT >::value( arg, with ); }

public:
    using type = Op< typename ArgumentSub< Args >::type... >;

private:
    template< size_t... Is >
    static constexpr type 
    value_helper( Op< Args... > const& op, WithT const& with, seq< Is... > )
    { return { sub_argument( get_argument< Is >( op ), with )... }; }

public:
    static constexpr type 
    value( Op< Args... > const& op, WithT const& with )
    { return value_helper( op, with, make_seq< sizeof...( Args )>{} ); }
};

/// @brief trait to substitute variable Var with expression SubU in formula
///        ExprT
template< typename ExprT, variable Var, typename SubU >
struct SubstituteFor: PredicateSubstitution< 
    ForVariable< Var::id >::template Is, ExprT, SubU >
{ };

template< typename ExprT, variable Var, typename SubU >
using substitute_for_t = SubstituteFor< ExprT, Var, SubU >::type;

template< typename ExprT, variable Var, typename SubU >
constexpr substitute_for_t< ExprT, Var, SubU > 
substitute_for( ExprT const& expr, Var const& var, SubU const& sub )
{ return SubstituteFor< ExprT, Var, SubU >::value( expr, sub ); }

///////////////////////////////////
/// Substituter Implementation ///
/////////////////////////////////
///
template< typename ExprT, typename... Args >
//requires( sizeof...( Args ) == free_variables_t< ExprT >::size )
// DT: Do we need this requires if the substituter is only used in private contexts?
//requires( is_compatible_substitution_v< ExprT, Args... > ) //and
//    ( not is_variable_v< ExprT > or variable_order_v< ExprT > == 1 ))
struct Substituter {
private:
    typedef make_seq< sizeof...( Args )> for_arguments;

    using expression_type = ExprT;
    using substitution_type = Substitution< ExprT, Args... >;
    using bound_variables_type = bound_variables_t< substitution_type >;
    using bound_variable_set = bound_variables_type::variable_set;
    using referent_tuple = bound_variables_type::referent_tuple;

    template< size_t N >
    using bound_variable_t = bound_variable_set::template element_t< N >;

    template< size_t N >
    static constexpr size_t variable_id_of = 
        variable_id_v< bound_variable_t< N >>;

    template< size_t N >
    using referent_t = std::tuple_element_t< N, referent_tuple >;

    // this is a sequence indexing the variable set and referent_tuple
    using binding_order = bound_variables_type::binding_order_seq;
    // we reverse the binding order since these will be nested
    using substitution_order = reverse_integer_sequence_t< binding_order >;

    template< size_t N >
    static constexpr bound_variable_t< N >
    bound_var( bound_variables_type const& bindings )
    { return bindings.variables().template at< N >(); }

    template< size_t N >
    static constexpr referent_t< N >
    referent( bound_variables_type const& bindings )
    { return std::get< N >( bindings.referents()); }

    template< typename ExprU, typename SubstitutionOrderSeq >
    struct Helper;

    // null case for substitution of no arguments into an non-expression
    template< typename ExprU >
    struct Helper< ExprU, seq< >>
    {
        using type = ExprU;
        static constexpr type value( ExprU const& expr, 
            bound_variables_type const& bound_vars )
        { return expr; }
    };

    template< typename ExprU, size_t I, size_t... Is >
    struct Helper< ExprU, seq< I, Is... >>
    {
        using sub_type = substitute_for_t< ExprU, 
            bound_variable_t< I >, referent_t< I >>;
        using type = Helper< sub_type, seq< Is... >>::type;

        static constexpr type value( ExprU const& expr, 
            bound_variables_type const& bindings )
        { return Helper< sub_type, seq< Is... >>::value(
            substitute_for( expr, bound_var< I >( bindings ), 
                referent< I >( bindings )), bindings ); }
    };

public:
    using helper_type = Helper< expression_type, binding_order >;
    //using helper_type = Helper< expression_type, substitution_order >;

    // Type Manipulator Requirements //
    //using type = Helper< expression_type, binding_order >::type;
    using type = helper_type::type;

    static constexpr type value( expression_type const& expr, 
        Args const&... args )
    { 
        substitution_type sub{ expr, args... };
        bound_variables_type bound_vars{ sub };

        return helper_type::value( expr, bound_vars ); 
    }
    // //
};

// specialization of Substituter for non-expressions (idempotent)
template< typename T, typename... Args >
requires( not expression< T > )
struct Substituter< T, Args... >
{
    using type = T;
    static constexpr type value( T const& val, Args const&... )
    { return val; }
};

///////////////////////
/// Free Variables ///
/////////////////////
///
/// @brief trait to identify free variables in a tuple of expressions. 
template< typename... Ts >
struct GetFreeVariables< tuple< Ts... >>
{ 
    using type = merge_unique_variables_t< typename 
        GetFreeVariables< Ts >::type... >; 
    static constexpr type value( tuple< Ts... > const& expr )
    { 
        static constexpr make_seq< sizeof...( Ts )> for_elements;

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
        { return merge_unique_variables( GetFreeVariables< Ts >::value( 
            std::get< Is >( expr ))... ); };

        return helper( for_elements );
    }
};

/// @brief trait to identify free variables in a compound expression.  We reduce
///        to the tuple-case.
template< template< typename... > class Op, typename... Args >
requires( compound_expression< Op< Args... >> and 
    not is_substitution_expression_v< Op< Args... >> )
struct GetFreeVariables< Op< Args... >>: GetFreeVariables< tuple< Args... >> 
{ };

/// @brief variables have a single free variable
template< size_t I, typename T >
struct GetFreeVariables< Variable< I, T >>
{ 
    using type = unique_variables< Variable< I, T >>; 
    static constexpr type value( Variable< I, T > const& var )
    { return { var }; }
};

/// @brief trait to identify free variables in a functional (aka: variable
///        substitution) expression such as `auto g = f(x, y);`. 
template< size_t I, size_t J, typename ExprI, typename First, typename... Rest >
requires( functional_expression< Substitution< Variable< I, Variable< J, ExprI >>,
    First, Rest... >> )
struct GetFreeVariables< Substitution< Variable< I, Variable< J, ExprI >>, 
    First, Rest... >> 
{
private:
    using expression_type = Substitution< Variable< I, Variable< J, ExprI >>, 
        First, Rest... >;

    // referent type of variable I will be a substitution of the values
    // of First, Rest...
    using referent_type = Substitution< Variable< I, ExprI >, 
        variable_value_t< First >, variable_value_t< Rest >... >;

public:
    // variable J (with referent_type replacing nested variable) is 
    // free, as are any free variables in the value_types of First, Rest...
    using type = make_unique_variables_t< Variable< J, referent_type >,
        variable_value_t< First >, variable_value_t< Rest >... >;

    
    static constexpr type value( expression_type const& expr )
    { 
        static constexpr make_seq< sizeof...( Rest ) > for_rest;


        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
        { return make_unique_variables(
            Variable< I, referent_type >{ std::get< 0 >( expr ).name() },
            variable_value_t< First >{ std::get< 1 >( expr ).name() },
            variable_value_t< Rest...[ Is ]>{ 
                std::get< 2 + Is >( expr ).name() }... ); };

        return helper( for_rest );
    }
};

/// Free variables in a substitution expression must be defined early
/// TODO: we need the topological sorting logic from the Substituter here
///       to identify which variables remain open after substitution occurs
/// DT:  See the DESIGN decision for substitution variable numbering and ordering.
///      We may not need topological sorting afterall, and instead just let the 
///      user shoot themselves in the foot, in true c++ fashion.
///
/// TODO: Assess whether the change to elevate subs of a variable substitution must
///       be undone here?
/// DT: I think we just need to consider it when applying the substition...
template< typename ExprT, typename... Subs >
requires( not functional_expression< Substitution< ExprT, Subs... >> )
struct GetFreeVariables< Substitution< ExprT, Subs... >> {

    //static_assert( not is_same_v< ExprT, 
    //    Substitution< Variable< 12, Variable< 2, float >>, Variable< 11, Variable< 0, float >>>>, "TEST" );
private:
    using formula_type = ExprT;
    using expression_type = Substitution< formula_type, Subs... >;

    using formula_free_variable_set = GetFreeVariables< formula_type >::type;
    // theres the problem
    using bound_variables_type = BoundVariables< expression_type >;

    template< size_t BoundSize >
    struct UnboundIndexSeq;

    template< size_t BoundSize >
    requires( is_less( BoundSize, formula_free_variable_set::size ))
    struct UnboundIndexSeq< BoundSize >
    { using type = make_seq< formula_free_variable_set::size - BoundSize >; };

    template< size_t BoundSize >
    requires( not is_less( BoundSize, formula_free_variable_set::size ))
    struct UnboundIndexSeq< BoundSize >
    { using type = seq< >; };

       
    static constexpr size_t bound_size = bound_variables_type::size;
    
    typedef UnboundIndexSeq< bound_size >::type for_unbound;
    typedef make_seq< sizeof...( Subs )> for_subs;

    template< typename Seq >
    struct MergedArgumentVars;

    template< size_t... Is >
    struct MergedArgumentVars< seq< Is... >>
    {
        using type = merge_unique_variables_t<
            typename GetFreeVariables< formula_type >::type,
            typename GetFreeVariables< Subs...[ Is ]>::type... >;

        static constexpr type value( formula_type const& formula,
            Subs const&... subs )
        {
            return merge_unique_variables(
                GetFreeVariables< formula_type >::value( formula ),
                GetFreeVariables< Subs...[ Is ]>::value( subs...[ Is ])... );
        }
    };
  
public:
    using type = subtract_unique_variables_t< 
        typename MergedArgumentVars< for_subs >::type,
        typename bound_variables_type::variable_set >;


    //static_assert( is_same_v< typename MergedArgumentVars< for_subs >::type, void > );
    //static_assert( is_same_v< typename bound_variables_type::variable_set, void > );

    static constexpr type value( expression_type const& expr )
    { 
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
        { return subtract_unique_variables( MergedArgumentVars< for_subs >::value(
            std::get< 0 >( expr ), std::get< 1 + Is >( expr )... ),
                bound_variables_type{ expr }.variables()); };

        return helper( for_subs{} );
    }
};

/// @brief unique_variable list of free variables in an expression
template< typename ExprT >
using free_variables_t = 
    GetFreeVariables< std::remove_cv_t< ExprT >>::type;

template< typename ExprT >
constexpr free_variables_t< ExprT > 
get_free_variables( ExprT const& expr )
{ return GetFreeVariables< ExprT >::value( expr ); }

template< typename T >
struct IsClosedExpression: std::integral_constant< bool, true > { };

template< expression T >
struct IsClosedExpression< T >: std::integral_constant< bool,
    ( free_variables_t< T >::size == 0 )> { };

/// @brief A static expression contains no variables
template< typename T >
concept closed_expression = IsClosedExpression< T >::value; 
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
        is_same_v< result_t< Var >, result_t< TestT >>> 
    { using matches_type = tuple< match< Var, TestT >>; };
};

/// @brief specialization for compound expressions with dependent variables
///
template< template< typename... > class Op, typename... Args >
requires( compound_expression< Op< Args... >> and 
    is_greater( free_variables_t< Op< Args... >>::size, 0 ))
struct ForExpression< Op< Args... >> {
private:
    using expression_type = Op< Args... >;
    using arguments_tuple = expression_type::arguments_tuple;

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
    struct AreArgumentMatchesCompatible< MatchesT >: integral_constant< bool, 
        MatchesT::value >
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

/////////////////////////////////
/// Compatible Substitutions ///
///////////////////////////////
///
/// Trait to verify whether a set of substitution arguments may bind the free
/// variables in an expression.
///
namespace detail {
template< typename ExprT, typename... Subs >
struct IsCompatibleSubstitution;

/// @brief Helper validates a single variable against a substitution argument
template< typename Var, typename Sub >
struct IsCompatibleVariableSubstitution;

/// Case: First-order compatibility
///
/// If the result type of the variable and substitution do not match
/// then this is not a compatible variable substitution.  We have a separate
/// logical path for higher-order variables.
///
template< size_t I, typename ExprT, typename Sub >
requires( not is_substitution_expression_v< ExprT > )
struct IsCompatibleVariableSubstitution< Variable< I, ExprT >, Sub >: 
    integral_constant< bool, 
        std::is_convertible_v< result_t< ExprT >, result_t< Sub >>> { }; 

/// Case: Higher-order compatibility
///
/// This variable has been substituted into by ...SubSubs.  We must check the
/// result, but also we must ensure that the substitution into the variable
/// will still be valid after Sub is substituted for Variable< I, ExprT >.
///
/// DT: This is creating a circular reference between variable sub compatability and
///     overall expression sub compatibility.
/// DT: With temporary variables, the ids don't have to match
template< size_t I, size_t J, typename ExprT, typename... SubSubs, typename Sub >
// DT: free_variables_t sticks the Substitution expression into the value_type
//     of the second-order variables, but they could be part of a tuple...
//requires( is_substitution_expression_v< ExprT > ) // redundant but kept for clarity
struct IsCompatibleVariableSubstitution< 
    Variable< I, Substitution< Variable< J, ExprT >, SubSubs... >>, Sub >:
        integral_constant< bool,
            std::is_convertible_v< result_t< ExprT >, result_t< Sub >> and
            IsCompatibleSubstitution< Sub, SubSubs... >::value >
{ };

///
/// @brief Helper decides whether substitution into tuple< Vars... > by 
/// tuple< Subs... > is allowed
///
/// TODO: remove failed assertion here to allow SFINAE
///
template< typename UniqueFreeVariables, typename SubTuple >
struct IsCompatibleSubstitutionHelper;
//
//{ 
//    static_assert( IsCompatibleVariableSubstitution<
//        std::tuple_element_t< 0, typename UniqueFreeVariables::variables_tuple >,
//        std::tuple_element_t< 0, SubTuple >>::value,
//            "NO COMPATIBLE SUBSTITUTION" ); 
//};

/// Case: The result of the first substitution argument is convertible to
///       the result of the first variable.  Move on to the next variable and
///       substitution pair.
template< typename Var, typename... Vars, typename Sub, typename... Subs >
requires( IsCompatibleVariableSubstitution< Var, Sub >::value )
struct IsCompatibleSubstitutionHelper< 
    unique_variables< Var, Vars... >, tuple< Sub, Subs... >>: 
        IsCompatibleSubstitutionHelper< unique_variables< Vars... >, tuple< Subs... >> 
{ };

/// Case: The first variable's and substitution's results are not convertible
///       so this substitution is incompatible.
template< typename Var, typename... Vars, typename Sub, typename... Subs >
requires( not IsCompatibleVariableSubstitution< Var, Sub >::value )
struct IsCompatibleSubstitutionHelper< 
    unique_variables< Var, Vars... >, tuple< Sub, Subs... >>: integral_constant< bool,
        false > 
{ };

/// Case: We allow a partial substitution
template< typename Var, typename... Vars >
struct IsCompatibleSubstitutionHelper< unique_variables< Var, Vars... >, tuple<> >: 
    integral_constant< bool, true > { };

/// Case: We allow over-substitutions and empty substitutions
template< typename... Subs >
struct IsCompatibleSubstitutionHelper< unique_variables< >, tuple< Subs... >>:
    integral_constant< bool, true > { };

/// Finally we have the primary class to verify substitutions
///
/// Case: Empty substitutions are always valid 
template< typename ExprT >
struct IsCompatibleSubstitution< ExprT >: std::true_type { };

/// Case: Substitution into a variable expression are valid if the result
///       types are convertible, regardless of variable order
template< variable Var, typename U >
//requires( variable_traits< Var >::order == 1 )
struct IsCompatibleSubstitution< Var, U >: std::is_convertible< 
    result_t< U >, variable_value_t< Var >> 
{ }; 

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
//struct IsCompatibleSubstitution< Var, Subs... >: std::true_type { };

template< typename... Ts, typename... Subs >
struct IsCompatibleSubstitution< tuple< Ts... >, Subs... >:
    IsCompatibleSubstitutionHelper< free_variables_t< tuple< Ts... >>,
        tuple< Subs... >>
{ };

template< shape ShapeT, typename... Ts, typename... Subs >
struct IsCompatibleSubstitution< Tensor< ShapeT, Ts... >, Subs... >:
    IsCompatibleSubstitutionHelper< free_variables_t< Tensor< ShapeT, Ts... >>,
        tuple< Subs... >>
{ };

template< typename ExprT, typename... Subs >
requires( compound_expression< ExprT > )
struct IsCompatibleSubstitution< ExprT, Subs... >:
    IsCompatibleSubstitutionHelper< 
        free_variables_t< ExprT >, tuple< Subs... >>
{ };
} // namespace detail

template< typename ExprT, typename... Subs >
constexpr bool is_compatible_substitution_v = 
    detail::IsCompatibleSubstitution< ExprT, Subs... >::value;

// I'm not specializing IsCompatibleSubstitution properly.  When passed a substitution
// It doesn't divide it into the formula and arguments properly, or something...
//static_assert( detail::IsCompatibleSubstitution<Substitution<tuple<StaticValue<float>, Variable<1, float>>, tuple<StaticValue<float>, Variable<0, float>>>, float>::value );

//////////////////
/// Evaluator ///
////////////////
///
/// An evaluator is an expression manipulator that returns the result_type
/// corresponding to the expressions calculated value.
///
/// @brief default evaluator
template< typename ScopeT = void >
struct Evaluator;

template< typename T >
concept non_expression = not expression< T >;

template< typename T >
concept open_expression = non_expression< T > or not closed_expression< T >;

/// @brief void evaluator will recursively evaluate compound expressions
///        using the static value method, and understands non-expressions,
///        Constant<...> and StaticValue<...> types. It cannot evaluate
///        Variable<...> types and is idempotent expressions containing
///        free variables.
///
/// EXCEPTION: unlike other expression manipulator's Evaluator<void> handles
///            the recursion into compound expressions itself.  This is
///            required because Evaluator<void> is used by Arguments<...>
///            to implement operator() (BOOTSTRAPING)
///
/// DT: I've gone back and forth on where to put the logic for re-recursing
///     expressions to complete their evaluation/manipulator application.  It
///     could go in the evaluator or in the applier.  It may be needed in both?
///
template< >
struct Evaluator< void >
{ 
    template< open_expression T >
    constexpr T operator ()( T const& val ) const
    { return val; }

    template< auto Value >
    constexpr auto operator ()( Constant< Value > const& expr ) const
    { return Value; }

    template< typename T >
    constexpr T operator ()( StaticValue< T > const& expr ) const
    { return expr.get_value(); } 
};

//////////////////////////////////////////////////////
/// Application of a Manipulator to an Expression ///
////////////////////////////////////////////////////
/// 
/// The Applier class recurses into compound expressions that are not
/// understood directly by a manipulator and uses the expression's static
/// value method to return a final manipulated expression.  This allows
/// manipulator types to only implement invocation (operator()) on types
/// they care about, and the applier handles the recursion including 
/// substitutions.
///
/// @brief unspecialized Applier is idempotent
template< typename ExprT, typename ManipulatorT >
struct Applier
{
    using expression_type = ExprT;
    using manipulator_type = ManipulatorT;

    static constexpr size_t max_processing_depth = 100;
    static constexpr size_t max_processing_steps = 10;

    template< typename Current, typename... History > 
    struct Processor;

    template< typename Current, typename... History >
    requires( sizeof...( History ) >= max_processing_depth )
    struct Processor< Current, History... >
    { static_assert( sizeof...( History ) < max_processing_depth,
        "maximum processing depth" ); };

    template< int Steps, typename Current, typename... History >
    struct Repeater;

    template< int Steps, typename Current, typename... History >
    requires( Steps >= max_processing_steps )
    struct Repeater< Steps, Current, History... >
    { static_assert( Steps < max_processing_steps, 
        "maximum processing steps" ); };

    template< auto Value, typename... History >
    requires( sizeof...( History ) < max_processing_depth )
    struct Processor< Constant< Value >, History... >
    {
        using type = std::remove_cvref_t< decltype( Value )>;
        static constexpr type value( Constant< Value > const&, manipulator_type& )
        { return Value; }
    };

    template< typename T, typename... History >
    requires( sizeof...( History ) < max_processing_depth )
    struct Processor< StaticValue< T >, History... >
    {
        using type = T;
        static constexpr type value( StaticValue< T > const& expr, manipulator_type& )
        { return expr.get_value(); }
    };

    template< open_expression T, typename... History >
    requires( sizeof...( History ) < max_processing_depth )
    struct Processor< T, History... >
    {
        using type = T;
        static constexpr type value( T const& open_expr, manipulator_type& )
        { return open_expr; }
    };

    // Case: This is a compound expression
    template< template< typename... > class Op, typename... Args, 
        typename... History >
    requires( compound_expression< Op< Args... >> and 
    //    not is_substitution_expression_v< Op< Args... >> and
        sizeof...( History ) < max_processing_depth )
    struct Processor< Op< Args... >, History... >
    { 
        typedef make_seq< sizeof...( Args )> for_arguments;
    
        template< typename Seq >
        struct Helper;
    
        // (1) applies the manipulator on the arguments of the compound expression and
        //     calls the Op< Args... >::value method on the result.
        template< size_t... Is >
        struct Helper< seq< Is... >>
        {
            using type = std::remove_cvref_t< decltype( Op< Args... >::value( 
                typename Processor< Args...[ Is ], Op< Args... >, History... >::type{}... )) >;
    
            // apply the manipulator to the arguments and recombind them with the value method
            static constexpr type value( Op< Args... > const& expr, manipulator_type& f )
            { return Op< Args... >::value( Processor< Args...[ Is ], Op< Args... >, History... >::
                value( std::get< Is >( expr ), f )... ); }
        };

        using type = Helper< for_arguments >::type;
   
        static constexpr type
        value( Op< Args... > const& expr, manipulator_type& f )
        { return Helper< for_arguments >::value( expr, f ); }
    };

    // Case: This is a substitution expression. Substitutions are processed top down (maybe...)
    //template< typename FormulaT, typename... Subs, typename... History >
    //requires( sizeof...( History ) < max_processing_depth )
    //struct Processor< Substitution< FormulaT, Subs... >, History... >
    //{
    //    using substituter = Substituter< FormulaT, Subs... >;
    //    using substituted_type = substituter::type;
    //    using type = Processor< substituted_type, 
    //       Substitution< FormulaT, Subs... >, History... >;

    //    static constexpr type value( Substitution< FormulaT, Subs... > const& sub, 
    //        manipulator_type& f )
    //    { 
    //        static constexpr make_seq< sizeof...( Subs )> for_subs;

    //        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
    //        { return Processor< substituted_type, 
    //            Substitution< FormulaT, Subs... >, History... >::value( 
    //                substituter::value( std::get< 0 >( sub ), 
    //                    std::get< 1 + Is >( sub )... ), f ); };

    //        return helper( for_subs );
    //    }
    //};

    // our manipulator accepts this result
    template< size_t Steps, typename ExprU, typename... History >
    requires( std::is_invocable_v< manipulator_type, ExprU > and Steps < max_processing_steps )
    struct Repeater< Steps, ExprU, History... >
    {
        using type = std::invoke_result_t< manipulator_type, ExprU >;

        static constexpr type 
        value( ExprU const& expr, manipulator_type& f )
        { return std::invoke( f, expr ); }
    };

    // as long as the processor accepts it and the manipulator doesn't, keep processing
    template< size_t Steps, typename Current, typename... History >
    requires( not std::is_invocable_v< manipulator_type, Current > and requires { 
        typename Processor< Current, History... >::type; } and
            Steps < max_processing_steps )
    struct Repeater< Steps, Current, History... >
    {
        using processed_type = Processor< Current, History... >::type;
        using type = Repeater< Steps + 1, processed_type, Current, History... >::type;

        static constexpr type value( Current const& expr, manipulator_type& f )
        { return Repeater< Steps + 1, processed_type, Current, History... >::value(
            Processor< Current, History... >::value( expr, f ), f ); }
    };
            
    // if the processor does not accept it, return it
    template< size_t Steps, typename Current, typename... History >
    requires( not std::is_invocable_v< manipulator_type, Current > and not requires { 
        typename Processor< Current, History... >::type; } and
            Steps < max_processing_steps )
    struct Repeater< Steps, Current, History... >
    {
        using type = Current;
        static constexpr size_t execution_steps = Steps;
        using processing_history = std::tuple< History... >;

        static constexpr type value( Current const& expr, manipulator_type& )
        { return expr; }
    };

    using type = Repeater< 0, expression_type >::type;
//    static constexpr type value( expression_type const& expr, ManipulatorT& f )
//    { return Repeater< 0, expression_type >::value( expr, f ); }

    static constexpr type value( expression_type expr, ManipulatorT& f )
    { return Repeater< 0, expression_type >::value( expr, f ); }
};

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
struct Arguments;

/// @brief base class that specializations of Arguments inherit from
template< template< typename... > class Op, typename... Args >
struct ArgumentsBase: tuple< Args... >
{
    using arguments_tuple = tuple< Args... >;
    static constexpr size_t arguments_size = sizeof...( Args );
    static constexpr make_seq< arguments_size > for_arguments;

    constexpr arguments_tuple const& arguments() const
    { return *this; }

    using expression_type = Op< Args... >;

    /// @brief reconstructs the expression from arguments using the Curiously
    ///        Recurring Template Pattern (CRTP)
    constexpr expression_type 
    expression() const
    {
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr ->
            expression_type
        { return { std::get< Is >( static_cast< arguments_tuple >( *this ))... }; };

        return helper( for_arguments ); 
    }

    /// @brief Substitution via invocation operator
    /// Case: our expression contains free variables so we return a substitution
    ///       expression.
    template< typename First, typename... Rest >
    requires( is_compatible_substitution_v< expression_type, First, Rest... > and
        is_greater( free_variables_t< Substitution< expression_type, typename 
            MakeExpression< First >::type, typename 
                MakeExpression< Rest >::type... >>::size, 0 ))
    constexpr Substitution< expression_type, typename MakeExpression< First >::type, 
        typename MakeExpression< Rest >::type... >
    operator ()( First first, Rest... rest ) const
    { 
        using substituter_type = Substituter< expression_type, typename MakeExpression< First >::type, 
            typename MakeExpression< Rest >::type... >;

//        static_assert( not is_same_v< expression_type, 
//            Substitution< Variable< 12, Variable< 2, float >>, Variable< 11, Variable< 0, float >>>> or
//                is_same_v< First, float >, "TEST" );

//        return substituter_type::value( expression(), make_expression( first ), 
//            make_expression( rest )... ); 
        return { expression(), make_expression( first ), make_expression( rest )... };
    }
  
    /// @brief Substitution results in an expression with no free variables,
    /// Case:  no free variables remain so evaluate it and return the result
    template< typename First, typename... Rest >
    requires( is_compatible_substitution_v< expression_type, First, Rest... > and
        free_variables_t< Substitution< expression_type, First, Rest...>>::size == 0 )
    constexpr result_t< expression_type >
    operator ()( First first, Rest... rest ) const
    { return Substituter< expression_type, typename MakeExpression< First >::type, 
        typename MakeExpression< Rest >::type... >::value(
        expression(), make_expression( first ), make_expression( rest )... )(); }

    constexpr ArgumentsBase( Args const&... args ): arguments_tuple( args... ) { }
    constexpr ArgumentsBase( ArgumentsBase const& ) = default;
    constexpr ArgumentsBase() = default;
};

/// @brief specialization of Arguments to handle free variables
///
/// Our substitution and invocation operators for expressions containing free 
/// variables
///
template< template< typename... > class Op, typename... Args >
requires( is_greater( free_variables_t< Op< Args... >>::size, 0 ))
struct Arguments< Op, Args... >: ArgumentsBase< Op, Args... >
{
    using expression_type = ArgumentsBase< Op, Args... >::expression_type;

    template< typename First, typename... Rest >
    constexpr auto 
    operator ()( First first, Rest... rest ) const
    { return ArgumentsBase< Op, Args... >::operator ()( first, rest... ); }

    // invocation of an expression with free variables is idempotent
    constexpr expression_type
    operator ()() const
    { return ArgumentsBase< Op, Args... >::expression(); }

protected:
    constexpr Arguments( Args const&... args ): 
        ArgumentsBase< Op, Args... >{ args... } { }
    constexpr Arguments( Arguments const& ) = default;
    constexpr Arguments() = default;
};

/// @brief specialization of Arguments with no free variables
template< template< typename... > class Op, typename... Args >
requires( free_variables_t< Op< Args... >>::size == 0 )
struct Arguments< Op, Args... >: ArgumentsBase< Op, Args... >
{
    using expression_type = ArgumentsBase< Op, Args... >::expression_type;

    static constexpr make_seq< sizeof...( Args )> for_arguments;

    template< typename First, typename... Rest >
    constexpr auto
    operator ()( First first, Rest... rest ) const
    { return ArgumentsBase< Op, Args... >::operator ()( first, rest... ); }    

    // invocation of an expression with no free variables evaluates it
    constexpr result_t< expression_type >
    operator ()() const
    { 
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> 
            result_t< expression_type >
        { return expression_type::value( 
            std::get< Is >( ArgumentsBase< Op, Args... >::arguments() )()... ); };

        return helper( for_arguments );
    }

protected:
    constexpr Arguments( Args const&... args ): 
        ArgumentsBase< Op, Args... >{ args... } { }
    constexpr Arguments( Arguments const& ) = default;
    constexpr Arguments() = default;
};

//////////////////////////////////
/// Evaluation of Expressions ///
////////////////////////////////
///
///
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

// TODO: remove the specializations for this, and let the Applier do it
template< typename ScopeT >
struct Evaluator
{
    using scope_type = ScopeT;

    // OPTION: standardizing manipulator return type deduction
    template< typename ExprT >
    struct ResultOf
    { using type = result_t< ExprT >; };

    template< variable Var >
    constexpr typename Var::value_type
    operator ()( Var const& var ) const
    { return _scope_ptr->get_value( var ); }

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

//    template< typename ExprT, typename... Subs >
//    constexpr auto
//    operator ()( Substitution< ExprT, Subs... > const& sub_expr ) const;

    constexpr Evaluator(): _scope_ptr{ nullptr } { };
    constexpr Evaluator( Evaluator const& ) = default;
    constexpr Evaluator( scope_type const& scope ): _scope_ptr{ &scope } { }

    scope_type const* _scope_ptr;
};

// TODO: remove the specializations for this, and let the Applier do it
// TODO: Create a mechanism to determine the return type of applying this
//       manipulator to an expression
//       This should be standardized across manipulators
//
//       Maybe manipulator_traits?
//       Or a nested templated struct with a standard name?

template< typename FuncT >
constexpr detail::ManipulatorFunctor< FuncT > 
manipulate( FuncT&& func )
{ return { func }; }

template< typename ScopeT >
constexpr Evaluator< ScopeT > 
eval( ScopeT const& scope )
{ return { scope }; }

constexpr Evaluator< void > 
eval()
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
constexpr auto
operator |( T const& value_or_expression, ManipulatorT&& manipulator )
{ return manipulator( value_or_expression ); }

/// @brief application of a compound expression against a manipulator reference
/// that does not have an overloaded invocation operator starts a dual recursion
/// between the compound expression's apply method and the application operator|
template< compound_expression ExprT, typename ManipulatorT >
requires( not std::invocable< ManipulatorT, ExprT > )
constexpr typename Applier< ExprT, ManipulatorT >::type
operator |( ExprT const& expr, ManipulatorT&& f )
{ return Applier< ExprT, ManipulatorT >::value( expr, f ); }

/// @brief applier specialization for constants
template< auto Value, typename ManipulatorT >
requires( not std::invocable< ManipulatorT, Constant< Value >> )
constexpr typename Applier< Constant< Value >, ManipulatorT >::type
operator |( Constant< Value > const& const_expr, ManipulatorT&& f )
{ return Applier< Constant< Value >, ManipulatorT >::value( const_expr, f ); }

/// @brief applier specialization for static values
template< typename T, typename ManipulatorT >
requires( not std::invocable< ManipulatorT, StaticValue< T >> )
constexpr typename Applier< StaticValue< T >, ManipulatorT >::type
operator |( StaticValue< T > const& static_expr, ManipulatorT&& f )
{ return Applier< StaticValue< T >, ManipulatorT >::value( static_expr, f ); }

/// @brief applier specialization for variables
template< variable Var, typename ManipulatorT >
requires( not std::invocable< ManipulatorT, Var > )
constexpr typename Applier< Var, ManipulatorT >::type
operator |( Var const& var, ManipulatorT&& f )
{ return Applier< Var, ManipulatorT >::value( var, f ); }


/// @brief application of a value or expression against a const manipulator
/// reference that has an overloaded invocation operator yields the invocation of
/// the manipulator on the value or expression
/// 
/// NOTE: I'm not sure we want to handle const manipulators. I'm going to comment 
///       this out for now
//template< typename T, typename ManipulatorT >
//requires std::invocable< const ManipulatorT, T >
//constexpr auto operator |( T const& value_or_expression, ManipulatorT const& manipulator )
//{ return manipulator( value_or_expression ); }

/// @brief application of a compound expression against a const manipulator
/// reference that does not have an overloaded inovcation operator starts a dual
/// recursion between the compound expression's apply method and the application
/// operator|
///
/// This is where the rubber meets the road: (3*x | eval()) fails because product 
/// doesn't know evaluator yet, so neither can determine the return type
///
/// NOTE: I don't know if we want to handle const Manipulators...  Going to comment
///       this out for now.
//template< compound_expression ExprT, typename ManipulatorT >
//requires( not std::invocable< const ManipulatorT, ExprT >)
//constexpr auto operator |( ExprT const& expr, ManipulatorT const& manipulator )
//{ return expr.apply( manipulator ); }

template< typename TupleT, typename ManipulatorT >
struct TupleApplier;

template< typename... Ts, typename ManipulatorT >
struct TupleApplier< tuple< Ts... >, ManipulatorT > {
private:
    typedef make_seq< sizeof...( Ts )> for_tuple_elements;

    template< typename Seq >
    struct Helper;

    template< size_t... Is >
    struct Helper< seq< Is... >>
    {
        using type = tuple< typename Applier< Ts...[ Is ], ManipulatorT >::type
            ... >;

        static constexpr type value( tuple< Ts... > const& tup, ManipulatorT& f )
        { return { Applier< Ts...[ Is ], ManipulatorT >::value( 
            std::get< Is >( tup ), f )... }; }
    };

public:
    using type = Helper< for_tuple_elements >::type;

    static constexpr type value( tuple< Ts... > const& tup, ManipulatorT& f )
    { return Helper< for_tuple_elements >::value( tup, f ); }
};

template< typename TensorT, typename ManipulatorT >
struct TensorApplier;

template< shape S, typename... Ts, typename ManipulatorT >
struct TensorApplier< Tensor< S, Ts... >, ManipulatorT > {
private:
    typedef make_seq< sizeof...( Ts )> for_tensor_elements;

    template< typename Seq >
    struct Helper;

    template< size_t... Is >
    struct Helper< seq< Is... >>
    {
        using type = Tensor< S, typename Applier< Ts...[ Is ], ManipulatorT >::type
            ... >;

        static constexpr type value( Tensor< S, Ts... > const& ten, ManipulatorT& f )
        { return { Applier< Ts...[ Is ], ManipulatorT >::value( 
            tensor_get< Is >( ten ), f )... }; }
    };

public:
    using type = Helper< for_tensor_elements >::type;

    static constexpr type value( Tensor< S, Ts... > const& ten, ManipulatorT& f )
    { return Helper< for_tensor_elements >::value( ten, f ); }
};

/// @brief application of a tuple of at least one expression and a manipulator
/// that is not invocable on the tuple yields a tuple of the result of applying
/// the manipulator against each element of the tuple.
template< typename... Ts, typename ManipulatorT >
requires( expression< tuple< Ts... >> and 
    not std::invocable< ManipulatorT, tuple< Ts... >> )
constexpr typename TupleApplier< tuple< Ts... >, ManipulatorT >::type
operator |( tuple< Ts... > const& expr_tup, ManipulatorT& f )
{ return TupleApplier< tuple< Ts... >, ManipulatorT >::value( expr_tup, f ); }

/// @brief application of a tensor of at least one expression and a manipulator
/// that is not invocable on the tensor yields a tensor of the result of applying
/// the manipulator against each element of the tensor.
template< typename ShapeT, typename... Exprs, typename ManipulatorT >
requires( expression< Tensor< ShapeT, Exprs... >> and
    not std::invocable< ManipulatorT, Tensor< ShapeT, Exprs... >> )
constexpr typename TensorApplier< Tensor< ShapeT, Exprs... >, ManipulatorT >::type
operator |( Tensor< ShapeT, Exprs... > const& expr_ten, ManipulatorT& f )
{ return TensorApplier< Tensor< ShapeT, Exprs... >, ManipulatorT >::value(
    expr_ten, f ); }

// expression value type: trait for the return type of calling the value method
// on an expression, and can handle expression leaves and non-expressions as well
template< typename T >
struct ExpressionValue
{ using type = T; };

template< auto Value >
struct ExpressionValue< Constant< Value >>
{ using type = std::remove_cv_t< decltype( Value )>; };

template< typename T >
struct ExpressionValue< StaticValue< T >>
{ using type = T; };

template< size_t I, typename T >
struct ExpressionValue< Variable< I, T >>
{ using type = T; };

template< template< typename... > class Op, typename... Args >
requires compound_expression< Op< Args... >>
struct ExpressionValue< Op< Args... >>
{ using type = std::remove_cv_t< decltype( Op< Args... >::value( Args{}... ))>; };

template< typename T >
using expression_value_t = ExpressionValue< T >::type;

/////////////////////////////////////
/// Scope< ...Vars >::operator() ///
///////////////////////////////////
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
// DEBUG: remove this forward decl
template< typename... >
struct Product;

template< >
struct IsExpressionOperation< Product >: std::true_type { };

template< typename... Args >
struct Sum;

template< >
struct IsExpressionOperation< Sum >: std::true_type { };

/////////////////////
/// Substitution ///
///////////////////
/// 
/// @brief expression manipulator for substitution of first order variables
///
// evaluation of substitutions
// Substitution< ExprT, ...Subs > are really a recursive definition:
//  Substitution< 
//      Substitution<
//          Substitution< 
//              ExprT, 
//              Subs...[ Is...[ 0 ]]>,
//          Subs...[ Is...[ 1 ]]>,
//      Subs...[ Is...[ 2 ]]>
//
//template< typename ExprT, typename... Subs >
//requires( free_variables_t< Substitution< ExprT, Subs... >>::size == 0 )
//struct Evaluator< void >::Helper< Substitution< ExprT, Subs... >>
//{
//    using expression_type = Substitution< ExprT, Subs... >;
//    using substituted_type = Substituter< ExprT, Subs... >::type;
//    using bound_variables_type = BoundVariables< expression_type >;
//
//    using type = Evaluator< void >::Helper< substituted_type >::type;
//
//    
//
//    static constexpr type value( expression_type const& expr )
//    { 
//        static constexpr typename bound_variables_type::binding_order_seq 
//            for_subs;
//
//        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> 
//            substituted_type
//        { return Substituter< ExprT, Subs... >::value( std::get< 0 >( expr ),
//            std::get< 1 + Is >( expr )... ); };
//
//        return Evaluator< void >::Helper< substituted_type >::value( 
//            helper( for_subs ));
//    }
//};

// helper function for substituter
//template< typename ExprT, typename... Subs >
//constexpr typename Substituter< ExprT, Subs... >::type
//do_substitution( Substitution< ExprT, Subs... > const& expr )
//{ 
//    static constexpr make_seq< sizeof...( Subs )> for_subs;
//
//    auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> 
//        typename Substituter< ExprT, Subs... >::type
//    { return Substituter< ExprT, Subs... >::value( get_argument< 0 >( expr ),
//        get_argument< Is >( expr )... ); };
//
//    return helper( for_subs );
//};

/// @brief substitution for higher-order variables results in an expression 
///
/// Substitutions are complex compound expressions. 
///  (1) COLLECT FREE VARIABLES:
///      Free variables are found in ExprT (see GetFreeVariables)
///  (2) BIND VARIABLES:
///      Each free variable is matched with a substitution expression from 
///      ...Subs in var::id order (see BoundVariables)
///  (3) DEPENDENCY SORTING:
///      Bindings are sorted topologically by dependencies between the
///      substitution arguments (...Subs) and the free variables from
///      ExprT.
///  (4) SUBSTITUTION EVALUATOIN:
///      A `Substitution< ExprT, Subs... >` is evaluated by evaluating
///      a recursive list of SubstitutionFor pseudo-expressions:
///
///      Variable< 0, int > x;
///      Variable< 1, int > y;
///      
///      assert( 
///         substitute( x + y, 3, 4 ) ==
///         substitute_for( substitute_for( x + y, x, 3 ), y, 4 ));
///     
///      
///
/// 
/// This is returened from the operator() of Arguments
template< typename ExprT, typename... Subs >
//requires( is_compatible_substitution_v< ExprT, Subs... > )
struct Substitution: Arguments< Substitution, ExprT, Subs... >
{
    using expression_type = ExprT;
    static constexpr make_seq< sizeof...( Subs )> for_arguments;

private:

public:
    // Compound Expression Requirements //
    using result_type = result_t< expression_type >;

    constexpr expression_type 
    formula() const
    { return get_argument< 0 >( *this ); }

    template< size_t I >
    constexpr Subs...[ I ]
    arg() const 
    { return get_argument< I + 1 >( *this ); }

    // DT: this seems suspicious and likely to create some problems since
    //     a Substitution expression is returned from the substitute
    //     method, isn't it?
    // DT: No, a Substituter is returned... So I really haven't solved
    //     the second-order expression problem yet, which is why 
    //     the Minimizer and Minimize are so difficult.
    // DT: Is this a pattern or just a one off for Substitutions?  I'd
    //     like it to just be a one off since substitutions are foundational
    // DT: Could the other higher-order expressions use substitutions
    //     so that we keep the blast radius of this bootstrapping contained?
    template< typename ExprU, typename... OtherSubs >
    static constexpr typename Substituter< ExprU, OtherSubs... >::type 
    value( ExprU formula, OtherSubs... other_subs )
    { return substitute( formula, other_subs... ); } 

    // when called with no arguments the substitution is applied
    // DT: Arguments has an operator() that calls Applier on Evaluator<void>
//    constexpr typename Substituter< expression_type, Subs... >::type 
//    value() const
//    { 
//        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr
//        { return substitute( formula(), arg< Is >()... ); };
//
//        return helper( for_arguments );
//    };
    
    constexpr Substitution() = default;
    constexpr Substitution( Substitution const& ) = default;
    constexpr Substitution( expression_type v, Subs... subs ):
        Arguments< Substitution, ExprT, Subs... >{ v, subs... } { }
    // //
    
};

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

template< variable Var, typename ExprT >
constexpr bool depends_on_variable_v = free_variables_t< ExprT >::template 
    contains< Var >(); 

namespace detail {
template< typename ExprT >
struct NextVariableId;

template< typename ExprT >
requires( not requires { typename free_variables_t< ExprT >; })
struct NextVariableId< ExprT >: integral_constant< size_t, 0 > { };

template< typename ExprT >
requires( free_variables_t< ExprT >::size == 0 )
struct NextVariableId< ExprT >: integral_constant< size_t, 0 > { };

template< typename ExprT >
requires( free_variables_t< ExprT >::size != 0 )
struct NextVariableId< ExprT >: integral_constant< size_t, 
    variable_traits< typename free_variables_t< ExprT >::last_type >::
        id + 1 > { };
} // namespace detail

template< typename ExprT >
constexpr size_t next_variable_id_v = detail::NextVariableId< ExprT >::value;

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

template< template< typename... > class Op, typename... Args >
requires( compound_expression< Op< Args... >>)
struct expression_traits< Op< Args... >>
{
    using variables = free_variables_t< Op< Args... >>;
    static constexpr size_t variables_size = variables::size;
    using result_type = result_t< Op< Args... >>;
    using variable_values_tuple = variables::values_tuple;
    using scope_type = variables::scope_type;
    using arguments_tuple = tuple< Args... >;

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
// specialization of substitution for non-expression arguments
//template< typename T, typename... Ts >
//requires( not expression< T > )
//struct Substitution< T, Ts... >: Arguments< Substitution, T, Ts... >
//{
//    using free_variables_type = unique_variables< >;
//    using bound_variables_type = unique_variables< >;
//    using value_type = T;
//
//    static constexpr value_type value( value_type&& val, Ts&&... )
//    { return val; }
//
//    constexpr Substitution() = default;
//    constexpr Substitution( Substitution const& ) = default;
//    constexpr Substitution( value_type v, Ts... ts ): 
//        Arguments< Substitution, T, Ts... >{ v, ts... } { }
//};

// appliers have a special implementation for Substitutions
// DT: maybe we just return the variables as is instead of their value type?
// DT: then the call to substitute in Substituter<...>::value will need to be
//     sent to the manipulator
//     so substitutions DO require a slightly different implementation of Applier
//
/// Case: indirect substitutions (substitutions into expressions that aren't variables)
//template< typename ExprT, typename... Subs, typename ManipulatorT >
//requires( not std::is_invocable_v< Substitution< ExprT, Subs... >, ManipulatorT > )
//requires( not variable< ExprT > )
//struct Applier< Substitution< ExprT, Subs... >, ManipulatorT > {
//private:
//    static constexpr make_seq< sizeof...( Subs )> for_subs;
//
//    using formula_type = Substitution< ExprT, Subs... >;
//    using manipulator_type = ManipulatorT;
//
//    // 1. Apply the manipulator to the substitution arguments and substitute
//    //    them back into the formula
//    using substituted_formula_type = Substituter< ExprT, typename
//        Applier< Subs, ManipulatorT >::type... >::type;
//
//public:
//    // 2. Apply the manipulator on the substituted expression
//    using type = Applier< substituted_formula_type, manipulator_type >::type;
//
//    static constexpr type value( formula_type const& expr, manipulator_type& f )
//    { 
//        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
//        { return Applier< substituted_formula_type, manipulator_type >::value(
//           expr.value( expr.formula(), 
//                Applier< Subs...[ Is ], manipulator_type >::value( 
//                    expr.template arg< Is >(), f )... ), f ); };
//
//        return helper( for_subs );
//    }
//};

/// Case: first-order variable substitutions with at least one sub
//template< variable Var, typename FirstSub, typename... RestSubs, 
//    typename ManipulatorT >
//requires( variable_order_v< Var > == 1 )
//struct Applier< Substitution< Var, FirstSub, RestSubs... >, ManipulatorT > {
//private:
//    using expression_type = Substitution< Var, FirstSub, RestSubs... >;
//
//public:
//    using type = Applier< FirstSub, ManipulatorT >::type;
//    static constexpr type value( expression_type const& expr, ManipulatorT& f )
//    { return Applier< FirstSub, ManipulatorT >::value( expr.template arg< 0 >(), f ); }
//};

// Case: first-order variable substitutions with zero subs (idempotent)
//template< variable Var, typename ManipulatorT >
//requires( variable_order_v< Var > == 1 )
//struct Applier< Substitution< Var >, ManipulatorT > {
//private:
//    using expression_type = Substitution< Var >;
//
//public:
//    using type = Applier< Var, ManipulatorT >::type;
//    static constexpr type value( expression_type const& expr, ManipulatorT& f )
//    { return Applier< Var, ManipulatorT >::value( expr.formula(), f ); }
//};

// Case: higher-order variables with at least one sub.  
//       These come from direct substitution into variables like:
//          
//       ```
//       Variable< 0xF, float > f;
//       Variable< 0, float > x;
//       Variable< 1, float > y;
//
//       auto g = f( x, y ); 
//       /* decltype( g ) ~= Substitution< Variable< 0xF, Variable< 0xF, float >>,
//              Variable< 0, float >, Variable< 1, float >>; */
//
//       auto k = f( x + 2.f, y );
//       /* Substitution< Variable< 0xF, Variable< 0xF, float >>,
//              Sum< Variable< 0, float >, StaticValue< float >>, Variable< 1, float >> */
//
//       auto l = k( 2.f, 3.f );
//       /* Substitution< Substitution< Variable< 0xF, Variable< 0xF, float >>,
//              Sum< Variable< 0, float >, StaticValue< float >>, Variable< 1, float >>,
//                  float, float > 
//
//          After application:
//
//          Substitution< Variable< 0xF, Variable< 0xF, float >>, float, float > */
//
//          Do we need variable order at all if we have partial substitutions?
//
//          auto g = f(x) // ~= Substitution< Variable< 0xF, float >, Variable< 0, float >>
//          auto h = g(3) // ~= Substitution< Variable< 0xF, float >, float{3.f} >
//          assert( h(5.f * x) | eval() == 15.f )
//              // ~= Substitution< Substitution< Variable< 0xF, float >, float{ 3.f } >,
//              //        Product< StaticValue< float >{5}, Variable< 0, float >>> 
//              // |> Substitution< Product< StaticValue< float >{5}, Variable< 0, float >>,
//              //        float{ 3.f }>
//              // |> Product< StaticValue< float >{ 5 }, float{ 3.f }>
//              // |> 15.f
//
//          So as long as substitutions do the extra apply things should unroll normally?
//
//
//       auto h = g( 2.f, 3.f );
//       /* Substitution< Substitution< Variable< 0xF, Variable< 0xF, float >>,
//              Variable< 0, float >, Variable< 1, float >>,
//                  float, float >
//
//          After Applying a Manipulator:
//
//          AppliedSubstitution< Variable< 0xF, float >, 
//              SubstituteFor< 0, float >, SubstituteFor< 1, float >> 
//
//          Substitution< Substitution< Variable< 0xF, FormulaT >, 
//
//          decltype( h ) ~= Substitution< Variable< 0xF, float >, float, float >; */
//
//       assert( h( x + y ) == 5.f );
//       /* decltype( h( x + y )) ~= Substitution< Sum< Variable< 0, float >, Variable< 1, float >>,
//              float, float > */
//
//       assert( h( x * y ) == 6.f );
//       /* decltype( h( x * y )) ~= Substitution< Product< Variable< 0, float >, Variable< 1, float >>,
//              float, float > */
//
//       assert( g( 2.f, 3.f, x + y ) == 5.f );
//       /* decltype( g( 2.f, 3.f, x + y ) ~= Substitution< 
//              Sum< Variable< 0, float >, Variable< 1, float >>, float, float > */
//
//       ```
//static_assert( std::is_same_v< 
//    free_variables_t< Variable< 0, int >>, 
//        unique_variables< Variable< 0, int >> >);
//static_assert( std::is_same_v< free_variables_t< Variable< 1, int >>,
//    unique_variables< Variable< 1, int >> > );
//static_assert( std::is_same_v< free_variables_t< 
//    Substitution< Variable< 0, int >, Variable< 1, int >>>, 
//        unique_variables< Variable< 1, int >>> );
//static_assert( std::is_same_v< free_variables_t<
//    Substitution< Variable< 0, int >, Variable< 1, int >>>,
//        unique_variables< Variable< 1, int >>> );
//
// DT: Does a direct substitution into a second order variable require a different
//     implementation of substitution compared to an indirect one?  
//
//     Substitution< Variable< 0, Variable< 0, int >>, int >
//     Substitution< Sum< Variable< 0, Variable< 0, int >>, int >, int >
//     Substitution< Sum< Variable< 0, Variable< 0, int >>, Variable< 1, int >>, int, int >
//
//     f(1)
//     (f + 1)(1) => f(1) + 1
//     (f + y)(1, 2) => f(1) + 2
// 
//     Maybe not, maybe we just need to look at the order of the variable to determine
//     if it remains free or not.
//     but then do we need to check unique_variables again in case f is substituted differently?
//
//     (f + f)(1, x) => 1 + 1
//     (f + g)(1, x) => 1 + x
//     (f + f)(g)    => g + g
//     (f + g)(1, x)(2) => 1 + 2
//     (f + g)(1, x)    => 1 + x
//     (f(x) + g)(x + 1, x)(2) => 2 + 1 + 2
//     (f(x) + g)(x + 1, x, 2) => 2 + 1 + 2
//
//static_assert( is_same_v< 
//    unique_variables< Variable< 0, Substitution< Variable< 0, int >, Constant< 0 >>>>,
//    free_variables_t< Substitution< Variable< 0, Variable< 0, int >>, Constant< 0 >>>> );
//static_assert( is_same_v< 
//    unique_variables< Variable< 0, Substitution< Variable< 0, int >, Variable< 1, int >>>, 
//        Variable< 1, int >>,
//    free_variables_t< Substitution< Variable< 0, Variable< 0, int >>, Variable< 1, int >>>> );

// when evaluating a substitution, apply the substitution then continue evaluating
//template< typename ExprT, typename... Subs >
//constexpr auto Evaluator< void >::
//operator ()( Substitution< ExprT, Subs... > const& sub_expr ) const
//{ return sub_expr.value() | *this; }
//
//template< typename ScopeT >
//template< typename ExprT, typename... Subs >
//constexpr auto Evaluator< ScopeT >::
//operator ()( Substitution< ExprT, Subs... > const& sub_expr ) const
//{ return sub_expr.value() | *this; }
//
//template< variable Var, expression ExprT, expression SubU >
//constexpr typename SubstituteFor< ExprT, Var, SubU >::type
//substitute_for( ExprT const& expr, Var, SubU const& sub )
//{ return SubstituteFor< ExprT, Var, SubU >::value( expr, sub ); }
//
//template< variable Var, expression ExprT, expression SubU >
//constexpr typename SubstituteFor< ExprT, Var, SubU >::type
//substitute_for( ExprT const& expr, SubU const& sub )
//{ return SubstituteFor< ExprT, Var, SubU >::value( expr, sub ); }
//
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
//template< typename... Ts >
//struct Sum;
//
//template< >
//struct IsExpressionOperation< Sum >: std::true_type { };
//
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

//template< typename... >
//struct Product;
//
//template< >
//struct IsExpressionOperation< Product >: std::true_type { };

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
//static_assert( not ForExpression< Variable< 0, int >>::template 
//    Is< Constant< units::Length{ 5 }>>::value );

static_assert( ForExpression< Sum< Variable< 0, int >, Variable< 1, int >>>::
    template Is< Sum< Constant< 5 >, Constant< 6 >>>::value, 
        "FAILED: <int> + <int> =matches=> 5 + 6" );

//static_assert( not ForExpression< Sum< Variable< 0, int >, Variable< 1, int >>>::template Is<
//    Sum< Constant< units::Length{ 5 } >, Constant< units::Length{ 6 } >>>::value, 
//        "FAILED: <int> + <int> =not-matches=> 5m + 6m" );

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
