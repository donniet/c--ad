#ifndef __EXPRESSIONS_SOLVERS_HPP__
#define __EXPRESSIONS_SOLVERS_HPP__

#include "expressions/expressions.hpp"
#include "expressions/normalize.hpp"

namespace expressions {

using namespace normalization; 

///////////
/// Solvers
///
/// Solution cases:
/// - BooleanExpression
/// - [ Minimum | Maximum | Any | All ]( ArithmeticExpression ) | BooleanExpression
/// 
/// Syntax Options
///
/// if( auto solution = Solve{ x, y }.minimum( arithmetic_expr ).any() )
/// { /* ... */ }
///
/// if( auto solution = Solve{ x, y }.constrained_by( criteria_expr ).any() )
/// { /* ... */ }
///
/// solve::for_( x, y ).minimize( error_expr ).constrained_by( criteria_expr ).any();
///
/// solve_for( x, y ).minimize( error_expr ).constrain_by( criteria_expr ).all();
///
/// auto solution = solve_for( x, y ).minimize( error_expr ).
///	constrain_by( criteria_expr ).all();
///
/// auto solution = minimize< NewtonsMethod >( error_expr, { x, y }, 
///	{ .learning_rate = pow< 2 >( 0.1_m ), .maximum_iterations = 1e4 })
///	.constrain( constraints_expr );
///
/// auto f = 5 * pow< 2 >( y - 3 ) * 4 * pow< 2 >( x - 2 );
///
/// auto solution = eval( argmin( f ), method< GradientDescent >
///	.learning_rate( 
///
///

///
/// audo grad = get_gradient( para2 );
///
/// auto [ min_value, steps ] = eval( iteratation( p, n ).
///    initial_values( { 0_ft, 0_ft }, 0 ).
///    update( p - rate * para2( p ) * grad( p ), n + 1 ).
///    until( n == 1000 or norm( grad( p )) < 0.001_sqft ));
///
///
/// if( auto solution : solve( all_of( arithmetic_expr ), criteria_expr ))
/// { /* ... */ }
///
/// solve_by< GradientDescent >( { x, y }, minimum( arithmetic_expr ), criteria_expr )
///
/// 
///

// template< expression ExprT, tuple_of< is_variable_v > VarsTuple, 
//     solution_method MethodT = DefaultMethod< ExprT, VarsTuple >>
// struct Solver;
// 
// template< expression... Exprs, variable... Vars >
// struct Solver< Conjunction< Exprs... >, tuple< Vars... >>;

/// @brief Represents an iterative expression. An instance of an iteration is 
/// created by the builder types IterationInitializer and IterationUpdater
/// @tparam UntilE is the type of the until expression when the iteration
/// terminates. 
/// @tparam UpdatesTuple is a tuple of N expressions which update the cor-
/// responding variables in the VariablesTuple each iteration
/// @tparam VariablesTuple is a tuple of N variables which will be updated
/// and whose final values will be returned when the iteration is evaluated
template< expression UntilE, typename UpdatesTuple, typename VariablesTuple >
struct Iteration;

/// @brief Helper class to complete the building of an Iteration expression
template< typename UpdatesTuple, typename VariablesTuple >
struct IterationUpdater;

/// @brief Helper class to begin building an Iteration expression
template< variable... Vars >
struct IterationInitializer;

/// @brief this is the function used to begin building an iteration. The
/// update( exprs... ) must be called with the same number of expressions 
/// as variables. Finally the until( expr ) must be called on the builder 
/// object returned from the update method to complete the construction.
/// You may also initialize the variables before the iteration begins by 
/// calling the initial_values( values... ) method before calling update.
/// @tparam ...Vars are the variables this iteration will update
template< typename... Vars >
requires(( variable_traits< Vars >::is_variable and ... ))
constexpr IterationInitializer< typename 
    variable_traits< Vars >::variable_type... > iteration( Vars... );

template< variable... Vars >
struct IterationInitializer
{
    IterationInitializer& initial_values( typename Vars::result_type... inits )
    { 
        _inits = make_tuple( inits... ); 
        return *this;
    }

    template< expression... Updates >
    IterationUpdater< tuple< Updates... >, tuple< Vars... >> update( 
        Updates const&... updates );

    IterationInitializer( Vars... vars ): _vars{ vars... } { }

    tuple< Vars... > _vars;
    tuple< typename Vars::result_type... > _inits;
};

template< expression... Updates, variable... Vars >
struct IterationUpdater< tuple< Updates... >, tuple< Vars... >>: 
    IterationInitializer< Vars... >
{
    template< expression UntilE >
    Iteration< UntilE, tuple< Updates... >, tuple< Vars... >> until( 
        UntilE const& until_expr ) const;

    IterationUpdater( IterationInitializer< Vars... > const& init, 
        Updates const&... updates ): 
            IterationInitializer< Vars... >{ init }, 
            _updates{ updates... }
    { }

    tuple< Updates... > _updates;
};

template< variable... Vars >
template< expression... Updates >
IterationUpdater< tuple< Updates... >, tuple< Vars... >> 
IterationInitializer< Vars... >::update( 
    Updates const&... updates )
{ return { *this, updates... }; }

template< expression UntilE, expression... Updates, variable... Vars >
struct Iteration< UntilE, tuple< Updates... >, tuple< Vars... >>: detail::ExpressionTag 
{
    using argument_types = tuple< UntilE, Updates..., Vars... >;
    using result_type = tuple< result_t< Vars >... >;

    constexpr UntilE until_expr() const { return _until_expr; }
    constexpr tuple< Updates... > updates() const{ return _updates; }
    constexpr tuple< Vars... > vars() const { return _vars; }

    template< typename ManipulatorT >
    constexpr auto operator |( ManipulatorT const& manipulator ) const
    {
        auto scope = Scope< Vars... >{};

        auto scope_initializer = [&]< size_t... Is >( seq< Is... > )
        { ( scope.template set_value< Vars...[ Is ]>( std::get< Is >( _inits )), ...); };

        // scope update helper
        // TODO: enforce the tuple-type here with a requires on the IterationBuilder
        auto scope_updater = [&]< size_t... Is >( seq< Is... > )
        { (( scope.template set_value< Vars...[Is]>( std::get< Is >( _updates ) | scope )), ... ); };

        scope_initializer( make_seq< sizeof...( Updates )>{} );

        while( not ( until_expr() | scope ))
            scope_updater( make_seq< sizeof...( Updates )>{} );

        return vars() | scope;
    }

    constexpr Iteration() = default;
    constexpr Iteration( UntilE until_expr, tuple< Updates... > updates, 
        tuple< Vars... > vars, tuple< typename Vars::result_type... > inits ): 
            _until_expr{ until_expr }, _updates{ updates }, 
            _vars{ vars }, _inits{ inits } { }

    UntilE _until_expr;
    tuple< Updates... > _updates;
    tuple< Vars... > _vars;
    tuple< typename Vars::result_type... > _inits;
};

template< expression... Updates, variable... Vars >
template< expression UntilE >
Iteration< UntilE, tuple< Updates... >, tuple< Vars... >> 
IterationUpdater< tuple< Updates... >, tuple< Vars... >>::
    until( UntilE const& until_expr ) const
{ return { until_expr, _updates, IterationInitializer< Vars... >::_vars, 
    IterationInitializer< Vars... >::_inits }; }

template< typename... Vars >
requires(( variable_traits< Vars >::is_variable and ... ))
constexpr IterationInitializer< typename 
    variable_traits< Vars >::variable_type... > iteration( Vars... )
{ return { variable_traits< Vars >::variable()... }; }



template< expression ExprT, variable... Vars >
struct ArgumentMinimum: Arguments< ArgumentMinimum, ExprT >
{
    using variables_tuple = tuple< Vars... >;
    using expression_type = ExprT;

    // DT: how should we handle iterative
    template< typename ManipulatorT >
    constexpr auto operator |( ManipulatorT const& manipulator ) const
    {
        // evaluate the expression against the manipulator
        auto value = _expr | manipulator;

        
    }

    constexpr ArgumentMinimum( expression_type const& expr, Vars const&... ):
        _expr{ expr } { }
    constexpr ArgumentMinimum() = default;

    expression_type _expr;
};

template< expression ExprT, typename... Vars >
ArgumentMinimum< ExprT, Vars... > argmin( ExprT const& expr, Vars const&... vars )
{ return { expr, vars... }; }

template< expression ExprT, typename VariableTuple, typename... Params >
result_t< VariableTuple > newtons_method( ExprT const& expr, 
    VariableTuple const& vars, Params&... params );


struct IterationCounter
{
    constexpr IterationCounter& operator++()
    { ++_iterations; return *this; }

    constexpr explicit operator size_t() const
    { return _iterations; }

    constexpr operator bool() const
    { return _iterations < _maximum_iterations; }

    constexpr IterationCounter( size_t maximum_iterations = 0 ):
        _maximum_iterations{ maximum_iterations }, _iterations{ 0 }
    { }

    size_t _maximum_iterations;
    size_t _iterations;
};

template< expression LeftT, expression RightT, variable Var, 
    typename... Params >
result_t< Var > newtons_method( Equals< LeftT, RightT > const& expr, 
    Var const& v, Params&... params )
{
    auto scope = declare_variables(
        var< typename Var::value_type >( "x" ),
        var< size_t >( "n" ));

    auto [ x, n ] = scope.variables();

    auto f = ( expr.left_arg() - expr.right_arg() );
    size_t max_iterations = 10;
    auto df_dx = differential_for< Var >( f );
    
    return eval( interation( x, n ).
        initial_values( v.value(), 0 ).
        update( x - f(x) / df_dx( x ), n + 1 ).
        until( n == max_iterations ), params... );
}

/// @brief trait to list the combinations of non-boolean valued boolean 
/// expressions that must be true for the expression to be true
// template< typename ExprT >
// struct Satisfaction
// {
//     auto operator ()( ExprT const& expr )
//     {
// 	// put the expression into disjunctive normal form
// 	auto normally_formed_expr = normalize< Disjunction, Conjunction, Compliment >( expr );
// 	
// 	// Disjunction< Conjunction< [ Compliment< Ts > | Ts ]... >... >
// 	for( auto const& conj : normally_formed_expr )
// 	    if( auto solution = solve< ParallelError >( conj ))
// 		return solution( expr );
// 
// 	return {};
//    }
//};

struct MaximumIterations: optional< size_t >
{ 
    constexpr MaximumIterations( value_type const& value ): 
        optional< size_t >{ value } { }
    constexpr MaximumIterations() = default;
};

// TODO: this requires a unit!
struct LearningRate: optional< long double >
{
    constexpr LearningRate( value_type const& value ): 
        optional< long double >{ value } { }
    constexpr LearningRate() = default;
};

// TODO: this requires a unit!
struct MinimumError: optional< long double >
{
    constexpr MinimumError( value_type const& value ): 
        optional< long double >{ value } { }
    constexpr MinimumError() = default;
};



constexpr MaximumIterations maximum_iterations = { 0 };
constexpr LearningRate learning_rate = { 0.l };
constexpr MinimumError minimum_error = { 0.l };

template< typename... Parameters >
struct SolutionMethod: tuple< Parameters... >
{
    template< typename T >
    auto& operator []( T )
    { return get< std::remove_const_t< T >>( *this ); }

    template< typename T >
    auto param( T, T::value_type def )
    { 
        if( auto p = get< std::remove_const_t< T >>( *this ); p )
            return *p;
        return def;
    }
};

template< variable... Vars >
struct GradientDescent: 
    SolutionMethod< MaximumIterations, LearningRate, MinimumError >
{
    static constexpr size_t variables_size = sizeof...( Vars );
    static constexpr size_t default_max_iterations = 100;
    static constexpr long double default_learning_rate = 1e-3;
    static constexpr long double default_minimum_error = 0;

    template< size_t I >
    auto& get_variable()
    { return std::get< I >( _vars ); }

    template< size_t... Is >
    void initialize_variables( seq< Is... > ) 
    { /* TODO: not sure what to do here yet */ }

    template< size_t I, typename ErrorT, typename GradElementT >
    auto descend_element( ErrorT err, GradElementT grad_n )
    {
        auto x = get_variable< I >(); 
        auto r = param( learning_rate, default_learning_rate );
        // TODO: check this math
        // gradient unit will be err unit / var unit
        // err * grad ~= err * err / var unit
        // rate unit must therefore be var unit * var unit / err unit / err unit
        // the learning rate must be in the same units

        using rate_type = result_t< decltype( x * x / err / err ) >;
        auto rate = static_cast< rate_type >( r );

        return eval( x ) - rate * err * grad_n;
    }

    template< typename ErrorT, typename GradT, size_t... Is >
    void descend( ErrorT err, GradT grad, seq< Is... > )
    { (( get_variable< Is >() = descend_element< Is >( err, tensor_get< Is >( grad ))), ... ); }

    template< size_t... Is >
    auto get_gradient( seq< Is... > )
    { return gradient( get_variable< Is >()... ); }

    template< typename ExprT >
    void operator ()( ExprT const& expr )
    {
        using result_type = result_t< ExprT >;
        auto grad = get_gradient( make_seq< variables_size >{} );

        auto iterations = param( maximum_iterations, default_max_iterations );
        auto thresh = param( minimum_error, default_minimum_error );

        // initialize variables...
        initialize_variables( make_seq< variables_size >{} );

        // calculate the gradient
        auto g = grad( expr );

        for( size_t step = 0; step < iterations; ++step )
        {
            auto err_n = eval( expr );
            if( static_cast< long double >( err_n ) < thresh )
                break;

            auto g_n = eval( g );

            descend( err_n, g_n, make_seq< variables_size >{} );
        }
    };

    GradientDescent( Vars... vars ): _vars{ vars... } { }

    tuple< Vars... > _vars;
};

template< variable... Vars >
GradientDescent< Vars... > gradient_descent( Vars... vars )
{ return { vars... }; }


} // namespace expressions

#endif // __SOLVERS_HPP__
