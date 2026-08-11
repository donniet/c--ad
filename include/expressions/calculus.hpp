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

template< size_t Id, typename ExprT >
struct Derivative;

template< >
struct IsDiscriminatedOperation< Derivative >: true_type { };

template< size_t Id, typename ExprT >
requires( not expression< ExprT > )
struct Derivative< Id, ExprT >: Compound< Derivative< Id, ExprT >>
{
    using variable_type = Var< Id, ExprT >;

    using type = Func< Constant< 0 >, variable_type >; // scalar
    static constexpr type
    value( ExprT const& )
    { return {}; }

    using Compound< Derivative< Id, ExprT >>::Compound;
};

template< size_t Id, typename ExprT >
requires( expression< ExprT > and 
    not free_variables_t< ExprT >::contains_id( Id ) )
struct Derivative< Id, ExprT >: Compound< Derivative< Id, ExprT>>
{
    using variable_type = Var< Id, result_t< ExprT >>;
    using type = Func< Constant< 0 >, variable_type >; // scalar
        
    static constexpr type
    value( ExprT const& )
    { return {}; }

    using Compound< Derivative< Id, ExprT>>::Compound;
};

template< size_t Id, variable ExprV >
requires( free_variables_t< ExprV >::contains_id( Id ) and
    var_id_v< ExprV > == Id )
struct Derivative< Id, ExprV >: Compound< Derivative< Id, ExprV >>
{
    using type = Constant< 1 >; // scalar
    static constexpr type
    value( ExprV const& )
    { return {}; }

    using Compound< Derivative< Id, ExprV >>::Compound;
};

template< size_t Id, variable ExprV >
requires( free_variables_t< ExprV >::contains_id( Id ) and
    var_id_v< ExprV > != Id )
struct Derivative< Id, ExprV >: Compound< Derivative< Id, ExprV >>
{ static_assert( false, "second-order variable derivative not implemented" ); };

template< size_t Id, expression T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derivative< Id, Negation< T > >: 
    Compound< Derivative< Id, Negation< T >>>
{
    using variable_type = free_variables_t< T >::template variable_t< Id >;
    using type = Quotient< Negation< typename Derivative< Id, T >::type >,
        Constant< result_t< variable_type >{ 1 }>>;
    
    static constexpr type
    value( Negation< T > const& expr )
    { return {{ Derivative< Id, T >::value( expr.template arg< 0 >() ) }, 
        {}}; }

    using Compound< Derivative< Id, Negation< T >>>::Compound;
};

template< size_t Id, typename... Ts >
requires( free_variables_t< Sum< Ts... >>::contains_id( Id ) )
struct Derivative< Id, Sum< Ts... > >: 
    Compound< Derivative< Id, Sum< Ts... >>>
{
    using variable_type = free_variables_t< Sum< Ts... >>::template 
        variable_t< Id >;
    using type = Quotient< Sum< typename Derivative< Id, Ts >::type... >,
        Constant< result_t< variable_type >{ 1 }>>;

    static constexpr type
    value( Sum< Ts... > const& expr )
    {
        static constexpr make_seq< sizeof...( Ts )> for_args;

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
        { return {{ Derivative< Id, Ts...[ Is ] >::value( expr.template 
            arg< Is >() )... }, {}}; };

        return helper( for_args );
    }

    using Compound< Derivative< Id, Sum< Ts... >>>::Compound;
};

template< size_t Id, typename... Ts >
requires( free_variables_t< Difference< Ts... >>::contains_id( Id ) )
struct Derivative< Id, Difference< Ts... > >: 
    Compound< Derivative< Id, Difference< Ts... >>>
{
    using variable_type = free_variables_t< Difference< Ts... >>::template 
        variable_t< Id >;
    using type = Quotient< Difference< typename Derivative< Id, Ts >::type... >,
        Constant< result_t< variable_type >{ 1 }>>;

    static constexpr type
    value( Difference< Ts... > const& expr )
    {
        static constexpr make_seq< sizeof...( Ts )> for_args;

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
        { return {{ Derivative< Id, Ts...[ Is ] >::value( expr.template 
            arg< Is >() )... }, {}}; };

        return helper( for_args );
    }

    using Compound< Derivative< Id, Difference< Ts... >>>::Compound;
};

template< size_t Id, typename... Ts >
requires( free_variables_t< Product< Ts... >>::contains_id( Id ) )
struct Derivative< Id, Product< Ts... > >: 
    Compound< Derivative< Id, Product< Ts... >>>
{
    typedef make_seq< sizeof...( Ts )> for_args;

    template< size_t J, size_t K >
    struct Element
    {
        using type = Ts...[ K ];
        static constexpr type
        value( Product< Ts... > const& expr )
        { return expr.template arg< K >(); }
    };

    template< size_t J, size_t K >
    requires( J == K )
    struct Element< J, K >
    {
        using type = Derivative< Id, Ts...[ K ] >::type;
        static constexpr type
        value( Product< Ts... > const& expr )
        { return { expr.template arg< K >(), var }; }
    };
    
    template< size_t J, typename Seq >
    struct Term;

    template< size_t J, size_t... Is >
    struct Term< J, seq< Is... >>
    {
        using type = Product< typename Element< J, Is >::type... >;
        static constexpr type
        value( Product< Ts... > const& expr )
        { return { Element< J, Is >::value( expr, var )... }; } 
    };

    template< typename Seq >
    struct Helper;

    template< size_t... Is >
    struct Helper< seq< Is... >>
    {
        using type = Sum< typename Term< Is, for_args >::type... >;
        static constexpr type
        value( Product< Ts... > const& expr )
        { return { Term< Is, for_args >::value( expr, var )... }; }
    };

    using variable_type = free_variables_t< Product< Ts... >>::template 
        variable_t< Id >;
    using type = Quotient< typename Helper< for_args >::type,
        Constant< result_t< variable_type >{ 1 }>>;

    static constexpr type
    value( Product< Ts... > const& expr )
    { return { Helper< for_args >::value( expr, var ), {}}; }

    using Compound< Derivative< Id, Product< Ts... >>>::Compound;
};

template< size_t Id, typename T, typename... Ts >
requires( free_variables_t< Quotient< T, Ts... >>::contains_id( Id ) )
struct Derivative< Id, Quotient< T, Ts... > >: 
    Compound< Derivative< Id, Quotient< T, Ts... >>>
{
    using variable_type = free_variables_t< Quotient< T, Ts...>>::template 
        variable_t< Id >;
    using type = Quotient< Quotient< 
        Difference< 
            Product< typename Derivative< Id, T >::type, Quotient< Ts... >>,
            Product< T, typename Derivative< Id, Quotient< Ts... > >::type >>,
        Pow< Quotient< Ts... >, Constant< 2 >>>,
        Constant< result_t< variable_type >{ 1 }>>;

    static constexpr type
    value( Quotient< T, Ts... > const& expr )
    { 
        static constexpr make_seq< sizeof...( Ts )> for_rest;
        auto first = expr.template arg< 0 >();
        auto rest = [&]< size_t... Is >( seq< Is... > ) constexpr ->
            Quotient< Ts... >
        { return { expr.template arg< 1 + Is >()... }; }( for_rest );

        return {{{ Derivative< Id, T >::value( first, var ), rest },
            { first, Derivative< Id, Quotient< Ts... > >::
                value( rest, var ) }}, { rest, Constant< 2 >{} }};
    }

    using Compound< Derivative< Id, Quotient< T, Ts... >>>::Compound;
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derivative< Id, SquareRoot< T > >: 
    Compound< Derivative< Id, SquareRoot< T >>>
{
    using variable_type = free_variables_t< T >::template variable_t< Id >;
    using type = Quotient< Product< Constant< -0.5 >, Quotient< 
        typename Derivative< Id, T >::type, SquareRoot< T >>>,
            Constant< result_t< variable_type >{ 1 }>>;

    static constexpr type
    value( SquareRoot< T > const& expr )
    {
        auto [ arg ] = expr.args();

        return {{ Constant< -0.5 >{}, { Derivative< Id, T >::value( arg, var ), 
            { expr.template arg< 0 >() }}, {}}}; 
    }

    using Compound< Derivative< Id, SquareRoot< T >>>::Compound;
};

template< size_t Id, typename T, auto N >
requires( free_variables_t< T >::contains_id( Id ) and 
    std::is_arithmetic_v< decltype( N )> )
struct Derivative< Id, Pow< T, Constant< N >> >: 
    Compound< Derivative< Id, Pow< T, Constant< N >>>>
{
    using variable_type = free_variables_t< T >::template variable_t< Id >;
    using type = Quotient< Product< Constant< N >, Pow< T, Constant< N - 1 >>,
        typename Derivative< Id, T >::type >, 
            Constant< result_t< variable_type >{ 1 }>>;

    static constexpr type
    value( Pow< T, Constant< N >> const& expr )
    { 
        auto [ arg ] = expr.args();

        return { Constant< N >{}, { arg, Constant< N - 1 >{} }, 
            Derivative< Id, T >::value( arg, var ) };
    }

    using Compound< Derivative< Id, Pow< T, Constant< N >>>>::Compound;
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derivative< Id, Sine< T > >: Compound< Derivative< Id, Sine< T >>>
{
    using variable_type = free_variables_t< T >::template variable_t< Id >;
    using type = Quotient< Product< Cosine< T >, 
        typename Derivative< Id, T >::type >, 
            Constant< result_t< variable_type >{ 1 }>>;

    static constexpr type
    value( Sine< T > const& expr )
    { 
        auto [ arg ] = expr.args();

        return {{{ arg }, Derivative< Id, T >::value( arg )}, {}};
    }

    using Compound< Derivative< Id, Sine< T >>>::Compound;
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derivative< Id, Cosine< T > >: 
    Compound< Derivative< Id, Cosine< T >>>
{
    using variable_type = free_variables_t< T >::template variable_t< Id >;
    using type = Quotient< Product< Negation< Sine< T >>, 
        typename Derivative< Id, T >::type >, 
            Constant< result_t< variable_type >{ 1 }>>;

    static constexpr type
    value( Cosine< T > const& expr )
    { 
        auto [ arg ] = expr.args();

        return {{{{ arg }}, Derivative< Id, T >::value( arg )}, {}};
    }

    using Compound< Derivative< Id, Cosine< T >>>::Compound;
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derivative< Id, Tangent< T > >: 
    Compound< Derivative< Id, Tangent< T >>>
{
    using variable_type = free_variables_t< T >::template variable_t< Id >;
    using type = Quotient< Quotient< typename Derivative< Id, T >::type, 
        Pow< Cosine< T >, Constant< 2 >>>,
            Constant< result_t< variable_type >{ 1 }>>;

    static constexpr type
    value( Tangent< T > const& expr )
    { 
        auto [ arg ] = expr.args();

        return {{ Derivative< Id, T >::value( arg ), {{ arg }, {}}}, {}};
    }

    using Compound< Derivative< Id, Tangent< T >>>::Compound;
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derivative< Id, Arcsine< T > >: 
    Compound< Derivative< Id, Arcsine< T >>>
{
    using variable_type = free_variables_t< T >::template variable_t< Id >;
    using type = Quotient< Quotient< typename Derivative< Id, T >::type,
        SquareRoot< Difference< Constant< 1 >, Pow< T, Constant< 2 >>>>>,
            Constant< result_t< variable_type >{ 1 }>>;

    static constexpr type
    value( Arcsine< T > const& expr )
    { 
        auto [ arg ] = expr.args();

        return {{ Derivative< Id, T >::value( arg ),
            {{{}, { arg, {}}}}}, {}};
    }

    using Compound< Derivative< Id, Arcsine< T >>>::Compound;
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derivative< Id, Arccosine< T > >: 
    Compound< Derivative< Id, Arccosine< T >>>
{
    using variable_type = free_variables_t< T >::template variable_t< Id >;
    using type = Quotient< Negation< Quotient< typename Derivative< Id, T >::type,
        SquareRoot< Difference< Constant< 1 >, Pow< T, Constant< 2 >>>>>>,
            Constant< result_t< variable_type >{ 1 }>>;

    static constexpr type
    value( Arccosine< T > const& expr )
    { 
        auto [ arg ] = expr.args();
        
        return {{{ Derivative< Id, T >::value( arg ),
            {{{}, { arg, {} }}}}}, {}};
    }

    using Compound< Derivative< Id, Arccosine< T >>>::Compound;
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derivative< Id, Arctangent< T > >: 
    Compound< Derivative< Id, Arctangent< T >>>
{
    using variable_type = free_variables_t< T >::template variable_t< Id >;
    using type = Quotient< Quotient< typename Derivative< Id, T >::type,
        SquareRoot< Sum< Constant< 1 >, Pow< T, Constant< 2 >>>>>,
            Constant< result_t< variable_type >{ 1 }>>;

    static constexpr type
    value( Arctangent< T > const& expr )
    { 
        auto [ arg ] = expr.args();
        
        return {{ Derivative< Id, T >::value( expr ),
            {{ {}, { arg, {} }}}, {}}};
    }

    using Compound< Derivative< Id, Arctangent< T >>>::Compound;
};

template< size_t Id, typename T, typename U >
requires( free_variables_t< T >::contains_id( Id ) or
    free_variables_t< U >::contains_id( Id ) )
struct Derivative< Id, Arctangent2< T, U > >: 
    Compound< Derivative< Id, Arctangent2< T, U >>>
{
    using type = Derivative< Id, Arctangent< Quotient< T, U >> >::type;

    static constexpr type
    value( Arctangent2< T, U > const& expr )
    { 
        auto [ num, den ] = expr.args();

        return Derivative< Id, Arctangent< Quotient< T, U >> >::
            value({{ num, den }}); 
    }

    using Compound< Derivative< Id, Arctangent2< T, U >>>::Compound;
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derivative< Id, Log< T > >: Compound< Derivative< Id, Log< T >>>
{
    using variable_type = free_variables_t< T >::template variable_t< Id >;
    using type = Quotient< 
        Quotient< typename Derivative< Id, T >::type, Abs< T >>,
            Constant< result_t< variable_type >{ 1 }>>;

    static constexpr type
    value( Log< T > const& expr )
    { 
        auto [ arg ] = expr.args();

        return {{ Derivative< Id, T >::value( arg ), { arg }}, {}};
    }

    using Compound< Derivative< Id, Log< T >>>::Compound;
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derivative< Id, Exp< T > >: Compound< Derivative< Id, Exp< T >>>
{
    using variable_type = free_variables_t< T >::template variable_t< Id >;
    using type = Quotient< 
        Product< Exp< T >, typename Derivative< Id, T >::type >,
            Constant< result_t< variable_type >{ 1 }>>;

    static constexpr type
    value( Exp< T > const& expr )
    {
        auto [ arg ] = expr.args();

        return {{{ arg }, Derivative< Id, T >::value( arg )}, {}};
    }

    using Compound< Derivative< Id, Exp< T >>>::Compound;
};

template< size_t Id, typename T >
requires( free_variables_t< T >::contains_id( Id ) )
struct Derivative< Id, Abs< T > >: Compound< Derivative< Id, Abs< T >>>
{ static_assert( false, "piece-wise functions are not implemented yet" ); };

template< size_t Id, typename... Ts >
requires( free_variables_t< tuple< Ts... >>::contains_id( Id ) )
struct Derivative< Id, tuple< Ts... > >: 
    Compound< Derivative< Id, tuple< Ts... >>>
{
    using type = tuple< typename Derivative< Id, Ts >::type... >;

    static constexpr type
    value( tuple< Ts... > const& tup )
    { 
        static constexpr make_seq< sizeof...( Ts )> for_elements;

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
        { return { Derivative< Id, Ts...[ Is ] >::
            value( get< Is >( tup ))... }; };

        return helper( for_elements );
    }

    using Compound< Derivative< Id, tuple< Ts... >>>::Compound;
};

template< size_t Id, shape S, typename... Ts >
requires( free_variables_t< Tensor< S, Ts... >>::contains_id( Id ) )
struct Derivative< Id, Tensor< S, Ts... > >: 
    Compound< Derivative< Id, Tensor< S, Ts... >>>
{
    using type = Tensor< S, typename Derivative< Id, Ts >::type... >;

    static constexpr type
    value( Tensor< S, Ts... > const& ten )
    {
        static constexpr make_seq< sizeof...( Ts )> for_elements;

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
        { return { Derivative< Id, Ts...[ Is ] >::
            value( tensor_get< Is >( ten ))... }; };

        return helper( for_elements );
    }

    using Compound< Derivative< Id, Tensor< S, Ts... >>>::Compound;
};

template< typename ExprT, typename VarU >
using derivative_t = Derivative< var_id_v< VarU >, ExprT >::type;

template< typename ExprT, typename VarU >
constexpr derivative_t< ExprT, VarU >
derivative( ExprT const& expr, VarU const& )
{ return Derivative< var_id_v< VarU >, ExprT >::value( expr ); }

template< typename FuncT >
struct Gradient;

template< >
struct IsCompoundOperation< Gradient >: true_type { };

template< typename FuncT >
struct Gradient: Compound< Gradient< FuncT >> {
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

    using Compound< Gradient< FuncT >>::Compound;
};

template< typename ExprT, variable... Vars >
struct Gradient< Func< ExprT, Vars... >>: 
    Compound< Gradient< Func< ExprT, Vars... >>> 
{
    using type = Tensor< Shape< sizeof...( Vars )>, 
        derivative_t< Func< ExprT, Vars... >, Vars >... >;

    static constexpr type
    value( Func< ExprT, Vars... > const& func )
    { return { derivative( func, Vars{} )... }; }

    using Compound< Gradient< Func< ExprT, Vars... >>>::Compound;
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

