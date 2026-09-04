/// work in progress ///

#ifndef __EXPRESSIONS_CALCULUS_HPP__
#define __EXPRESSIONS_CALCULUS_HPP__

#include "expressions/expressions.hpp"
#include "expressions/arithmetic.hpp"

#include <type_traits>

using std::true_type, std::false_type;

namespace expressions {

using namespace units;

template< typename T >
struct IsMetric: integral_constant< bool, unit< T > or is_arithmetic_v< T >>
{ };

/////////////////////
/// Metric Types ///
///////////////////
///
/// trait to identify types that allow basic arithmetic operations including
/// units
template< typename T >
constexpr bool is_metric_v = IsMetric< T >::value;

template< typename T >
concept metric = is_metric_v< T >;

template< typename T >
struct IsMetricFunction: false_type {};

template< typename ExprT, variable... Vars >
struct IsMetricFunction< Func< ExprT, Vars... >>: integral_constant< bool,
    is_metric_v< result_t< ExprT >> and 
        ( is_metric_v< result_t< Vars >> and ... and true )> { };

////////////////////////
/// Metric Function ///
//////////////////////
/// 
/// trait to identify whether an expression is a function which takes metric
/// variables and returns a metric result.
template< typename T >
constexpr bool is_metric_function_v = IsMetricFunction< T >::value;

template< typename T >
concept metric_function = is_metric_function_v< T >;

//////////////////////
/// derive method ///
////////////////////
/// 
/// This method calculates the derivative of an expression

// by default the derivative will return 0
template< size_t Id, typename ExprT >
struct Derive
{
    static constexpr auto
    value( ExprT const& )
    { return Constant< 0 >{}; }
};
//////////////////////
/// derive method ///
////////////////////
///
template< size_t Id, typename ExprT >
constexpr auto
derive( ExprT const& expr )
{ return Derive< Id, ExprT >::value( expr ); }

template< typename ExprT, variable X >
constexpr auto 
derive( ExprT const& expr, X const& x )
{ return derive< var_id_v< X >>( expr ); }

template< size_t Id, typename ExprT >
struct Derivative;

template< >
struct IsDiscriminatedOperation< Derivative >: true_type { };

//////////////////////////////
/// Derivative expression ///
////////////////////////////
///
template< size_t Id, typename ExprT >
struct Derivative: Compound< Derivative< Id, ExprT >>
{
    static constexpr auto
    value( ExprT const& expr )
    { return derive< Id >( expr ); }

    using Compound< Derivative< Id, ExprT >>::Compound;
};

///////////////////////////////
/// Derive specializations ///
/////////////////////////////
///
template< size_t Id, variable X >
requires( var_id_v< X > == Id and var_order_v< X > == 1 )
struct Derive< Id, X >
{
    static constexpr auto
    value( X const& )
    { return Constant< result_t< X >{ 1 }>{}; }
};

// derivative of a higher order variable is terminal
template< size_t Id, variable X >
requires( var_id_v< X > == Id and is_greater( var_order_v< X >, 1 ))
struct Derive< Id, X >
{
    static constexpr Derivative< Id, X >
    value( X const& expr )
    { return { expr }; }
};

template< size_t Id, expression ArgT >
requires( free_variables_t< ArgT >::contains_id( Id ))
struct Derive< Id, Negation< ArgT >>
{
    static constexpr auto
    value( Negation< ArgT > const& expr )
    {
        auto [ t ] = expr.args();
        auto dt = Derive< Id, ArgT >::value( t );
        return - dt; 
    }
};

template< size_t Id, typename... Ts >
requires( free_variables_t< Sum< Ts... >>::contains_id( Id ) )
struct Derive< Id, Sum< Ts... >>
{
    static constexpr auto
    value( Sum< Ts... > const& expr )
    { 
        static constexpr make_seq< sizeof...( Ts )> for_args;

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
        { return ( Derive< Id, pack_element_t< Is, Ts... >>::value( 
            get_argument< Is >( expr )) + ... ); };

        return helper( for_args );
    }
};

template< size_t Id, typename... Ts >
requires( free_variables_t< Difference< Ts... >>::contains_id( Id ) )
struct Derive< Id, Difference< Ts... > >
{
    static constexpr auto
    value( Difference< Ts... > const& expr )
    { 
        auto [ ...args ] = expr.args();
        return ( derive< Id >( args ) - ... );
    }
};

// product rule encoded in helper structs
template< size_t Id, typename... Ts >
requires( free_variables_t< Product< Ts... >>::contains_id( Id ) )
struct Derive< Id, Product< Ts... >> {
private:
    typedef make_seq< sizeof...( Ts )> for_args;

    // helper for the Kth element in the Jth term 
    template< size_t J, size_t K >
    struct Element
    {
        using type = pack_element_t< K, Ts... >;
        static constexpr type
        value( Product< Ts... > const& expr )
        { return get_argument< K >( expr ); }
    };

    // The Jth element of the Jth term is the derivative of the element
    template< size_t J, size_t K >
    requires( J == K )
    struct Element< J, K >
    {
        static constexpr auto
        value( Product< Ts... > const& expr )
        { return Derive< Id, pack_element_t< K, Ts... >>::
            value( get_argument< K >( expr )); }
    };
    
    // helper for the Jth term
    template< size_t J, typename Seq >
    struct Term;

    // The Jth term is the product of ...K elements
    template< size_t J, size_t... Ks >
    struct Term< J, seq< Ks... >>
    {
        static constexpr auto
        value( Product< Ts... > const& expr )
        { return ( Element< J, Ks >::value( expr ) * ... * 1 ); } 
    };

    // helper to assemble the final result
    template< typename Seq >
    struct Helper;

    // the result is the sum of the ...J terms
    template< size_t... Js >
    struct Helper< seq< Js... >>
    {
        static constexpr auto
        value( Product< Ts... > const& expr )
        { return ( Term< Js, for_args >::value( expr ) + ... + 0 ); }
    };

public:

    // the final result is the result of our helper divided by the unit of the
    // variable being referenced by this differential
    static constexpr auto
    value( Product< Ts... > const& expr )
    { return Helper< for_args >::value( expr ); }
};

template< size_t Id, typename T, typename... Ts >
requires( free_variables_t< Quotient< T, Ts... >>::contains_id( Id ) )
struct Derive< Id, Quotient< T, Ts... > >
{
    typedef make_seq< sizeof...( Ts )> for_rest;

    template< typename Seq >
    struct RestHelper;

    template< size_t... Is >
    struct RestHelper< seq< Is... >>
    {
        using type = std::remove_cvref_t< decltype( 
            ( pack_element_t< Is, Ts... >{} / ... / 1 ))>;

        static constexpr type
        value( Quotient< T, Ts... > const& expr )
        { return ( get_argument< 1 + Is >( expr ) / ... / 1 ); }
    };

    static constexpr auto
    value( Quotient< T, Ts... > const& expr )
    { 
        auto top = get_argument< 0 >( expr );
        auto bot = RestHelper< for_rest >::value( expr );

        auto dtop = Derive< Id, T >::value( top );
        auto dbot = Derive< Id, typename RestHelper< for_rest >::type >::
            value( bot );

        // quotient rule
        return ( dtop * bot - top * dbot ) / ( bot * bot );
    }
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derive< Id, SquareRoot< T > > 
{
    static constexpr auto
    value( SquareRoot< T > const& expr )
    {
        auto [ t ] = expr.args();
        auto dt = Derive< Id, T >::value( t );

        // power rule for sqrt
        return ( 0.5 / sqrt( t )) * dt;
    }
};

// power rule for exponents
template< size_t Id, typename T, typename U >
requires( free_variables_t< T >::contains_id( Id ) and
    not is_constant_v< U > and
    not free_variables_t< U >::contains_id( Id )) 
struct Derive< Id, Pow< T, U >>
{
    static constexpr auto
    value( Pow< T, U > const& expr )
    { 
        auto [ t, u ] = expr.args();
        auto dt = Derive< Id, T >::value( t );

        // power rule
        return u * pow( t, u - 1 ) * dt;
    }
};

// this specialization is required for compile-time units
template< size_t Id, typename T, auto N >
requires( free_variables_t< T >::contains_id( Id ))
struct Derive< Id, Pow< T, Constant< N >>>
{
    static constexpr auto
    value( Pow< T, Constant< N >> const& expr )
    { 
        auto [ t, n ] = expr.args();
        auto dt = Derive< Id, T >::value( t );

        // power rule
        return n * pow( t, Constant< N - 1 >{} ) * dt;
    }
};

template< size_t Id, integral auto N, typename T >
requires( free_variables_t< T >::contains_id( Id ) and N != 0 )
struct Derive< Id, PowN< N, T >>
{
    static constexpr auto
    value( PowN< N, T > const& expr )
    requires( N != 1 )
    {
        auto [ t ] = expr.args();
        auto dt = Derive< Id, T >::value( t );
        Constant< N > n;

        // power rule
        return n * pow< N - 1 >( t ) * dt;
    }

    static constexpr auto
    value( PowN< N, T > const& expr )
    requires( N == 1 )
    {
        auto [ t ] = expr.args();
        auto dt = Derive< Id, T >::value( t );

        // derivative of a linear expression ( t(x) )^1
        return dt;
    }
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derive< Id, Sine< T > >
{
    static constexpr auto
    value( Sine< T > const& expr )
    { 
        auto [ t ] = expr.args();
        auto dt = Derive< Id, T >::value( t );

        // dsin = cos
        return cos( t ) * dt;
    }
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derive< Id, Cosine< T > > 
{
    static constexpr auto
    value( Cosine< T > const& expr )
    { 
        auto [ t ] = expr.args();
        auto dt = Derive< Id, T >::value( t );

        // dcos = -sin
        return -sin( t ) * dt;
    }
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derive< Id, Tangent< T > > 
{
    static constexpr auto
    value( Tangent< T > const& expr )
    { 
        auto [ t ] = expr.args();
        auto dt = Derive< Id, T >::value( t );

        // dtan = sec^2
        return dt/ cos( t ) / cos( t );
    }
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derive< Id, Arcsine< T > > 
{
    static constexpr auto
    value( Arcsine< T > const& expr )
    { 
        auto [ t ] = expr.args();
        auto dt = Derive< Id, T >::value( t );
        auto one = Constant< 1 >{};

        // dasin(t) = dt/sqrt(1-t^2)
        return dt/ sqrt( one - t * t );
    }
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derive< Id, Arccosine< T > > 
{
    static constexpr auto
    value( Arccosine< T > const& expr )
    { 
        auto [ t ] = expr.args();
        auto dt = Derive< Id, T >::value( t );
        auto one = Constant< 1 >{};

        // dacos(t) = -dt/sqrt(1-t^2)
        return -dt/ sqrt( one - t * t );
    }
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derive< Id, Arctangent< T > > 
{
    static constexpr auto
    value( Arctangent< T > const& expr )
    { 
        auto [ t ] = expr.args();
        auto dt = Derive< Id, T >::value( t );
        auto one = Constant< 1 >{};

        // datan = dt/(1 + t^2)
        return dt/ ( one + t * t );
    }
};

template< size_t Id, typename T, typename U >
requires( free_variables_t< T >::contains_id( Id ) or
    free_variables_t< U >::contains_id( Id ) )
struct Derive< Id, Arctangent2< T, U > > 
{
    static constexpr auto
    value( Arctangent2< T, U > const& expr )
    { 
        auto [ num, den ] = expr.args();

        return Derive< Id, Arctangent< Quotient< T, U >>>::value(
            {{ num, den }});
    }
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derive< Id, Log< T > >
{
    static constexpr auto
    value( Log< T > const& expr )
    { 
        auto [ t ] = expr.args();
        auto dt = Derive< Id, T >::value( t );

        // dlog(t) = dt/abs(t)
        return dt/ abs( t );
    }
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derive< Id, Exp< T >>
{
    static constexpr auto
    value( Exp< T > const& expr )
    {
        auto [ t ] = expr.args();
        auto dt = Derive< Id, T >::value( t );

        return exp( t ) * dt;
    }
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derivative< Id, Abs< T > >: Compound< Derivative< Id, Abs< T >>>
{ static_assert( false, "piece-wise functions are not implemented yet" ); };

template< size_t Id, typename... Ts >
requires( free_variables_t< tuple< Ts... >>::contains_id( Id ) )
struct Derive< Id, tuple< Ts... >>
{
    static constexpr auto
    value( tuple< Ts... > const& tup )
    { 
        static constexpr make_seq< sizeof...( Ts )> for_elements;

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
        { return make_tuple( Derive< Id, pack_element_t< Is, Ts... > >::
            value( get< Is >( tup ))... ); };

        return helper( for_elements );
    }
};

template< size_t Id, shape S, typename... Ts >
requires( free_variables_t< Tensor< S, Ts... >>::contains_id( Id ) )
struct Derive< Id, Tensor< S, Ts... > >
{
    static constexpr auto
    value( Tensor< S, Ts... > const& ten )
    {
        static constexpr make_seq< sizeof...( Ts )> for_elements;

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
        { return make_tensor< S >( Derivative< Id, pack_element_t< Is, Ts... > >::
            value( tensor_get< Is >( ten ))... ); };

        return helper( for_elements );
    }
};

// first order functional derivative is a derivative of the formula
template< size_t Id, typename ExprT, variable... Vars >
requires( free_variables_t< Func< ExprT, Vars... >>::contains_id( Id )) 
struct Derive< Id, Func< ExprT, Vars... >>
{
    static constexpr auto
    value( Func< ExprT, Vars... > const& func )
    {
        static constexpr make_seq< sizeof...( Vars )> for_vars;
        auto df = Derive< Id, ExprT >::value( func.formula() );

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr
        { return make_function( df, func.template var< Is >()... ); };

        return helper( for_vars );
    }
};
//
//template< typename ExprT, typename VarU >
//using derivative_t = Func< typename Derivative< var_id_v< VarU >, ExprT >::type,
//    VarU >;
//
//template< typename ExprT, typename VarU >
//constexpr derivative_t< ExprT, VarU >
//derivative( ExprT const& expr, VarU const& var )
//{ return { Derivative< var_id_v< VarU >, ExprT >::value( expr ), var }; }

template< typename FuncT >
struct Gradient;

template< >
struct IsCompoundOperation< Gradient >: true_type { };

template< typename FormulaT, variable... Vars >
auto grad( Func< FormulaT, Vars... > const& func )
{
    typedef Shape< sizeof...( Vars )> S;
    static constexpr make_seq< sizeof...( Vars )> for_vars;

    auto f = func.formula();

    auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr
    { return make_tensor< S >( derive( f, func.template var< Is >())... ); };

    return helper( for_vars );
}

template< typename T >
struct Gradient: Compound< Gradient< T >> 
{
    template< typename FreeVars, typename Seq = make_seq< FreeVars::size >>
    struct FuncHelper;

    template< variable... Vars, size_t... Is >
    struct FuncHelper< unique_variables< Vars... >, seq< Is... >>
    {
        using type = Func< T, pack_element_t< Is, Vars... >... >;
        static constexpr type
        value( T const& t, unique_variables< Vars... > const& vars )
        { return { t, vars.template at< Is >()... }; }
    };

    static constexpr typename FuncHelper< free_variables_t< T >>::type
    function( T const& t )
    { return FuncHelper< free_variables_t< T >>::value( t ); }

    static constexpr auto
    value( T const& t )
    { return grad( function( t )); }

    using Compound< Gradient< T >>::Compound;
};

// specialization of the argument is already a function
template< typename ExprT, variable... Vars >
struct Gradient< Func< ExprT, Vars... >>: 
    Compound< Gradient< Func< ExprT, Vars... >>> 
{
    static constexpr auto
    value( Func< ExprT, Vars... > const& func )
    { return grad( func ); }

    using Compound< Gradient< Func< ExprT, Vars... >>>::Compound;
};
//
//template< typename FuncT >
//using gradient_t = Gradient< FuncT >::type;
//
//template< typename FuncT >
//constexpr gradient_t< FuncT >
//gradient( FuncT const& func )
//{ return Gradient< FuncT >::value( func ); }


///////////////////////////////////
/// Experiment: Infinitesimals ///
/////////////////////////////////
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




} // namespace expressions

#endif

