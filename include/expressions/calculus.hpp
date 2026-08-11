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

template< typename ExprT, typename VarU >
struct Derivative;

template< >
struct IsExpressionOperation< Derivative >: true_type { };

template< typename ExprT, typename X >
requires( not expression< ExprT > )
struct Derivative< ExprT, X >: Arguments< Derivative, ExprT, X >
{
    using type = Func< Constant< 0 >, X >;
    static constexpr type
    value( ExprT const&, X const& )
    { return {}; }

    using Arguments< Derivative, ExprT, X >::Arguments;
};

template< typename ExprT, variable Var >
requires( not free_variables_t< ExprT >::template contains< Var >() )
struct Derivative< ExprT, Var >: Arguments< Derivative, ExprT, Var >
{
    using type = Func< Constant< 0 >, Var >;
    static constexpr type
    value( ExprT const&, Var const& )
    { return {}; }

    using Arguments< Derivative, ExprT, Var >::Arguments;
};

template< variable ExprV, variable Var >
requires( free_variables_t< ExprV >::template contains< Var >() )
struct Derivative< ExprV, Var >: Arguments< Derivative, ExprV, Var >
{
    using type = Constant< ( var_id_v< ExprV > == var_id_v< Var > ? 1 : 0 )>;
    static constexpr type
    value( ExprV const&, Var const& )
    { return {}; }

    using Arguments< Derivative, ExprV, Var >::Arguments;
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Negation< T >, Var >: 
    Arguments< Derivative, Negation< T >, Var >
{
    using type = Negation< typename Derivative< T, Var >::type >;
    static constexpr type
    value( Negation< T > const& expr, Var const& var )
    { return { Derivative< T, Var >::value( expr.template arg< 0 >() ) }; }

    using Arguments< Derivative, Negation< T >, Var >::Arguments;
};

template< typename... Ts, variable Var >
requires( free_variables_t< Sum< Ts... >>::template contains< Var >() )
struct Derivative< Sum< Ts... >, Var >: 
    Arguments< Derivative, Sum< Ts... >, Var >
{
    using type = Sum< typename Derivative< Ts, Var >::type... >;
    static constexpr type
    value( Sum< Ts... > const& expr, Var const& )
    {
        static constexpr make_seq< sizeof...( Ts )> for_args;

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
        { return { Derivative< Ts...[ Is ], Var >::value( expr.template 
            arg< Is >() )... }; };

        return helper( for_args );
    }

    using Arguments< Derivative, Sum< Ts... >, Var >::Arguments;
};

template< typename... Ts, variable Var >
requires( free_variables_t< Difference< Ts... >>::template contains< Var >() )
struct Derivative< Difference< Ts... >, Var >: 
    Arguments< Derivative, Difference< Ts... >, Var >
{
    using type = Difference< typename Derivative< Ts, Var >::type... >;
    static constexpr type
    value( Difference< Ts... > const& expr, Var const& )
    {
        static constexpr make_seq< sizeof...( Ts )> for_args;

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
        { return { Derivative< Ts...[ Is ], Var >::value( expr.template 
            arg< Is >() )... }; };

        return helper( for_args );
    }

    using Arguments< Derivative, Difference< Ts... >, Var >::Arguments;
};

template< typename... Ts, variable Var >
requires( free_variables_t< Product< Ts... >>::template contains< Var >() )
struct Derivative< Product< Ts... >, Var >: 
    Arguments< Derivative, Product< Ts... >, Var >
{
    typedef make_seq< sizeof...( Ts )> for_args;

    template< size_t J, size_t K >
    struct Element
    {
        using type = Ts...[ K ];
        static constexpr type
        value( Product< Ts... > const& expr, Var const& )
        { return expr.template arg< K >(); }
    };

    template< size_t J, size_t K >
    requires( J == K )
    struct Element< J, K >
    {
        using type = Derivative< Ts...[ K ], Var >::type;
        static constexpr type
        value( Product< Ts... > const& expr, Var const& var )
        { return { expr.template arg< K >(), var }; }
    };
    
    template< size_t J, typename Seq >
    struct Term;

    template< size_t J, size_t... Is >
    struct Term< J, seq< Is... >>
    {
        using type = Product< typename Element< J, Is >::type... >;
        static constexpr type
        value( Product< Ts... > const& expr, Var const& var )
        { return { Element< J, Is >::value( expr, var )... }; } 
    };

    template< typename Seq >
    struct Helper;

    template< size_t... Is >
    struct Helper< seq< Is... >>
    {
        using type = Sum< typename Term< Is, for_args >::type... >;
        static constexpr type
        value( Product< Ts... > const& expr, Var const& var )
        { return { Term< Is, for_args >::value( expr, var )... }; }
    };

    using type = Helper< for_args >::type;
    static constexpr type
    value( Product< Ts... > const& expr, Var const& var )
    { Helper< for_args >::value( expr, var ); }

    using Arguments< Derivative, Product< Ts... >, Var >::Arguments;
};

template< typename T, typename... Ts, variable Var >
requires( free_variables_t< Quotient< T, Ts... >>::template contains< Var >() )
struct Derivative< Quotient< T, Ts... >, Var >: 
    Arguments< Derivative, Quotient< T, Ts... >, Var >
{
    using type = Quotient< 
        Difference< 
            Product< typename Derivative< T, Var >::type, Quotient< Ts... >>,
            Product< T, typename Derivative< Quotient< Ts... >, Var >::type >>,
        Pow< Quotient< Ts... >, Constant< 2 >>>;

    static constexpr type
    value( Quotient< T, Ts... > const& expr, Var const& var )
    { 
        static constexpr make_seq< sizeof...( Ts )> for_rest;
        auto first = expr.template arg< 0 >();
        auto rest = [&]< size_t... Is >( seq< Is... > ) constexpr ->
            Quotient< Ts... >
        { return { expr.template arg< 1 + Is >()... }; }( for_rest );

        return {{{ Derivative< T, Var >::value( first, var ), rest },
            { first, Derivative< Quotient< Ts... >, Var >::
                value( rest, var ) }}, { rest, Constant< 2 >{} }};
    }

    using Arguments< Derivative, Quotient< T, Ts... >, Var >::Arguments;
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< SquareRoot< T >, Var >: 
    Arguments< Derivative, SquareRoot< T >, Var >
{
    using type = Product< Constant< -0.5 >, Quotient< 
        typename Derivative< T, Var >::type, SquareRoot< T >>>;

    static constexpr type
    value( SquareRoot< T > const& expr, Var const& var )
    {
        auto [ arg ] = expr.args();

        return { Constant< -0.5 >{}, { Derivative< T, Var >::value( arg, var ), 
            { expr.template arg< 0 >() }}}; 
    }

    using Arguments< Derivative, SquareRoot< T >, Var >::Arguments;
};

template< typename T, auto N, variable Var >
requires( free_variables_t< T >::template contains< Var >() and 
    std::is_arithmetic_v< decltype( N )> )
struct Derivative< Pow< T, Constant< N >>, Var >: 
    Arguments< Derivative, Pow< T, Constant< N >>, Var >
{
    using type = Product< Constant< N >, Pow< T, Constant< N - 1 >>,
        typename Derivative< T, Var >::type >;

    static constexpr type
    value( Pow< T, Constant< N >> const& expr, Var const& var )
    { 
        auto [ arg ] = expr.args();

        return { Constant< N >{}, { arg, Constant< N - 1 >{} }, 
            Derivative< T, Var >::value( arg, var ) };
    }

    using Arguments< Derivative, Pow< T, Constant< N >>, Var >::Arguments;
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Sine< T >, Var >: Arguments< Derivative, Sine< T >, Var >
{
    using type = Product< Cosine< T >, typename Derivative< T, Var >::type >;

    static constexpr type
    value( Sine< T > const& expr, Var const& var )
    { 
        auto [ arg ] = expr.args();

        return {{ arg }, Derivative< T, Var >::value( arg, var )};
    }

    using Arguments< Derivative, Sine< T >, Var >::Arguments;
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Cosine< T >, Var >: 
    Arguments< Derivative, Cosine< T >, Var >
{
    using type = Product< Negation< Sine< T >>, 
          typename Derivative< T, Var >::type >;

    static constexpr type
    value( Cosine< T > const& expr, Var const& var )
    { 
        auto [ arg ] = expr.args();

        return {{{ arg }}, Derivative< T, Var >::value( arg, var )};
    }

    using Arguments< Derivative, Cosine< T >, Var >::Arguments;
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Tangent< T >, Var >: 
    Arguments< Derivative, Tangent< T >, Var >
{
    using type = Quotient< typename Derivative< T, Var >::type, 
        Pow< Cosine< T >, Constant< 2 >>>;

    static constexpr type
    value( Tangent< T > const& expr, Var const& var )
    { 
        auto [ arg ] = expr.args();

        return { Derivative< T, Var >::value( arg, var ), {{ arg }, {}}};
    }

    using Arguments< Derivative, Tangent< T >, Var >::Arguments;
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Arcsine< T >, Var >: 
    Arguments< Derivative, Arcsine< T >, Var >
{
    using type = Quotient< typename Derivative< T, Var >::type,
        SquareRoot< Difference< Constant< 1 >, Pow< T, Constant< 2 >>>>>;

    static constexpr type
    value( Arcsine< T > const& expr, Var const& var )
    { 
        auto [ arg ] = expr.args();

        return { Derivative< T, Var >::value( arg, var ),
            {{ {}, { arg, {} }}}};
    }

    using Arguments< Derivative, Arcsine< T >, Var >::Arguments;
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Arccosine< T >, Var >: 
    Arguments< Derivative, Arccosine< T >, Var >
{
    using type = Negation< Quotient< typename Derivative< T, Var >::type,
        SquareRoot< Difference< Constant< 1 >, Pow< T, Constant< 2 >>>>>>;

    static constexpr type
    value( Arccosine< T > const& expr, Var const& var )
    { 
        auto [ arg ] = expr.args();
        
        return {{ Derivative< T, Var >::value( arg, var ),
            {{ {}, { arg, {} }}}}};
    }

    using Arguments< Derivative, Arccosine< T >, Var >::Arguments;
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Arctangent< T >, Var >: 
    Arguments< Derivative, Arctangent< T >, Var >
{
    using type = Quotient< typename Derivative< T, Var >::type,
        SquareRoot< Sum< Constant< 1 >, Pow< T, Constant< 2 >>>>>;

    static constexpr type
    value( Arctangent< T > const& expr, Var const& var )
    { 
        auto [ arg ] = expr.args();
        
        return { Derivative< T, Var >::value( expr, var ),
            {{ {}, { arg, {} }}}};
    }

    using Arguments< Derivative, Arctangent< T >, Var >::Arguments;
};

template< typename T, typename U, variable Var >
requires( free_variables_t< T >::template contains< Var >() or
    free_variables_t< U >::template contains< Var >() )
struct Derivative< Arctangent2< T, U >, Var >: 
    Arguments< Derivative, Arctangent2< T, U >, Var >
{
    using type = Derivative< Arctangent< Quotient< T, U >>, Var >::type;

    static constexpr type
    value( Arctangent2< T, U > const& expr, Var const& var )
    { 
        auto [ num, den ] = expr.args();

        return Derivative< Arctangent< Quotient< T, U >>, Var >::
            value({{ num, den }}, var ); 
    }

    using Arguments< Derivative, Arctangent2< T, U >, Var >::Arguments;
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Log< T >, Var >: Arguments< Derivative, Log< T >, Var >
{
    using type = Quotient< typename Derivative< T, Var >::type, Abs< T >>;

    static constexpr type
    value( Log< T > const& expr, Var const& var )
    { 
        auto [ arg ] = expr.args();

        return { Derivative< T, Var >::value( arg, var ), { arg }};
    }

    using Arguments< Derivative, Log< T >, Var >::Arguments;
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Exp< T >, Var >: Arguments< Derivative, Exp< T >, Var >
{
    using type = Product< Exp< T >, typename Derivative< T, Var >::type >;

    static constexpr type
    value( Exp< T > const& expr, Var const& var )
    {
        auto [ arg ] = expr.args();

        return {{ arg }, Derivative< T, Var >::value( arg, var )};
    }

    using Arguments< Derivative, Exp< T >, Var >::Arguments;
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Abs< T >, Var >: Arguments< Derivative, Abs< T >, Var >
{ static_assert( false, "piece-wise functions are not implemented yet" ); };

template< typename... Ts, variable Var >
requires( free_variables_t< tuple< Ts... >>::template contains< Var >() )
struct Derivative< tuple< Ts... >, Var >: 
    Arguments< Derivative, tuple< Ts... >, Var >
{
    using type = tuple< typename Derivative< Ts, Var >::type... >;

    static constexpr type
    value( tuple< Ts... > const& tup, Var const& var )
    { 
        static constexpr make_seq< sizeof...( Ts )> for_elements;

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
        { return { Derivative< Ts...[ Is ], Var >::
            value( get< Is >( tup ), var )... }; };

        return helper( for_elements );
    }

    using Arguments< Derivative, tuple< Ts... >, Var >::Arguments;
};

template< shape S, typename... Ts, variable Var >
requires( free_variables_t< Tensor< S, Ts... >>::template contains< Var >() )
struct Derivative< Tensor< S, Ts... >, Var >: 
    Arguments< Derivative, Tensor< S, Ts... >, Var >
{
    using type = Tensor< S, typename Derivative< Ts, Var >::type... >;

    static constexpr type
    value( Tensor< S, Ts... > const& ten, Var const& var )
    {
        static constexpr make_seq< sizeof...( Ts )> for_elements;

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
        { return { Derivative< Ts...[ Is ], Var >::
            value( tensor_get< Is >( ten ), var )... }; };

        return helper( for_elements );
    }

    using Arguments< Derivative, Tensor< S, Ts... >, Var >::Arguments;
};

template< typename ExprT, typename VarU >
using derivative_t = Derivative< ExprT, VarU >::type;

template< typename ExprT, typename VarU >
constexpr derivative_t< ExprT, VarU >
derivative( ExprT const& expr, VarU const& var )
{ return Derivative< ExprT, VarU >::value( expr, var ); }

template< typename FuncT >
struct Gradient;

template< >
struct IsExpressionOperation< Gradient >: true_type { };

template< typename FuncT >
struct Gradient: Arguments< Gradient, FuncT > {
private:
    template< typename VarsSet >
    struct Helper;

    template< variable... Vars >
    struct Helper< unique_variables< Vars... >>
    {
        using type = Tensor< Shape< sizeof...( Vars )>, 
            derivative_t< FuncT, Vars >... >;
        static constexpr type
        value( FuncT const& func )
        { return { derivative( func, Vars{} )... }; }
    };

public:
    using type = Helper< free_variables_t< FuncT >>::type;

    static constexpr type
    value( FuncT const& func )
    { return Helper< free_variables_t< FuncT >>::value( func ); }

    using Arguments< Gradient, FuncT >::Arguments;
};

template< typename ExprT, variable... Vars >
struct Gradient< Func< ExprT, Vars... >>: 
    Arguments< Gradient, Func< ExprT, Vars... >> 
{
    using type = Tensor< Shape< sizeof...( Vars )>, 
        derivative_t< Func< ExprT, Vars... >, Vars >... >;

    static constexpr type
    value( Func< ExprT, Vars... > const& func )
    { return { derivative( func, Vars{} )... }; }

    using Arguments< Gradient, Func< ExprT, Vars... >>::Arguments;
};

template< typename FuncT >
using gradient_t = Gradient< FuncT >::type;

template< typename FuncT >
constexpr gradient_t< FuncT >
gradient( FuncT const& func )
{ return Gradient< FuncT >::value( func ); }


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

