#include <print>
#include <tuple>
#include <utility>
#include <type_traits>

#include <cmath>
#include <cassert>
#include <array>

using std::integral_constant;
using std::tuple, std::make_tuple;
using std::isgreater, std::isless;
using std::is_same_v;

template< size_t... Is >
using seq = std::index_sequence< Is... >;

template< size_t Size >
using make_seq = std::make_index_sequence< Size >;

template< typename... Xs >
requires(( std::is_arithmetic_v< Xs > and ... ))
constexpr auto sum_of( Xs... xs )
{ return ( xs + ... ); }

template< typename... Xs >
requires(( std::is_arithmetic_v< Xs > and ... ))
constexpr auto product_of( Xs... xs )
{ return ( xs * ... ); }

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

// distribute< And, Or >( )
template< template< typename... > class S, template< typename... > class P, typename T >
class Distributor;

template< template< typename... > class S, template< typename... > class P, typename... Ts >
class Distributor< S, P, P< Ts... >>
{
    // example:
    // (a*b*(c+d+e)*(f+g)*h*(i+j)) -> 
    // abcfhi + abdfhi + abefhi +
    // abcghi + abdghi + abeghi +
    // abcfhj + abdfhj + abefhj +
    // abcghj + abdghj + abeghj

    // ex: 6
    static constexpr size_t product_term_size = sizeof...( Ts );

    template< typename U >
    struct TermSize: integral_constant< size_t, 1 > { };

    template< typename... Us >
    struct TermSize< S< Us... >>: integral_constant< size_t, sizeof...( Us )> { };

    template< typename U >
    static constexpr size_t term_size_v = TermSize< U >::value;

    // ex: 12
    static constexpr size_t terms_size = product_of( term_size_v< Ts >... );

    template< size_t I >
    struct TermDivisor: integral_constant< size_t, 
        term_size_v< Ts...[I - 1] > * TermDivisor< I - 1 >::value > { };

    template< >
    struct TermDivisor< 0 >: integral_constant< size_t, 1 > { };

    template< size_t I >
    static constexpr size_t term_divisor_v = TermDivisor< I >::value;

    template< typename T, size_t I >
    struct ProductTerm;

    template< typename T >
    requires( term_size_v< T > == 1 )
    struct ProductTerm< T, 0 >
    { 
        using type = T; 
        static constexpr type get( T t ) { return t; }
    };

    template< typename... Us, size_t I >
    requires( isgreater( sizeof...( Us ), 1 ))
    struct ProductTerm< S< Us... >, I >
    { 
        using type = Us...[I]; 
        static constexpr type get( S< Us... > s ) 
        { return std::get< I >( s ); }
    };

    template< typename T, size_t I >
    using product_term_t = ProductTerm< T, I >::type;

    template< size_t I, typename T >
    static constexpr product_term_t< T, I > get_product_term( T term )
    { return ProductTerm< T, I >::get( term ); }

    template< size_t I, typename Seq >
    struct TermHelper;

    template< size_t I, size_t... Js >
    struct TermHelper< I, seq< Js... >>
    { 
        template< size_t J >
        static constexpr size_t term_index_v = I / term_divisor_v< J > % term_size_v< Ts...[ J ]>;

        using type = P< product_term_t< Ts...[Js], term_index_v< Js >>... >; 

        static constexpr type get( P< Ts... > p )
        { return { get_product_term< term_index_v< Js >>( std::get< Js >( p ))... }; }
    };

    template< size_t I >
    struct Term: TermHelper< I, make_seq< product_term_size >> { };

    template< size_t I >
    using term_t = Term< I >::type;

    template< typename Seq >
    struct Helper;

    template< size_t... Is >
    struct Helper< seq< Is... >>
    { 
        using type = S< term_t< Is >... >; 
        static constexpr type get( P< Ts... > p )
        { return { TermHelper< Is, make_seq< product_term_size >>::get( p )... }; }
    };

public:
    using type = Helper< make_seq< terms_size >>::type;
    static constexpr type get( P< Ts... > p )
    { return Helper< make_seq< terms_size >>::get( p ); }
};

template< typename T >
using normal_form_t = Distributor< Or, And, T >::type;

static_assert( is_same_v< Or< And< bool >>, normal_form_t< And< bool >>> );
static_assert( is_same_v< Or< And< bool, bool >, And< bool, bool >>, 
    normal_form_t< And< bool, Or< bool, bool >>>> );
static_assert( is_same_v< Or< And< bool, bool >, And< bool, bool >>, 
    normal_form_t< And< Or< bool, bool >, bool >>> );
static_assert( is_same_v< 
    Or< And< bool, bool >, And< bool, bool >, And< bool, bool >, And< bool, bool >>, 
    normal_form_t< And< Or< bool, bool >, Or< bool, bool >>>> );

template< typename T >
constexpr normal_form_t< T > normalize( T expr )
{ return Distributor< Or, And, T >::get( expr ); }

template< template< typename... > class S, typename T >
class Commutator { 
public:
    static constexpr size_t terms_size = 1;

    template< size_t I >
    struct Term;

    template< >
    struct Term< 0 >
    {
        using type = T;
        static constexpr type get( T t ) { return t; };
    };

    using type = Term< 0 >::type;
    static constexpr type get( T t ) 
    { return Term< 0 >::get( t ); }
};

template< template< typename... > class S, typename... Ts >
class Commutator< S, S< Ts... >>
{
    template< size_t I >
    static constexpr size_t term_size = Commutator< S, Ts...[I] >::terms_size;

    template< typename Seq >
    struct TermsSizeHelper;

    template< size_t... Is >
    struct TermsSizeHelper< seq< Is... >>: integral_constant< size_t, 
        ( term_size< Is > + ... )> { };

public:
    static constexpr size_t terms_size = 
        TermsSizeHelper< make_seq< sizeof...( Ts )>>::value;
    
private:
    using terms_seq = make_seq< terms_size >;

    template< size_t I, typename Seq >
    struct TermHelper;

    template< size_t R, size_t J, size_t... Js >
    requires( isless( R, term_size< J > ))
    struct TermHelper< R, seq< J, Js... >>
    {
        using type = Commutator< S, Ts...[J] >::template Term< R >::type;
        static constexpr type get( S< Ts... > s )
        { return Commutator< S, Ts...[J] >::template Term< R >::get( std::get<J>( s )); }
    };

    template< size_t R, size_t J, size_t... Js >
    requires( not isless( R, term_size< J > ))
    struct TermHelper< R, seq< J, Js... >>:
        TermHelper< R - term_size< J >, seq< Js... >> { };

public:
    template< size_t I >
    struct Term: TermHelper< I, make_seq< sizeof...( Ts )>> { };

private:
    template< typename Seq >
    struct Helper;

    template< size_t... Is >
    struct Helper< seq< Is... >>
    { 
        using type = S< typename Term< Is >::type... >;
        static constexpr type get( S< Ts... > s )
        { return { Term< Is >::get( s )... }; }
    };

    using helper_type = Helper< terms_seq >;

public:
        
    using type = helper_type::type;
    static constexpr type get( S< Ts... > s )
    { return helper_type::get( s ); }
};

template< typename T >
using commute_and_t = Commutator< And, T >::type;

template< typename T >
using commute_or_t = Commutator< Or, T >::type;

// static_assert( is_same_v< And<bool,bool,bool>, 
//     commute_and_t< And<And<bool,bool>,bool>>> );
// static_assert( is_same_v< And<bool,bool,bool>, 
//     commute_and_t< And<bool,And<bool,bool>>>> );
// static_assert( is_same_v< And<bool,bool,bool,bool>, 
//     commute_and_t< And<And<bool,bool>,And<bool,bool>>>> );
// static_assert( is_same_v< And<bool,bool,bool,bool,bool,bool,bool,bool>, 
//     commute_and_t< And<And<And<bool,bool>,And<bool,bool>>,And<And<bool,bool>,And<bool,bool>>> >> );

enum normalize_operation
{ Leaf, Sum, Product };

template< template< typename... > class S, template< typename... > class P, typename T >
class Normalizer;

template< template< typename... > class S, template< typename... > class P, typename T >
using normalized_t = Normalizer< S, P, T >::type;

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
    struct TermElement: TermElementHelper< I, K, make_seq< sizeof...( Ts )>> { };

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

static_assert( is_same_v< normalized_t< Or, And, bool >, bool > );
static_assert( is_same_v< normalized_t< Or, And, And<And<bool,bool>,bool>>, 
    And< bool, bool, bool >> );
static_assert( is_same_v< normalized_t< Or, And, Or<bool,Or<bool,bool>>>,
    Or< bool,bool,bool >> );

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

int main( int ac, char* av[] )
{
    static_assert( and_( true, true ));
    static_assert( not and_( true, false ));
    static_assert( or_( true, false ));
    static_assert( not or_( false, false ));
    static_assert( not not_( true ));
    static_assert( not_( false ));

    // ~(~(F&T)|F) == ~(~(F&T)) == ~~F == F
    // ~((~F|~T)|F)
    // ~(~F|~T)&~F
    // (F&T)&~F == F

    constexpr auto a = not_( or_( not_( and_( false, true )), false ));
    constexpr auto an = normalize( a );



    return EXIT_SUCCESS;
}




// template< typename T >
// struct Normalize
// {   using type = T;
//     static constexpr type make( T t ) { return t; } };

// template< typename T >
// using normalize_t = Normalize< T >::type;

// template< typename T >
// constexpr normalize_t< T > normalize( T t )
// { return Normalize< T >::make( t ); }

// // double negatives
// template< typename T >
// struct Normalize< Not< Not< T >>>
// {   using type = normalize_t< T >;
//     static constexpr type make( Not< Not< T >> t ) 
//     { return normalize( get< 0 >( get< 0 >( t ))); }};

// // demorgans law
// template< typename... Ts >
// struct Normalize< Not< And< Ts... >>>
// {
//     using intermediate_type = Or< Not< Ts >... >;
//     using type = normalize_t< intermediate_type >;
//     static constexpr type make( Not< And< Ts... >> arg )
//     { return helper( arg, make_seq< sizeof...( Ts )>{}); }

//     template< size_t... Is >
//     static constexpr type helper( Not< And< Ts... >> arg, seq< Is... > )
//     { return normalize< intermediate_type >({{ get< Is >( arg ) }... }); }
// };

// // demorgans law
// template< typename... Ts >
// struct Normalize< Not< Or< Ts... >>>
// {
//     using intermediate_type = And< Not< Ts >... >;
//     using type = normalize_t< intermediate_type >;
//     static constexpr type make( Not< Or< Ts... >> arg )
//     { return helper( arg, make_seq< sizeof...( Ts )>{}); }

//     template< size_t... Is >
//     static constexpr type helper( Not< Or< Ts... >> arg, seq< Is... > )
//     { return normalize< intermediate_type >({{ get< Is >( arg ) }... }); }
// };

