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

namespace expressions {

using std::size_t;
using std::tuple, std::make_tuple, std::tuple_element_t, std::get;
using std::map, std::set;
using std::any, std::any_cast;
using std::optional;

using namespace tensors;

namespace detail {
/// @brief base class used to identify an expression
struct ExpressionTag 
{ };
} // namespace detail

/// @brief trait to identify a type as an expression type
/// @tparam T is the type to be checked
template< typename T >
struct is_expression: integral_constant< size_t, 
    std::is_base_of_v< detail::ExpressionTag, T >> { };

/// @brief is true if T is an expression type
/// @tparam T is the type to be checked
template< typename T >
static constexpr bool is_expression_v = is_expression< T >::value;

template< typename T >
concept expression = is_expression_v< T >;

//////////////////////////////////////////
/// Operators /// Forward Declaration ///
////////////////////////////////////////
/// 

// negation
template< expression T >
constexpr auto operator -( T const& arg );

// addition
template< expression T, expression U >
constexpr auto operator +( T const& left, U const& right );

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator +( T const& left, U const& right );

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator +( T const& left, U const& right );

// subtraction
template< expression T, expression U >
constexpr auto operator -( T const& left, U const& right );

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator -( T const& left, U const& right );

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator -( T const& left, U const& right );

// multiplication
template< expression T, expression U >
constexpr auto operator *( T const& left, U const& right );

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator *( T const& left, U const& right );

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator *( T const& left, U const& right );

// division
template< expression T, expression U >
constexpr auto operator /( T const& left, U const& right );

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator /( T const& left, U const& right );

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator /( T const& left, U const& right );

// trig functions
template< expression T >
constexpr auto sin( T const& arg );

template< expression T >
constexpr auto cos( T const& arg );

template< expression T >
constexpr auto tan( T const& arg );

template< expression T >
constexpr auto asin( T const& arg );

template< expression T >
constexpr auto acos( T const& arg );

template< expression T >
constexpr auto atan( T const& arg );

template< expression T, expression U >
constexpr auto atan2( T const& num, U const& den );

// sqrt
template< expression T >
constexpr auto sqrt( T const& arg );

// pow
template< int Exp, expression T >
constexpr auto pow( T const& arg );

// equality
template< expression T, expression U >
constexpr auto operator ==( T const& left, U const& right );

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator ==( T const& left, U const& right );

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator ==( T const& left, U const& right );

template< typename... Ts, typename... Us >
requires( sizeof...( Ts ) == sizeof...( Us ) and 
    (( expression< Ts > or ... ) or ( expression< Us > or ... )))
constexpr auto operator==( tuple< Ts... > const& left, 
    tuple< Us... > const& right );

template< typename... Ts, typename... Us >
requires( sizeof...( Ts ) == sizeof...( Us ) and 
    (( expression< Ts > or ... ) or ( expression< Us > or ... )))
constexpr auto operator!=( tuple< Ts... > const& left, 
    tuple< Us... > const& right );

template< typename ShapeT, typename... Ts, typename... Us >
requires( (( expression< Ts > or ... ) or ( expression< Us > or ... )))
constexpr auto operator==( Tensor< ShapeT, Ts... > const& left,
    Tensor< ShapeT, Us... > const& right );

template< typename ShapeT, typename... Ts, typename... Us >
requires( (( expression< Ts > or ... ) or ( expression< Us > or ... )))
constexpr auto operator!=( Tensor< ShapeT, Ts... > const& left,
    Tensor< ShapeT, Us... > const& right );

// greater than
template< expression T, expression U >
constexpr auto operator >( T const& left, U const& right );

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator >( T const& left, U const& right );

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator >( T const& left, U const& right );

// logical operations
template< expression T, expression U >
constexpr auto operator and( T const& left, U const& right );

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator and( T const& left, U const& right );

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator and( T const& left, U const& right );

template< expression T, expression U >
constexpr auto operator or( T const& left, U const& right );

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator or( T const& left, U const& right );

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator or( T const& left, U const& right );

template< expression T >
constexpr auto operator not( T const& arg );


template< typename ResultT, typename ExprT = void >
struct Expression;

/// @brief type erased expression container
template< typename ResultT >
struct Expression< ResultT, void >: detail::ExpressionTag
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
    explicit operator value_type() const
    { return _value; } 

    template< typename ManipulatorT >
    constexpr auto operator |( ManipulatorT const& manipulator ) const
    { return manipulator( *this ); }

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
StaticValue< T > static_expr( T const& value )
{ return StaticValue< T >{ value }; }

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

    template< typename ManipulatorT >
    constexpr auto operator |( ManipulatorT const& manipulator ) const
    { return manipulator( *this ); }

    constexpr Constant() = default;
};

template< auto Value >
using constant = Constant< Value >;

// NOTE: not sure about the constants
constexpr constant< 0 > constant_zero = constant< 0 >{};
constexpr constant< 1 > constant_one = constant< 1 >{};
constexpr constant< true > constant_true = constant< true >{};
constexpr constant< false > constant_false = constant< false >{};

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
constexpr bool is_variable_v = IsVariable< T >::value;

template< typename T >
concept variable = is_variable_v< T >;

// forward decl
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
    ScopeT::template has_value< Variable< I, bool >>> { };
} // namespace detail

template< size_t I, typename ScopeT >
constexpr bool scope_contains_variable_v = detail::ScopeContainsVariable< I, ScopeT >::value;

template< typename T, size_t I >
concept scope_containing = scope_contains_variable_v< I, T >;

/// @brief a placeholder in an expression whose value can change
/// @tparam I is the index in the declared variables to this variable
/// @tparam T is the type of this variable
///
template< size_t I, typename T >
struct Variable: detail::ExpressionTag
{ 
    using value_type = T;
    using result_type = value_type;
    static constexpr size_t index = I;

    template< typename ManipulatorT >
    constexpr auto operator |( ManipulatorT const& manipulator ) const
    { return manipulator( *this ); }

    constexpr Variable() = default;
};

template< typename Var >
struct variable_traits
{ static constexpr bool is_variable = false; };

template< size_t I, typename T >
struct variable_traits< Variable< I, T >>
{
    static constexpr bool is_variable = true;
    using value_type = T;
    static constexpr size_t index = I;
    using variable_type = Variable< index, value_type >;
    static constexpr variable_type variable() { return {}; }
};

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

template< size_t Start, typename... Ts >
struct VariablesTuple
{ 
    template< typename Seq >
    struct Helper;

    template< size_t... Is >
    struct Helper< seq< Is... >>
    { using type = tuple< Variable< Start + Is, Ts...[ Is ]>... >; };

    using type = Helper< make_seq< sizeof...( Ts )>>::type; 
};

template< size_t Start, typename... Ts >
using variables_tuple_t = VariablesTuple< Start, Ts... >::type;

template< variable Var, typename ScopeT >
struct ScopedVariable: Var 
{
    using value_type = Var::value_type;
    using variable_type = Var;
    using scope_type = ScopeT;

    template< typename T >
    constexpr ScopedVariable& operator=( T const& other )
    { _scope.template set< variable_type >( other ); return *this; }

    constexpr value_type value() const
    { return _scope.template get< variable_type >(); }

    constexpr operator value_type() const
    { return _scope.template get< variable_type >(); }

    constexpr string const& name() const
    { return _scope.template name< variable_type >(); }

    constexpr ScopedVariable( ScopedVariable const& other ):
        _scope{ other._scope } { }
    constexpr ScopedVariable( scope_type& scope ):
        _scope{ scope } { }

private:
    scope_type& _scope;
};

template< variable Var, typename ScopeT >
struct variable_traits< ScopedVariable< Var, ScopeT >>
{
    static constexpr bool is_variable = true;
    using value_type = Var::value_type;
    static constexpr size_t index = Var::index;
    using variable_type = Variable< index, value_type >;
    static constexpr variable_type variable() { return {}; }
};

//template< variable Var, typename ScopeT >
//struct IsVariable< ScopedVariable< Var, ScopeT >>: integral_constant< bool,
//    true > { };

/// @brief container and factory for Variables.  Scope is a manipulator and
/// application of scope via operator| evaluates dependent variables against
/// scoped values.
///
template< variable... Vars >
struct Scope: tuple< Vars... >
{
    using values_tuple_type = tuple< typename Vars::value_type... >;
    using variables_tuple_type = tuple< Vars... >;
    static constexpr size_t size = sizeof...( Vars );

protected:
    template< size_t I, typename Seq >
    struct Helper;

    template< size_t I, size_t J, size_t... Js >
    requires( I == Vars...[ J ]::index )
    struct Helper< I, seq< J, Js... >>
    { 
        static constexpr tuple_element_t< J, values_tuple_type > 
        get( values_tuple_type const& vals )
        { return std::get< J >( vals ); }

        static constexpr void 
        set( values_tuple_type& vals, tuple_element_t< J, values_tuple_type > const& val )
        { std::get< J >( vals ) = val; }

        static constexpr string const& name( std::array< string, size > const& names )
        { return names[ J ]; }

        static constexpr bool has_value = true; 
    };

    template< size_t I, size_t J, size_t... Js >
    requires( I != Vars...[ J ]::index )
    struct Helper< I, seq< J, Js... >>:
        Helper< I, seq< Js... >>
    { };

    template< size_t I >
    struct Helper< I, seq<>>
    { static constexpr bool has_value = false; };

    template< variable Var >
    using helper_for = Helper< Var::index, make_seq< size >>;

public:
    /// @brief determines if the scope has a value for variable I
    template< variable Var >
    static constexpr bool has_value = Helper< Var::index, make_seq< size >>::has_value;

    /// @brief retrieves the value of variable Var in this scope
    template< variable Var >
    constexpr typename Var::value_type get() const
    { return helper_for< Var >::get( _values ); }

    /// @brief assigns other to the scoped value of Var
    template< variable Var >
    constexpr void set( typename Var::value_type const& other )
    { helper_for< Var >::set( _values, other ); }

    /// @brief retrieves the name of variable Var in this scope
    template< variable Var >
    constexpr string const& name() const
    { return helper_for< Var >::name( _names ); }

    /// @brief returns a tuple of scoped variables
    constexpr tuple< ScopedVariable< Vars, Scope >... > variables() 
    { return { ScopedVariable< Vars, Scope >{ *this }... }; }

    /// @brief invocation against a constant will return the constant's value
    template< auto Value >
    decltype( Value ) operator ()( Constant< Value > ) const
    { return Value; }

    /// @brief invocation against a static will return the static's value
    template< typename T >
    T operator ()( StaticValue< T > const& static_value ) const
    { return static_cast< T >( static_value ); }

    /// @brief invocation against a scoped variable will return the
    /// scoped value
    /// TODO: should this return a ScopedVariable?
    template< size_t I, typename T >
    requires( scope_contains_variable_v< I, Scope< Vars... >> )
    constexpr T operator ()( Variable< I, T > const& var ) const 
    { return helper_for< Variable< I, T >>::get( _values ); }
 
    template< typename... Names >
    requires( sizeof...( Names ) == size )
    Scope( Names const&... names ): 
        _values{}, _names{ names... }  
    { }

    Scope() = default;

private:
    values_tuple_type _values;
    array< string, size > _names;
};

template< typename Seq, typename... Values >
struct SimpleScopeHelper;

template< size_t... Is, typename... Values >
struct SimpleScopeHelper< seq< Is... >, Values... >
{ 
    using type = Scope< Variable< Is, Values...[ Is ]>... >;
    static constexpr type value( Values const&... values )
    {
        type ret;
        ( ret.template set< Variable< Is, Values...[ Is ]>>( values...[ Is ] ), ... );
        return ret;
    }
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
declare_variables( Decls... decls )
{ return { decls.name()... }; }

namespace detail {

/// @brief evaluates a type T
/// @tparam T the type to be evaluated
///
template< typename T >
struct Result;

/// @brief evaluator for expressions of type T
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

namespace detail {
template< typename... Exprs >
struct DependentVariableIDs;

template< >
struct DependentVariableIDs< >
{ using type = seq< >; };

template< size_t I, typename T >
struct DependentVariableIDs< Variable< I, T >>
{ using type = seq< I >; };

template< size_t I, typename T, typename ScopeT >
struct DependentVariableIDs< ScopedVariable< Variable< I, T >, ScopeT >>
{ using type = seq< I >; };

template< typename T >
struct DependentVariableIDs< StaticValue< T >>
{ using type = seq< >; };

template< auto Value >
struct DependentVariableIDs< Constant< Value >>
{ using type = seq< >; };

template< typename ExprT >
struct ExpressionArguments
{ using type = ExprT::argument_types; };

template< typename... Exprs >
struct ExpressionArguments< tuple< Exprs... >>
{ using type = tuple< Exprs... >; };

template< typename ExprT >
struct DependentVariableIDs< ExprT >
{ 
    using argument_types = ExpressionArguments< ExprT >::type;

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
        tuple_element_t< J, typename ExpressionArguments< ExprT >::type >>::type; };

    template< size_t J, size_t... Js >
    requires( isgreater( sizeof...( Js ), 0 ))
    struct helper< seq< J, Js... >>
    { 
        using jth = VariableType< I, tuple_element_t< J, 
            typename ExpressionArguments< ExprT >::type >>::type;

        using type = std::conditional_t< not is_same_v< void, jth >, jth,
            typename helper< seq< Js...>>::type >;
    };

public:
    using type = helper< make_seq< tuple_size_v< 
        typename ExpressionArguments< ExprT >::type >>>::type;
};

template< size_t I, size_t J, typename T >
requires( I == J )
struct VariableType< I, Variable< J, T >>
{ using type = T; };

template< size_t I, size_t J, typename T >
requires( I != J )
struct VariableType< I, Variable< J, T >>
{ using type = void; };

template< size_t I, size_t J, typename T, typename ScopeT >
requires( I == J )
struct VariableType< I, ScopedVariable< Variable< J, T >, ScopeT >>
{ using type = T; };

template< size_t I, size_t J, typename T, typename ScopeT >
requires( I != J )
struct VariableType< I, ScopedVariable< Variable< J, T >, ScopeT >>
{ using type = void; };

template< size_t I, typename T >
struct VariableType< I, StaticValue< T >>
{ using type = void; };

template< size_t I, auto Value >
struct VariableType< I, Constant< Value >>
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

template< size_t I, typename ExprT >
struct DependsOnVariableIndex: integral_constant< bool, 
    sequence_contains_v< I, typename DependentVariableIDs< ExprT >::type >> { };

} // namespace detail

template< typename ExprT >
using dependent_variables_t = detail::DependentVariables< ExprT >::type;

template< typename ExprT >
using dependent_variable_id_seq = detail::DependentVariableIDs< ExprT >::type;

template< size_t I, typename ExprT >
constexpr bool depends_on_variable_index_v = 
    detail::DependsOnVariableIndex< I, ExprT >::value;

///////////////////////////////////
/// Substitution and Arguments ///
/////////////////////////////////
///
template< typename T >
concept compound_expression = expression< T > and requires( T )
{ typename T::argument_types; };

template< typename SubTuple, typename VarsTuple >
struct CompatibleSubstitutionHelper;

template< typename... Subs, typename... Vars >
requires( sizeof...( Subs ) != sizeof...( Vars ))
struct CompatibleSubstitutionHelper< tuple< Subs... >, tuple< Vars... >>: 
    integral_constant< bool, false > { };

template< typename Sub, typename... Subs, typename Var, typename... Vars >
requires( sizeof...( Subs ) == sizeof...( Vars ) and 
    std::is_convertible_v< result_t< Sub >, result_t< Var >> )
struct CompatibleSubstitutionHelper< 
    tuple< Sub, Subs... >, tuple< Var, Vars... >>: 
        CompatibleSubstitutionHelper< tuple< Subs... >, tuple< Vars... >> 
{ };

template< typename Sub, typename... Subs, typename Var, typename... Vars >
requires( sizeof...( Subs ) == sizeof...( Vars ) and 
    not std::is_convertible_v< result_t< Sub >, result_t< Var >> )
struct CompatibleSubstitutionHelper< 
    tuple< Sub, Subs... >, tuple< Var, Vars... >>: integral_constant< bool,
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
        tuple< Subs... >, dependent_variables_t< ExprT >>
{ };

template< typename ExprT, typename... Subs >
constexpr bool is_compatible_substitution_v = 
    CompatibleSubstitution< ExprT, Subs... >::value;

// forward declaration
template< typename ExprT, typename... Args >
struct Substitution;

/// forward delaration
template< typename ExprT, typename... Args >
constexpr typename Substitution< ExprT, Args... >::type 
substitution( ExprT expr, Args... args );

/// @brief Base class for compound operations. 
///
/// Provides a base implementation of substitution for dependent variables via
/// operator() and application of a manipulator via operator|.
///
template< template< typename... > class Op, typename... Args >
struct Arguments: detail::ExpressionTag, tuple< Args... >
{
    using argument_types = tuple< Args... >;

private:
    using expression_type = Op< Args... >;

    template< size_t... Is >
    constexpr expression_type expression_helper( seq< Is... > ) const
    { return { std::get< Is >( static_cast< argument_types >( *this ))... }; }

    /// @brief reconstructs the expression from arguments using the curiously
    /// recurring template pattern
    constexpr expression_type expression() const
    { return expression_helper( make_seq< sizeof...( Args )>{} ); }

public:
    /// @brief Substitution via invocation operator
    ///
    /// For each dependent variable in this expression, ordered by I by it's 
    /// variable index, substitute subs...[I] and return the new expression.
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
    { return substitution( expression(), subs... ); }

    // @brief Apply a manipulator
    //
    // Manipulators are applied to the arguments of this expression and 
    // recombined into a new expression by the operation.
    //
    // example: 
    //
    // assert( 1_cardinal + 2_cardinal                     | 
    //         manipulate( [&]( auto n ){ return n + 1; }) | 
    //         evalulate() == 5 );
    //
    template< typename ManipulatorT >
    constexpr auto operator |( ManipulatorT const& manipulator ) const
    {
        using std::get;

        // apply the manipulator to each of the arguments and return the expression
        // operation of the results
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr
        { return expression_type::value(( get< Is >( *this ) | manipulator )... ); };

        return helper( make_seq< sizeof...( Args )>{} );
    }

protected:
    constexpr Arguments( Args... args ): 
        argument_types{ args... } { }

    constexpr Arguments() = default;
};

template< expression... Exprs, typename ManipulatorT >
constexpr auto operator |( tuple< Exprs... > const& expr_tup, ManipulatorT const& manipulator )
{
    auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
    { return std::make_tuple(( std::get< Is >( expr_tup ) | manipulator )... ); };

    return helper( make_seq< sizeof...( Exprs )>{} );
}

template< typename ShapeT, expression... Exprs, typename ManipulatorT >
constexpr auto operator |( Tensor< ShapeT, Exprs... > const& expr_ten, ManipulatorT const& manipulator )
{
    auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
    { return make_tensor< ShapeT >(( tensor_get< Is >( expr_ten ) | manipulator )... ); };

    return helper( make_seq< sizeof...( Exprs )>{} );
}

/// @brief type trait for extracting an argument from a compound expression
/// @tparam I is the index of the argument
/// @tparam ExprT is the type of the compound expression
template< size_t I, typename ExprT >
struct GetArgument
{
    using type = tuple_element_t< I, typename ExprT::argument_types >;
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
///
///

/// @brief predicate class for the Ith variable index
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
requires( tuple_size_v< dependent_variables_t< ExprT >> == 0 )
struct ForExpression< ExprT >
{
    // default case
    template< typename TestT >
    struct Is: integral_constant< bool, false > { };

    // expression matches
    template< >
    struct Is< ExprT >: integral_constant< bool, true > { };
};

// @brief specialization for individual variables
template< size_t I, typename T >
struct ForExpression< Variable< I, T >>
{
    // default case
    template< typename TestT >
    struct Is: integral_constant< bool, false > {};

    // expression matches
    template< >
    struct Is< Variable< I, T >>: integral_constant< bool, true > {};
};

// TODO: write a pattern matcher for asts
// template< template< typename... > class Op, typename... Args >
// requires( isgreater( tuple_size_v< dependent_variables_t< Op< Args... >>>, 0 ))
// struct ForExpression< Op< Args... >>
// {
// // private:
//     template< typename Seq, typename... TestArgs >
//     struct Helper;
// 
//     template< size_t... Js, typename... TestArgs >
//     struct Helper< seq< Js... >, TestArgs... > {
//     private:
//         using matches_tuple = tuple< typename
//             ForExpression< Args...[ Js ]>::template Is< TestArgs...[ Js ]>
//                 ... >;
// 
//         using compatible_matches = CompatibleMatches< typename
//             tuple_element_t< Js, matches_tuple >::matches_type... >;
// 
//     public:
//         // we match if our arguments match, and if there are no conflicts
//         // in the variable matches
//         static constexpr bool value = 
//             ( tuple_element_t< Js, matches_tuple >::value and ... ) and
//                 compatible_matches::value;
// 
//         using matches_type = compatible_matches::matches_type;
//     };
// 
//     // default case
//     template< typename TestT >
//     struct Is: integral_constant< bool, false > { };
// 
//     // possible match
//     template< typename... TestArgs >
//     requires( sizeof...( Args ) == sizeof...( TestArgs ))
//     struct Is< Op< TestArgs... >>: 
//         Helper< make_seq< sizeof...( Args )>, TestArgs... > 
//     { };
// };
// 

/// @brief expression manipulator that replaces any argument in an expression
/// with the given WithT argument.
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

/// @brief expression manipulator for substitution of the variable with an 
/// expression
template< typename ExprT, typename... Args >
requires( sizeof...( Args ) == tuple_size_v< dependent_variables_t< ExprT >> )
struct Substitution< ExprT, Args... > {
private:
    using dependent_variables_tuple = dependent_variables_t< ExprT >;

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
        // fetch the index of the Ith variable
        static constexpr size_t Ith = tuple_element_t< I, dependent_variables_tuple >::index;
        // alias for the Ith argument substitutor
        using ith_substituter = PredicateSubstitution< ForVariable< Ith >::template Is,
            ExprU, Args...[ I ]>;

    public:
        // recursively define our resultant type
        using type = Helper< typename ith_substituter::type, seq< Is... >>::type;
        // recursively define our final substitution value
        static constexpr type value( ExprU expr, Args const&... args )
        { return Helper< typename ith_substituter::type, seq< Is... >>::value( 
            ith_substituter::value( expr, args...[ I ]), args... ); }
    };

    using helper_type = Helper< ExprT, make_seq< sizeof...( Args )>>;

public:
    using type = helper_type::type;

    static constexpr type value( ExprT const& expr, Args const&... args )
    { return helper_type::value( expr, args... ); }
};

/// @brief substitutes arguments for the dependent variables of an expression
/// @tparam ExprT type of the expression to be substituted into
/// @tparam Args... types of the args being substituted
/// @param expr is the instance of the original expression
/// @param args... are the 
template< typename ExprT, typename... Args >
constexpr typename Substitution< ExprT, Args... >::type 
substitution( ExprT expr, Args... args )
{ return Substitution< ExprT, Args... >::value( expr, args... ); }


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
//     using argument_types = tuple< expression_type >;
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
//     using argument_types = tuple< expression_type >;
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

template< size_t I, typename T, typename ScopeT >
std::ostream& operator <<( std::ostream& os, expressions::ScopedVariable< 
        expressions::Variable< I, T >, ScopeT > const& adapt )
{ return os << adapt.value(); }

template< size_t I, typename T, typename ScopeT >
struct formatter< expressions::ScopedVariable< 
    expressions::Variable< I, T >, ScopeT >, char >: formatter< T, char >
{
    using value_type = expressions::ScopedVariable< expressions::Variable< I, T >, ScopeT >;
    
    template< typename ParseContext >
    constexpr typename ParseContext::iterator parse( ParseContext& ctx )
    { return formatter< T, char >::parse( ctx ); }

    template< typename FormatContext >
    constexpr FormatContext::iterator format( value_type const& value, 
        FormatContext& ctx ) const
    { return formatter< T, char >::format( value.value(), ctx ); }
};


} // namespace std 


#endif // __EXPRESSIONS_EXPRESSIONS_HPP__
