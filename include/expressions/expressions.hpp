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
/// - variable< V >: is a Var< I, T > expression placeholder
/// - derivation< D >: a manipulator that obeys the product and chain rules
///   of differentiation.
///
/// # Base Types
/// - Constant< value > is a template-aware constant expression
/// - StaticValue< T > is a typed expression with an unchanging value
/// - Var< I, T > is a placeholder uniquely identified by the size_t 
///   template argument I exposed as Var< I, T >::index
/// - Scope< Vars... > is a tuple-like object that stores names and values
///   of Var< I, T > placeholder types and acts as a manipulator
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
/// - Sub: `new_expr = expr( args... ); // ExprU ExprT::operator ()( Subs... )`
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
///   expression (Constant, StaticValue, and Var types).  
/// - Solving: `expr | solve_for( vars... )[ Params... ];` returns a scope-type object
///   which, when invoked against `expr` results in a true value. 
///
/// # Expression Type Requirements
///   (i) Expressions MUST be literal types.
///  (ii) Compound expressions MUST be a tuple-like object of their arguments
/// (iii) Compound expressions MUST be constructible from a parameter pack of 
///       it's arguments.
///  (iv) Expressions MUST implement a static constexpr value method that 
///       calculates the result of the expression from it's arguments
///   (v) IsExpression< E< Ts... >> or IsExpressionOperation< E > must evaluate
///       to a true_type without the expression class being defined.
///   
/// # Example of an Expression Class
/// ```
/// template< typename... Args >
/// class Sum;
///
/// template< >
/// struct IsExpressionOperation< Sum >: 
///     std::true_type { };                 // requirement (v)
///
/// template< typename... Args >
/// struct Sum: Arguments< Sum, Args... >   // requirement (ii) 
/// {
///     static constexpr auto 
///     value( Args... args ) 
///     { return ( args + ... + 0 ); }      // requirement (iv) 
///
///     using Arguments< Sum, Args... >::
///         Arguments;                      // requirements (i),(iii)
/// };
/// ```
/// - The Arguments base template is tuple-like object of the ...Args
/// - Arguments imbues parent class ExprT with:
///     ExprT::operator()( subs&&... );  // substitution with evaluation
///     ExprT::operator();               // evaluation (for closed or static
///                                      // expressions)
///   
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
/// Arguments base class and the provided operator| in the following manner:
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

//////////////////////////////////////////////////////
/// Arguments Base Class for Compound Expressions ///
////////////////////////////////////////////////////
///
/// Provides storage for expression arguments and the implementation of the
/// evaluation and substitution operator().
///
/// @brief default case is a tuple that calls the parent's value method upon
///        evaluation
///
/// @pre   Op< Args... > implementation MUST define a static value method
///        taking the ...Args as parameters
///
template< template< typename... > class Op, typename... Args >
struct Arguments: tuple< Args... > 
{
    using expression_type = Op< Args... >;
    using arguments_tuple = tuple< Args... >;

    static constexpr size_t arguments_size = sizeof...( Args );
    static constexpr make_seq< arguments_size > for_args;

    constexpr arguments_tuple const&
    args() const
    { return *this; }

    // calls Op< Args... >::value static method to calculate the result of the
    // operation
    constexpr auto
    operator ()() const
    {
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
        { return expression_type::value( get< Is >( args() )... ); };

        return helper( for_args );
    }

    constexpr Arguments( Args const&... args ): tuple< Args... >{ args... } { }
    constexpr Arguments( Arguments const& ) = default;
    constexpr Arguments() = default;
};

/// @brief specialization of Arguments when there is at least one expression
///        argument. We allow substitutions via operator() with at least one
///        parameter.  We calculate our result type from the nullary
///        operator() of the default implementation (which in turn calls the
///        Op< Args... >::value( Args... ) static method)
template< template< typename... > class Op, typename... Args >
requires(( expression< Args > or ... or false ))
struct Arguments< Op, Args... >: tuple< Args... > 
{
    using expression_type = Op< Args... >;
    using arguments_tuple = tuple< Args... >;
    using result_type = std::remove_cvref_t< decltype(
//        Op< result_t< Args >... >::value( result_t< Args >{}... )) >;
        Arguments< Op, result_t< Args >... >{}() )>;

    static constexpr size_t arguments_size = sizeof...( Args );
    typedef make_seq< arguments_size > for_args;

    constexpr arguments_tuple const& 
    args() const
    { return *this; }

private:
    // DT: is it ok to static_cast this to a derived class pointer?
    // DT: seems so: https://en.cppreference.com/cpp/language/static_cast
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

    // NOTE: this does not guard against open expression idempotency on 
    //       Args...[I]::operator()** because Arguments< Op, Args... >::
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

public:
    constexpr result_type 
    operator ()() const 
    requires(( closed_expression< Args > and ... and true ))
    {
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> 
            result_type
        { return Op< typename ArgEvaluator< Is >::type... >{ 
            ArgEvaluator< Is >::value( args() )... }(); };

        return helper( for_args{} );
    }

    constexpr expression_type const&
    operator ()() const
    requires(( open_expression< Args > or ...  or false ))
    { return expr(); }

    /// @brief Sub via invocation operator
    template< typename First, typename... Rest >
    requires( is_compatible_substitution_v< expression_type, First, Rest... > )
    constexpr substitute_t< expression_type, make_expression_t< First >, 
        make_expression_t< Rest >... >
    operator ()( First first, Rest... rest ) const
    { return substitute( expr(), make_expression( first ), 
        make_expression( rest )... ); }

    // DEBUG:
//    template< typename First, typename... Rest >
//    constexpr int debug_sub( First const& first, Rest const&... rest )
//    //{ static_assert( is_same_v< void, free_variables_t< expression_type >> ); }
//    { static_assert( is_same_v< void, std::tuple< 
//        substitute_t< expression_type, First, Rest... >,
//        Sub< expression_type, First, Rest... >>> ); }
//    //{ static_assert( is_same_v< void, Sub< expression_type, First, Rest... >> ); }

    constexpr Arguments( Args const&... args ): tuple< Args... >{ args... } { }
    constexpr Arguments( Arguments const& ) = default;
    constexpr Arguments() = default; 
};



/////////////////////////////////////////////////
/// Scope Contains Free Expression Variables ///
///////////////////////////////////////////////
///
namespace detail {

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

/// @brief void evaluator will recursively evaluate compound expressions
///        using the static value method, and understands non-expressions,
///        Constant<...> and StaticValue<...> types. It cannot evaluate
///        Var<...> types and is idempotent expressions containing
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

template< template< typename... > class Op, typename... Args, typename ManipulatorT >
requires( compound_expression< Op< Args... >> )
struct Applier< Op< Args... >, ManipulatorT >
{
    typedef make_seq< sizeof...( Args )> for_args;

    template< size_t I >
    using arg_t = Applier< Args...[ I ], ManipulatorT >::type;

    template< size_t I >
    static constexpr arg_t< I >
    arg( Args...[ I ] const& a, ManipulatorT& f )
    { return Applier< Args...[ I ], ManipulatorT >::value( a, f ); }

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

//template< typename ExprT, typename ManipulatorT >
//struct Applier
//{
//    using expression_type = ExprT;
//    using manipulator_type = ManipulatorT;
//
//    static constexpr size_t max_processing_depth = 100;
//    static constexpr size_t max_processing_steps = 10;
//
//    template< typename Current, typename... History > 
//    struct Processor;
//
//    template< typename Current, typename... History >
//    requires( sizeof...( History ) >= max_processing_depth )
//    struct Processor< Current, History... >
//    { static_assert( sizeof...( History ) < max_processing_depth,
//        "maximum processing depth" ); };
//
//    template< int Steps, typename Current, typename... History >
//    struct Repeater;
//
//    template< int Steps, typename Current, typename... History >
//    requires( Steps >= max_processing_steps )
//    struct Repeater< Steps, Current, History... >
//    { static_assert( Steps < max_processing_steps, 
//        "maximum processing steps" ); };
//
//    template< auto Value, typename... History >
//    requires( sizeof...( History ) < max_processing_depth )
//    struct Processor< Constant< Value >, History... >
//    {
//        using type = std::remove_cvref_t< decltype( Value )>;
//        static constexpr type value( Constant< Value > const&, manipulator_type& )
//        { return Value; }
//    };
//
//    template< typename T, typename... History >
//    requires( sizeof...( History ) < max_processing_depth )
//    struct Processor< StaticValue< T >, History... >
//    {
//        using type = T;
//        static constexpr type value( StaticValue< T > const& expr, manipulator_type& )
//        { return expr.get_value(); }
//    };
//
//    template< non_expression T, typename... History >
//    requires( sizeof...( History ) < max_processing_depth )
//    struct Processor< T, History... >
//    {
//        using type = T;
//        static constexpr type value( T const& open_expr, manipulator_type& )
//        { return open_expr; }
//    };
//
//    // Case: This is a compound expression
//    template< template< typename... > class Op, typename... Args, 
//        typename... History >
//    requires( compound_expression< Op< Args... >> and 
//    //    not is_substitution_expression_v< Op< Args... >> and
//        sizeof...( History ) < max_processing_depth )
//    struct Processor< Op< Args... >, History... >
//    { 
//        typedef make_seq< sizeof...( Args )> for_arguments;
//    
//        template< typename Seq >
//        struct Helper;
//    
//        // (1) applies the manipulator on the arguments of the compound expression and
//        //     calls the Op< Args... >::value method on the result.
//        template< size_t... Is >
//        struct Helper< seq< Is... >>
//        {
//            using type = std::remove_cvref_t< decltype( Op< Args... >::value( 
//                typename Processor< Args...[ Is ], Op< Args... >, History... >::type{}... )) >;
//    
//            // apply the manipulator to the arguments and recombind them with the value method
//            static constexpr type value( Op< Args... > const& expr, manipulator_type& f )
//            { return Op< Args... >::value( Processor< Args...[ Is ], Op< Args... >, History... >::
//                value( std::get< Is >( expr ), f )... ); }
//        };
//
//        using type = Helper< for_arguments >::type;
//   
//        static constexpr type
//        value( Op< Args... > const& expr, manipulator_type& f )
//        { return Helper< for_arguments >::value( expr, f ); }
//    };
//
//    // Case: This is a substitution expression. Subs are processed top down (maybe...)
//    //template< typename FormulaT, typename... Subs, typename... History >
//    //requires( sizeof...( History ) < max_processing_depth )
//    //struct Processor< Sub< FormulaT, Subs... >, History... >
//    //{
//    //    using substituter = Substituter< FormulaT, Subs... >;
//    //    using substituted_type = substituter::type;
//    //    using type = Processor< substituted_type, 
//    //       Sub< FormulaT, Subs... >, History... >;
//
//    //    static constexpr type value( Sub< FormulaT, Subs... > const& sub, 
//    //        manipulator_type& f )
//    //    { 
//    //        static constexpr make_seq< sizeof...( Subs )> for_subs;
//
//    //        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
//    //        { return Processor< substituted_type, 
//    //            Sub< FormulaT, Subs... >, History... >::value( 
//    //                substituter::value( std::get< 0 >( sub ), 
//    //                    std::get< 1 + Is >( sub )... ), f ); };
//
//    //        return helper( for_subs );
//    //    }
//    //};
//
//    // our manipulator accepts this result
//    template< size_t Steps, typename ExprU, typename... History >
//    requires( std::is_invocable_v< manipulator_type, ExprU > and Steps < max_processing_steps )
//    struct Repeater< Steps, ExprU, History... >
//    {
//        using type = std::invoke_result_t< manipulator_type, ExprU >;
//
//        static constexpr type 
//        value( ExprU const& expr, manipulator_type& f )
//        { return std::invoke( f, expr ); }
//    };
//
//    // as long as the processor accepts it and the manipulator doesn't, keep processing
//    template< size_t Steps, typename Current, typename... History >
//    requires( not std::is_invocable_v< manipulator_type, Current > and requires { 
//        typename Processor< Current, History... >::type; } and
//            Steps < max_processing_steps )
//    struct Repeater< Steps, Current, History... >
//    {
//        using processed_type = Processor< Current, History... >::type;
//        using type = Repeater< Steps + 1, processed_type, Current, History... >::type;
//
//        static constexpr type value( Current const& expr, manipulator_type& f )
//        { return Repeater< Steps + 1, processed_type, Current, History... >::value(
//            Processor< Current, History... >::value( expr, f ), f ); }
//    };
//            
//    // if the processor does not accept it, return it
//    template< size_t Steps, typename Current, typename... History >
//    requires( not std::is_invocable_v< manipulator_type, Current > and not requires { 
//        typename Processor< Current, History... >::type; } and
//            Steps < max_processing_steps )
//    struct Repeater< Steps, Current, History... >
//    {
//        using type = Current;
//        static constexpr size_t execution_steps = Steps;
//        using processing_history = std::tuple< History... >;
//
//        static constexpr type value( Current const& expr, manipulator_type& )
//        { return expr; }
//    };
//
//    using type = Repeater< 0, expression_type >::type;
////    static constexpr type value( expression_type const& expr, ManipulatorT& f )
////    { return Repeater< 0, expression_type >::value( expr, f ); }
//
//    static constexpr type value( expression_type expr, ManipulatorT& f )
//    { return Repeater< 0, expression_type >::value( expr, f ); }
//};
/// @brief substitution for higher-order variables results in an expression 
///
/// LAWS:
///   (i) Associative:
///       sub( sub( expr, a, b ), c, d ) <=> sub( expr, a, b, c, d )
///
///   
/// Subs are complex compound expressions. 
///  (1) COLLECT FREE VARIABLES:
///      Free variables are found in ExprT (see GetFreeVars)
///  (2) BIND VARIABLES:
///      Each free variable is matched with a substitution expression from 
///      ...Subs in var::id order (see BoundVars)
///  (3) DEPENDENCY SORTING:
///      Bindings are sorted topologically by dependencies between the
///      substitution arguments (...Subs) and the free variables from
///      ExprT.
///  (4) SUBSTITUTION EVALUATOIN:
///      A `Sub< ExprT, Subs... >` is evaluated by evaluating
///      a recursive list of SubFor pseudo-expressions:
///
///      Var< 0, int > x;
///      Var< 1, int > y;
///      
///      assert( 
///         substitute( x + y, 3, 4 ) ==
///         substitute_for( substitute_for( x + y, x, 3 ), y, 4 ));
/// 
/// This is returened from the operator() of Arguments
///
//
///// @brief specialization for full substitutions 
//template< typename ExprT, typename... Ss >
////requires( free_variables_t< substitute_t< ExprT, Subs... >>::size == 0 )
////requires( is_compatible_substitution_v< ExprT, Subs... > )
//struct Sub: std::tuple< ExprT, Ss... >
//{
//    using formula_type = ExprT;
//    static constexpr make_seq< sizeof...( Ss )> for_subs;
//    using arguments_tuple = std::tuple< ExprT, Ss... >;
//    using result_type = result_t< formula_type >;
//
//    constexpr formula_type
//    formula() const
//    { return std::get< 0 >( *this ); }
//
//    template< size_t I >
//    constexpr Ss...[ I ]
//    arg() const 
//    { return std::get< 1 + I >( *this ); }
//
//    constexpr std::tuple< Ss... >
//    subs() const
//    { 
//        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr ->
//            std::tuple< Ss... >
//        { return { arg< Is >()... }; };
//
//        return helper( for_subs );
//    };
//
//    // evaluation of a closed substitution
//    constexpr substitute_t< ExprT, Ss... >
//    operator ()() const
//    { 
//        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> 
//            substitute_t< ExprT, Ss... >
//        { return substitute( formula(), arg< Is >()... ); };
//
//        return helper( for_subs );
//    }
//
//    // substitution into a closed substitution
//    template< typename... SSubs >
//    constexpr substitute_t< ExprT, Ss..., SSubs... >
//    operator ()( SSubs const&... subsubs ) const
//    {
//        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr ->
//            substitute_t< ExprT, Ss..., SSubs... >
//        { return substitute( formula(), arg< Is >()..., subsubs... ); };
//
//        return helper( for_subs );
//    }
//
//    constexpr Sub( formula_type const& formula, Ss const&... subs ):
//        std::tuple< formula_type, Ss... >{ formula, subs... }    
//    { }
//    constexpr Sub() = default;
//    constexpr Sub( Sub const& ) = default;
//};
//
///// @brief specialization for functional expressions
//template< size_t I, typename T, variable... Vars >
//struct Var< I, Func< Var< I, T >, Vars... >>
//{
//    using value_type = Func< Var< I, T >, Vars... >;
//    static constexpr size_t id = I;
//    static constexpr size_t vars_size = sizeof...( Vars );
//    typedef make_seq< vars_size > for_vars;
//
//    using this_type = Var< I, Func< Var< I, T >, Vars... >>;
//
//    constexpr string const& name() const 
//    { return _name; }
//
//    constexpr void set_name( string const& new_name )
//    { _name = new_name; }
//
//    /// @brief substitution operator for closed expressions
//    template< typename... Ss >
//    requires( sizeof...( Ss ) == sizeof...( Vars ) and
//        not open_expression< Sub< this_type, make_expression_t< Ss >... >> )
//    constexpr result_t< T >
//    operator ()( Ss const&... subs )
//    { return Sub< this_type, make_expression_t< Ss >... >{ 
//        *this, make_expression( subs )... }(); }
//
//    /// @brief substitution operator for open expressions
//    template< typename... Ss >
//    requires( sizeof...( Ss ) == sizeof...( Vars ) and
//        open_expression< Sub< this_type, make_expression_t< Ss >... >> )
//    constexpr Sub< this_type, make_expression_t< Ss >... >
//    operator ()( Ss const&... subs )
//    { return { *this, make_expression( subs )... }; }
//
//    // variables evaluate to themselves
//    constexpr Var const& 
//    operator ()() const
//    { return *this; }
//
//    constexpr Var( string const& name = "var" ): _name{ name } { };
//    constexpr Var( Var const& ) = default; 
//
//private:
//    string _name;
//    std::tuple< Vars... > _vars;
//};
//

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
struct ExpressionValue< Var< I, T >>
{ using type = T; };

template< template< typename... > class Op, typename... Args >
requires compound_expression< Op< Args... >>
struct ExpressionValue< Op< Args... >>
{ using type = std::remove_cv_t< decltype( Op< Args... >::value( Args{}... ))>; };

template< typename T >
using expression_value_t = ExpressionValue< T >::type;

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

//////////////////////////
/// substitute method ///
////////////////////////
///
/// @brief substitutes arguments for the dependent variables of an expression
///
/// @tparam ExprT type of the expression to be substituted into
/// @tparam Args... types of the args being substituted
/// @param expr is the instance of the original expression
/// @param args... are the 
//template< typename ExprT, typename... Args >
//constexpr typename Substituter< make_expression_t< ExprT >, 
//    make_expression_t< Args >... >::type 
//substitute( ExprT expr, Args... args )
//{ return Substituter< make_expression_t< ExprT >, 
//    make_expression_t< Args >... >::value( make_expression( expr ), 
//        make_expression( args )... ); }

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
} // namespace expressions


#endif // __EXPRESSIONS_EXPRESSIONS_HPP__
