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

template< >
struct IsExpressionOperation< Derivative >: true_type { };

///////////////
/// Derivations
///
/// Mathematical operators like derivations which transform expressions are
/// implemented as manipulators

struct Derivation
{ };

namespace detail {
template< typename D >
struct IsDerivation: integral_constant< bool,
    std::is_base_of_v< Derivation, D >> { };
};

template< typename D >
concept derivation = detail::IsDerivation< D >::value;


template< typename ExprT >
struct Delta: Derivation
{
    
};

template< size_t I, typename T >
struct Differential
{
    using variable_type = Var< I, T >;

    // derivative of a static value is 0
    template< typename U >
    auto operator()( StaticValue< U > const& expr )
    { 
        using result_type = decltype( U{} / T{} );
        return Constant< static_cast< result_type >( 0 ) >{}; 
    }

    // derivative of a constant is 0
    template< auto Value >
    auto operator()( Constant< Value > const& expr )
    { 
        using result_type = decltype( typename Constant< Value >::value_type{} / T{} );
        return Constant< static_cast< result_type >( 0 )>{}; 
    }

    template< size_t J, typename U >
    auto operator()( Var< J, U > const& expr )
    { return Constant< static_cast< decltype( U{} / T{} ) >( I == J ? 1 : 0 )>{ }; }

    template< typename U >
    auto operator()( Negation< U > const& expr )
    { return -(*this)( expr.arg() ); }

    template< typename U, typename V >
    auto operator()( Sum< U, V > const& expr )
    { return (*this)( expr.left_arg() ) + (*this)( expr.right_arg() ); }

    template< typename U, typename V >
    auto operator()( Difference< U, V > const& expr )
    { return (*this)( expr.left_arg() ) - (*this)( expr.right_arg() ); }

    template< typename U, typename V >
    auto operator()( Product< U, V > const& expr )
    { return (*this)( expr.left_arg() ) * expr.right_arg() + 
        expr.left_arg() * (*this)( expr.right_arg() ); }

    template< typename U, typename V >
    auto operator()( Quotient< U, V > const& expr )
    { return ( expr.numerator_arg() * (*this)( expr.denominator_arg() ) - 
        (*this)( expr.numerator_arg() ) * expr.denominator_arg() ) /
        expr.denominator_arg() / expr.denominator_arg(); }

    template< typename U >
    auto operator()( SquareRoot< U > const& expr )
    { 
        using result_type = decltype( result_t< U >{} / result_t< U >{} );
        return Constant< static_cast< result_type >( 0.5 ) >{} / 
            expr * (*this)( expr.arg() ); 
    }

//    template< int Exp, typename U >
//    auto operator()( power_of< Exp, U > const& expr )
//    { 
//        using result_type = decltype( result_t< U >{} / result_t< U >{} );
//        return Constant<  static_cast< result_type >( Exp ) >{} * 
//            pow< Exp-1 >( expr.arg() ) * (*this)( expr.arg() ); 
//    }

    template< typename U >
    auto operator()( Sine< U > const& expr )
    { return cos( expr.arg() ) * (*this)( expr.arg() ); }

    template< typename U >
    auto operator()( Cosine< U > const& expr )
    { return -sin( expr.arg() ) * (*this)( expr.arg() ); }

    template< typename U >
    auto operator()( Tangent< U > const& expr )
    { return (*this)( expr.arg() ) / pow( cos( expr.arg(), 2_c )); }

    template< typename U >
    auto operator()( Arcsine< U > const& expr )
    { return (*this)( expr.arg() ) / sqrt( 1_c - pow( expr.arg(), 2_c )); }

    template< typename U >
    auto operator()( Arccosine< U > const& expr )
    { return -(*this)( expr.arg() ) / sqrt( 1_c - pow( expr.arg(), 2_c )); }

    template< typename U >
    auto operator()( Arctangent< U > const& expr )
    { return (*this)( expr.arg() ) / ( 1_c - pow( expr.arg(), 2_c )); }


    // TODO: double check the math here
    // TODO: also could we have multiple options for these derivative expressions
    //       if one evaluates to infinity and we can tell at compile time?
    //template< typename U, typename V >
    //auto operator()( Arctangent2< U, V > const& expr )
    //{ return (*this)( expr.numerator_arg() / expr.denominator_arg() ) / 
    //    ( constant_one - pow< 2 >( expr.numerator_arg() / expr.denominator_arg() )); }
    
};

template< typename X >
struct DifferentialFor;

template< size_t I, typename T >
struct DifferentialFor< Var< I, T >>
{ using type = Differential< I, T >; };

template< typename X >
using differential_for_t = DifferentialFor< X >::type;

template< size_t I, typename T >
Differential< I, T > 
differential( Var< I, T > const& )
{ return {}; }

template< variable Var, expression Expr >
auto differential_for( Expr const& expr )
{ return differential_for_t< Var >{}( expr ); }

template< typename ExprT >
struct GradientOperator
{
    using expression_type = ExprT;
    using free_variables_tuple = free_variables_t< expression_type >;
    static constexpr size_t size = tuple_size_v< free_variables_tuple >;

private:
    template< size_t I >
    struct Element
    {
//        using differential_type = differential_for_t< tuple_element_t< I, 
//            free_variables_tuple >, expression_type >;
//
//        static constexpr auto value( expression_type const& expr )
//        { return differential_type{}( expr ); }
    };

    template< typename Seq >
    struct Helper;

    template< size_t... Is >
    struct Helper< seq< Is... >>
    { 
        using differential_type = Tensor< Shape< sizeof...( Is )>, 
            Element< Is >... >;

        static constexpr auto value( ExprT const& expr ) ;
//        { make_tensor< Shape< sizeof...( Is )>>( Element< Is >::value( expr )
//            ... ); }
    };

public:
    using type = Helper< make_seq< size >>::type;
    static constexpr auto value( ExprT const& expr )
    { return Helper< make_seq< size >>::value( expr ); }
};

template< expression ExprT >
constexpr auto gradient( ExprT const& expr )
{ return GradientOperator< ExprT >::value( expr ); }

template< expression ExprT > 
struct JacobianOperator
{
    template< size_t I, size_t J >
    struct Element
    {
//        using differential_type = differential_for_t< tuple_element_t< I,
//            free_variables_tuple >, tensor_element_t< J, expression_type >>;
    };
};



/////////////////////////////
/// Experiment: Calculus ///
///////////////////////////
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

