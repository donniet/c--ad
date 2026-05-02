#ifndef __NORMALIZE_HPP__
#define __NORMALIZE_HPP__

#include "utility.hpp"

template< template< typename... > class S, template< typename... > class P, typename T >
class Normalizer;

template< template< typename... > class S, template< typename... > class P, typename T >
using normalized_t = Normalizer< S, P, T >::type;

template< 
    template< typename... > class S, 
    template< typename... > class P, 
    typename T >
constexpr normalized_t< S, P, T > normalize( T expr )
{ return Normalizer< S, P, T >::value( expr ); }

/*
(a + b)*(c + de + f)*g*(h+i)
acgh+acgi + adegh+adegi + afgh+afgi +
bcgh+bcgi + bdegh+bdegi + bfgh+bfgi
*/

/// @brief leaf case for the normalizer (ie: not a sum or a product)
/// @tparam T 
template< template< typename... > class S, template< typename... > class P, typename T >
class Normalizer {
public: 
    // treat this as having a single term
    static constexpr size_t terms_size = 1;

    // and a single element
    template< size_t I >
    struct TermElementSize;
    
    template< > 
    struct TermElementSize< 0 >: integral_constant< size_t, 1 > { };

    // and the type of this single element is T
    template< size_t I, size_t K >
    struct TermElement;

    template< >
    struct TermElement< 0, 0 >
    { 
        using type = T; 
        static constexpr type value( T expr )
        { return expr; }
    };

    using type = T;
    // TODO: const& ?
    static constexpr type value( T expr )
    { return expr; }
};

template< template< typename... > class S, template< typename... > class P, typename... Ts >
class Normalizer< S, P, S< Ts... >> {
public:
    // I, Is => indices to additive terms of the normalization
    // J, Js => indices to the Ts... product element types
    // K, Ks => indices to the product elements of a term in this normalization

    static constexpr size_t terms_size = 
        ( Normalizer< S, P, Ts >::terms_size + ... );
        
    template< size_t J >
    using subnormalizer = Normalizer< S, P, Ts...[J] >;

    template< size_t I, typename Seq >
    struct TermElementSizeHelper;

    template< size_t I, size_t J, size_t... Js >
    requires( isless( I, subnormalizer< J >::terms_size ))
    struct TermElementSizeHelper< I, seq< J, Js... >>: 
        integral_constant< size_t, subnormalizer< J >::
            template TermElementSize< I >::value > { };
        
    template< size_t I, size_t J, size_t... Js >
    requires( not isless( I, subnormalizer< J >::terms_size ))
    struct TermElementSizeHelper< I, seq< J, Js... >>: 
        TermElementSizeHelper< I - subnormalizer< J >::terms_size, seq< Js... >>
    { };

    template< size_t I >
    struct TermElementSize: 
        TermElementSizeHelper< I, make_seq< sizeof...( Ts )>> { };

    template< size_t I >
    static constexpr size_t term_element_size = TermElementSize< I >::value;

    /// @brief examines each Ts... (indexed by Seq) until we find the Kth
    /// product element of the Ith additive term of the normalized expression
    template< size_t I, size_t K, typename Seq >
    struct TermElementHelper;

    /// @brief case where Ts...[J] contains the Ith term
    template< size_t I, size_t K, size_t J, size_t... Js >
    requires( isless( I, subnormalizer< J >::terms_size ))
    struct TermElementHelper< I, K, seq< J, Js... >>
    { 
        using type = subnormalizer< J >::template TermElement< I, K >::type; 
        static constexpr type value( S< Ts... > expr )
        { return subnormalizer< J >::template TermElement< I, K >::value( 
            std::get< J >( expr )); }
    };

    /// @brief case where Ts...[J] contains the Ith term
    template< size_t I, size_t K, size_t J, size_t... Js >
    requires( not isless( I, subnormalizer< J >::terms_size ))
    struct TermElementHelper< I, K, seq< J, Js... >>:
        TermElementHelper< I - subnormalizer< J >::terms_size, K, seq< Js... >>
    { };

    /// @brief the Kth element of the Ith term of the normalization
    template< size_t I, size_t K >
    struct TermElement: TermElementHelper< I, K, make_seq< sizeof...( Ts )>> 
    { };

    template< size_t I, size_t K >
    using term_element_t = TermElement< I, K >::type;

    template< size_t, typename >
    struct TermHelper;

    /// @brief represents the Ith term of a sum of products in the result. terms
    /// are products (P<...>) of things that are not Sums (S<...>)
    /// @tparam I is the term index (0 <= I <= terms_size)
    /// @tparam ...Js is an index sequence to the Ts... elements of P<...>
    template< size_t I, size_t... Ks >
    struct TermHelper< I, seq< Ks... >>
    { 
        using type = P< term_element_t< I, Ks >... >; 
        static constexpr type value( S< Ts... > expr )
        { return { TermElement< I, Ks >::value( expr )... }; }
    };

    template< size_t I >
    struct Term;

    template< size_t I >
    requires( isgreater( term_element_size< I >, 1 ))
    struct Term< I >: TermHelper< I, make_seq< term_element_size< I >>> { };

    template< size_t I >
    requires( term_element_size< I > == 1 )
    struct Term< I >
    { 
        using type = term_element_t< I, 0 >; 
        static constexpr type value( S< Ts... > expr )
        { return TermElement< I, 0 >::value( expr ); }
    };

    template< typename Seq >
    struct Helper;

    template< >
    struct Helper< seq< 0 >>
    {
        using type = Term< 0 >::type;
        static constexpr type value( S< Ts... > expr )
        { return Term< 0 >::value( expr ); }
    };

    template< size_t... Is >
    struct Helper< seq< Is... >>
    // final type is a sum of the term types
    { 
        using type = S< typename Term< Is >::type... >; 
        static constexpr type value( S< Ts... > expr )
        { return { Term< Is >::value( expr )... }; }
    };

    using type = Helper< make_seq< terms_size >>::type;
    static constexpr type value( S< Ts... > expr )
    { return Helper< make_seq< terms_size >>::value( expr ); }
};

template< template< typename... > class S, template< typename... > class P, 
    typename... Ts >
class Normalizer< S, P, P< Ts... >> {
public:
    // I, Is => indices to additive terms of the normalization
    // J, Js => indices to the Ts... product element types
    // K, Ks => indices to the product elements of a term in this normalization

    static constexpr size_t terms_size = 
        ( Normalizer< S, P, Ts >::terms_size * ... );

    template< size_t J >
    using subnormalizer = Normalizer< S, P, Ts...[J] >;

    template< size_t J >
    struct ElementDivisor: integral_constant< size_t, 
        subnormalizer< J - 1 >::terms_size * 
            ElementDivisor< J - 1 >::value > { };

    template< >
    struct ElementDivisor< 0 >: integral_constant< size_t, 1 > { };

    template< size_t J >
    static constexpr size_t divisor = ElementDivisor< J >::value;

    /// @brief the final sum of products term I has a contribution from the Jth
    /// element of the product P<Ts...>
    template< size_t J, size_t I >
    static constexpr size_t subterm = I / divisor< J > % 
        subnormalizer< J >::terms_size;

    template< size_t I, typename Seq >
    struct TermElementSizeHelper;

    template< size_t I, size_t... Js >
    struct TermElementSizeHelper< I, seq< Js... >>: integral_constant< size_t,
        ( Normalizer< S, P, Ts...[Js] >::
            template TermElementSize< subterm< Js, I >>::value + ... )> { };

    template< size_t I >
    struct TermElementSize: 
        TermElementSizeHelper< I, make_seq< sizeof...( Ts )>> { };

    template< size_t I >
    static constexpr size_t term_element_size = TermElementSize< I >::value;

    template< size_t J, size_t I >
    static constexpr size_t subterm_element_size = 
        subnormalizer< J >::
            template TermElementSize< subterm< J, I >>::value;

    /// @brief examines each Ts... (indexed by Seq) until we find the Kth
    /// product element of the Ith additive term of the normalized expression
    template< size_t I, size_t K, typename Seq >
    struct TermElementHelper;

    /// @brief case where Ts...[J] contributes the Kth element of the Ith term
    template< size_t I, size_t K, size_t J, size_t... Js >
    requires( isless( K, subterm_element_size< J, I >))
    struct TermElementHelper< I, K, seq< J, Js... >>
    { 
        using type = subnormalizer< J >::
            template TermElement< subterm< J, I >, K >::type; 
        static constexpr type value( P< Ts... > expr )
        { return subnormalizer< J >::
            template TermElement< subterm< J, I >, K >::
                value( std::get< J >( expr )); }
    };

    /// @brief case where Ts...[J] does not contribute enough elements to the
    /// Ith term.  We subtract the number of elements Ts...[J] contributes and
    /// search the remaining Js...
    template< size_t I, size_t K, size_t J, size_t... Js >
    requires( not isless( K, subterm_element_size< J, I >))
    struct TermElementHelper< I, K, seq< J, Js... >>: 
        TermElementHelper< I, K - subterm_element_size< J, I >, seq< Js... >>
    { };

    /// @brief the Kth element of the Ith term of the normalization
    template< size_t I, size_t K >
    struct TermElement: 
        TermElementHelper< I, K, make_seq< sizeof...( Ts )>> { };

    template< size_t I, size_t K >
    using term_element_t = TermElement< I, K >::type;

    template< size_t, typename >
    struct TermHelper;

    /// @brief represents the Ith term of a sum of products in the result. terms
    /// are products (P<...>) of things that are not Sums (S<...>)
    /// @tparam I is the term index (0 <= I <= terms_size)
    /// @tparam ...Js is an index sequence to the Ts... elements of P<...>
    template< size_t I, size_t... Ks >
    struct TermHelper< I, seq< Ks... >>
    { 
        using type = P< term_element_t< I, Ks >... >; 
        static constexpr type value( P< Ts... > expr )
        { return { TermElement< I, Ks >::value( expr )... }; }
    };

    template< size_t I >
    struct Term;

    template< size_t I >
    requires( isgreater( term_element_size< I >, 1 ))
    struct Term< I >: TermHelper< I, make_seq< term_element_size< I >>> { };

    template< size_t I >
    requires( term_element_size< I > == 1 )
    struct Term< I >
    { 
        using type = term_element_t< I, 0 >; 
        static constexpr type value( P< Ts... > expr )
        { return TermElement< I, 0 >::value( expr ); }
    };

    template< typename Seq >
    struct Helper;

    template< >
    struct Helper< seq< 0 >>
    {
        using type = Term< 0 >::type;
        static constexpr type value( P< Ts... > expr )
        { return Term< 0 >::value( expr ); }
    };

    template< size_t... Is >
    struct Helper< seq< Is... >>
    { 
        using type = S< typename Term< Is >::type... >; 
        static constexpr type value( P< Ts... > expr )
        { return { Term< Is >::value( expr )... }; }
    };

    using type = Helper< make_seq< terms_size >>::type;
    static constexpr type value( P< Ts... > expr )
    { return Helper< make_seq< terms_size >>::value( expr ); }
};


enum op_type { NOT, AND, OR };

template< typename... Ts >
struct And: tuple< Ts... >
{ 
    static constexpr op_type op = AND; 
    
    template< size_t... Is >
    constexpr bool result_helper( seq< Is... > ) const
    { return ( get< Is >( *this ) and ... ); }

    constexpr operator bool() const
    { return result_helper( make_seq< sizeof...( Ts )>{} ); }

    constexpr And( Ts... ts ): tuple< Ts... >{ ts... } { }
};

template< typename... Ts >
struct Or: tuple< Ts... >
{ 
    static constexpr op_type op = OR; 
    
    template< size_t... Is >
    constexpr bool result_helper( seq< Is... > ) const
    { return ( get< Is >( *this ) or ... ); }

    constexpr operator bool() const
    { return result_helper( make_seq< sizeof...( Ts )>{} ); }

    constexpr Or( Ts... ts ): tuple< Ts... >{ ts... } { }
};

template< typename T >
struct Not: tuple< T >
{ 
    static constexpr op_type op = NOT; 
    constexpr operator bool() const
    { return not get< 0 >( *this ); }

    constexpr Not( T t ): tuple< T >{ t } { }
};

template< typename... Ts >
constexpr And< Ts... > and_( Ts... ts )
{ return { ts... }; }

template< typename... Ts >
constexpr Or< Ts... > or_( Ts... ts )
{ return { ts... }; }

template< typename T >
constexpr Not< T > not_( T t )
{ return { t }; }

template< typename T >
constexpr T not_( Not< T > t )
{ return t; }

template< typename... Ts >
constexpr auto not_( And< Ts... > a )
{
    auto helper = [&]< size_t... Is >( seq< Is... > )
    { return or_( not_( get< Is >( a ))... ); };

    return helper( make_seq< sizeof...( Ts )>{} );
}

template< typename... Ts >
constexpr auto not_( Or< Ts... > o )
{
    auto helper = [&]< size_t... Is >( seq< Is... > )
    { return and_( not_( get< Is >( o ))... ); };

    return helper( make_seq< sizeof...( Ts )>{} );
}


struct b0 { }; struct b1 { }; struct b2 { }; struct b3 { }; struct b4 { }; 
struct b5 { }; struct b6 { }; struct b7 { }; struct b8 { }; struct b9 { };

static_assert( is_same_v< normalized_t< Or, And, bool >, bool > );
static_assert( is_same_v< normalized_t< Or, And, b0 >, b0 > );
static_assert( is_same_v< normalized_t< Or, And, And<And<b0,b1>,b2>>, 
    And< b0, b1, b2 >> );
static_assert( is_same_v< normalized_t< Or, And, Or<b0,Or<b1,b2>>>,
    Or< b0,b1,b2 >> );
static_assert( is_same_v< normalized_t< Or, And, And<Or<b0,b1>,b2>>,
    Or< And<b0,b2>, And<b1,b2>>> );
static_assert( is_same_v< normalized_t< Or, And, Or< And<b0,b1>, Or<b2,b3>>>,
    Or< And< b0,b1 >, b2, b3 >> );
static_assert( is_same_v< normalized_t< Or, And, And< Or< b0,b1 >, Or< b2,b3,b4 > >>,
    Or< And< b0,b2 >, And< b1,b2 >, 
        And< b0,b3 >, And< b1,b3 >, 
        And< b0,b4 >, And< b1,b4 >>> );
static_assert( is_same_v< normalized_t< Or, And, And< Or< b0,b1,b2 >, Or< b3,b4 > >>,
    Or< And< b0,b3 >, And< b1,b3 >, And< b2,b3 >, 
        And< b0,b4 >, And< b1,b4 >, And< b2,b4 >>> );


#endif // __NORMALIZE_HPP__