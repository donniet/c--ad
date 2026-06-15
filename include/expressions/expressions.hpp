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

using std::size_t;
using std::tuple, std::make_tuple, std::tuple_element_t, std::get;
using std::map, std::set;
using std::any, std::any_cast;
using std::optional;

using namespace tensors;

namespace detail {
/// @brief base class used to identify an expression
struct ExpressionTag { };
} // namespace detail

/// @brief trait to identify a type as an expression type
/// @tparam T is the type to be checked
template< typename T >
struct is_expression: integral_constant< bool, 
    std::is_base_of_v< detail::ExpressionTag, T >> { };

template< typename... Ts >
struct is_expression< tuple< Ts... >>: integral_constant< bool,
    true and ( is_expression< Ts >::value and ... )> { };


/// @brief is true if T is an expression type
/// @tparam T is the type to be checked
template< typename T >
static constexpr bool is_expression_v = is_expression< T >::value;

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
    constexpr Expression( ExprT& expr ):
        _any_expr{ expr }
    { }

private:
    std::any _any_expr;
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

/////////////////
/// Variable ///
///////////////
/// 
/// A typed placeholder in an expression
///
/// @brief 
template< size_t I, typename T >
struct Variable;

/// @brief trait to identify variables
/// @tparam T the type to be tested
///
template< typename T >
struct IsVariable: integral_constant< bool, false > { };

template< size_t I, typename T >
struct IsVariable< Variable< I, T >>: integral_constant< bool, true > { };

template< typename T >
constexpr bool is_variable_v = IsVariable< std::remove_cv_t< T >>::value;

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
constexpr bool scope_contains_variable_v = detail::ScopeContainsVariable< I, ScopeT >::value;

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
template< variable Var, typename ScopeT >
constexpr string get_name( ScopeT const& scope )
{ scope.template get_name< Var >(); }

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



/// @brief a placeholder in an expression whose value can change
/// @tparam I is the id in the declared variables to this variable
/// @tparam T is the type of this variable
///
template< size_t I, typename T >
struct Variable: detail::ExpressionTag
{ 
    using value_type = T;
    using result_type = value_type;
    static constexpr size_t id = I;
    using this_type = Variable< id, value_type >;

    constexpr SetVariableValue< Variable > 
    operator=( value_type const& other ) const
    { return { *this, other }; }

    // TODO: string copy constructors are not constepr?
    constexpr string const& name() const 
    { return _name; }

    constexpr void set_name( string const& new_name )
    { _name = new_name; }

    constexpr Variable( string const& name = "var" ): _name{ name } { };
    constexpr Variable( Variable const& ) = default; 
    constexpr Variable( Variable&& ) = default;

private:
    string _name;
};

template< typename Var >
struct variable_traits: integral_constant< bool, false > { };

template< size_t I, typename T >
struct variable_traits< Variable< I, T >>: integral_constant< bool, true > 
{
    using value_type = T;
    static constexpr size_t id = I;
    using variable_type = Variable< id, value_type >;
    static constexpr variable_type variable() { return {}; }
};

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
using sequenctial_variables_t = SequentialVariables< Start, Ts... >::type;

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

/// @brief Trait for the result type of an expression
/// @tparam T the type of the expression
///
template< typename T >
requires( is_expression_v< T > )
struct Result< T >
{ using type = T::result_type; };

template< typename... Ts >
struct Result< tuple< Ts... >>
{ using type = tuple< typename Result< Ts >::type... >; };

template< shape S, typename... Ts >
struct Result< Tensor< S, Ts... >>
{ using result_type = Tensor< S, typename Result< Ts >::type... >; };

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

/////////////////////////////////////
/// Experiment: Scope References ///
///////////////////////////////////
///
/// Reference to a tuple of values for variables
///
template< size_t I, typename... Values >
struct TupleRef
{
    static constexpr size_t tuple_index = I;
    using tuple_type = tuple< Values... >;
    using value_type = Values...[ tuple_index ];

    constexpr value_type* value_ptr() const
    { return &std::get< tuple_index >( *_tuple_ptr ); }

    template< size_t J >
    constexpr TupleRef( TupleRef< J, Values... > other ): 
        _tuple_ptr{ other._tuple_ptr } { }


    tuple< Values... >* _tuple_ptr;
};

template< variable... Vars >
struct VariableValues;

template< >
struct VariableValues< >
{ };

template< variable First, variable... Rest >
struct VariableValues< First, Rest... >:
    VariableValues< Rest... >
{
    using first_variable_type = First;
    using first_value_type = first_variable_type::value_type;

    template< variable Var >
    constexpr typename Var::value_type& get_value()
    { return VariableValues< Rest... >::get_value(); }

    template< >
    constexpr first_value_type& get_value< first_variable_type >()
    { return *_first_value_ptr; }

    template< variable Var >
    constexpr typename Var::value_type const& get_value() const
    { return VariableValues< Rest... >::get_value(); }

    template< >
    constexpr first_value_type const& get_value< first_variable_type >() const
    { return *_first_value_ptr; }

    template< size_t I, typename... Values >
    constexpr VariableValues( TupleRef< I, Values... > tuple_ref ):
        VariableValues< Rest... >{ TupleRef< I + 1, Values... >{ tuple_ref }},
        _first_value_ptr{ tuple_ref.value_ptr() }
    { }

    first_value_type* _first_value_ptr;    
};

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
    requires( isgreater( sizeof...( Args ), 1 ))
    constexpr auto 
    operator ()( Args const&... args )
    { return ( operator ()( args ), ... ); }

    // @brief invoking on a compound expression
    template< compound_expression ExprT >
    //requires( scope_contains_dependent_variables_v< ExprT, Scope< Vars... >> )
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

    constexpr Scope( Vars&&... vars ) requires( isgreater( sizeof...( Vars ), 0 )): 
        tuple< Vars... >{ vars... } 
    { initialize_flags( make_seq< size >{} ); }

    constexpr Scope( Scope const& other ) = default;

    constexpr Scope(): tuple< Vars... >{}, _values{}
    { initialize_flags( make_seq< size >{} ); }

private:
    values_tuple_type _values;
    dirty_tuple_type _flags;
};

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
requires( isgreater( sizeof...( OtherScopes ), 1 ))
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
requires( isgreater( sizeof...( OtherScopes ), 1 ))
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
/// 

template< variable... >
class dependent_variables;

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

    constexpr variables_tuple as_tuple() const
    { return std::tuple_cat( tuple< first_type >{ first() },
        dependent_variables< Rest... >::as_tuple() ); }

    constexpr operator variables_tuple() const
    { return as_tuple(); }

    template< size_t I >
    using element_t = std::tuple_element_t< I, variables_tuple >;

    constexpr scope_type make_scope() const
    { return scope_type::from_tuple( 
        operator tuple< first_type, Rest... >() ); }

    template< variable... Vars >
    static consteval bool is_valid_scope( Scope< Vars... > scope = {} )
    { return dependent_variables< Rest... >::is_valid_scope( scope ) and
        (( variable_traits< First >::id == variable_traits< Vars >::id ) or ... ); }

    template< variable V >
    static consteval bool contains( V v = {} )
    { 
        static constexpr size_t vid = variable_traits< V >::id;

        if( variable_traits< first_type >::id == vid )
            return true;

        return dependent_variables< Rest... >::contains( v );
    }

    constexpr first_type const& first() const
    { return _first; } 

    constexpr last_type const& last() const
    { return std::get< sizeof...( Rest )>( 
        operator tuple< first_type, Rest... >() ); }

    constexpr dependent_variables< Rest... > rest() const
    { return *this; }

    constexpr dependent_variables() = default;
    constexpr dependent_variables( dependent_variables const& ) = default;
    constexpr dependent_variables( first_type&& first, Rest&&... rest ):
        dependent_variables< Rest... >{ std::forward( rest )... }, 
            _first{ first } { }

private:
    first_type _first;
};

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
requires( isless( variable_traits< T >::id, variable_traits< U >::id ))
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
requires( isgreater( variable_traits< T >::id, variable_traits< U >::id ))
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

namespace detail {

// Default case
template< typename ExprT >
struct GetDependentVariables
{ 
    using type = dependent_variables< >;
    static constexpr size_t size = 0;
    static constexpr type value( ExprT const& )
    { return { }; }
};

// Single variable case
template< typename Var >
requires( variable< Var >)
struct GetDependentVariables< Var >
{
    using type = dependent_variables< Var >;
    static constexpr size_t size = 1;
    static constexpr type value( Var const& var )
    { return { var }; }
};

// non-empty tuple case
// NOTE: empty case handled by default implementation
template< typename T, typename... Ts >
struct GetDependentVariables< tuple< T, Ts... >>
{
    using type = merge_dependent_variables_t< 
        typename GetDependentVariables< T >::type,
        typename GetDependentVariables< tuple< Ts... >>::type >;

    static constexpr size_t size = type::size;

    static constexpr type value( tuple< T, Ts... > const& tup )
    { 
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr
        { return GetDependentVariables< tuple< Ts... >>::
            value( std::get< 1 + Is >( tup )... ); };

        return merge_dependent_variables(
            GetDependentVariables< T >::value( std::get< 0 >( tup )),
            helper( make_seq< sizeof...( Ts )>{} ));
    }
};

template< shape S, typename... Ts >
struct GetDependentVariables< Tensor< S, Ts... >>:
    GetDependentVariables< tuple< Ts... >> { };

template< typename ExprT >
requires( compound_expression< ExprT >)
struct GetDependentVariables< ExprT >:
    GetDependentVariables< typename ExprT::arguments_tuple > { };

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
    using variables = dependent_variables_t< ExprT >;
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
    using variables = dependent_variables_t< ExprT >;
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


////////////////////////////////////////////
/// Compound Expressions with Arguments ///
//////////////////////////////////////////
///
namespace detail {
template< typename VarsTule, typename SubTuple >
struct CompatibleSubstitutionHelper;

template< typename... Vars, typename... Subs >
requires( sizeof...( Vars ) != sizeof...( Subs ))
struct CompatibleSubstitutionHelper< tuple< Vars... >, tuple< Subs... >>: 
    integral_constant< bool, false > { };

template< typename Var, typename... Vars, typename Sub, typename... Subs >
requires( sizeof...( Vars ) == sizeof...( Subs ) and 
    std::is_convertible_v< result_t< Sub >, result_t< Var >> )
struct CompatibleSubstitutionHelper< 
    tuple< Var, Vars... >, tuple< Sub, Subs... >>: 
        CompatibleSubstitutionHelper< tuple< Vars... >, tuple< Subs... >> 
{ };

template< typename Var, typename... Vars, typename Sub, typename... Subs >
requires( sizeof...( Vars ) == sizeof...( Subs ) and 
    not std::is_convertible_v< result_t< Sub >, result_t< Var >> )
struct CompatibleSubstitutionHelper< 
    tuple< Var, Vars... >, tuple< Sub, Subs... >>: integral_constant< bool,
        false > 
{ };

template< >
struct CompatibleSubstitutionHelper< tuple<>, tuple<> >: 
    integral_constant< bool, true > { };

template< typename ExprT, typename... Subs >
struct CompatibleSubstitution;

template< typename ExprT, typename... Subs >
requires( not variable< ExprT > and not compound_expression< ExprT > )
struct CompatibleSubstitution< ExprT, Subs... >: integral_constant< bool, 
    false > { };

template< size_t I, typename T, typename U >
struct CompatibleSubstitution< Variable< I, T >, U >:
    std::is_convertible< result_t< U >, T > { }; 

template< typename ExprT, typename... Subs >
requires( compound_expression< ExprT > )
struct CompatibleSubstitution< ExprT, Subs... >:
    CompatibleSubstitutionHelper< 
        dependent_variables_t< ExprT >, tuple< Subs... >>
{ };
} // namespace detail

template< typename ExprT, typename... Subs >
constexpr bool is_compatible_substitution_v = 
    detail::CompatibleSubstitution< ExprT, Subs... >::value;

// forward declaration
template< typename ExprT, typename... Args >
struct Substitution;

/// forward delaration
template< typename ExprT, typename... Args >
constexpr typename Substitution< ExprT, Args... >::type 
substitution( ExprT expr, Args... args );

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
    constexpr typename Substitution< Op< Args... >, Subs... >::type
    operator ()( Subs... subs ) const
    { return substitute( expression(), subs... ); }

    /// @brief Apply a manipulator
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
    template< typename ManipulatorT >
    constexpr auto apply( ManipulatorT& manipulator ) const
    {
        using std::get;

        // apply the manipulator to each of the arguments and return the expression
        // operation of the results
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr
        { return expression_type::value(( get< Is >( *this ) | manipulator )... ); };

        return helper( for_arguments );
    }

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
    isgreater( std::tuple_size_v< dependent_variables_t< ExprT >>, 0 ))
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
    
    /// @brief substitution operator for second-order variables
    ///
    /// Case 1: no dependent variables in substitution
    template< typename... Subs >
    requires( is_compatible_substitution_v< result_type, Subs... > and
        std::tuple_size_v< dependent_variables_t< tuple< Subs... >>> == 0 )
    constexpr typename Substitution< result_type, Subs... >::type
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

    template< size_t I, typename T >
    constexpr T
    operator ()( Variable< I, T > const& var ) const
    { return _scope.template get_value< Variable< I, T >>(); }

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

    constexpr Evaluator() = delete;
    constexpr Evaluator( Evaluator const& ) = default;
    constexpr Evaluator( scope_type const& scope ): _scope{ scope } { }

    scope_type const& _scope;
};

template< >
struct Evaluator< void >
{
    template< auto Value >
    constexpr typename Constant< Value >::value_type
    operator ()( Constant< Value > const& constant ) const
    { return Value; }

    template< typename T >
    constexpr T
    operator ()( StaticValue< T > const& static_value ) const
    { return static_value.get_value(); }
};

template< typename FuncT >
constexpr detail::ManipulatorFunctor< FuncT > manipulate( FuncT&& func )
{ return { func }; }

template< typename ScopeT >
constexpr Evaluator< ScopeT > eval( ScopeT const& scope )
{ return { scope }; }

constexpr Evaluator< void > eval()
{ return {}; }


//////////////////////////////
/// Manipulation Operator ///
////////////////////////////
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

/// @brief compound expressions MUST implement an apply method for manipulator
/// types which is primarily accomplished by inheriting from Arguments< Op, Args... >
template< variable... Vars >
template< compound_expression ExprT >
//requires( scope_contains_dependent_variables_v< ExprT, Scope< Vars... >> )
constexpr result_t< ExprT > 
Scope< Vars... >::operator ()( ExprT const& expr ) const
{ return expr.apply( *this ); }

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

//////////////////////////////////
/// Substitution / Predicates ///
////////////////////////////////
///
/// predicates used to identify parts of expressions to substitute

template< variable Var, expression ExprT >
struct match
{
    using variable_type = Var;
    using expression_type = ExprT;
};

struct no_match;

template< typename MatchT >
using match_variable_t = MatchT::variable_type;

template< typename MatchT >
using match_expression_t = MatchT::expression_type;

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

template< typename ExprT >
struct ForExpression;

/// @brief predicate for matching an expression which contains no dependent
/// variables
template< typename ExprT >
requires( dependent_variables_t< ExprT >::size == 0 )
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

// @brief specialization for individual variables
//
// variables always match, 
template< variable Var > 
requires( variable< Var >)
struct ForExpression< Var >
{
    // variables match anything with the appropriate result type 
    template< typename TestT >
    struct Is: integral_constant< bool, 
        is_same_v< typename variable_traits< Var >::value_type, result_t< TestT >>> 
    { using matches_type = tuple< match< Var, TestT >>; };
};

/// @brief specialization for compound expressions with dependent variables
///
template< template< typename... > class Op, typename... Args >
requires( compound_expression< Op< Args... >> and 
    isgreater( dependent_variables_t< Op< Args... >>::size, 0 ))
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
    requires( isgreater( sizeof...( RightMatches ), 0 )) // remove ambiguity
    struct CompatibilityHelper< tuple< >, tuple< RightMatches... >, seq< >>:
        integral_constant< bool, true >
    { using matches_type = tuple< RightMatches... >; };

    // neither side is empty
    template< typename LeftMatches, typename RightMatches, size_t... Is >
    requires( 
        isgreater( tuple_size_v< typename LeftMatches::matches_type >, 0 ) and
        isgreater( tuple_size_v< typename RightMatches::matches_type >, 0 ))
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
    requires( isgreater( sizeof...( Rest ), 1 ) and 
        AreArgumentMatchesCompatible< Rest... >::value )
    struct AreArgumentMatchesCompatible< First, Rest... >:
        AreArgumentMatchesCompatible< First, AreArgumentMatchesCompatible< Rest... >>
    { };

    // when the tail is incompatible simply evaluate to false with an empty
    // tuple of matches
    template< typename First, typename... Rest >
    requires( isgreater( sizeof...( Rest ), 1 ) and not
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
template< template< typename > class Predicate, typename ExprT, typename WithT >
struct PredicateSubstitution;

template< template< typename > class Predicate, typename ExprT, typename WithT >
requires( Predicate< ExprT >::value )
struct PredicateSubstitution< Predicate, ExprT, WithT >
{
    using type = WithT;
    static constexpr type value( ExprT const& expr, WithT const& with )
    { return with; } 
};

template< template< typename > class Predicate, size_t I, typename T, typename WithT >
requires( not Predicate< Variable< I, T >>::value )
struct PredicateSubstitution< Predicate, Variable< I, T >, WithT >
{
    using type = Variable< I, T >;
    static constexpr type value( Variable< I, T > const& var, WithT const& with )
    { return var; }
};

template< template< typename > class Predicate, auto Value, typename WithT >
requires( not Predicate< Constant< Value >>::value )
struct PredicateSubstitution< Predicate, Constant< Value >, WithT >
{
    using type = Constant< Value >;
    static constexpr type value( Constant< Value > const& const_value, WithT const& with )
    { return const_value; }
};

template< template< typename > class Predicate, typename T, typename WithT >
requires( not Predicate< StaticValue< T >>::value )
struct PredicateSubstitution< Predicate, StaticValue< T >, WithT >
{
    using type = StaticValue< T >;
    static constexpr type value( StaticValue< T > const& static_value, WithT const& with )
    { return static_value; }
};

template< template< typename > class Predicate, 
    template< typename... > class Op, typename... Args, typename WithT >
requires( not Predicate< Op< Args... >>::value )
struct PredicateSubstitution< Predicate, Op< Args... >, WithT > {
private:
    template< typename ArgT >
    using ArgumentSub = PredicateSubstitution< Predicate, ArgT, WithT >;

    template< typename ArgT >
    static constexpr typename ArgumentSub< ArgT >::type sub_argument( ArgT const& arg, WithT const& with )
    { return ArgumentSub< ArgT >::value( arg, with ); }

public:
    using type = Op< typename ArgumentSub< Args >::type... >;

private:
    template< size_t... Is >
    static constexpr type value_helper( Op< Args... > const& op, WithT const& with, seq< Is... > )
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


/// @brief expression manipulator for substitution of the variable with an 
/// expression
template< typename ExprT, typename... Args >
requires( sizeof...( Args ) == dependent_variables_t< ExprT >::size )
struct Substitution< ExprT, Args... >: 
    Arguments< Substitution, ExprT, Args... > 
{
    using expression_type = ExprT;
    using variables = dependent_variables_t< expression_type >;

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


    // Type Manipulator Requirements //
    using type = helper_type::type;

//    static constexpr type value( expression_type const& expr, 
//        Args const&... args )
//    { return helper_type::value( expr, args... ); }
    // //
    
    // Compound Expression Requirements //
    using result_type = type;

    constexpr expression_type expression_arg() const
    { return get_argument< 0 >( *this ); }

    template< size_t I >
    constexpr Args...[ I ] substitution_arg() const
    { return get_argument< I - 1 >( *this ); }

    constexpr Substitution( expression_type expr, Args... args ):
        Arguments< Substitution, expression_type, Args... >{ expr, args... }
    { }
    constexpr Substitution() = default;
    // //
    
    // Joint Type-Manipulator and Compound Expression Requirements //
    template< typename ExprU, typename... OtherArgs >
    static constexpr typename Substitution< ExprU, OtherArgs... >::template
        Helper< ExprU, make_seq< sizeof...( OtherArgs )>>::type
    value( ExprU const& expr, OtherArgs const&... args )
    { return Substitution< ExprU, OtherArgs... >::template
        Helper< ExprU, make_seq< sizeof...( OtherArgs )>>::
            value( expr, args... ); }
    // //

};

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
constexpr typename Substitution< ExprT, Args... >::type 
substitute( ExprT expr, Args... args )
{ return Substitution< ExprT, Args... >::value( expr, args... ); }

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

///////////////////
/// Operations ///
/////////////////
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
static_assert( is_same_v< dependent_variables_t< Sum< Variable< 0, int >, Variable< 1, int >>>,
    dependent_variables< Variable< 0, int >, Variable< 1, int >>> ); 
static_assert( dependent_variables_t< Sum< Variable< 0, int >, Variable< 1, int >>>::size == 2 );
static_assert( requires { typename ForExpression< Sum< Variable< 0, int >, Variable< 1, int >>>; } );

template< typename... Ts >
requires( isgreater( sizeof...( Ts ), 2 ))
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
requires( isgreater( sizeof...( Ts ), 2 ))
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
requires( isgreater( sizeof...( Ts ), 1 ))
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
//     using variable_types = dependent_variables_t< ExprT >;
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
    using dependent_variables_tuple = dependent_variables_t< expression_type >;
    static constexpr size_t size = tuple_size_v< dependent_variables_tuple >;

private:
    template< size_t I >
    struct Element
    {
//        using differential_type = differential_for_t< tuple_element_t< I, 
//            dependent_variables_tuple >, expression_type >;
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
//            dependent_variables_tuple >, tensor_element_t< J, expression_type >>;
    };
};




} // namespace expressions

namespace std {

template< expressions::expression ExprT >
constexpr auto sqrt( ExprT const& expr )
{ return expressions::sqrt( expr ); }


} // namespace std 


#endif // __EXPRESSIONS_EXPRESSIONS_HPP__
