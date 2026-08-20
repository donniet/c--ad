#ifndef __EXPRESSIONS_CONSTANT_HPP__
#define __EXPRESSIONS_CONSTANT_HPP__

#include "expressions/forward_decl.hpp"

namespace expressions {

/////////////////
/// Constant ///
///////////////
///
/// Wraps a compile-time constant as an expression. This allows
/// for expression sipmlification at compile-time.
///
/// @brief a compile-time constant
/// @tparam T the type of the constant
/// @tparam Value of the constant
///
template< auto >
struct Constant;

template< auto Value >
struct IsExpression< Constant< Value >>: std::true_type { };

template< auto Value >
struct Result< Constant< Value >>
{ using type = std::remove_cv_t< decltype( Value )>; };

template< auto Value >
struct Constant
{
    using value_type = std::remove_cv_t< decltype( Value )>;

    static constexpr value_type value = Value;

    constexpr auto operator-() const
    { return Constant< -Value >{}; }

    constexpr operator value_type() const
    { return value; }

    // any amount of substitution into a constant results in the constant's 
    // value
    template< typename... Ts >
    constexpr value_type operator ()( Ts const&... ) const
    { return value; }

    template< typename T >
    constexpr Chain< Constant< Value >, T >
    operator ,( T const& next ) const;
};

template< typename T >
struct IsConstant: std::false_type { };

template< auto N >
struct IsConstant< Constant< N >>: std::true_type { };

template< typename T >
constexpr bool is_constant_v = IsConstant< T >::value;

namespace detail {

// credit: Google Gemini [https://share.gemini.google/0Zw1JP77CQ1t]
// 2. Compile-time Lexer to detect if the literal is a float or integer
template <char... c>
consteval bool is_float_literal() 
{
    constexpr char str[] = { c..., '\0' };
    
    if( str[0] == '0' and ( str[1] == 'x' or str[1] == 'X' )) 
    {
        // Hexadecimal. It's a float if it contains '.' or 'p' / 'P'
        for( int i = 2; str[i] != '\0'; ++i ) 
            if( str[i] == '.' or str[i] == 'p' or str[i] == 'P' ) 
                return true;

        return false;
    }
    
    // Decimal or Octal (Binary floats don't exist). 
    // It's a float if it contains '.' or 'e' / 'E'
    for( int i = 0; str[i] != '\0'; ++i) 
        if( str[i] == '.' or str[i] == 'e' or str[i] == 'E' ) 
            return true;
    
    return false;
}

// 3. The Integer Parser (handles hex, octal, binary, dec, and ' separators)
template <char... c>
consteval int64_t parse_as_int() 
{
    constexpr char str[] = { c..., '\0' };
    int64_t val = 0;
    int base = 10;
    int i = 0;
    
    // Detect base
    if (str[0] == '0') 
    {
        if( str[1] == 'x' or str[1] == 'X' )     { base = 16; i = 2; }
        else if (str[1] == 'b' or str[1] == 'B') { base =  2; i = 2; }
        else                                     { base =  8; i = 1; }
    }
    
    for( ; str[i] != '\0'; ++i ) 
    {
        if( str[i] == '\'' ) 
            continue; // Skip digit separators
        
        char ch = str[i];
        int digit = 0;

        if( ch >= '0' and ch <= '9' )      digit = ch - '0';
        else if( ch >= 'a' and ch <= 'f' ) digit = ch - 'a' + 10;
        else if( ch >= 'A' and ch <= 'F' ) digit = ch - 'A' + 10;
        
        val = val * base + digit;
    }
    return val;
}

// 4. The Float Parser (handles decimals, exponents, and ' separators)
// (Note: To keep this brief, this covers decimal floats. 
// A fully standard-compliant parser would also need hex-float support).
template <char... c>
consteval double parse_as_double() 
{
    constexpr char str[] = { c..., '\0' };
    double val = 0.0;
    int i = 0;
    
    bool past_dot = false;
    double fraction_mult = 0.1;
    int exponent = 0;
    bool has_exponent = false;
    bool exp_negative = false;
    
    for( ; str[i] != '\0'; ++i ) 
    {
        if( str[i] == '\'' ) 
            continue;
        
        if( str[i] == 'e' or str[i] == 'E' ) 
        {
            has_exponent = true;
            i++;
            if( str[i] == '-' ) { exp_negative = true; i++; }
            else if( str[i] == '+' ) { i++; }
            break; 
        }
        
        if( str[i] == '.' ) 
        {
            past_dot = true;
            continue;
        }
        
        int digit = str[i] - '0';
        if( !past_dot ) 
            val = val * 10.0 + digit;
        else {
            val += digit * fraction_mult;
            fraction_mult /= 10.0;
        }
    }
    
    // Apply exponent (e.g., 1e-5)
    if( has_exponent ) 
    {
        for( ; str[i] != '\0'; ++i ) 
        {
            if( str[i] == '\'' ) 
                continue;
            exponent = exponent * 10 + (str[i] - '0');
        }

        double exp_mult = 1.0;
        for( int e = 0; e < exponent; ++e ) 
            exp_mult *= 10.0;

        if( exp_negative ) val /= exp_mult;
        else               val *= exp_mult;
    }
    
    return val;
}

} // namespace detail

// 5. The User-Defined Literal Operator
template <char... c>
consteval auto operator""_c() 
{
    // We evaluate the string structure at compile-time to decide which path to
    // take.
    if constexpr( detail::is_float_literal<c...>() ) 
        return Constant< detail::parse_as_double<c...>() >{};
    else 
        return Constant< detail::parse_as_int<c...>() >{};
    
}

} // namespace expressions

#endif
