#ifndef __NORMALIZE_HPP__
#define __NORMALIZE_HPP__

////////////////////////////////////////////////////////////////////////////////
/// Expression Normalization Library                                        ///
//////////////////////////////////////////////////////////////////////////////
/// 
/// Rewrites a lazily evaluated expression in a canonical form.  This enables
/// evaluation, truth-table generation, and solvers on generic expressions 
/// with little additional burden on the programmer, and without compromising
/// the lazy evaluation by manipulating the expression as it is being described
/// by the programmer (see Goals "1: Expressibility," "2: Validation", and 
/// "6: Simplicity")
///
/// Notes:
/// * A parameter to the associative disjunctive/additive operation is a "Term"
/// * A parameter to the associative and distriutive conjunctive/multipicative 
///   operation is an "Element"
///
/// Example Expression:
/// (a + b)*(c + de + f)*g*(h+i)
/// acgh+acgi + adegh+adegi + afgh+afgi +
/// bcgh+bcgi + bdegh+bdegi + bfgh+bfgi
/// 
/// References:
/// * Disjunctive Normal Form 
///   [https://en.wikipedia.org/wiki/Disjunctive_normal_form]
///

#include "utility.hpp"

namespace expressions {
namespace normalization {

/// @brief type manipulator for lazily evaluated expressions with distributive
/// and associative properties.  Three lazily evaluated operations are passed as 
/// template-template parameters that define tuple-like types.
/// TODO: document type restrictions (constructor reqs, etc)
/// @tparam SumOf lazily evaluated, associative operation
/// @tparam ProductOf lazily evaluated, associative operation that distributes 
/// over SumOf
/// @tparam ComplimentOf lazily evaluated unary operation that follows 
/// De Morgan's Laws (https://en.wikipedia.org/wiki/De_Morgan%27s_laws)
/// @tparam T the type of the expression to be normalized
/// 
/// Converts T to the equivilent of disjunctive normal form.  In other words,
/// resulting type will be structured as 
///
///     SumOf< ProductOf< [ ComplimentOf< Ts > | Ts ]... >... >
///
/// This is done by encoding propositional logic equivilance rules, such as
/// De Morgan's Laws, into the templating system.  The following rules are 
/// encoded by this normalizer:
/// 
/// * Law of the Excluded Middle (elimination of double-compliments/negation)
/// * Associative Law (nested SumOf and ProductOf operations are enumerated into
///   a single operation)
/// * Distributive Law for Conjunctions/Products (ProductOf distributes over 
///   SumOf)
/// * De Morgan's Laws (Compliment/Negations distribute over both SumOf and 
///   ProductOf operations)
///
/// The resulting normalized expressions evaluate exactly as the input 
/// expressions so long as the laws above are valid for the template-template
/// parameters.
///
/// NOTE: SumOf and ProductOf are elided(?) and not appear as a unary operation. 
/// This means the following structures may also describe the resulting type:
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

////////////////////////
/// Idempotent Case ///
//////////////////////
/// 
/// Specialization for leaves of a normalized expression.
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

///////////////////////////////
/// Associative Outer Case ///
/////////////////////////////
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

////////////////////////////////////////////////
/// Associative and Distributive Inner Case ///
//////////////////////////////////////////////
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

//////////////////////////
/// Double Compliment ///
////////////////////////
///
/// @brief Compliments eliminate compliments
template< template< typename... > class SumOf, 
          template< typename... > class ProductOf, 
          template< typename > class ComplimentOf, 
          typename T >
class Normalizer< SumOf, ProductOf, ComplimentOf,
/* Expression */  ComplimentOf< ComplimentOf< T >>>
{
    template< typename U >
    using normalizer_t = Normalizer< SumOf, ProductOf, ComplimentOf, U >;

    using reference_type = T;
    using reference_normalizer = normalizer_t< reference_type >;

public:
    using expression_type = ComplimentOf< ComplimentOf< T >>;
    static constexpr size_t terms_size = reference_normalizer::terms_size;

    template< size_t I >
    using TermElementSize = reference_normalizer::
        template TermElementSize< I >;

private:
    static constexpr reference_type reference_value( expression_type expr )
    { return std::get< 0 >( std::get< 0 >( expr )); }

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

/////////////////////////
/// De Morgan's Laws ///
///////////////////////
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

private:
    template< size_t... Js >
    static constexpr reference_type reference_value_helper( 
        expression_type expr, seq< Js... > )
    { return {{ std::get< Js >( std::get< 0 >( expr ))}... }; }

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

} // namespace normalization
} // namespace expressions

#endif // __NORMALIZE_HPP__
