/// work in progress ///

#ifndef __EXPRESSIONS_CALCULUS_HPP__
#define __EXPRESSIONS_CALCULUS_HPP__

#include "expressions/expressions.hpp"
#include "expressions/arithmetic.hpp"

#include <type_traits>

using std::true_type, std::false_type;

namespace expressions {

template< typename ExprT, typename VarU >
struct Derivative;

template< typename ExprT, variable Var >
requires( not expression< ExprT > )
struct Derivative< ExprT, Var >
{
    using type = Func< Constant< 0 >, Var >;
    static constexpr type
    value( ExprT const&, Var const& )
    { return {}; }
};

template< typename ExprT, variable Var >
requires( not free_variables_t< ExprT >::template contains< Var >() )
struct Derivative< ExprT, Var >
{
    using type = Func< Constant< 0 >, Var >;
    static constexpr type
    value( ExprT const&, Var const& )
    { return {}; }
};

template< variable ExprV, variable Var >
requires( free_variables_t< ExprV >::template contains< Var >() )
struct Derivative< ExprV, Var >
{
    using type = Constant< ( var_id_v< ExprV > == var_id_v< Var > ? 1 : 0 )>;
    static constexpr type
    value( ExprV const&, Var const& )
    { return {}; }
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Negation< T >, Var >
{
    using type = Negation< typename Derivative< T, Var >::type >;
    static constexpr type
    value( Negation< T > const& expr, Var const& var )
    { return { Derivative< T, Var >::value( expr.template arg< 0 >() ) }; }
};

template< typename... Ts, variable Var >
requires( free_variables_t< Sum< Ts... >>::template contains< Var >() )
struct Derivative< Sum< Ts... >, Var >
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
};

template< typename... Ts, variable Var >
requires( free_variables_t< Difference< Ts... >>::template contains< Var >() )
struct Derivative< Difference< Ts... >, Var >
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
};

template< typename... Ts, variable Var >
requires( free_variables_t< Product< Ts... >>::template contains< Var >() )
struct Derivative< Product< Ts... >, Var >
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
};

template< typename T, typename... Ts, variable Var >
requires( free_variables_t< Quotient< T, Ts... >>::template contains< Var >() )
struct Derivative< Quotient< T, Ts... >, Var >
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
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< SquareRoot< T >, Var >
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
};

template< typename T, auto N, variable Var >
requires( free_variables_t< T >::template contains< Var >() and 
    std::is_arithmetic_v< decltype( N )> )
struct Derivative< Pow< T, Constant< N >>, Var >
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
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Sine< T >, Var >
{
    using type = Product< Cosine< T >, typename Derivative< T, Var >::type >;

    static constexpr type
    value( Sine< T > const& expr, Var const& var )
    { 
        auto [ arg ] = expr.args();

        return {{ arg }, Derivative< T, Var >::value( arg, var )};
    }
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Cosine< T >, Var >
{
    using type = Product< Negation< Sine< T >>, 
          typename Derivative< T, Var >::type >;

    static constexpr type
    value( Cosine< T > const& expr, Var const& var )
    { 
        auto [ arg ] = expr.args();

        return {{{ arg }}, Derivative< T, Var >::value( arg, var )};
    }
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Tangent< T >, Var >
{
    using type = Quotient< typename Derivative< T, Var >::type, 
        Pow< Cosine< T >, Constant< 2 >>>;

    static constexpr type
    value( Tangent< T > const& expr, Var const& var )
    { 
        auto [ arg ] = expr.args();

        return { Derivative< T, Var >::value( arg, var ), {{ arg }, {}}};
    }
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Arcsine< T >, Var >
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
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Arccosine< T >, Var >
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
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Arctangent< T >, Var >
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
};

template< typename T, typename U, variable Var >
requires( free_variables_t< T >::template contains< Var >() or
    free_variables_t< U >::template contains< Var >() )
struct Derivative< Arctangent2< T, U >, Var >
{
    using type = Derivative< Arctangent< Quotient< T, U >>, Var >::type;

    static constexpr type
    value( Arctangent2< T, U > const& expr, Var const& var )
    { 
        auto [ num, den ] = expr.args();

        return Derivative< Arctangent< Quotient< T, U >>, Var >::
            value({{ num, den }}, var ); 
    }
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Log< T >, Var >
{
    using type = Quotient< typename Derivative< T, Var >::type, Abs< T >>;

    static constexpr type
    value( Log< T > const& expr, Var const& var )
    { 
        auto [ arg ] = expr.args();

        return { Derivative< T, Var >::value( arg, var ), { arg }};
    }
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Exp< T >, Var >
{
    using type = Product< Exp< T >, typename Derivative< T, Var >::type >;

    static constexpr type
    value( Exp< T > const& expr, Var const& var )
    {
        auto [ arg ] = expr.args();

        return {{ arg }, Derivative< T, Var >::value( arg, var )};
    }
};

template< typename T, variable Var >
requires( free_variables_t< T >::template contains< Var >() )
struct Derivative< Abs< T >, Var >
{ static_assert( false, "piece-wise functions are not implemented yet" ); };

template< typename... Ts, variable Var >
requires( free_variables_t< tuple< Ts... >>::template contains< Var >() )
struct Derivative< tuple< Ts... >, Var >
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
};

template< shape S, typename... Ts, variable Var >
requires( free_variables_t< Tensor< S, Ts... >>::template contains< Var >() )
struct Derivative< Tensor< S, Ts... >, Var >
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
};

template< typename ExprT, typename VarU >
using derivative_t = Derivative< ExprT, VarU >::type;

template< typename ExprT, typename VarU >
constexpr derivative_t< ExprT, VarU >
derivative( ExprT const& expr, VarU const& var )
{ return Derivative< ExprT, VarU >::value( expr, var ); }

template< typename ExprT, typename VarU >
struct Derive;

template< >
struct IsExpressionOperation< Derive >: true_type { };

template< typename ExprT, variable Var >
struct Derive< ExprT, Var >: Arguments< Derive, ExprT, Var >
{
    static constexpr derivative_t< ExprT, Var >
    value( ExprT const& expr, Var const& var )
    { return derivative( expr, var ); }

    using Arguments< Derive, ExprT, Var >::Arguments;
};


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

