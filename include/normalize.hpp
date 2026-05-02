#ifndef __NORMALIZE_HPP__
#define __NORMALIZE_HPP__

////////////////////////////////////////////////////////////////////////////////
/// Expression Normalization Library                                         
/// 
/// Notes:
/// * A parameter to the associative disjunctive/additive operation is a "Term"
/// * A parameter to the associative and distriutive conjunctive/multipicative 
///   operation is an "Element"
///
/// example:
/// (a + b)*(c + de + f)*g*(h+i)
/// acgh+acgi + adegh+adegi + afgh+afgi +
/// bcgh+bcgi + bdegh+bdegi + bfgh+bfgi
/// 

#include "utility.hpp"

namespace expressions {
namespace normalization {

/// @brief type manipulator for lazily evaluated expressions with distributive
/// and associative properties.  Three lazily evaluated operations are passed as 
/// template-template parameters that define tuple-like types.
/// TODO: document type restrictions (constructor reqs, etc)
/// @tparam SumOf represents a lazily evaluated, associative operation
/// @tparam ProductOf represents a lazily evaluated, associative operation that 
/// distributes over SumOf
/// @tparam ComplimentOf represents a lazily evaluated unary operation that
/// follows De Morgan's Laws (https://en.wikipedia.org/wiki/De_Morgan%27s_laws)
/// @tparam T the type of the expression to be normalized
/// 
/// Converts T to the equivilent of disjunctive normal form.  In other words,
/// resulting type will be structured as 
///
///     SumOf< ProductOf< [ ComplimentOf< Ts > | Ts ]... >... >
///
/// NOTE: SumOf and ProductOf will be elided(?) and not appear nary operation. 
/// meaning the following structures may also describe the resulting type:
///
///     ProductOf< [ ComplimentOf< Ts > | Ts ]... >
///     SumOf< [ ComplimentOf< Ts > | Ts ]... >
///     [ ComplimentOf< T > | T ]
///     T
/// 
template< template< typename... > class SumOf,      // associative, n-ary
          template< typename... > class ProductOf,  // associative, n-ary and
                                                    // distributes over SumOf
          template< typename > class ComplimentOf,  // obeys De Morgan's Laws
          typename T >
class Normalizer;

/// @brief helper alias to identify a normalizer using the given type
template< template< typename... > class SumOf,      
          template< typename... > class ProductOf, 
          template< typename > class ComplimentOf,
          typename T >
using normalized_t = Normalizer< SumOf, ProductOf, ComplimentOf, T >::type;

/// @brief method to transform the expression instance to the normalized form
template< template< typename... > class SumOf,
          template< typename... > class ProductOf,
          template< typename > class ComplimentOf,
          typename T >
constexpr normalized_t< SumOf, ProductOf, ComplimentOf, T > normalize( T expr )
{ return Normalizer< SumOf, ProductOf, ComplimentOf, T >::value( expr ); }

////////////////////
/// Idempotent Case
///
/// @brief default/leaf case for the normalizer.  Also exemplifies the required
/// static values and aliases for a normalizer
template< template< typename... > class SumOf,
          template< typename... > class ProductOf,
          template< typename > class ComplimentOf,
          typename T >
class Normalizer {
public: 
    /// @brief the type of the expression input to this normalizer
    using expression_type = T;

    /// @brief count of the additive terms in the resultant expression
    static constexpr size_t terms_size = 1;

    /// @brief count the multiplicative elements in Ith term
    template< size_t I >
    struct TermElementSize;

    /// @brief helper for the Kth element of the Ith term of the normalized
    /// expression of T
    template< size_t I, size_t K >
    struct TermElement;
    
    /// @brief the idempotent case has a single element in it's sole term
    template< > 
    struct TermElementSize< 0 >: 
        integral_constant< size_t, 1 > { };        

    /// @brief that single element is the expression type and value itself
    template< >
    struct TermElement< 0, 0 >
    { 
        using type = expression_type;
        static constexpr type value( expression_type expr )
        { return expr; }
    };

    /// @brief type manipulator result type is idempotent
    using type = expression_type;

    /// @brief type manipulator method is idempotent
    static constexpr type value( expression_type expr )
    { return expr; }
};

//////////////////////////
/// Associative Outer Case
/// 
/// @brief Normalizer specialization for disjunctions/summations.
template< template< typename... > class SumOf,
          template< typename... > class ProductOf,
          template< typename > class ComplimentOf,
          typename... Ts >
class Normalizer< SumOf, ProductOf, ComplimentOf, 
/* Expression: */ SumOf< Ts... >> 
{
    // I, Is => indices to additive terms of the normalization
    // J, Js => indices to the Ts... product element types
    // K, Ks => indices to the product elements of a term in this normalization
    // T, Ts => argument types to the operation

    template< typename T >
    using normalizer_t = Normalizer< SumOf, ProductOf, ComplimentOf, T >;
        
    template< size_t J >
    using subnormalizer = normalizer_t< Ts...[J] >;

public:
    using expression_type = SumOf< Ts... >;

    static constexpr size_t terms_size = 
        ( normalizer_t< Ts >::terms_size + ... );

private:
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

public:
    template< size_t I >
    struct TermElementSize: 
        TermElementSizeHelper< I, make_seq< sizeof...( Ts )>> { };

private:
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
        static constexpr type value( expression_type expr )
        { return subnormalizer< J >::template TermElement< I, K >::value( 
            std::get< J >( expr )); }
    };

    /// @brief case where Ts...[J] contains the Ith term
    template< size_t I, size_t K, size_t J, size_t... Js >
    requires( not isless( I, subnormalizer< J >::terms_size ))
    struct TermElementHelper< I, K, seq< J, Js... >>:
        TermElementHelper< I - subnormalizer< J >::terms_size, K, seq< Js... >>
    { };

public:
    /// @brief the Kth element of the Ith term of the normalization
    template< size_t I, size_t K >
    struct TermElement: TermElementHelper< I, K, make_seq< sizeof...( Ts )>> 
    { };

private:
    template< size_t I, size_t K >
    using term_element_t = TermElement< I, K >::type;

    template< size_t, typename >
    struct TermHelper;

    /// @brief represents the Ith term of a sum of products in the result. 
    /// Terms are ProductOf<...> things that are not SumOf<...>
    ///
    /// @tparam I is the term index (0 <= I <= terms_size)
    /// @tparam ...Js is an index sequence to the Ts... elements of P<...>
    template< size_t I, size_t... Ks >
    struct TermHelper< I, seq< Ks... >>
    { 
        using type = ProductOf< term_element_t< I, Ks >... >; 
        static constexpr type value( expression_type expr )
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
        static constexpr type value( expression_type expr )
        { return TermElement< I, 0 >::value( expr ); }
    };

    template< typename Seq >
    struct Helper;

    template< >
    struct Helper< seq< 0 >>
    {
        using type = Term< 0 >::type;
        static constexpr type value( expression_type expr )
        { return Term< 0 >::value( expr ); }
    };

    template< size_t... Is >
    struct Helper< seq< Is... >>
    // final type is a sum of the term types
    { 
        using type = SumOf< typename Term< Is >::type... >; 
        static constexpr type value( expression_type expr )
        { return { Term< Is >::value( expr )... }; }
    };

public:
    using type = Helper< make_seq< terms_size >>::type;
    static constexpr type value( expression_type expr )
    { return Helper< make_seq< terms_size >>::value( expr ); }
};

///////////////////////////////////////////
/// Associative and Distributive Inner Case
/// 
/// @brief Normalizer specialization for conjunctions/products
template< template< typename... > class SumOf,
          template< typename... > class ProductOf,
          template< typename > class ComplimentOf,
          typename... Ts >
class Normalizer< SumOf, ProductOf, ComplimentOf,
/* Expression: */ ProductOf< Ts... >> 
{
    // I, Is => indices to additive terms of the normalization
    // J, Js => indices to the Ts... product element types
    // K, Ks => indices to the product elements of a term in this normalization
    // T, Ts => argument types to the operation

    template< typename T >
    using normalizer_t = Normalizer< SumOf, ProductOf, ComplimentOf, T >;

    template< size_t J >
    using subnormalizer = normalizer_t< Ts...[J] >;

public:
    using expression_type = ProductOf< Ts... >;

    static constexpr size_t terms_size = 
        ( normalizer_t< Ts >::terms_size * ... );

private:
    template< size_t J >
    struct ElementDivisor: integral_constant< size_t, 
        subnormalizer< J - 1 >::terms_size * 
            ElementDivisor< J - 1 >::value > { };

    template< >
    struct ElementDivisor< 0 >: integral_constant< size_t, 1 > { };

    template< size_t J >
    static constexpr size_t divisor = ElementDivisor< J >::value;

    /// @brief the final sum of products term I has a contribution from the Jth
    /// element of the product ProductOf<Ts...>
    template< size_t J, size_t I >
    static constexpr size_t subterm = I / divisor< J > % 
        subnormalizer< J >::terms_size;

    template< size_t I, typename Seq >
    struct TermElementSizeHelper;

    template< size_t I, size_t... Js >
    struct TermElementSizeHelper< I, seq< Js... >>: integral_constant< size_t,
        ( normalizer_t< Ts...[Js] >::
            template TermElementSize< subterm< Js, I >>::value + ... )> { };

public:
    template< size_t I >
    struct TermElementSize: 
        TermElementSizeHelper< I, make_seq< sizeof...( Ts )>> { };

private:
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
        static constexpr type value( expression_type expr )
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

public:
    /// @brief the Kth element of the Ith term of the normalization
    template< size_t I, size_t K >
    struct TermElement: 
        TermElementHelper< I, K, make_seq< sizeof...( Ts )>> { };

private:
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
        using type = ProductOf< term_element_t< I, Ks >... >; 
        static constexpr type value( expression_type expr )
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
        static constexpr type value( expression_type expr )
        { return TermElement< I, 0 >::value( expr ); }
    };

    template< typename Seq >
    struct Helper;

    template< >
    struct Helper< seq< 0 >>
    {
        using type = Term< 0 >::type;
        static constexpr type value( expression_type expr )
        { return Term< 0 >::value( expr ); }
    };

    template< size_t... Is >
    struct Helper< seq< Is... >>
    { 
        using type = SumOf< typename Term< Is >::type... >; 
        static constexpr type value( expression_type expr )
        { return { Term< Is >::value( expr )... }; }
    };

public:
    using type = Helper< make_seq< terms_size >>::type;
    static constexpr type value( expression_type expr )
    { return Helper< make_seq< terms_size >>::value( expr ); }
};

////////////////////
/// De Morgan's Laws
/// 
/// @brief De Morgan's Law of disjunctions
template< template< typename... > class SumOf, 
          template< typename... > class ProductOf, 
          template< typename > class ComplimentOf, 
          typename... Ts >
class Normalizer< SumOf, ProductOf, ComplimentOf, 
/* Expression */  ComplimentOf< SumOf< Ts... >> > 
{ 
    template< typename T >
    using normalizer_t = Normalizer< SumOf, ProductOf, ComplimentOf, T >;

    using reference_type = ProductOf< ComplimentOf< Ts >... >;
    using reference_normalizer = normalizer_t< reference_type >;
    
public:
    using expression_type = ComplimentOf< SumOf< Ts... >>;
    static constexpr size_t terms_size = reference_normalizer::terms_size;

    template< size_t I >
    using TermElementSize = reference_normalizer::
        template TermElementSize< I >;

    template< size_t... Js >
    static constexpr reference_type reference_value_helper( 
        expression_type expr, seq< Js... > )
    { return {{ std::get< Js >( std::get< 0 >( expr ))}... }; }

private:
    static constexpr reference_type reference_value( expression_type expr )
    { return reference_value_helper( expr, make_seq< sizeof...( Ts )>{} ); }

public:
    template< size_t I, size_t K >
    struct TermElement {
    private:
        using reference_term_element = 
            reference_normalizer::template TermElement< I, K >;

    public:
        using type = reference_term_element::type;
        static constexpr type value( expression_type expr )
        { return reference_term_element::value( reference_value( expr )); }
    };

    using type = reference_normalizer::type;
    static constexpr type value( expression_type expr )
    { return reference_normalizer::value( reference_value( expr )); }
};

/// @brief De Morgan's Law of conjunctions
template< template< typename... > class SumOf, 
          template< typename... > class ProductOf, 
          template< typename > class ComplimentOf, 
          typename... Ts >
class Normalizer< SumOf, ProductOf, ComplimentOf, 
/* Expression */  ComplimentOf< ProductOf< Ts... >> >:
    Normalizer< SumOf, ProductOf, ComplimentOf, 
        SumOf< ComplimentOf< Ts >... >>
{ 
    template< typename T >
    using normalizer_t = Normalizer< SumOf, ProductOf, ComplimentOf, T >;

    using reference_type = SumOf< ComplimentOf< Ts >... >;
    using reference_normalizer = normalizer_t< reference_type >;
    
public:
    using expression_type = ComplimentOf< ProductOf< Ts... >>;
    static constexpr size_t terms_size = reference_normalizer::terms_size;

    template< size_t I >
    using TermElementSize = reference_normalizer::
        template TermElementSize< I >;

    template< size_t... Js >
    static constexpr reference_type reference_value_helper( 
        expression_type expr, seq< Js... > )
    { return {{ std::get< Js >( std::get< 0 >( expr ))}... }; }

private:
    static constexpr reference_type reference_value( expression_type expr )
    { return reference_value_helper( expr, make_seq< sizeof...( Ts )>{} ); }

public:
    template< size_t I, size_t K >
    struct TermElement {
    private:
        using reference_term_element = 
            reference_normalizer::template TermElement< I, K >;

    public:
        using type = reference_term_element::type;
        static constexpr type value( expression_type expr )
        { return reference_term_element::value( reference_value( expr )); }
    };

    using type = reference_normalizer::type;
    static constexpr type value( expression_type expr )
    { return reference_normalizer::value( reference_value( expr )); }
};


/// tests of the normalizer
namespace test {

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

// template< typename... Ts >
// constexpr auto not_( And< Ts... > a )
// {
//     auto helper = [&]< size_t... Is >( seq< Is... > )
//     { return or_( not_( get< Is >( a ))... ); };

//     return helper( make_seq< sizeof...( Ts )>{} );
// }

// template< typename... Ts >
// constexpr auto not_( Or< Ts... > o )
// {
//     auto helper = [&]< size_t... Is >( seq< Is... > )
//     { return and_( not_( get< Is >( o ))... ); };

//     return helper( make_seq< sizeof...( Ts )>{} );
// }

template< typename T >
using dnf = Normalizer< Or, And, Not, T >;

template< typename T >
using dnf_result_t = Normalizer< Or, And, Not, T >::type;

struct b0 { }; struct b1 { }; struct b2 { }; struct b3 { }; struct b4 { }; 
struct b5 { }; struct b6 { }; struct b7 { }; struct b8 { }; struct b9 { };

static_assert( is_same_v< dnf_result_t< bool >, bool > );
static_assert( is_same_v< dnf_result_t< b0 >, b0 > );
static_assert( is_same_v< dnf_result_t< And<And<b0,b1>,b2>>, 
    And< b0, b1, b2 >> );
static_assert( is_same_v< dnf_result_t< Or<b0,Or<b1,b2>>>,
    Or< b0,b1,b2 >> );
static_assert( is_same_v< dnf_result_t< And<Or<b0,b1>,b2>>,
    Or< And<b0,b2>, And<b1,b2>>> );
static_assert( is_same_v< dnf_result_t< Or< And<b0,b1>, Or<b2,b3>>>,
    Or< And< b0,b1 >, b2, b3 >> );
static_assert( is_same_v< dnf_result_t< And< Or< b0,b1 >, Or< b2,b3,b4 > >>,
    Or< And< b0,b2 >, And< b1,b2 >, 
        And< b0,b3 >, And< b1,b3 >, 
        And< b0,b4 >, And< b1,b4 >>> );
static_assert( is_same_v< dnf_result_t< And< Or< b0,b1,b2 >, Or< b3,b4 > >>,
    Or< And< b0,b3 >, And< b1,b3 >, And< b2,b3 >, 
        And< b0,b4 >, And< b1,b4 >, And< b2,b4 >>> );

} // namespace test
} // namespace normalization
} // namespace expressions

#endif // __NORMALIZE_HPP__