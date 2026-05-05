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
/// auto expr = min( f ) 
/// auto solution = 
///
///

///
///
/// if( auto solution : solve( all_of( arithmetic_expr ), criteria_expr ))
/// { /* ... */ }
///
///
///

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
struct Solver: tuple< Parameters... >
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
    Solver< MaximumIterations, LearningRate, MinimumError >
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
