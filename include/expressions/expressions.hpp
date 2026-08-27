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

/////////////////////////
/// Chain expression ///
///////////////////////
///
/// formed by comma's between other expressions.  We must bootstrap the
/// implementation since Compound depends on it
template< typename First, typename... Rest >
struct Chain: tuple< First, Rest... >
{
    using expression_type = Chain< First, Rest... >;
    using arguments_tuple = tuple< First, Rest... >;
    static constexpr size_t arguments_size = 1 + sizeof...( Rest );
    typedef make_seq< arguments_size > for_args;

    constexpr arguments_tuple const&
    args() const
    { return *this; }

    template< size_t I >
    constexpr tuple_element_t< I, arguments_tuple > const&
    arg() const
    { return get< I >( args() ); }

private:
    template< typename T >
    struct Link
    { 
        using type = T;
        static constexpr type
        value( T const& link )
        { return link; }
    };

    template< expression ExprT >
    struct Link< ExprT >
    {
        using type = result_t< ExprT >;
        static constexpr type
        value( ExprT const& expr )
        { return expr(); }
    };

    using last_t = tuple_element_t< arguments_size - 1, arguments_tuple >;

    constexpr last_t const&
    last() const
    { return get< arguments_size - 1 >( args() ); }

public:
    using result_type = result_t< last_t >;

    constexpr operator result_type() const
    requires( closed_expression< First > and ( closed_expression< Rest > and 
        ... ))
    { return Link< last_t >::value( last() ); }

    static constexpr last_t const&
    value( First const& first, Rest const&... rest )
    { 
        auto const& [ ...leading, last ] = 
            tuple< First const&, Rest const&... >{ first, rest... };
        return last;
    }

    constexpr result_type
    operator ()() const
    requires( closed_expression< First > and ( closed_expression< Rest > and 
        ... ))
    { return Link< last_t >::value( last() ); }

    template< typename FirstSub, typename... RestSub >
    requires( is_compatible_substitution_v< expression_type, FirstSub, 
        RestSub... > )
    constexpr substitute_t< expression_type, make_expression_t< FirstSub >, 
        make_expression_t< RestSub >... >
    operator ()( FirstSub const& first_sub, RestSub const&... rest_sub ) const
    { return substitute( *this, make_expression( first_sub ), 
        make_expression( rest_sub )... ); }

    template< typename NextT >
    constexpr Chain< First, Rest..., NextT >
    operator ,( NextT const& next ) const
    { 
        auto [ ...links ] = args();
        return { links..., next };
    }

    constexpr Chain( First const& first, Rest const&... rest ):
        tuple< First, Rest... >{ first, rest... }
    { }
    constexpr Chain( Chain const& ) = default;
    constexpr Chain( ) = default;
};

/// Chain implementations
template< size_t Id, typename ExprT >
template< typename T >
constexpr Chain< SetVar< Id, ExprT >, T >
SetVar< Id, ExprT >::operator ,( T const& next ) const
{ return { *this, next }; }

template< size_t I, typename T >
template< typename U >
constexpr Chain< Var< I, T >, U >
Var< I, T >::operator ,( U const& next ) const
{ return { *this, next }; }

template< auto Value >
template< typename T >
constexpr Chain< Constant< Value >, T >
Constant< Value >::operator ,( T const& next ) const
{ return { *this, next }; }

template< typename T >
template< typename U >
constexpr Chain< StaticValue< T >, U >
StaticValue< T >::operator ,( U const& next ) const
{ return { *this, next }; }

////////////////////
/// Link method ///
//////////////////
///
/// Associatively links expressions and chains together into new chains
template< typename... Links >
struct Linker;

template< typename Link >
struct Linker< Link >
{
    using type = Link;
    static constexpr type
    value( Link const& link )
    { return link; }
};

template< typename First, typename Second >
requires( not chain< First > and not chain< Second > )
struct Linker< First, Second >
{
    using type = Chain< First, Second >;
    static constexpr type
    value( First const& first, Second const& second )
    { return { first, second }; }
};

template< typename FirstFirst, typename... FirstRest, typename Second >
requires( not chain< Second > )
struct Linker< Chain< FirstFirst, FirstRest... >, Second >
{
    using type = Chain< FirstFirst, FirstRest..., Second >;
    static constexpr type
    value( Chain< FirstFirst, FirstRest... > const& first, 
        Second const& second )
    {
        auto [ ...firsts ] = first.args();
        return { firsts..., second };
    }
};

template< typename First, typename SecondFirst, typename... SecondRest >
requires( not chain< First > )
struct Linker< First, Chain< SecondFirst, SecondRest... >>
{
    using type = Chain< First, SecondFirst, SecondRest... >;
    static constexpr type
    value( First const& first, 
        Chain< SecondFirst, SecondRest... > const& second )
    {
        auto [ ...seconds ] = second.args();
        return { first, seconds... };
    }
};

template< typename FirstFirst, typename... FirstRest, typename SecondFirst,
    typename... SecondRest >
struct Linker< Chain< FirstFirst, FirstRest... >, 
    Chain< SecondFirst, SecondRest... >>
{
    using type = Chain< FirstFirst, FirstRest..., SecondFirst, SecondRest... >;
    static constexpr type
    value( Chain< FirstFirst, FirstRest... > const& first,
        Chain< SecondFirst, SecondRest... > const& second )
    {
        auto [ ...firsts ] = first.args();
        auto [ ...seconds ] = second.args();
        return { firsts..., seconds... };
    }
};

// recursive case
template< typename First, typename Second, typename... Rest >
requires( is_greater( sizeof...( Rest ), 0 ))
struct Linker< First, Second, Rest... >
{
    using type = Linker< typename Linker< First, Second >::type, Rest... >::
        type;

    static constexpr type
    value( First const& first, Second const& second, Rest const&... rest )
    { return Linker< typename Linker< First, Second >::type, Rest... >::
        value( Linker< First, Second >::value( first, second ), rest... ); }
};

template< typename... Links >
using link_t = Linker< Links... >::type;

template< typename... Links >
constexpr link_t< Links... >
link( Links const&... links )
{ return Linker< Links... >::value( links... ); }

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
requires( compound_expression< ExprT > )
struct CompoundCommon: tuple< Args... >
{
    // we keep the static members and typedefs 
    using expression_type = ExprT;
    using arguments_tuple = tuple< Args... >;
    static constexpr size_t arguments_size = sizeof...( Args );

protected:
    constexpr expression_type const&
    expr() const
    { return *static_cast< expression_type const* >( this ); }

public:
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
requires(( expression< Args > or ... or false ) and 
    compound_expression< ExprT > ) // DT: not sure if we need the extra compound
                                   //     expression requirement but it may 
                                   //     simplify compilation error messages
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
    requires( not is_same_v< reconstitute_t< expression_type, typename
        ArgEvaluator< Is >::type... >, expression_type > )
    struct ResultHelper< seq< Is... >>
    {
        // the result type of this expression will be the decltype of the
        // return value of the static value method of the parent class when
        // called on the result types of the arguments
        using type = std::remove_cvref_t< decltype( 
            reconstitute_t< expression_type, typename ArgEvaluator< Is >::
                type... >::value( typename ArgEvaluator< Is >::type{}... ))>;

        static constexpr type
        value( expression_type const& expr )
        { return reconstitute_t< expression_type, typename ArgEvaluator< Is >::
            type... >::value( ArgEvaluator< Is >::value( expr.args() )... ); }
    };

    template< size_t... Is >
    requires( is_same_v< reconstitute_t< expression_type, typename
        ArgEvaluator< Is >::type... >, expression_type > )
    struct ResultHelper< seq< Is... >>
    {
        using type = expression_type;
        static constexpr type const&
        value( expression_type const& expr )
        { return expr; }
    };

public:
    // result type of this expression if closed and evaluated.
    using result_type = ResultHelper< for_args >::type;

    // conversion operator to the result type in the case that it is closed
    constexpr operator result_type() const
    requires( not ( open_expression< Args > or ... or false ))
    { return ResultHelper< for_args >::value( expr() ); }
    
    // invocation operator also returns the result if this expression is closed
    constexpr result_type 
    operator ()() const 
    requires( not ( open_expression< Args > or ... or false ))
    { return ResultHelper< for_args >::value( expr() ); }

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
{
    using CompoundCommon< Op< Args... >, Args... >::expr;

    template< typename NextT >
    constexpr Chain< Op< Args... >, make_expression_t< NextT >>
    operator ,( NextT const& next ) const
    { return { expr(), make_expression( next ) }; }

    using CompoundCommon< Op< Args... >, Args... >::CompoundCommon; 
};

// Discriminated Expression Base Class
template< template< auto, typename... > class Op, auto Discriminator, 
    typename... Args >
struct Compound< Op< Discriminator, Args... >>: 
    CompoundCommon< Op< Discriminator, Args... >, Args... > 
{ 
    using CompoundCommon< Op< Discriminator, Args... >, Args... >::expr;

    template< typename NextT >
    constexpr Chain< Op< Discriminator, Args... >, make_expression_t< NextT >>
    operator ,( NextT const& next ) const
    { return { expr(), make_expression( next ) }; }

    using CompoundCommon< Op< Discriminator, Args... >, Args... >::
        CompoundCommon; 
};

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

/// default case is idempotent
template< typename ExprT, typename ManipulatorT >
struct Applier
{
    using type = ExprT;
    static constexpr type 
    value( ExprT const& expr, ManipulatorT& f )
    { return expr; }
};

/// if the manipulator accepts the expression return the result of the
/// manipulation
template< typename ExprT, typename ManipulatorT >
requires( std::is_invocable_v< ManipulatorT, ExprT > )
struct Applier< ExprT, ManipulatorT >
{
    using type = std::invoke_result_t< ManipulatorT, ExprT >;
    static constexpr type 
    value( ExprT const& expr, ManipulatorT& f )
    { return f( expr ); }
};

/// if the manipulator does not accept a tuple, apply to the elements
/// TODO: check if the applied tuple is accepted by the manipulator and apply
template< typename... Ts, typename ManipulatorT >
requires( not std::is_invocable_v< ManipulatorT, tuple< Ts... >> )
struct Applier< tuple< Ts... >, ManipulatorT > {
private:
    static constexpr size_t tuple_size = sizeof...( Ts );
    typedef make_seq< tuple_size > for_elements;

    template< typename Seq >
    struct Parser;

    template< size_t... Is >
    struct Parser< seq< Is... >>
    {
        using type = tuple< typename Applier< Ts...[ Is ], ManipulatorT >::
            type... >;
        static constexpr type
        value( tuple< Ts... > const& expr, ManipulatorT& f )
        { return { Applier< Ts...[ Is ], ManipulatorT >::value( 
            std::get< Is >( expr ), f )... }; }
    };

    template< typename Seq >
    struct Helper
    {
        using type = Parser< Seq >::type;
        static constexpr type
        value( tuple< Ts... > const& expr, ManipulatorT& f )
        { return Parser< Seq >::value( expr, f ); }
    };

    template< typename Seq >
    requires( std::is_invocable_v< ManipulatorT, typename 
        Helper< Seq >::type > )
    struct Helper< Seq >
    {
        using type = Applier< typename Parser< Seq >::type, ManipulatorT >::
            type;
        static constexpr type
        value( tuple< Ts... > const& expr, ManipulatorT& f )
        { return Applier< typename Parser< Seq >::type, ManipulatorT >::
            value( Parser< Seq >::value( expr, f ), f ); }
    };

public:
    using type = Helper< for_elements >::type;
    static constexpr type
    value( tuple< Ts... > const& expr, ManipulatorT& f )
    { return Helper< for_elements >::value( expr, f ); }
};

/// if the manipulator does not accept a tuple, apply to the elements
/// TODO: check if the applied tensor is accepted by the manipulator and apply
template< shape S, typename... Ts, typename ManipulatorT >
requires( not std::is_invocable_v< ManipulatorT, Tensor< S, Ts... >> )
struct Applier< Tensor< S, Ts... >, ManipulatorT > {
private:
    static constexpr size_t tensor_size = sizeof...( Ts );
    typedef make_seq< tensor_size > for_elements;

    template< typename Seq >
    struct Parser;

    template< size_t... Is >
    struct Parser< seq< Is... >>
    {
        using type = Tensor< S, typename Applier< Ts...[ Is ], ManipulatorT >::
            type... >;
        static constexpr type
        value( Tensor< S, Ts... > const& expr, ManipulatorT& f )
        { return { Applier< Ts...[ Is ], ManipulatorT >::value( 
            tensor_get< Is >( expr ), f )... }; }
    };

    template< typename Seq >
    struct Helper
    {
        using type = Parser< Seq >::type;
        static constexpr type
        value( Tensor< S, Ts... > const& expr, ManipulatorT& f )
        { return Parser< Seq >::value( expr, f ); }
    };

    template< typename Seq >
    requires( std::is_invocable_v< ManipulatorT, typename 
        Helper< Seq >::type > )
    struct Helper< Seq >
    {
        using type = Applier< typename Parser< Seq >::type, ManipulatorT >::
            type;
        static constexpr type
        value( Tensor< S, Ts... > const& expr, ManipulatorT& f )
        { return Applier< typename Parser< Seq >::type, ManipulatorT >::
            value( Parser< Seq >::value( expr, f ), f ); }
    };

public:
    using type = Helper< for_elements >::type;
    static constexpr type
    value( Tensor< S, Ts... > const& expr, ManipulatorT& f )
    { return Helper< for_elements >::value( expr, f ); }
};

/// if the manipulator does not accept a closed expression evaluate it and 
/// try again
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

/// if the manipulator does not accept an open expression, parse into the
/// expression
template< open_expression ExprT, typename ManipulatorT >
requires( not std::is_invocable_v< ManipulatorT, ExprT> and
    compound_expression< ExprT> )
struct Applier< ExprT, ManipulatorT >
{
    using arguments_tuple = ExprT::arguments_tuple;
    typedef make_seq< ExprT::arguments_size > for_args;

    template< size_t I >
    using arg_t = make_expression_t< typename 
        Applier< tuple_element_t< I, arguments_tuple >, ManipulatorT >::type >;

    template< size_t I >
    static constexpr arg_t< I >
    arg( tuple_element_t< I, arguments_tuple > const& a, ManipulatorT& f )
    { return make_expression( 
        Applier< tuple_element_t< I, arguments_tuple >, ManipulatorT >::
            value( a, f )); }

    template< typename Seq >
    struct Parser;

    template< size_t... Is >
    struct Parser< seq< Is... >>
    { 
        using reconstituted_type = reconstitute_t< ExprT,
            arg_t< Is >... >;

        using type = std::remove_cvref_t< decltype( reconstituted_type{}() )>;
        static constexpr type
        value( ExprT const& expr, ManipulatorT& f )
        { return reconstitute( expr, 
            arg< Is >( get_argument< Is >( expr ), f )... )(); }
    };

    template< typename Seq >
    struct Helper
    {
        using type = Parser< Seq >::type;
        static constexpr type
        value( ExprT const& expr, ManipulatorT& f )
        { return Parser< Seq >::value( expr, f ); }
    };

    // if the reconstituted type is now appliable then we apply it on the way 
    // back up the recursion. This allows for SetVar expressions to have their
    // values processed by a scope, then the SetVar itself to be reprocessed.
    //
    // Basically we are sending as much as we can to the manipulator
    template< typename Seq >
    requires( std::is_invocable_v< ManipulatorT, typename Parser< Seq >::type > )
    struct Helper< Seq >
    {
        using parsed_type = Parser< Seq >::type;
        using type = Applier< parsed_type, ManipulatorT >::type;
        static constexpr type
        value( ExprT const& expr, ManipulatorT& f )
        { return Applier< parsed_type, ManipulatorT >::value(
            Parser< Seq >::value( expr, f ), f ); }
    };

    using type = Helper< for_args >::type;
    static constexpr type
    value( ExprT const& expr, ManipulatorT& f )
    { return Helper< for_args >::value( expr, f ); }
};

/////////////////////
/// apply method ///
///////////////////
/// 
/// Entry point for the Applier class
template< typename ExprT, typename ManipulatorT >
constexpr Applier< ExprT, ManipulatorT >::type
apply( ExprT const& expr, ManipulatorT& f )
{ return Applier< ExprT, ManipulatorT >::value( expr, f ); }

////////////////////////////////
/// Manipulation: operator| ///
//////////////////////////////
/// 
/// We commandeer operator| on expression types as the "manipulation" operator
///
template< typename T, typename ManipulatorT >
requires( std::invocable< ManipulatorT, T > or expression< T > )
constexpr auto
operator |( T const& value_or_expression, ManipulatorT&& f )
{ return apply( value_or_expression, f ); }

////////////////////////
/// Scope Evaluator ///
//////////////////////
///
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

// TODO: remove the scope evaluator
template< scope ScopeT >
constexpr ScopeT&
eval( ScopeT& scope )
{ return scope; }

constexpr Evaluator< void > 
eval()
{ return {}; }

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
