#ifndef __EXPRESSIONS_SOLVERS_HPP__
#define __EXPRESSIONS_SOLVERS_HPP__

#include "expressions/expressions.hpp"
#include "expressions/normalize.hpp"
#include "expressions/format.hpp"

#include <print>

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
/// @brief forward declaration for solving an expression.  Not all expressions
/// will be solvable, so we do not expect this class to be specialized in 
/// every case.
///
///

template< typename T >
concept static_expression = expression< T > and
    free_variables_t< T >::size == 0;

/// @brief specializations of this template find values of the dependent
/// variables of ExprT which result in a true-like value for the 
/// expression.  Not all expressions are solvable, so we do not expect 
/// this class to be specialized in every case. 
///
/// Solvers take an ExprT as a constructor parameter, and implement an 
/// invocation operator() that takes an ScopeT parameter that scopes
/// the dependent variables of ExprT (at least).
///
/// @tparam ExprT is the type of the expression solved by this solver.
///
template< typename ExprT >
struct Solver
{
    static constexpr bool is_solvable = false;
    static_assert( is_solvable, "no solver defined for this expression" ); 

    static consteval void operator ()( auto... )
    { static_assert( is_solvable, "no solution operation defined for this expression" ); }
};

/// @brief solve method for scopes
template< typename ScopeT, typename... Params >
struct SolveScope
{
    using scope_type = ScopeT;

    constexpr operator bool() const
    { return _solved; }

    constexpr tuple< Params... > params() const
    { return _params; }

    constexpr scope_type& scope()
    { return _scope; }

    constexpr scope_type const& scope() const
    { return _scope; }

    // forward applicable invocations to our private scope
//    template< typename T >
//    requires( std::is_invocable_v< scope_type, T > )
//    auto operator ()( T const& expr ) 
//    { return _scope( expr ); }

    template< typename... MoreParams >
    constexpr SolveScope< scope_type, Params..., MoreParams... >
    operator []( MoreParams... more_params ) const
    { 
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr ->
            SolveScope< scope_type, Params..., MoreParams... >
        { return { std::get< Is >( _params )..., more_params... }; };

        return helper( make_seq< sizeof...( Params )>{} );
    }

    template< typename ExprT >
    constexpr result_t< ExprT > 
    solve( ExprT const& expr )
    { return solve_with_parameters( expr, make_seq< sizeof...( Params )>{} ); }

    constexpr SolveScope( scope_type& scope, Params... params ):
        _scope{ scope }, _params{ params... }, _solved{ false } { }

private:
    template< typename ExprT, size_t... Is >
    constexpr result_t< ExprT > 
    solve_with_parameters( ExprT const& expr, seq< Is... > )
    { return Solver< ExprT >{ expr }( _scope, std::get< Is >( _params )... ); }

    scope_type& _scope;
    tuple< Params... > _params;
    bool _solved;
};

template< typename ScopeT >
constexpr SolveScope< ScopeT >
solve( ScopeT& scope )
{ return { scope }; }

// HACK: specialize application operator until SolveScope is a proper compound
// expression
template< typename ExprT, typename ScopeT, typename... Params >
constexpr auto operator |( ExprT const& expr, SolveScope< ScopeT, Params... >&& solution )
{ return solution.solve( expr ); }

/// @brief an expression is considered solvable if a solver exists for it
/// TODO: this concept would be nice, but whenever we are trying to detect if 
/// a template specialization is complete we run into problems...
//template< typename ExprT >
//concept solvable = Solver< ExprT >::is_solvable(); 

/////////////////////////////////////////
/// solve_for expression manipulator ///
///////////////////////////////////////
///
/// @brief scope to store the values of variables that solve an expression
template< typename VarTuple, typename... Params >
struct SolveFor;

template< typename VarTuple, typename... Params >
struct SolverParams: expression_scope_t< VarTuple >
{
    using variables_tuple = VarTuple;
    using scope_type = expression_scope_t< VarTuple >;

    constexpr operator bool() const
    { return _solved; }

    constexpr tuple< Params... > params() const
    { return _params; }

    template< typename... MoreParams >
    constexpr SolveFor< VarTuple, Params..., MoreParams... >
    operator []( MoreParams... more_params ) const
    { 
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr ->
            SolveFor< VarTuple, Params..., MoreParams... >
        { return { std::get< Is >( _params )..., more_params... }; };

        return helper( make_seq< sizeof...( Params )>{} );
    }

    template< expression ExprT >
    constexpr bool solve( ExprT const& expr ) 
    { 
        std::println( "attempting so solve: {}", expr );

        auto scope = make_scope< ExprT >();
        auto solve_ = Solver< ExprT >{ expr };

        //static_assert( not is_same_v< ExprT, Variable< 0, bool >>, "debug" );

        // helper that calls our solver with our scope and parameters
        auto helper = [&]< size_t... Is >( Solver< ExprT >& solve_func, seq< Is... > ) constexpr
        { return solve_func( scope, std::get< Is >( _params )... ); };

        if( not helper( solve_, make_seq< sizeof...( Params )>{} ))
            return false;

        // return the value of expr using our solved scope
        _solved = static_cast< bool >( expr | scope );

        std::println( "result of solve: {}", _solved );

        auto res = variables_tuple{} | scope;

        std::println( "vars: {}", res );

        scope_type::take_from( scope );
        return _solved;
    }

    constexpr SolverParams( Params... params ): scope_type{ variables_tuple{} },
        _params{ params... }, _solved{ false } { }

private:
    tuple< Params... > _params;
    bool _solved;
};

template< variable... Vars >
constexpr SolveFor< tuple< Vars... >> 
solve_for( Vars... )
{ return {}; }

template< typename... Params >
struct SolveFor< tuple<>, Params... >: 
    SolverParams< tuple<>, Params... > 
{ 
    template< typename ExprT >
    constexpr bool operator ()( ExprT const& expr )
    { return SolverParams< tuple<>, Params... >::solve( expr ); }
};

/// @brief solve_for a single variable
template< variable Var, typename... Params >
struct SolveFor< tuple< Var >, Params... >: 
    SolverParams< tuple< Var >, Params... >
{ 
    using value_type = Var::value_type;

    template< typename ExprT >
    constexpr value_type operator ()( ExprT const& expr )
    {
        SolverParams< tuple< Var >, Params... >::solve( expr );
        return operator value_type(); 
    }

    constexpr operator value_type() const
    { return Scope< Var >::template get_value< Var >(); }
};

template< variable... Vars, typename... Params >
requires( is_greater( sizeof...( Vars ), 1 ))
struct SolveFor< tuple< Vars... >, Params... >:
    SolverParams< tuple< Vars... >, Params... >
{
    using value_type = tuple< typename Vars::value_type... >;

    template< typename ExprT >
    constexpr value_type operator ()( ExprT const& expr )
    { 
        SolverParams< tuple< Vars... >, Params... >::solve( expr );
        return operator value_type();
    }

    constexpr operator value_type() const
    {
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr ->
            tuple< typename Vars::value_type... >
        { return { Scope< Vars... >::template 
            get_value< Vars...[ Is ]>()... }; };

        return helper( make_seq< sizeof...( Vars )>{} ); 
    }
};

// HACK: define application operator until SolveFor is upgraded to a
// compound expression
template< expression ExprT, typename... Params >
constexpr auto operator |( ExprT const& expr, 
    SolveFor< tuple<>, Params... >&& scope )
{ return scope( expr ); }

template< expression ExprT, variable... Vars, typename... Params >
requires( is_greater( sizeof...( Vars ), 0 )) // and solvable< ExprT > )
constexpr auto operator |( ExprT const& expr, 
    SolveFor< tuple< Vars... >, Params... >&& solve_for )
{ return solve_for( expr ); }

//////////////////////////////////
/// Static Expression Solvers ///
////////////////////////////////
///
/// @brief solver for an expression with no dependent variables is trivial
template< static_expression ExprT >
struct Solver< ExprT >
{
    using expression_type = ExprT;
    using result_type = result_t< expression_type >;
    static constexpr bool is_solvable() { return true; }

    constexpr expression_type expression() const
    { return _expr; }

    template< typename ScopeT >
    constexpr result_type operator ()( ScopeT& scope ) const
    { return _expr | scope; }
    
    constexpr Solver( expression_type const& expr ): _expr{ expr } { }

private:
    expression_type _expr;
};

///////////////////////
/// Trival Solvers ///
/////////////////////
/// 
/// @brief solver for a boolean variable 
template< size_t I >
struct Solver< Variable< I, bool >>
{
    using expression_type = Variable< I, bool >;
    using variable_type = Variable< I, bool >;
    using result_type = bool;
    static constexpr bool is_solvable() { return true; }

    template< typename ScopeT >
    constexpr result_type operator()( ScopeT& scope ) const
    {
        scope.template set_value< variable_type >( true );
        return true;
    }

    constexpr expression_type expression() const
    { return _expr; }

    constexpr Solver( expression_type const& expr = {} ): _expr{ expr } { }

private:
    expression_type _expr;
};


template< size_t I, typename T, static_expression ExprT >
struct Solver< Equals< Variable< I, T >, ExprT >> 
{
    using expression_type = Equals< Variable< I, T >, ExprT >;
    using result_type = result_t< expression_type >;
    using variable_type = Variable< I, T >;
    using right_expression_type = ExprT;
    static constexpr bool is_solvable() { return true; }

    constexpr expression_type expression() const
    { return _expr; }

    template< typename ScopeT >
    constexpr result_type operator ()( ScopeT& scope ) const
    {
        auto value = _expr.right_arg() | scope;
        scope.template set_value< variable_type >( value );
        return _expr | scope;
    }

    constexpr Solver( expression_type const& expr = {} ): _expr{ expr } { }

private:
    expression_type _expr;
};


///////////////////////////////////
/// Boolean Expression Solvers ///
/////////////////////////////////
///
/// @brief solver for normalized boolean expressions
///
/// Canonical expressions are of the following forms:
///
/// Disjunction< [ Conjunction< EqualsZero< Exprs >... > | EqualsZero< Exprs > ]... >
/// Conjunction< EqualsZero< Exprs >... >
/// EqualsZero< ExprT >
///
/// ( f( args... ) == 0 and g( args... ) == 0 ) or h( args... ) == 0 
///

template< typename T >
requires( std::totally_ordered< T >)
struct positive
{
    constexpr T& value() 
    { return _value; }

    constexpr T const& value() const
    { return _value; }

    // I don't know if this should only return positive values or not...
    constexpr operator T() const
    //{ return _value < T{0} ? T{0} : value; }
    { return value(); }

    constexpr T error() const
    { return _value <= T{0} ? -_value : T{0}; }

    constexpr positive& operator =( T const& other )
    { _value = other; return *this; }

    T _value;
};

////////////////////////////////////
/// Comparison Canonicalization ///
//////////////////////////////////
///
/// Depth-First, Indexed Visitors for transforming numerical comparison 
/// expressions (like A < B) to EqualsZero expressions (like A - B + e == 0).
/// Since these will require new variables to be added to the expression we 
/// require a unique index I for each visited tree in the expression.  This 
/// will ensure that the variables added to the expression to represent the
/// differences between compared values will have a unique variable id.
///
/// We have twelve specializations because we simultaneously handle compliments
/// of comparisons.  In other words we handle all the following comparison
/// patterns:
///
///     A == B, not A != B, A != B, not A == B,
///     A <  B, not A >= B, A <= B, not A >  B,
///     A >  B, not A <= B, A >= B, not A <  B,
///
/// Equals and Not Equals
///
template< size_t I, expression ExprT >
struct ComparisonCanonicalizer
{ 
    using type = ExprT;
    static constexpr type value( ExprT const& expr )
    { return expr; }
};

// A == B --> A - B == 0
template< size_t I, expression LeftT, expression RightT >
struct ComparisonCanonicalizer< I, Equals< LeftT, RightT >>
{
    using type = EqualsZero< Difference< LeftT, RightT >>;
    static constexpr type value( Equals< LeftT, RightT > const& expr )
    { return {{ expr.left_arg(), expr.right_arg() }}; }
};

// not( A != B ) --> A - B == 0
template< size_t I, expression LeftT, expression RightT >
struct ComparisonCanonicalizer< I, Compliment< NotEquals< LeftT, RightT >>>:
    ComparisonCanonicalizer< I, Equals< LeftT, RightT >>
{
    using type = ComparisonCanonicalizer< I, Equals< LeftT, RightT >>::type;

    static constexpr type value( Compliment< NotEquals< LeftT, RightT >> const& expr )
    { return ComparisonCanonicalizer< I, Equals< LeftT, RightT >>::value(
        { expr.arg().left_arg(), expr.arg().right_arg() }); }
};

// A != B --> (A + e) - B == 0 or B - (A + e) == 0 [e > 0]
template< size_t I, expression LeftT, expression RightT >
struct ComparisonCanonicalizer< I, NotEquals< LeftT, RightT >>
{
    using positive_variable = Variable< I, positive< result_t< LeftT >>>;

    using type = Disjunction<
        EqualsZero< Difference< Sum< LeftT, positive_variable >, RightT >>,
        EqualsZero< Difference< RightT, Sum< LeftT, positive_variable >>>>;

    static constexpr type value( Equals< LeftT, RightT > const& expr )
    { return {
        {{{ expr.left_arg(), positive_variable{} }, expr.right_arg() }},
        {{ expr.right_arg(), { expr.left_arg(), positive_variable{} }}}}; }
};

// not( A == B ) --> (A + e) - B == 0 or B - (A + e) == 0 [e > 0] 
template< size_t I, expression LeftT, expression RightT >
struct ComparisonCanonicalizer< I, Compliment< Equals< LeftT, RightT >>>:
    ComparisonCanonicalizer< I, NotEquals< LeftT, RightT >>
{
    using type = ComparisonCanonicalizer< I, NotEquals< LeftT, RightT >>::type;

    static constexpr type value( Compliment< Equals< LeftT, RightT >> const& expr )
    { return ComparisonCanonicalizer< I, NotEquals< LeftT, RightT >>::value(
        { expr.arg().left_arg(), expr.arg().right_arg() }); }
};

/// Less Than and Greater Than

// A < B --> (A + e) - B == 0 [e > 0]
template< size_t I, expression LeftT, expression RightT >
struct ComparisonCanonicalizer< I, LessThan< LeftT, RightT >>
{
    using positive_variable = Variable< I, positive< result_t< LeftT >>>;

    using type = EqualsZero< Difference< Sum< LeftT, positive_variable >, RightT >>;

    static constexpr type value( LessThan< LeftT, RightT > const& expr )
    { return {{{ expr.left_arg(), positive_variable{} }, expr.right_arg() }}; }
};

// not( A >= B ) --> (A + e) - B == 0 [e > 0]
template< size_t I, expression LeftT, expression RightT >
struct ComparisonCanonicalizer< I, Compliment< GreaterThanOrEquals< LeftT, RightT >>>:
    ComparisonCanonicalizer< I, LessThan< LeftT, RightT >>
{
    using type = ComparisonCanonicalizer< I, LessThan< LeftT, RightT >>::type;

    static constexpr type value( Compliment< GreaterThanOrEquals< LeftT, RightT >> const& expr )
    { return ComparisonCanonicalizer< I, LessThan< LeftT, RightT >>::value(
        { expr.arg().left_arg(), expr.arg().right_arg() }); }
};

// A > B --> A - (B + e) == 0 [e > 0]
template< size_t I, expression LeftT, expression RightT >
struct ComparisonCanonicalizer< I, GreaterThan< LeftT, RightT >>
{
    using positive_variable = Variable< I, positive< result_t< LeftT >>>;

    using type = EqualsZero< Difference< LeftT, Sum< RightT, positive_variable >>>;

    static constexpr type value( GreaterThan< LeftT, RightT > const& expr )
    { return {{ expr.left_arg(), { expr.right_arg(), positive_variable{} }}}; }
};

// not(A <= B) --> A - (B + e) == 0 [e > 0]
template< size_t I, expression A, expression B >
struct ComparisonCanonicalizer< I, Compliment< LessThanOrEquals< A, B >>>:
    ComparisonCanonicalizer< I, GreaterThan< A, B >>
{
    using type = ComparisonCanonicalizer< I, GreaterThan< A, B >>::type;

    static constexpr type value( Compliment< LessThanOrEquals< A, B >> const& expr )
    { return ComparisonCanonicalizer< I, GreaterThan< A, B >>::value(
        { expr.arg().left_arg(), expr.arg().right_arg() }); }
};

/// [ LessThan | GreaterThan ] Or Equals

// A <= B --> (A + e) - B == 0 or A - B == 0 [e > 0]
template< size_t I, expression A, expression B >
struct ComparisonCanonicalizer< I, LessThanOrEquals< A, B >>
{
    using positive_variable = Variable< I, positive< result_t< A >>>;

    using type = Disjunction<
        EqualsZero< Difference< Sum< A, positive_variable >, B >>,
        EqualsZero< Difference< A, B >>>;

    static constexpr type value( LessThanOrEquals< A, B > const& expr )
    { return {
        {{{ expr.left_arg(), positive_variable{} }, expr.right_arg() }},
        {{ expr.left_arg(), expr.right_arg() }}}; }
};

// not( A > B ) --> (A + e) - B == 0 or A - B == 0 [e > 0]
template< size_t I, expression A, expression B >
struct ComparisonCanonicalizer< I, Compliment< GreaterThan< A, B >>>:
    ComparisonCanonicalizer< I, LessThanOrEquals< A, B >>
{
    using type = ComparisonCanonicalizer< I, LessThanOrEquals< A, B >>::type;

    static constexpr type value( Compliment< GreaterThan< A, B >> const& expr )
    { return ComparisonCanonicalizer< I, LessThanOrEquals< A, B >>::value(
        { expr.arg().left_arg(), expr.arg().right_arg() }); }
};

// A >= B --> A - (B + e) == 0 or A - B == 0 [e > 0]
template< size_t I, expression A, expression B >
struct ComparisonCanonicalizer< I, GreaterThanOrEquals< A, B >>
{
    using positive_variable = Variable< I, positive< result_t< B >>>;

    using type = Disjunction< 
        EqualsZero< Difference< A, Sum< B, positive_variable >>>,
        EqualsZero< Difference< A, B >>>;

    static constexpr type value( GreaterThanOrEquals< A, B > const& expr )
    { return {
        {{ expr.left_arg(), { expr.right_arg(), positive_variable{} }}},
        {{ expr.left_arg(), expr.right_arg() }}}; }
};

// not(A < B) --> A - (B + e) == 0 or A - B == 0 [e > 0]
template< size_t I, expression A, expression B >
struct ComparisonCanonicalizer< I, Compliment< LessThan< A, B >>>:
    ComparisonCanonicalizer< I, GreaterThanOrEquals< A, B >>
{
    using type = ComparisonCanonicalizer< I, GreaterThanOrEquals< A, B >>::type;

    static constexpr type value( Compliment< LessThan< A, B >> const& expr )
    { return ComparisonCanonicalizer< I, GreaterThanOrEquals< A, B >>::value(
        { expr.arg().left_arg(), expr.arg().right_arg() }); }
};

template< typename ExprT >
constexpr auto canonical_comparison( ExprT const& expr )
{ 
    using dependent_vars_tuple = free_variables_t< ExprT >;

    static constexpr size_t first_variable_id = next_variable_id_v< ExprT >;

    return ComparisonCanonicalizer< first_variable_id, ExprT >::value( expr );
}

template< typename ExprT >
constexpr auto canonicalize( ExprT const& expr )
{
    // 1) we normalize the expression to push the compliments down to the comparison
    //    operations
    auto norm = normalize< Disjunction, Conjunction, Compliment >( expr );

    // 2) we replace comparisons with their canonical representations using special 
    //    variables e > 0
    //    - A >= B --> A - B - e == 0 or A - B == 0
    //    - A <= B --> B - A - e == 0 or A - B == 0
    //    - A > B  --> A - B - e == 0
    //    - A < B  --> B - A - e == 0
    //    - A == B --> A - B == 0
    //    - A != B --> A - B - e == 0 or B - A - e == 0
    auto can_comp = canonical_comparison( norm );

    // 3) we re-distribute to put the expression back into disjunctive normal form,
    //    this time without any compliements and only A == 0 logical operations
    return distribute< Disjunction, Conjunction >( can_comp );
}

//////////////////////////////////////////
/// Linear System of Equations Solver ///
////////////////////////////////////////
///
/// Solver for a linear system of equations
///
namespace detail {
template< typename ExprT >
struct IsLinear: integral_constant< bool, false > { };

/// Static expressions are linear
template< typename ExprT >
requires( static_expression< ExprT >)
struct IsLinear< ExprT >: integral_constant< bool, true > { };

/// Variables are linear
template< size_t I, typename T >
struct IsLinear< Variable< I, T >>: integral_constant< bool, true > { };

/// Sums of linear expressions are also linear
template< typename... Ts >
requires( not static_expression< Sum< Ts... >>)
struct IsLinear< Sum< Ts... >>: integral_constant< bool,
    ( IsLinear< Ts >::value and ... )> { };

/// Differences of linear expressions are also linear
template< typename... Ts >
requires( not static_expression< Difference< Ts... >>)
struct IsLinear< Difference< Ts... >>: integral_constant< bool,
    ( IsLinear< Ts >::value and ... )> { };

/// Products are linear they are static or exactly one of the arguments
/// is linear.
template< typename... Ts >
requires( not static_expression< Product< Ts... >>)
struct IsLinear< Product< Ts... >> 
{
    // count how many linear expressions are arguments to this product
    static constexpr size_t linear_arguments_size = 
        (( IsLinear< Ts >::value and not static_expression< Ts > ? 1 : 0 ) + ... );

    // count how many static expressions are arguments to this product
    static constexpr size_t static_arguments_size = 
        (( static_expression< Ts > ? 1 : 0 ) + ... );

    // if one or less arguments to a product is linear and the rest are static
    // then the resulting product is linear
    static constexpr bool value = linear_arguments_size <= 1 and
        linear_arguments_size + static_arguments_size == sizeof...( Ts );
};

/// A quotient is linear if the numerator is linear and the denominator is static
/// NOTE: we do not consider 1/(1/x) to be linear because of division by zero.
template< typename NumT, typename DenU >
requires( not static_expression< Quotient< NumT, DenU >>)
struct IsLinear< Quotient< NumT, DenU >>: integral_constant< bool,
    IsLinear< NumT >::value and static_expression< DenU >> { };

/// A quotient of arbitrary parameters is linear if the folded expression is 
/// linear
template< typename First, typename... Rest >
requires( not static_expression< Quotient< First, Rest... >> and 
    is_greater( sizeof...( Rest ), 1 ))
struct IsLinear< Quotient< First, Rest... >>: 
    IsLinear< Quotient< First, Quotient< Rest... >>> { };

template< variable Var, typename ExprT >
struct IsLinearOf
{
    using dependent_variables = free_variables_t< ExprT >;

    template< size_t I >
    using ith_var = dependent_variables::template element_t< I >;

    template< size_t I >
    using ith_var_result_type = result_t< ith_var< I >>;

    template< typename Seq >
    struct Helper;

    // if the Is-th var is Var then replace it with itself, otherwise replace it with a constant
    template< size_t... Is >
    struct Helper< seq< Is... >>: Substituter< ExprT, 
        std::conditional_t< ith_var< Is >::id == Var::id, 
            Var, Constant< ith_var_result_type< Is >{ 0 }>>... >
    { };

    static constexpr bool value = IsLinear< typename
        Helper< make_seq< dependent_variables::size >>::type >::value;
};

template< variable Var, typename ExprT >
struct ScalarOf;

template< variable Var, typename ExprT >
requires( not is_boolean_expression_v< ExprT > and 
    not depends_on_variable_v< Var, ExprT >)
struct ScalarOf< Var, ExprT >
{ 
    using type = Constant< result_t< ExprT >{ 0 }>;
    static constexpr type value( ExprT const& )
    { return {}; }
};

template< variable Var, typename ExprT >
requires( not is_boolean_expression_v< ExprT > and 
    depends_on_variable_v< Var, ExprT > and
    IsLinearOf< Var, ExprT >::value )
struct ScalarOf< Var, ExprT >
{
    // First we substitute zero for any subexpression that doesn't depend on Var
    using zero_type = Constant< result_t< ExprT >{ 0 }>;
    using one_type = Constant< result_t< Var >{ 1 }>;

    struct ScalarEvaluator
    {
        // if we don't depend on Var then this term should be zero
        template< typename ExprU >
        constexpr result_t< ExprU > operator ()( ExprU const& expr ) const
        requires( not depends_on_variable_v< Var, ExprU >)
        { return 0; }

        // if we depend on Var and only Var then set Var equal to 1
        template< typename ExprU >
        constexpr result_t< ExprU > operator ()( ExprU const& expr ) const
        requires( depends_on_variable_v< Var, ExprU > and
            free_variables_t< ExprU >::size == 1 )
        { return expr | make_scope< Var >( 1 ); }

        // otherwise continue parsing the expression (default manipulation behavior)
    };

    using type = decltype( result_t< ExprT >{} / result_t< Var >{} );

    static constexpr type value( ExprT const& expr )
    { 
//        auto zeroed_non_dependents = not_dependent_substituter::value( expr, zero_type{} );
//        static_assert( depends_on_variable_v< Var, decltype( zeroed_non_dependents )> );
//        return substitute_for< Var >( zeroed_non_dependents, one_type{} ) | eval();
//        return substitute( zeroed_non_dependents, one_type{} ) | eval();
//        return substitute( expr, one_type{} ) | eval();
        return expr | ScalarEvaluator{};
    }
};


template< typename ExprT >
struct NonHomogeneousTerm
{
    using variables = free_variables_t< ExprT >;

    template< typename Vars >
    struct Helper;

    template< variable... Vars >
    struct Helper< unique_variables< Vars... >>
    {
        using type = result_t< ExprT >;

        static constexpr type value( ExprT const& expr )
        { return substitute( expr, Constant< result_t< Vars >{ 0 }>{}... ) | eval(); }
    };

    using type = Helper< variables >::type;
    static constexpr type value( ExprT const& expr )
    { return Helper< variables >::value( expr ); }
};

template< variable Var, typename ExprT >
requires( IsLinear< ExprT >::value )
constexpr typename ScalarOf< Var, ExprT >::type 
scalar_of( ExprT const& expr, Var = {} )
{ return ScalarOf< Var, ExprT >::value( expr ); }

template< typename ExprT >
constexpr typename NonHomogeneousTerm< ExprT >::type
non_homogeneous_term_of( ExprT const& expr )
{ return NonHomogeneousTerm< ExprT >::value( expr ); }

/// A zero or one power of a linear equation is linear
/// NOTE: sqrt( pow< 2 >( x )) is not considered linear because 
///       sqrt( pow< 2 >( -1 )) == 1
/// TODO: write canonicalizer for pow expressions
//template< typename ExprT >
//requires( not static_expression< typename Power< 0 >::template Of< ExprT >>) 
//struct IsLinear< typename Power< 0 >::template Of< ExprT >>: integral_constant< bool, 
//    IsLinear< ExprT >::value > { };
//
//template< typename ExprT >
//requires( not static_expression< typename Power< 1 >::template Of< ExprT >>) 
//struct IsLinear< typename Power< 1 >::template Of< ExprT >>: integral_constant< bool, 
//    IsLinear< ExprT >::value > { };
//
template< typename ExprT >
struct IsLinearEquation: integral_constant< bool, false > { };

template< typename A, typename B >
struct IsLinearEquation< Equals< A, B >>: integral_constant< bool, 
    IsLinear< A >::value and IsLinear< B >::value > { };

template< typename ExprT >
struct LinearSystem: integral_constant< bool, false > { };

// a linear system of equations is a conjuction of linear equations:
//     Ax = b
template< typename... Exprs >
struct LinearSystem< Conjunction< Exprs... >>:
    integral_constant< bool, ( IsLinearEquation< Exprs >::value and ... )> 
{
    using expression_type = Conjunction< Exprs... >;
    using variables = free_variables_t< expression_type >;

    static constexpr size_t size = sizeof...( Exprs );

private:
    using A_shape = Shape< size, variables::size >;
    using b_shape = Shape< size >;

    template< size_t I >
    using var = variables::template element_t< I >;

    template< size_t I >
    using var_t = var< I >::value_type;
    
    template< typename LeftT, typename RightT >
    static constexpr auto bterm( Equals< LeftT, RightT > const& eq )
    { return non_homogeneous_term_of( eq.right_arg() - eq.left_arg() ); }

    template< typename LeftT >
    static constexpr auto bterm( EqualsZero< LeftT > const& eq )
    { return non_homogeneous_term_off( -eq.arg() ); }

    static constexpr auto b( expression_type const& expr )
    { 
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
        { return make_tensor< b_shape >( bterm( get_argument< Is >( expr ))... ); };

        return helper( make_seq< size >{} );
    }

    template< size_t J, typename LeftT, typename RightT >
    static constexpr auto Aterm( Equals< LeftT, RightT > const& eq )
    { return scalar_of< var< J >>( eq.left_arg() - eq.right_arg() ); }

    template< size_t J, typename LeftT >
    static constexpr auto Aterm( EqualsZero< LeftT > const& eq )
    { return scalar_of< var< J >>( eq.arg() ); }

    static constexpr auto A( expression_type const& expr )
    {
        auto helper = [&]< size_t... Js >( seq< Js... > ) constexpr
        { return make_tensor< A_shape >(
            Aterm< A_shape::from_element( Js )[ 1 ]>( 
                get_argument< A_shape::from_element( Js )[ 0 ]>( expr ))... ); };

        return helper( make_seq< A_shape::size() >{} );
    }

public:
    static constexpr bool is_solvable = 
        ( size == variables::size );

    static constexpr auto solution( expression_type const& expr )
    { return matmul( inverse( A( expr )), b( expr )); }

};


} // namespace detail

template< typename ExprT >
constexpr bool is_linear_equation_v = detail::IsLinearEquation< ExprT >::value;

template< variable Var, typename ExprT >
constexpr bool is_linear_of_v = detail::IsLinearOf< Var, ExprT >::value;

template< typename ExprT >
consteval bool is_linear_equation( ExprT const& expr )
{ return is_linear_equation_v< ExprT >; }

template< variable Var, typename ExprT >
consteval bool is_linear_of( ExprT const& expr, Var = {} )
{ return is_linear_of_v< Var, ExprT >; }

template< typename ExprT >
constexpr bool is_linear_system_v = detail::LinearSystem< ExprT >::value;

template< typename ExprT >
requires( is_linear_system_v< ExprT >)
constexpr auto solve_linear_system( ExprT const& expr )
{ return detail::LinearSystem< ExprT >::solution( expr ); }

template< typename T >
concept linear_system = is_linear_system_v< T >;

/// @brief Solver specialization for linear systems
template< linear_system ExprT >
struct Solver< ExprT >
{
    using expression_type = ExprT;
    using variables_tuple = free_variables_t< ExprT >::variables_tuple;

    static constexpr bool is_solvable() 
    { return detail::LinearSystem< ExprT >::is_solvable; }

    template< typename ScopeT, typename... Params >
    constexpr bool operator ()( ScopeT& scope, Params... params )
    {
        auto sol = detail::LinearSystem< ExprT >::solution( _expr ) | eval();
        auto vars = variables_tuple{};
        
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
        {( scope.set_value( tensor_get< Is >( sol ), std::get< Is >( vars )), ... ); };

        helper( make_seq< std::tuple_size_v< variables_tuple >>{} );
        return _expr | scope;
    }

    constexpr Solver( expression_type const& expr ): _expr{ expr } { }
    expression_type _expr;
};

// @brief Solver for boolean expressions in disjunctive normal form.  The
// default case defers to an ordinary solver 
template< expression ExprT >
struct DNFSolver: Solver< ExprT > { };

/// @brief normalized disjunctions can be solved term by term
template< expression... Terms >
struct DNFSolver< Disjunction< Terms... >>
{
    using expression_type = Disjunction< Terms... >;

private:
    template< expression ExprU >
    struct TermSolver: Solver< ExprU > { };

    template< expression... Elements >
    struct TermSolver< Conjunction< Elements... >>:
        DNFSolver< Conjunction< Elements... >>
    { };

    template< expression ContraU >
    struct TermSolver< Compliment< ContraU >>:
        DNFSolver< Compliment< ContraU >>
    { };

    template< expression ExprU, typename ScopeT, typename... Params >
    constexpr bool solve_term( ExprU const& term, ScopeT& scope, Params... params )
    { return TermSolver< ExprU >{ term }( scope, params... ); }

    constexpr bool helper( auto& scope, seq< > )
    { return false; }
    
    // this could be rewritten to allow for parallel compilation.
    template< size_t I, size_t... Is >
    constexpr bool helper( auto& scope, seq< I, Is... > )
    {
        if( solve_term( get_argument< I >( _expr ), scope ))
            return true;

        return helper( scope, seq< Is... >{} );
    }

public:
    template< typename ScopeT, typename... Params >
    constexpr bool operator ()( ScopeT& scope, Params... params )
    { return helper( scope, make_seq< sizeof...( Terms )>{} ); }

    constexpr DNFSolver( expression_type const& expr ): _expr{ expr } { }

private:
    expression_type _expr;
};

template< expression LeftT, expression RightT >
struct DNFSolver< Compliment< NotEquals< LeftT, RightT >>>:
    Solver< Equals< LeftT, RightT >> 
{
    constexpr DNFSolver( Compliment< NotEquals< LeftT, RightT >> const& expr ):
        Solver< Equals< LeftT, RightT >>{{ expr.arg().left_arg(), expr.arg().right_arg() }}
    { }
};

template< expression LeftT, expression RightT >
struct DNFSolver< Compliment< Equals< LeftT, RightT >>>:
    Solver< NotEquals< LeftT, RightT >> 
{
    constexpr DNFSolver( Compliment< Equals< LeftT, RightT >> const& expr ):
        Solver< NotEquals< LeftT, RightT >>{{ expr.arg().left_arg(), expr.arg().right_arg() }}
    { }
};

template< expression LeftT, expression RightT >
struct DNFSolver< Compliment< LessThan< LeftT, RightT >>>:
    Solver< GreaterThanOrEquals< LeftT, RightT >> 
{
    constexpr DNFSolver( Compliment< LessThan< LeftT, RightT >> const& expr ):
        Solver< GreaterThanOrEquals< LeftT, RightT >>{{ expr.arg().left_arg(), expr.arg().right_arg() }}
    { }
};

template< expression LeftT, expression RightT >
struct DNFSolver< Compliment< LessThanOrEquals< LeftT, RightT >>>:
    Solver< GreaterThan< LeftT, RightT >> 
{
    constexpr DNFSolver( Compliment< LessThanOrEquals< LeftT, RightT >> const& expr ):
        Solver< GreaterThan< LeftT, RightT >>{{ expr.arg().left_arg(), expr.arg().right_arg() }}
    { }
};

template< expression LeftT, expression RightT >
struct DNFSolver< Compliment< GreaterThan< LeftT, RightT >>>:
    Solver< LessThanOrEquals< LeftT, RightT >> 
{
    constexpr DNFSolver( Compliment< GreaterThan< LeftT, RightT >> const& expr ):
        Solver< LessThanOrEquals< LeftT, RightT >>{{ expr.arg().left_arg(), expr.arg().right_arg() }}
    { }
};

template< expression LeftT, expression RightT >
struct DNFSolver< Compliment< GreaterThanOrEquals< LeftT, RightT >>>:
    Solver< LessThan< LeftT, RightT >> 
{
    constexpr DNFSolver( Compliment< GreaterThanOrEquals< LeftT, RightT >> const& expr ):
        Solver< LessThan< LeftT, RightT >>{{ expr.arg().left_arg(), expr.arg().right_arg() }}
    { }
};

template< expression... Elements >
struct DNFSolver< Conjunction< Elements... >>
{
    using expression_type = Conjunction< Elements... >;

private:
    template< expression ExprU >
    struct ElementSolver: Solver< ExprU > { };

    template< expression ExprU, typename ScopeT, typename... Params >
    constexpr bool solve_element( ExprU const& element, ScopeT& scope, Params... params )
    { return ElementSolver< ExprU >{ element }( scope, params... ); }

    template< typename ScopeT, size_t... Is, typename... Params >
    constexpr bool helper( ScopeT& scope, seq< Is... >, Params... params )
    {
        // create an array of scopes to be checked for compabibility
        std::array< ScopeT, sizeof...( Is )> scopes;

        // solve each element in it's own scope
        // NOTE: do we need "solve_all" here to loop through each possible sub-solution?
        bool solved = ( solve_element( get_argument< Is >( _expr ), 
            std::get< Is >( scopes ), params... ) and ... );

        if( not solved or not compatible_scopes( scopes ))
            return false;
        
        // we found a set of compatible solutions for this conjunction, merge them
        scope = merge_compatible_scopes( scopes );
        return true;
    }   

public:
    template< typename ScopeT, typename... Params >
    constexpr bool operator ()( ScopeT& scope, Params... params )
    { return helper( scope, make_seq< sizeof...( Elements )>{} ); }

    constexpr DNFSolver( expression_type const& expr ): _expr{ expr } { }

private:
    expression_type _expr;
};

/// @brief boolean expressions are normalized before solving with a bespoke
/// solver
template< expression ExprT >
requires(( is_disjunction_v< ExprT > or is_conjunction_v< ExprT > or is_compliment_v< ExprT >) and not linear_system< ExprT > )
struct Solver< ExprT >: 
    DNFSolver< normalized_t< Disjunction, Conjunction, Compliment, ExprT >>
{
    using expression_type = ExprT;
    using normalized_expression_type = 
        normalized_t< Disjunction, Conjunction, Compliment, expression_type >;
    using base_solver = DNFSolver< normalized_expression_type >;

    constexpr Solver( expression_type const& expr ):
        base_solver{ normalize< Disjunction, Conjunction, Compliment >( expr )}
    { }
};


/// @brief solvers for boolean expressions
///

/// @brief the simplest solvers
//template< size_t I, typename T, static_expression ExprT >
//requires( is_convertible_v< result_t< ExprT >, T > )
//struct Solver< Equals< Variable< I, T >, ExprT >>:
//    Scope< Variable< I, T >>
//{
//    using right_expression_type = ExprT;
//    using variable_type = Variable< I, T >;
//    using expression_type = Equals< variable_type, right_expression_type >;
//    using scope_type = Scope< variable_type >;
//
//    // this solver does not need to iterate
//    static constexpr bool is_iterative() { return false; }
//    // this solver is guaranteed to complete
//    static constexpr bool is_bounded() { return true; }
//    // this solver will always converge on the correct solution
//    static constexpr bool is_convergent() { return true; }
//
////    template< typename ExprU >
////    requires( is_scope_for_v< scope_type, ExprU > )
////    constexpr result_t< ExprU > operator ()( ExprU const& expr ) const
////    { return scope_type::operator ()( expr ); }
//
//    // the solution is trivial here
//    constexpr Solver( expression_type const& expr = {} ): scope_type{ }, _equal_to{ expr.right_arg() }
//    { scope_type::template set_value< Variable< I, T >>( 
//        scope_type::operator ()( _equal_to )); }
//
//private:
//    right_expression_type _equal_to;
//};
//
// TODO: handle StaticValues which will require copying the values and not
// just matching types
template< expression LeftT, expression RightT >
requires requires { typename Solver< Equals< RightT, LeftT >>; }
struct Solver< Equals< LeftT, RightT >>: Solver< Equals< RightT, LeftT >>
{ 
    using expression_type = Equals< LeftT, RightT >;
    using base_solver = Solver< Equals< RightT, LeftT >>;

    constexpr Solver( expression_type const& expr = {} ):
        base_solver{{ expr.right_arg(), expr.left_arg() }}
    { }
};

/// @brief solvers for -X = C
template< variable Var, static_expression ExprT >
struct Solver< Equals< Negation< Var >, ExprT >>:
    Solver< Equals< Var, Negation< ExprT >>>
{ 
    using expression_type = Equals< Negation< Var >, ExprT >;
    using base_solver = Solver< Equals< Var, Negation< ExprT >>>;

    constexpr Solver( expression_type const& expr = {} ):
        base_solver{{ expr.left_arg().arg(), { expr.right_arg() }}}
    { }
};

/// @brief solvers for X + ... = C
///
template< size_t I, typename T, static_expression... Addends, 
    static_expression ExprT >
requires( is_greater( sizeof...( Addends ), 1 ))
struct Solver< Equals< Sum< Variable< I, T >, Addends... >, ExprT >>:
    Solver< Equals< Variable< I, T >, Difference< ExprT, Sum< Addends... >>>>
{
    using expression_type = Equals< Sum< Variable< I, T >, Addends... >, ExprT >;
    using solver_expression_type = Equals< Variable< I, T >, Difference< ExprT, Sum< Addends... >>>;
    using base_solver = Solver< solver_expression_type >;

    static constexpr solver_expression_type 
    translate( expression_type const& expr )
    {
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr ->
            solver_expression_type
        { return { get_argument< 0 >( expr.left_arg() ), { expr.right_arg(),
            { get_argument< 1 + Is >( expr.left_arg() )... }}}; };

        return helper( make_seq< sizeof...( Addends )>{} );
    }

    constexpr Solver( expression_type const& expr = {} ):
        base_solver{ translate( expr )} { }
};

template< size_t I, typename T, static_expression AddendT, 
    static_expression ExprT >
struct Solver< Equals< Sum< Variable< I, T >, AddendT >, ExprT >>:
    Solver< Equals< Variable< I, T >, Difference< ExprT, AddendT >>>
{ 
    using expression_type = Equals< Sum< Variable< I, T >, AddendT >, ExprT >;
    using base_solver = Solver< Equals< Variable< I, T >, Difference< ExprT, AddendT >>>;

    constexpr Solver( expression_type const& expr = {} ):
        base_solver{{ expr.left_arg().left_arg(), { expr.right_arg(), expr.left_arg().right_arg() }}}
    { }
};

template< size_t I, typename T, static_expression AddendT,
    static_expression ExprT >
struct Solver< Equals< Sum< AddendT, Variable< I, T >>, ExprT >>:
    Solver< Equals< Variable< I, T >, Difference< ExprT, AddendT >>>
{ 
    using expression_type = Equals< Sum< AddendT, Variable< I, T >>, ExprT >;
    using base_solver = Solver< Equals< Variable< I, T >, Difference< ExprT, AddendT >>>;

    constexpr Solver( expression_type const& expr = {} ):
        base_solver{{ expr.left_arg().left_arg(), { expr.right_arg(), expr.left_arg().right_arg() }}}
    { }
};

template< size_t I, typename T, static_expression AddendT,
    static_expression... Addends, static_expression ExprT >
requires( is_greater( sizeof...( Addends ), 0 ))
struct Solver< Equals< Sum< AddendT, Variable< I, T >, Addends... >, ExprT >>:
    Solver< Equals< Sum< Variable< I, T >, Addends... >, Difference< ExprT, AddendT >>>
{ 
    using expression_type = Equals< Sum< AddendT, Variable< I, T >, Addends... >, ExprT >;
    using solver_expression_type = Equals< Sum< Variable< I, T >, Addends... >, Difference< ExprT, AddendT >>;
    using base_solver = Solver< solver_expression_type >;

    constexpr static solver_expression_type 
    translate( expression_type const& expr )
    {
        auto helper = [&]< size_t... Is >( seq< Is... >) constexpr ->
            solver_expression_type
        { return {{ get_argument< 1 >( expr.left_arg() ), get_argument< 2 + Is >( expr.left_arg() )... },
            { expr.right_arg(), get_argument< 0 >( expr.left_arg() ) }}; };

        return helper( make_seq< sizeof...( Addends )>{} );
    }

    constexpr Solver( expression_type const& expr = {} ):
        base_solver{ translate( expr )} { }
};

/// @brief solvers for X - ... = C
template< size_t I, typename T, static_expression... Subends, 
    static_expression ExprT >
requires( is_greater( sizeof...( Subends ), 1 ))
struct Solver< Equals< Difference< Variable< I, T >, Subends... >, ExprT >>:
    Solver< Equals< Variable< I, T >, Sum< ExprT, Difference< Subends... >>>>
{ 
    using expression_type = Equals< Difference< Variable< I, T >, Subends... >, ExprT >;
    using solver_expression_type = Equals< Variable< I, T >, Sum< ExprT, Difference< Subends... >>>;
    using base_solver = Solver< solver_expression_type >;

    constexpr static solver_expression_type
    translate( expression_type const& expr )
    {
        auto helper = [&]< size_t... Is >( seq< Is... >) constexpr ->
            solver_expression_type
        { return { get_argument< 0 >( expr.left_arg() ), { expr.right_arg(),
            { get_argument< 1 + Is >( expr.left_arg() )... }}}; };

        return helper( make_seq< sizeof...( Subends )>{} ); 
    }

    constexpr Solver( expression_type const& expr = {} ):
        base_solver{ translate( expr )} { }
};

template< size_t I, typename T, static_expression SubendT, 
    static_expression ExprT >
struct Solver< Equals< Difference< Variable< I, T >, SubendT >, ExprT >>:
    Solver< Equals< Variable< I, T >, Sum< ExprT, SubendT >>>
{ 
    using expression_type = Equals< Difference< Variable< I, T >, SubendT >, ExprT >;
    using base_solver = Solver< Equals< Variable< I, T >, Sum< ExprT, SubendT >>>;

    constexpr Solver( expression_type const& expr = {} ):
        base_solver{{ expr.left_arg().expr.left_arg(), { expr.right_arg(), expr.left_arg().expr.right_arg() }}}
    { }
};

template< size_t I, typename T, static_expression SubendT,
    static_expression ExprT >
struct Solver< Equals< Difference< SubendT, Variable< I, T >>, ExprT >>:
    Solver< Equals< Variable< I, T >, Sum< ExprT, SubendT >>>
{ 
    using expression_type = Equals< Difference< SubendT, Variable< I, T >>, ExprT >;
    using base_solver = Solver< Equals< Variable< I, T >, Sum< ExprT, SubendT >>>;

    constexpr Solver( expression_type const& expr = {} ):
        base_solver{{ expr.left_arg().expr.right_arg(), { expr.right_arg(), expr.left_arg().expr.left_arg() }}}
    { }
};

template< size_t I, typename T, static_expression SubendT,
    static_expression... Subends, static_expression ExprT >
requires( is_greater( sizeof...( Subends ), 0 ))
struct Solver< Equals< Difference< SubendT, Variable< I, T >, Subends... >, ExprT >>:
    Solver< Equals< Difference< Variable< I, T >, Subends... >, Difference< SubendT, ExprT >>>
{ 
    using expression_type = Equals< Difference< SubendT, Variable< I, T >, Subends... >, ExprT >;
    using solver_expression_type = Solver< Equals< Difference< Variable< I, T >, Subends... >, Difference< SubendT, ExprT >>>;
    using base_solver = Solver< solver_expression_type >;

    constexpr static solver_expression_type
    translate( expression_type const& expr )
    {
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr ->
            solver_expression_type 
        { return {{ get_argument< 1 >( expr.left_arg() ), get_argument< 2 + Is >( expr.left_arg() )... }, 
            { get_argument< 0 >( expr.left_arg() ), expr.right_arg() }}; };

        return helper( make_seq< sizeof...( Subends )>{} );
    }

    constexpr Solver( expression_type const& expr = {} ):
        base_solver{ translate( expr )} { }
};

template< std::invocable UntilFunc >
constexpr UntilFunc 
loop_until( UntilFunc until )
{ for(;;) { if( until() ) return until; } }

template< std::invocable WhileFunc >
constexpr WhileFunc
loop_while( WhileFunc cond )
{ for(;;) { if( not cond() ) return cond; } }

template< typename UntilExpr >
struct LoopUntil;

template< typename WhileExpr >
struct LoopWhile;

template< expression UntilE >
constexpr LoopUntil< UntilE >
loop_until( UntilE until_expr );

template< expression WhileE >
constexpr LoopWhile< WhileE >
loop_while( WhileE while_expr );

template< expression UntilExpr >
struct LoopUntil< UntilExpr >: Arguments< LoopUntil, UntilExpr >
{
    using until_expression_type = UntilExpr;
    using scope_type = expression_traits< until_expression_type >::scope_type;
    
    template< std::invocable U >
    static constexpr U value( U expr )
    { return loop_until( expr ); }

    template< expression U >
    static constexpr U value( U expr )
    {
        
        return loop_until( expr ); 
    }

    constexpr until_expression_type
    until_arg() const
    { return _until; }

    constexpr LoopUntil( until_expression_type const& until ):
        _until{ until } { }

private:
    until_expression_type _until;
};

// We need a version of an iteration that works as a compound_expression. 
// Perhaps it's a sequence, something that can accomodate:
//
// x_n == x_{n-1} + x_{n-2}
// x_0 == 0
// x_1 == 1
//
// x:N->R
// 
// x(n) == x(n-1) + x(n-2) and x(0) == 0 and x(1) == 1
// 

template< typename VarTuple >
struct IsSequenceVariables: std::false_type {};

// sequencees have a single dependent variable that is unsigned and integral
template< variable N >
struct IsSequenceVariables< unique_variables< N >>: 
    integral_constant< bool, std::is_unsigned_v< typename N::value_type > and
        std::is_integral_v< typename N::value_type >> { };

template< typename T >
constexpr bool is_sequence_variables_v = IsSequenceVariables< T >::value;

template< typename T >
struct IsSequence: integral_constant< bool, 
    expression< T > and is_sequence_variables_v< free_variables_t< T >>> 
{ };

template< typename T >
constexpr bool is_sequence_v = IsSequence< T >::value;

template< template< typename... > class Op, typename... Args >
requires( is_sequence_v< Op< Args... >> )
struct SequenceSolver
{
    
};


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
requires(( variable_traits< Vars >::value and ... ))
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
struct Iteration< UntilE, tuple< Updates... >, tuple< Vars... >>: 
    detail::ExpressionTag 
{
    using argument_types = tuple< UntilE, Updates..., Vars... >;
    using result_type = tuple< result_t< Vars >... >;
    using scope_type = Scope< Vars... >;
    using until_expression_type = UntilE;
    using updates_tuple = tuple< Updates... >;
    using variables_tuple = tuple< Vars... >; 

    static constexpr size_t variables_size = sizeof...( Vars );
    static constexpr auto for_variables = make_seq< variables_size >{};

    constexpr until_expression_type until_expr() const { return _until_expr; }
    constexpr updates_tuple updates() const{ return _updates; }
    constexpr variables_tuple vars() const { return _vars; }

    template< size_t Is >
    typename Vars...[ Is ]::result_type const&
    initial_value() const
    { return std::get< Is >( _inits ); }

    template< size_t Is >
    Updates...[ Is ] const& 
    update_expr() const
    { return std::get< Is >( _updates ); }

    template< typename ScopeT, size_t... Is >
    constexpr void initialize( ScopeT& scope, seq< Is... >) const 
    { ( scope( set_variable< Vars...[ Is ]>( initial_value< Is >() )), 
        ... ); }

    template< typename ScopeT, typename... Ts, size_t... Is >
    static constexpr void 
    initialize( ScopeT& scope, tuple< Ts... > const& inits, 
        seq< Is... > )
    { ( scope( set_variable< Vars...[ Is ]>( std::get< Is >( inits ))),
        ... ); }

    template< typename ScopeT, size_t... Is >
    constexpr void 
    update( ScopeT& scope, seq< Is... > ) const
    { ( scope( set_variable< Vars...[ Is ]>( update_expr< Is >() | scope )), 
        ... ); }

    template< typename ScopeT, typename... Ts, size_t... Is >
    static constexpr void update( ScopeT& scope, tuple< Ts... > const& updates,
        seq< Is... > )
    { ( scope( set_variable< Vars...[ Is ]>( std::get< Is >( updates ))),
        ... ); }

    // a static method cannot read the values of static_expr inside the
    // updates and until expressions. 
    // is there a static way to do an iteration so that it can be a full
    // citizen of the expressions?  Maybe only with Constant<...> and no
    // StaticValues.
    template< typename... InitialValues >
    requires( sizeof...( InitialValues ) == sizeof...( Vars ))
    static auto value( InitialValues... inits )
    {
        scope_type scope;


    }

    // TODO: should this be re-wrtten as the value method which would let
    // manipulator do it's thing?
    template< typename ManipulatorT >
    constexpr auto apply( ManipulatorT& manipulator ) const
    {
        scope_type scope;

        initialize( scope, for_variables );

        while( not ( until_expr() | scope ))
            update( scope, for_variables );

        return make_tuple(( scope( Vars{} ) | manipulator )... );
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
requires(( variable_traits< Vars >::value and ... ))
constexpr IterationInitializer< typename 
    variable_traits< Vars >::variable_type... > 
iteration( Vars... )
{ return { variable_traits< Vars >::variable()... }; }

// HACK: overload application operator until Iteration can be formalized as a 
// compound expression properly
template< typename UntilE, typename... Updates, variable... Vars, 
    typename ManipulatorT >
constexpr auto 
operator |( Iteration< UntilE, tuple< Updates... >, tuple< Vars... >> const& iteration,
    ManipulatorT& applicand )
{ return iteration.apply( applicand ); }



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
