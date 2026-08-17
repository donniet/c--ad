////////////////////////////
/// Expressions Library ///
//////////////////////////
///
/// Lazily evaluated general-purpose expressions using templates as the abstract
/// syntax tree. The goal is to keep an expression unevaluated as long as
/// possible to allow for parsing, manipulation/transformation, substitution, 
/// auto-differentiation, solving, and other operations as standard library 
/// methods.
///
/// # Expression Type Requirements
///   (i) Expressions MUST be literal types.
///  (ii) Compound expressions MUST be a tuple-like object of their arguments
/// (iii) Compound expressions MUST be constructible from a parameter pack of 
///       it's arguments.
///  (iv) Expressions MUST implement a static constexpr value method that 
///       calculates the result of the expression from it's arguments
///   (v) IsExpression< E< Ts... >>, IsCompoundOperation< E >, or 
///       IsDiscriminatedOperation< E > must evaluate to a true_type before the
///       expression class itself is instantiated.
///   
/// # Example of an Expression Class
/// ```
/// template< typename... Args >
/// class Sum;
///
/// template< >
/// struct IsCompoundOperation< Sum >: 
///     std::true_type { };                 // requirement (v)
///
/// template< typename... Args >
/// struct Sum: Compound< Sum< Args... >>   // requirement (ii) 
/// {
///     static constexpr auto 
///     value( Args... args ) 
///     { return ( args + ... + 0 ); }      // requirement (iv) 
///
///     using Compound< Sum, Args... >::
///         Compound;                       // requirements (i),(iii)
/// };
///
/// # Concepts
/// - expression< T >: must be true for types that can be composed into
///   expressions in this library.
/// - compound_expression< T >: is an expression with arguments
/// - variable< V >: is a Var< I, T > expression placeholder
/// - open_expression< E > is true of E is an expression containing at least one
///   free variable
/// - closed_expression< E > is an expression with no free variables (not open)
///
/// # Key Operations
///
/// - Evaluation via operator(): 
///     auto value = expr();
/// - Evaluation via conversion operator: 
///     if( expr ) { /*...*/ }
/// - Substitution via operator(): 
///     auto new_expr = expr( args... );
/// - Manipulation/Transformation via operator|: 
///     auto new_expr = expr | manipulator; 
///     auto value = expr | scope;
/// - Evaluation by a scope: 
///     auto [ ...values ] = scope( exprs... );
/// - Solving via manipulation/transformation: 
///     auto scope = expr | solve()[ params... ]; 
///     auto scope = expr | solve_for( Vars... )[ params... ];
/// ```
///
/// # Expression Evaluation [IN PROGRESS]
///
/// Expressions are evaluated using operator|, referred to as the application
/// operator.  
///
/// assert(( constant< 5 > + constant< 6 > | eval()) == 11 );
/// 
/// Constant and StaticValue classes simply evaluate to their compile-time or
/// run time values respectively.  Compound expressions are evaluated using the
/// Compound base class and the provided operator| in the following manner:
///
/// 1. Each argument of the compound expression is evaluated
/// 2. The evaluated arguments are passed to the static value method of the
///    expression's class, and the result is returned
///
/// # Expressions with Vars and Scopes
///
/// Expressions containing variables including variables themselves have an
/// undefined value and require a Scope to evaluate.  The `eval` function
/// accepts a scope as an arguement.  When given a scope a variable's evaluated
/// value is the result of `get_value< variable_type >( scope )`.  
///
///     auto in_scope = simple_scope( 2.f );            // create a simple scope
///     Var< 0, float > x;                         // name the zeroeth variable x
///     assert(( x + 2.f | eval( in_scope )) == 4.f );  // evalute an expression against the scope
/// 
/// # Subs and Free Vars
/// 
/// Expressions can be substituted into the free variables of other expressions 
/// via operator(). The result is a Sub expression. Vars are 
/// considered free if they are not part of a substitution expression.
///
///     auto in_scope = simple_scope( 3.f, 4.f );
///     Var< 0, float > x;
///     Var< 1, float > y;
///     auto f = x + y;                     // f has free variables x and y
///     auto g = f( 2.f, 2.f );             // g has no free variables
///     auto h = f( 5.f );                  // h has one free variable
///     assert(( g | eval()) == 4.f );
///     assert(( f | eval( in_scope )) == 7.f );
///     assert(( h | eval( in_scope )) == 9.f );
///     assert(( h( x ) | eval( in_scope )) == 8.f );
/// 
/// Subs must be compatible with the variables they are replacing:
///
/// 1. The result type of the substitution argument must be convertible to
///    the value type of the variable it is replacing. 
/// 2. If a substitution is being made into a variable which is part of a 
///    substitution expression itself then the expression being substituted
///    for that variable must be compatible with the original substitution.
/// 
///     Var< 1, float > x;
///     Var< 0, float > f;  
///     auto g = f(x);           // g has free variables f and x
///     assert(( g( x * x, 3.f ) | eval() ) == 9.f );
///     assert(( g( x * x )( 3.f ) | eval() ) == 9.f );
///     
/// 

#ifndef __EXPRESSIONS_EXPRESSIONS_HPP__
#define __EXPRESSIONS_EXPRESSIONS_HPP__

#include "expressions/forward_decl.hpp"
#include "expressions/unique_variables.hpp"
#include "expressions/constant.hpp"
#include "expressions/static_value.hpp"
#include "expressions/scope.hpp"
#include "expressions/variable.hpp"


namespace expressions {

/////////////////
/// Compound ///
///////////////
/// 
/// Base class for compound expressions.  
///
/// A Compound expression may have a literal as it's first template parameter, 
/// then must have one or more typenames as template arguments.  Compound
/// expression classes must have a static value method which accepts the 
/// arguments as parameters and returns the result of that operation. The
/// class must also inherit the constructors from the Compound base class.
///
/// Example:
/// ```
/// // represents a lazily evaluated sum of it's arguments
/// template< typename... Args >
/// struct Sum;
///
/// template< >
/// struct IsCompoundOperation< Sum >: true_type { };
///
/// template< typename... Args >
/// struct Sum: Compound< Sum< Args... >>
/// {
///     static constexpr auto value( Args const&... args )
///     { return ( args + ... + 0 ); }
///
///     using Compound< Sum< Args... >>::Compound;
/// };
///
/// // represents a lazily evaluated element array accessor
/// template< size_t I, typename ArrayT >
/// struct Element;
///
/// template< >
/// struct IsCompoundOperation< Element >: true_type { };
///
/// template< size_t I, typename ArrayT >
/// struct Element: Compound< Element< I, ArrayT >>
/// {
///     static constexpr auto value( ArrayT const& arr )
///     { return get< I >( arr ); }
///
///     using Compound< Element< I, Arr >>::Compound;
/// };
/// ```
///
/// Expressions represent a non-evaluated function and should be named a noun.
/// The corresponding function or operation should be a verb, for example
/// `Derivative` would be the expression class name and `derive` would be the
/// corresponding function.  
///
/// Expressions may operate on and return other expressions. In these cases 
/// there is a risk of circular logic. An expression whose value method returns
/// an object of the same type is considered "terminal" and will not be 
/// evaluated to prevent this. One may still construct pairwise infinite 
/// recursions or other more complex infinite loops though.
///
template< typename Op > /* , typename... Results > */ // TODO: consider Results
struct Compound;

///////////////////////
/// CompoundCommon ///
/////////////////////
/// 
/// @brief Base class for compound and discriminated expressions
///
/// This is necessary to provide a terminal case for template instantiation of
/// compound expressions and prevent circular dependencies during compilation.
///
template< typename ExprT, typename... Args >
struct CompoundCommon: tuple< Args... > 
{
    // we keep the static members and typedefs 
    using expression_type = ExprT;
    using arguments_tuple = tuple< Args... >;
    static constexpr size_t arguments_size = sizeof...( Args );

    // but nothing else will be used because this type of compound expression
    // (one with no arguments that are themselves expression types) will ever
    // be instantiated **fingers crossed**

    constexpr CompoundCommon()
    { static_assert( false, 
        "implementation of compound expression with no expression arguments "
        "should not be instantiated" ); }
    constexpr CompoundCommon( Args const&... ): CompoundCommon() { };
};

/// specialization for a compound with at least one expression as an argument
template< typename ExprT, typename... Args >
requires(( expression< Args > or ... or false )) 
struct CompoundCommon< ExprT, Args... >: tuple< Args... >
{
    using expression_type = ExprT;
    using arguments_tuple = tuple< Args... >;
    static constexpr size_t arguments_size = sizeof...( Args );
    typedef make_seq< arguments_size > for_args;

    constexpr arguments_tuple const&
    args() const
    { return *this; }

    template< size_t I >
    constexpr Args...[ I ] const&
    arg() const
    { return get< I >( args() ); }

protected:    
    constexpr expression_type const&
    expr() const
    { return *static_cast< expression_type const* >( this ); }

private:
    template< size_t I >
    struct ArgEvaluator
    { 
        using type = Args...[ I ];
        static constexpr type
        value( arguments_tuple const& tup )
        { return std::get< I >( tup ); }
    };

    // NOTE: this does not guard against terminal expression idempotency on 
    //       Args...[I]::operator()** because Compound< Op, Args... >::
    //       operator()*** guards itself so there be no circular logic.
    template< size_t I >
    requires( expression< Args...[ I ]> )
    struct ArgEvaluator< I >
    {
        using type = result_t< Args...[ I ]>;
        static constexpr type
        value( arguments_tuple const& tup )
        { return std::get< I >( tup )(); } // ** the invocation operator()
    }; 

    template< typename Seq >
    struct ResultHelper;

    template< size_t... Is >
    struct ResultHelper< seq< Is... >>
    {
        // the result type of this expression will be the decltype of the
        // return value of the static value method of the parent class when
        // called on the result types of the arguments
        using type = std::remove_cvref_t< decltype( 
            reconstitute_t< expression_type, typename ArgEvaluator< Is >::
                type... >::value( typename ArgEvaluator< Is >::type{}... ))>;

        static constexpr type
        value( arguments_tuple const& tup )
        { return reconstitute_t< expression_type, typename ArgEvaluator< Is >::
            type... >::value( ArgEvaluator< Is >::value( tup )... ); }
    };

public:
    // result type of this expression if closed and evaluated.
    using result_type = ResultHelper< for_args >::type;

    // conversion operator to the result type in the case that it is closed
    constexpr operator result_type() const
    requires(( closed_expression< Args > and ... and true ))
    { return ResultHelper< for_args >::value( args() ); }
    
    // invocation operator also returns the result if this expression is closed
    constexpr result_type 
    operator ()() const 
    requires(( closed_expression< Args > and ... and true ))
    { return ResultHelper< for_args >::value( args() ); }

    // if the expression is open the invocation operator is idempotent, 
    // which signals that this is a terminal expression (one whose evaluation
    // results in the same type)
    constexpr expression_type const&
    operator ()() const
    requires(( open_expression< Args > or ...  or false ))
    { return expr(); }

    // substitution into this expression's free variables.
    template< typename First, typename... Rest >
    requires( is_compatible_substitution_v< expression_type, First, Rest... > )
    constexpr substitute_t< expression_type, make_expression_t< First >, 
        make_expression_t< Rest >... >
    operator ()( First const& first, Rest const&... rest ) const
    { return substitute( expr(), make_expression( first ), 
        make_expression( rest )... ); }

    // construction via arguments (required by expression semantics)
    constexpr CompoundCommon( Args const&... args ): 
        tuple< Args... >{ args... } { }

    // copy and default constructors are default implemented
    constexpr CompoundCommon( CompoundCommon const& ) = default;
    constexpr CompoundCommon() = default;

};

// Compound Expression Base Class
template< template< typename... > class Op, typename... Args >
struct Compound< Op< Args... >>: 
    CompoundCommon< Op< Args... >, Args... >
{ using CompoundCommon< Op< Args... >, Args... >::CompoundCommon; };

// Discriminated Expression Base Class
template< template< auto, typename... > class Op, auto Discriminator, 
    typename... Args >
struct Compound< Op< Discriminator, Args... >>: 
    CompoundCommon< Op< Discriminator, Args... >, Args... > 
{ using CompoundCommon< Op< Discriminator, Args... >, Args... >::
    CompoundCommon; };

///////////////////////////
/// Element expression ///
/////////////////////////
///
template< size_t I, typename ArrayT >
struct GetElement
{
    using type = std::tuple_element_t< I, ArrayT >;

    static constexpr type
    value( ArrayT const& arr )
    { return std::get< I >( arr ); }
};

template< size_t I, typename ArrayT >
using get_element_t = GetElement< I, ArrayT >::type;

/// get_element default implementation
template< size_t I, typename ArrayT >
constexpr get_element_t< I, ArrayT >
get_element( ArrayT const& arr )
{ return GetElement< I, ArrayT >::value( arr ); }

template< size_t I, typename ArrayT >
struct Element;

template< >
struct IsDiscriminatedOperation< Element >: true_type { };

/// Element expression class 
template< size_t I, typename ArrayT >
struct Element: Compound< Element< I, ArrayT >>
{
    static constexpr size_t index = I;

    static constexpr tuple_element_t< I, ArrayT >
    value( ArrayT const& arg )
    { return get_element< I >( arg ); }

    using Compound< Element< I, ArrayT >>::Compound;
};

/// get_element expression specialization
template< size_t I, expression ExprT >
struct GetElement< I, ExprT >
{
    using type = Element< I, ExprT >;
    static constexpr type
    value( ExprT const& expr )
    { return { expr }; }
};

/////////////////////////////////////////////////
/// Scope Contains Free Expression Variables ///
///////////////////////////////////////////////
///
namespace detail {

template< size_t Id, typename ScopeT >
struct ScopeContainsVar: false_type { };

template< size_t Id, typename... Vars >
struct ScopeContainsVar< Id, Scope< Vars... >>: std::integral_constant< bool,
    (( var_id_v< Vars > == Id ) or ... or false )> { };

template< expression ExprT, typename ScopeT >
requires( is_scope_v< ScopeT >)
struct ScopeContainsFreeVars {
private:
    template< typename Deps >
    struct Helper;

    template< variable... Vars >
    struct Helper< unique_variables< Vars... >>: integral_constant< bool,
        ( ScopeContainsVar< var_id_v< Vars >, ScopeT >::value 
            and ... )> { };

public:
    static constexpr bool value = 
        Helper< typename GetFreeVars< ExprT >::type >::value;
};

} // namespace detail

template< expression ExprT, typename ScopeT >
constexpr bool scope_contains_free_variables_v = 
    detail::ScopeContainsFreeVars< ExprT, ScopeT >::value;


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

/// @brief void evaluator understands non-exprssions, constants and 
///        static_values, and the applier handles the parsing of the expression
template< >
struct Evaluator< void >
{ 
    template< typename T >
    requires( non_expression< T > or open_expression< T > )
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
/// NOTE: We can rework the applier now that substitutions self-recurse
template< typename ExprT, typename ManipulatorT >
struct Applier
{
    using type = ExprT;
    static constexpr type value( ExprT const& expr, ManipulatorT& f )
    { return expr; }
};

template< typename ExprT, typename ManipulatorT >
requires( std::is_invocable_v< ManipulatorT, ExprT > )
struct Applier< ExprT, ManipulatorT >
{
    using type = std::invoke_result_t< ManipulatorT, ExprT >;
    static constexpr type value( ExprT const& expr, ManipulatorT& f )
    { return f( expr ); }
};

template< closed_expression ExprT, typename ManipulatorT >
requires( not std::is_invocable_v< ManipulatorT, ExprT > )
struct Applier< ExprT, ManipulatorT >
{
    using type = Applier< std::remove_cvref_t< decltype( ExprT{}() )>, 
        ManipulatorT >::type;

    static constexpr type
    value( ExprT const& expr, ManipulatorT& f )
    { return Applier< std::remove_cvref_t< decltype( ExprT{}() )>,
        ManipulatorT >::value( expr(), f ); }
};

template< template< typename... > class Op, typename... Args, typename ManipulatorT >
requires( compound_expression< Op< Args... >> )
struct Applier< Op< Args... >, ManipulatorT >
{
    typedef make_seq< sizeof...( Args )> for_args;

    template< size_t I >
    using arg_t = make_expression_t< typename 
        Applier< Args...[ I ], ManipulatorT >::type >;

    template< size_t I >
    static constexpr arg_t< I >
    arg( Args...[ I ] const& a, ManipulatorT& f )
    { return make_expression( 
        Applier< Args...[ I ], ManipulatorT >::value( a, f )); }

    template< typename Seq >
    struct Helper;

    template< size_t... Is >
    struct Helper< seq< Is... >>
    { 
        using type = std::remove_cv_t< decltype( Op< arg_t< Is >... >{}() )>;
        static constexpr type
        value( Op< Args... > const& expr, ManipulatorT& f )
        { return Op< arg_t< Is >... >{ 
            arg< Is >( get_argument< Is >( expr ), f )... }(); }
    };

    using type = Helper< for_args >::type;
    static constexpr type
    value( Op< Args... > const& expr, ManipulatorT& f )
    { return Helper< for_args >::value( expr, f ); }
};

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
//    operator ()( Sub< ExprT, Subs... > const& sub_expr ) const;

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
//
//template< typename FuncT >
//constexpr detail::ManipulatorFunctor< FuncT > 
//manipulate( FuncT&& func )
//{ return { func }; }

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

// HACK: temporary way to get iterations to work, switch iteration to support
// the substitution mechanism
//template< expression ExprT, typename ManipulatorT >
//requires( not std::invocable< ManipulatorT > and 
//    not compound_expression< ExprT > )
//constexpr auto
//operator |( ExprT const& expr, ManipulatorT&& f )
//{ 
//    auto [ ...args ] = expr.args();
//    return ExprT::value( args... );
//};

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

/// @brief bespoke applier for tuples
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

/// @brief bespoke applier for tensors
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
        using type = Tensor< S, typename 
            Applier< Ts...[ Is ], ManipulatorT >::type... >;

        static constexpr type 
        value( Tensor< S, Ts... > const& ten, ManipulatorT& f )
        { return { Applier< Ts...[ Is ], ManipulatorT >::value( 
            tensor_get< Is >( ten ), f )... }; }
    };

public:
    using type = Helper< for_tensor_elements >::type;

    static constexpr type 
    value( Tensor< S, Ts... > const& ten, ManipulatorT& f )
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
/// that is not invocable on the tensor yields a tensor of the result of 
/// applying the manipulator against each element of the tensor.
template< typename ShapeT, typename... Exprs, typename ManipulatorT >
requires( expression< Tensor< ShapeT, Exprs... >> and
    not std::invocable< ManipulatorT, Tensor< ShapeT, Exprs... >> )
constexpr typename TensorApplier< Tensor< ShapeT, Exprs... >, ManipulatorT >::
    type
operator |( Tensor< ShapeT, Exprs... > const& expr_ten, ManipulatorT& f )
{ return TensorApplier< Tensor< ShapeT, Exprs... >, ManipulatorT >::value(
    expr_ten, f ); }

///////////////////
/// Var Traits ///
/////////////////
///
template< typename Var >
struct variable_traits: integral_constant< bool, false > { };

template< size_t I, typename T >
struct variable_traits< Var< I, T >>: integral_constant< bool, true > 
{
    using value_type = T;
    using result_type = result_t< value_type >;
    static constexpr size_t id = I;
    static constexpr size_t order = var_order_v< Var< I, T >>; 
    using variable_type = Var< id, value_type >;
    static constexpr variable_type variable() { return {}; }
};

template< variable Var, typename ExprT >
constexpr bool depends_on_variable_v = free_variables_t< ExprT >::template 
    contains< Var >(); 

namespace detail {
template< typename ExprT >
struct NextVarId;

template< typename ExprT >
requires( not requires { typename free_variables_t< ExprT >; })
struct NextVarId< ExprT >: integral_constant< size_t, 0 > { };

template< typename ExprT >
requires( free_variables_t< ExprT >::size == 0 )
struct NextVarId< ExprT >: integral_constant< size_t, 0 > { };

template< typename ExprT >
requires( free_variables_t< ExprT >::size != 0 )
struct NextVarId< ExprT >: integral_constant< size_t, 
    variable_traits< typename free_variables_t< ExprT >::last_type >::
        id + 1 > { };
} // namespace detail

template< typename ExprT >
constexpr size_t next_var_id_v = detail::NextVarId< ExprT >::value;

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

} // namespace expressions


#endif // __EXPRESSIONS_EXPRESSIONS_HPP__
