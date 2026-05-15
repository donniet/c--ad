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

/// @brief expressions which require iterative 
struct Iterative: ExpressionTag
{ };

template< typename T >
constexpr bool is_iterative_v = std::is_base_of_v< Iterative, T >;

/// @brief trait to identify a type as an expression type
/// @tparam T is the type to be checked
template< typename T >
struct is_expression: integral_constant< size_t, 
    std::is_base_of_v< ExpressionTag, T >> { };

/// @brief is true if T is an expression type
/// @tparam T is the type to be checked
///
template< typename T >
static constexpr bool is_expression_v = is_expression< T >::value;

template< typename T >
concept expression = is_expression_v< T >;

template< size_t I, typename T >
struct Variable;

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

// forward decl
template< typename... Vars >
struct Scope;

template< typename T >
struct IsScope: integral_constant< bool, false > { };

template< typename... Vars >
struct IsScope< Scope< Vars... >>: integral_constant< bool, true > { };

template< typename T >
constexpr bool is_scope_v = IsScope< T >::value;

template< size_t I, typename ScopeT >
struct ScopeContainsVariable;

template< size_t I, typename ScopeT >
requires( not is_scope_v< ScopeT >)
struct ScopeContainsVariable< I, ScopeT >: integral_constant< bool, false > { };

template< size_t I, typename ScopeT >
requires( is_scope_v< ScopeT >)
struct ScopeContainsVariable< I, ScopeT >: integral_constant< bool,
    ScopeT::template has_value< Variable< I, bool >>> { };

template< size_t I, typename ScopeT >
constexpr bool scope_contains_variable_v = ScopeContainsVariable< I, ScopeT >::value;

template< typename T, size_t I >
concept scope_containing = scope_contains_variable_v< I, T >;

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

    template< typename FirstParam, typename... RestParams >
    constexpr T eval( FirstParam&, RestParams&... ) const;

    constexpr T eval() const
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

/// @brief container for variable values
/// @tparam ...Vars are the variables in this scope
template< typename... Vars >
struct Scope: tuple< typename Vars::result_type... >
{ 
    static constexpr size_t size = sizeof...( Vars );
    using variables_type = tuple< Vars... >;

    template< typename Var >
    static constexpr bool has_value = (( Vars::index == Var::index ) or ... );

    template< typename Var, typename Seq >
    struct GetValueHelper;

    template< size_t I, typename T, size_t J, size_t... Js >
    requires( I == Vars...[ J ]::index )
    struct GetValueHelper< Variable< I, T >, seq< J, Js... >>
    {
        static constexpr T& value( Scope& scope )
        { return std::get< J >( scope ); }

        static constexpr T const& value( Scope const& scope )
        { return std::get< J >( scope ); }
    };

    template< size_t I, typename T, size_t J, size_t... Js >
    requires( I != Vars...[ J ]::index )
    struct GetValueHelper< Variable< I, T >, seq< J, Js... >>:
        GetValueHelper< Variable< I, T >, seq< Js... >>
    { };

    template< typename Var >
    struct GetValue;

    template< size_t I, typename T >
    requires( has_value< Variable< I, T >>)
    struct GetValue< Variable< I, T >>:
        GetValueHelper< Variable< I, T >, make_seq< size >> 
    { }; 

    template< typename Var >
    constexpr typename Var::result_type& value( Var var = {} )
    { return GetValue< Var >::value( *this ); }

    template< typename Var >
    constexpr typename Var::result_type const& value( Var var = {} ) const
    { return GetValue< Var >::value( *this ); }
               
    template< size_t... Is >
    constexpr variables_type variables_helper( seq< Is... > )
    { return {{ &std::get< Is >( *this ), _names[ Is ] }... }; }

    constexpr variables_type variables() 
    { return variables_helper( make_seq< size >{} ); }

    template< typename Var, typename... OtherVars >
    constexpr void import_variable_value( Scope< OtherVars... > const& other )
    {
        using other_scope = Scope< OtherVars... >;

        if( other_scope::template has_value< Var >)
            value< Var >() = other_scope::template value< Var >();
    }

    template< typename... OtherVars, size_t I, size_t... Is >
    constexpr void import_values_helper( Scope< OtherVars... > const& other, 
        seq< I, Is... > )
    {
        using other_scope = Scope< OtherVars... >;

        if( other_scope::template has_value< Vars...[ I ]> )
            value< Vars...[ I ]>() = other_scope::template value< Vars...[ I ]>(); 
    }

    template< typename... OtherVars >
    constexpr Scope( Scope< OtherVars... > const& other )
    { import_values_helper( other, make_seq< size >{} ); }

    constexpr Scope() = default;

    std::array< string, size > _names;
};

template< size_t I, typename T >
template< typename FirstParam, typename... RestParams >
constexpr T Variable< I, T >::eval( FirstParam& first, RestParams&... rest ) const
{
    if constexpr( scope_contains_variable_v< I, FirstParam >)
        return first.value( *this );

    return eval( rest... ); 
}

template< variable Var, typename... Vars >
constexpr typename Var::value_type scoped_value( Scope< Vars... > const& scope )
{ return scope.variable_value( Var{} ); }


template< size_t... Is >
array< string, sizeof...( Is ) > default_variable_names_helper( seq< Is... > )
{ return {( string( "var ") + std::to_string( Is ))... }; }

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

    template< typename... Params >
    constexpr result_type operator ()( T const& expr, Params&... params ) const
    { return expr.eval( params... ); }
};

template< typename... Ts >
struct Evaluator< tuple< Ts... >>
{
    using result_type = tuple< typename Evaluator< Ts >::result_type... >;
    using tuple_type = tuple< Ts... >;

    template< size_t... Is, typename... Params >
    constexpr result_type evaluate_helper( tuple_type const& tup, 
        seq< Is... >, Params&... params ) const
    { return { Evaluator< Ts...[Is] >{}( std::get< Is >( tup ), params... )... }; }

    template< typename... Params >
    constexpr result_type operator ()( tuple< Ts... > const& tup, Params&... params ) const
    { return evaluate_helper( tup, make_seq< sizeof...( Ts )>{}, params... ); }
};

template< shape S, typename... Ts >
struct Evaluator< Tensor< S, Ts... >>
{ 
    using result_type = Tensor< S, typename Evaluator< Ts >::result_type... >;
    using tensor_type = Tensor< S, Ts... >;

    template< size_t... Is, typename... Params >
    constexpr result_type evaluate_helper( Tensor< S, Ts... > const& ten, 
        seq< Is... >, Params&... params ) const
    { return { Evaluator< Ts...[Is] >{}( ten, params... )... }; }

    // always return the value itself
    template< typename... Params >
    constexpr result_type operator ()( Tensor< S, Ts... > const& value, Params&... params ) const
    { return evaluate_helper( value, make_seq< Tensor< S, Ts... >::size() >{}, params... ); }
};

/// @brief evaluator for non-expression types
/// @tparam T the type of the non-expression
///
template< typename T >
requires( not is_expression_v< T > and not is_tensor_v< T > and not is_tuple_v< T > )
struct Evaluator< T >
{ 
    using result_type = T;

    // always return the value itself
    template< typename... Params >
    constexpr result_type operator ()( T const& value, Params&... ) const
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
template< typename T, typename... Params >
constexpr result_t< T > eval( T const& expr, Params&... params )
{ return detail::Evaluator< T >{}( expr, params... ); }

/// @brief wrapper to turn any type into an expression
/// @tparam T the wrapped type
///
template< typename T >
requires( not is_expression_v< T > ) // not sure if we need this.. meta expressions?
struct StaticValue : ExpressionTag
{ 
    using argument_types = tuple< >;
    using value_type = T;
    using result_type = value_type;

    template< typename... Params >
    constexpr result_type eval( Params&... ) const
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
template< auto Value >
struct Constant : ExpressionTag
{
    using argument_types = tuple< >;
    using value_type = std::remove_cv_t< decltype( Value )>;
    using result_type = value_type;

    static constexpr value_type value = Value;

    constexpr operator value_type() const
    { return value; }

    template< typename... Params >
    consteval result_type eval( Params&... ) const
    { return value; }
};

template< auto Value >
using constant = Constant< Value >;

// NOTE: not sure about the constants
constexpr constant< 0.l > constant_zero = constant< 0.l >{};
constexpr constant< 1.l > constant_one = constant< 1.l >{};
constexpr constant< true > constant_true = constant< true >{};
constexpr constant< false > constant_false = constant< false >{};

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

} // namespace details

template< typename ExprT >
using dependent_variables_t = details::DependentVariables< ExprT >::type;

template< typename ExprT >
using dependent_variable_id_seq = details::DependentVariableIDs< ExprT >::type;

template< size_t I, typename ExprT >
constexpr bool depends_on_variable_index_v = 
    details::DependsOnVariableIndex< I, ExprT >::value;

///////////////////////////////////
/// Substitution and Arguments ///
/////////////////////////////////
///
///
///

template< typename SubTuple, typename VarsTuple >
struct CompatibleSubstitutionHelper;

template< typename Ten, typename VarsTuple >
struct CompatibleTensorSubstitution;

template< size_t N, typename... Subs, typename... Vars >
struct CompatibleTensorSubstitution< Tensor< Shape< N >, Subs... >, 
    tuple< Vars... >>: 
        CompatibleSubstitutionHelper< tuple< Subs... >, tuple< Vars... >>
{ };

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

template< size_t I, typename T, typename U >
struct CompatibleSubstitution< Variable< I, T >, U >:
    std::is_convertible< result_t< U >, T > { }; 

template< typename ExprT, typename Ten >
requires( tensor< result_t< Ten >> )
struct CompatibleSubstitution< ExprT, Ten >:
    CompatibleTensorSubstitution< result_t< Ten >, 
        dependent_variables_t< ExprT >>
{ };
 
template< typename ExprT, typename... Subs >
requires(( not tensor< result_t< Subs >> and ... ))
struct CompatibleSubstitution< ExprT, Subs... >:
    CompatibleSubstitutionHelper< 
        tuple< Subs... >, dependent_variables_t< ExprT >>
{ };

template< typename ExprT, typename... Subs >
constexpr bool is_compatible_substitution_v = // true;
// DEBUG: turning off substitution compatibility checking to debug substitutions
//
    CompatibleSubstitution< ExprT, Subs... >::value;

template< typename ExprT, typename... Args >
struct Substitution;

template< template< typename... > class Op, typename... Args >
struct Arguments;

template< typename T >
concept compound_expression = expression< T > and requires( T )
{ typename T::argument_types; };

template< template< typename... > class Op, typename... Args >
struct Arguments: ExpressionTag, tuple< Args... >
{
    using argument_types = tuple< Args... >;

private:
    using expression_type = Op< Args... >;

    template< size_t... Is >
    constexpr expression_type expression_helper( seq< Is... > ) const
    { return { std::get< Is >( static_cast< argument_types >( *this ))... }; }

    constexpr expression_type expression() const
    { return expression_helper( make_seq< sizeof...( Args )>{} ); }
public:
    template< typename... Subs >
    requires( is_compatible_substitution_v< Op< Args... >, Subs... > )
    constexpr typename Substitution< Op< Args... >, Subs... >::type
    operator ()( Subs... subs );

//    template< typename... Subs >
//    requires( is_compatible_substitution_v< Op< Args... >, Subs... > )
//    constexpr typename Substitution< Op< Args... >, Subs... >::type
//    operator ()( Tensor< Shape< sizeof...( Subs )>, Subs... > sub );

    constexpr Arguments( Args... args ): tuple< Args... >{ args... } { }
    constexpr Arguments() = default;
};

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
    
    constexpr ArrayT arg() const { return std::get< 0 >( *this ); }
    
    template< typename... Params >
    constexpr result_type eval( Params&... params ) const
    { return std::get< I >( expressions::eval( arg(), params... )); }
    
    constexpr Of( ArrayT const& arr ): Arguments< Of, ArrayT >{ arr } { };
    constexpr Of() = default;
};

template< size_t I >
template< typename ArrayT >
requires( tensor< result_t< ArrayT >> )
struct Element< I >::Of< ArrayT >: Arguments< Of, ArrayT >
{
    using result_type = tensor_element_t< I, result_t< ArrayT >>;

    constexpr ArrayT arg() const { return std::get< 0 >( *this ); }
        
    template< typename... Params >
    constexpr result_type eval( Params&... params ) const
    { return std::get< I >( expressions::eval( arg(), params... )); }
    
    constexpr Of( ArrayT const& arr ): Arguments< Of, ArrayT >{ arr } { };
    constexpr Of() = default;
};

template< size_t I, typename T >
using element_of = Element< I >::template Of< T >;

template< size_t I, typename T >
constexpr element_of< I, T > element( T const& arr )
{ return { arr }; }


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
template< size_t I, typename ExprT, typename WithT >
struct VariableSubstitution;

template< size_t I, size_t J, typename ValueT, typename WithT >
requires( I == J and std::is_convertible_v< result_t< WithT >, result_t< ValueT >> )
struct VariableSubstitution< I, Variable< J, ValueT >, WithT >
{
    using type = WithT;
    static constexpr type value( Variable< J, ValueT > const&, 
        WithT const& with )
    { return with; }
};

template< size_t I, typename ExprT, typename WithT >
requires( not depends_on_variable_index_v< I, ExprT > )
struct VariableSubstitution< I, ExprT, WithT >
{
    using type = ExprT;
    static constexpr type value( ExprT const& expr, WithT const& )
    { return expr; }
};

template< size_t I, template< typename... > class Op,
    typename... Args, typename WithT >
requires( expression< Op< Args... >> and 
    depends_on_variable_index_v< I, Op< Args... >>)
struct VariableSubstitution< I, Op< Args... >, WithT >
{
    using type = Op< typename VariableSubstitution< I, Args, WithT >::type... >;
    static constexpr type value( Op< Args... > const& expr, WithT with )
    { return value_helper( expr, with, make_seq< sizeof...( Args )>{} ); }

private:
    template< size_t... Is >
    static constexpr type value_helper( Op< Args... > const& expr, 
        WithT const& with, seq< Is... > )
    { return { VariableSubstitution< I, Args...[ Is ], WithT >::value(
        get_argument< Is >( expr ), with )... }; }
};

template< typename ExprT, typename ArgTuple, typename Seq >
struct SubstitutionHelper;

template< typename ExprT, typename Ten, typename Seq >
struct SubstitutionTensorHelper;

template< typename ExprT, typename Ten >
struct SubstitutionTensor:
    SubstitutionTensorHelper< ExprT, Ten, make_seq< tensor_shape_t< result_t< Ten >>::size() >>
{ };

template< typename ExprT, typename Ten, size_t... Is >
struct SubstitutionTensorHelper< ExprT, Ten, seq< Is... >>:
    SubstitutionHelper< ExprT, tuple< element_of< Is, Ten >... >, 
        dependent_variable_id_seq< ExprT >>
{
    using helper_type = SubstitutionHelper< ExprT, tuple< element_of< Is, Ten >... >, 
        dependent_variable_id_seq< ExprT >>;

    using type = helper_type::type;
    
    static constexpr type value( ExprT const& expr, Ten const& ten )
    { return helper_type::value( expr, element< Is >( ten )... ); }
};

template< typename ExprT >
struct SubstitutionHelper< ExprT, tuple<>, seq<> >
{
    using type = ExprT;
    static constexpr type value( ExprT expr )
    { return expr; }
};

/// @brief substitutes First for Variable< J, T >, and Rest... for 
/// Variable< Js, Ts >... 
template< typename ExprT, typename First, typename... Rest, 
    size_t J, size_t... Js >
struct SubstitutionHelper< ExprT, tuple< First, Rest... >, seq< J, Js... >>:
    SubstitutionHelper< typename VariableSubstitution< J, ExprT, First >::type,
        tuple< Rest... >, seq< Js... >>
{
    using first_substitution = VariableSubstitution< J, ExprT, First >;
    using first_substitution_type = first_substitution::type;
    using rest_helper = SubstitutionHelper< first_substitution_type, 
        tuple< Rest... >, seq< Js... >>;

    using type = rest_helper::type;

    static constexpr type value( ExprT expr, First first, Rest... rest )
    { return rest_helper::value( first_substitution::value( expr, first ), rest... ); }
};

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

//template< typename ExprT, typename Ten >
//requires( tensor< result_t< Ten >> )
//struct Substitution< ExprT, Ten >:
//    SubstitutionTensor< ExprT, Ten >
//{ };

//template< typename ExprT, typename... Args >
//requires(( not tensor< result_t< Args >> and ... ))
//struct Substitution< ExprT, Args... >: 
//    SubstitutionHelper< ExprT, tuple< Args... >, dependent_variable_id_seq< ExprT >>
//{ };

/// @brief substitutes arguments for the dependent variables of an expression
/// @tparam ExprT type of the expression to be substituted into
/// @tparam Args... types of the args being substituted
/// @param expr is the instance of the original expression
/// @param args... are the 
template< typename ExprT, typename... Args >
constexpr typename Substitution< ExprT, Args... >::type 
substitution( ExprT expr, Args... args )
{ return Substitution< ExprT, Args... >::value( expr, args... ); }

template< template< typename... > class Op, typename... Args >
template< typename... Subs >
requires( is_compatible_substitution_v< Op< Args... >, Subs... > )
constexpr typename Substitution< Op< Args... >, Subs... >::type
Arguments< Op, Args... >::operator ()( Subs... subs )
{ return substitution( expression(), subs... ); }

//template< template< typename... > class Op, typename... Args >
//template< typename... Subs >
//requires( is_compatible_substitution_v< Op< Args... >, Subs... > )
//constexpr typename Substitution< Op< Args... >, Subs... >::type
//Arguments< Op, Args... >::operator ()( Tensor< Shape< sizeof...( Subs )>, Subs... > ten )
//{ 
//    static auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr
//    { return substitution( expression(), tensor_get< Is >( ten )... ); };
//
//    return helper( make_seq< sizeof...( Subs )>{} ); 
//}


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

    constexpr T arg() const 
    { return get_argument< 0 >( *this ); }

    template< typename... Params >
    constexpr result_type eval( Params&... params ) const
    { return -expressions::eval( arg(), params... ); }

    constexpr Negation( T arg ): Arguments< Negation, T >{ arg } { } 
    constexpr Negation() = default;
};

static_assert( eval( Negation< Variable< 0, int >>{}( 5 )) == -5 );

template< typename... Ts >
struct Sum;

/// @brief sum expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct Sum< T, U >: Arguments< Sum, T, U >
{ 
    using result_type = decltype( result_t< T >{} + result_t< U >{} );
    //using dependent_vars = dependent_variables_t< argument_types >;

    constexpr T left_arg() const { return get_argument< 0 >( *this ); }
    constexpr U right_arg() const { return get_argument< 1 >( *this ); }
    
    template< typename... Params >
    constexpr auto eval( Params&... params ) const
    { return expressions::eval( left_arg(), params... ) + 
        expressions::eval( right_arg(), params... ); }

    constexpr Sum( T left, U right ): Arguments< Sum, T, U >{ left, right } { } 
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

    template< typename... Params >
    constexpr result_type eval( Params&... params ) const
    { return expressions::eval( left_arg(), params... ) - 
        expressions::eval( right_arg(), params... ); }

    constexpr Difference( T left, U right ): 
        Arguments< Difference, T, U >{ left, right } { } 
    constexpr Difference() = default;
};

/// @brief product expression
/// @tparam T 
/// @tparam U 
template< typename T, typename U >
struct Product: Arguments< Product, T, U > 
{ 
    using result_type = decltype( result_t< T >{} * result_t< U >{} );

    constexpr T left_arg() const { return get_argument< 0 >( *this ); }
    constexpr U right_arg() const { return get_argument< 1 >( *this );; }

    template< typename... Params >
    constexpr result_type eval( Params&... params ) const
    { return expressions::eval( left_arg(), params... ) * 
        expressions::eval( right_arg(), params... ); }

    constexpr Product( T left, U right ): 
        Arguments< Product, T, U >{ left, right } { }
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

    template< typename... Params >
    constexpr result_type eval( Params&... params ) const
    { return expressions::eval( numerator_arg(), params... ) / 
        expressions::eval( denominator_arg(), params... ); }

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

    template< typename... Params >
    constexpr result_type eval( Params&... params ) const
    { return std::sqrt( expressions::eval( arg(), params... )); }

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
    template< typename T >
    struct Of: Arguments< Of, T >
    {
        using result_type = decltype( std::pow< Exp >( result_t< T >{} ));

        constexpr T arg() const { return get_argument< 0 >( *this ); }
   
        template< typename... Params >
        constexpr result_type eval( Params&... params ) const
        { return std::pow< Exp >( expressions::eval( arg(), params... )); }

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

    template< typename... Params >
    constexpr result_type eval( Params&... params ) const
    { return std::sin( expressions::eval( arg(), params... )); }

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

    template< typename... Params >
    constexpr result_type eval( Params&... params ) const
    { return std::cos( expressions::eval( arg(), params... )); }

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

    template< typename... Params >
    constexpr result_type eval( Params&... params ) const
    { return std::tan( expressions::eval( arg(), params... )); }

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

    template< typename... Params >
    constexpr result_type eval( Params&... params ) const
    { return std::asin( expressions::eval( arg(), params... )); }

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

    template< typename... Params >
    constexpr result_type eval( Params&... params ) const
    { return std::acos( expressions::eval( arg(), params... )); }

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

    template< typename... Params >
    constexpr result_type eval( Params&... params ) const
    { return std::atan( expressions::eval( arg(), params... )); }

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
    template< typename... Params >
    constexpr result_type eval( Params&... params ) const;

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
    
    template< typename... Params >
    constexpr result_type eval( Params&... params ) const
    { return expressions::eval( left_arg(), params... ) == 
        expressions::eval( right_arg(), params... ); }

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
    
    template< typename... Params >
    constexpr result_type eval( Params&... params ) const
    { return expressions::eval( left_arg(), params... ) >
        expressions::eval( right_arg(), params... ); }

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

    template< size_t... Is, typename... Params >
    constexpr result_type eval_helper( seq< Is... >, Params&... params ) const
    { return ( expressions::eval( arg< Is >(), params... ) and ... ); }

    template< typename... Params >
    constexpr result_type eval( Params&... params ) const
    { return eval_helper( make_seq< arguments_size() >{}, params... ); }

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

    template< size_t... Is, typename... Params >
    constexpr result_type eval_helper( seq< Is... >, Params&... params ) const
    { return ( expressions::eval( arg< Is >(), params... ) or ... ); }

    template< typename... Params >
    constexpr result_type eval( Params&... params ) const
    { return eval_helper( make_seq< arguments_size() >{}, params... ); }

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

    template< typename... Params >
    constexpr result_type eval( Params&... params ) const
    { return not expressions::eval( arg(), params... ); }

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

template< typename ExprT >
struct Minimum: Iterative 
{
    using expression_type = ExprT;
    using argument_types = tuple< expression_type >;
    using result_type = result_t< ExprT >;

    constexpr expression_type expr() const { return _expr; }

    constexpr Minimum( expression_type expr ): _expr{ expr } { }
    constexpr Minimum() = default;
    
    expression_type _expr;
};

template< typename ExprT >
struct ArgumentMinimum: Iterative
{
    using expression_type = ExprT;
    using argument_types = tuple< expression_type >;
    using variable_types = dependent_variables_t< ExprT >;

    template< typename TupleT >
    struct ResultHelper;

    template< typename... Vars >
    struct ResultHelper< tuple< Vars... >>
    { using type = tuple< result_t< Vars >... >; };

    using result_type = ResultHelper< variable_types >::type;

    constexpr expression_type expr() const { return _expr; }

    constexpr ArgumentMinimum( expression_type expr ): _expr{ expr } { }
    constexpr ArgumentMinimum() = default;

    expression_type _expr;
};

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

template< typename ExprT >
constexpr auto min( ExprT const& expr )
{ return Minimum< ExprT >{ expr }; }

template< typename ExprT >
constexpr auto argmin( ExprT const& expr )
{ return ArgumentMinimum< ExprT >{ expr }; }

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


#endif // __EXPRESSIONS_EXPRESSIONS_HPP__
