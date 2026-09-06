#ifndef __EXPRESSIONS_UNIQUE_VARIABLES_HPP__
#define __EXPRESSIONS_UNIQUE_VARIABLES_HPP__

#include "expressions/forward_decl.hpp"

using std::is_same_v;
using std::remove_cv_t;
using std::string;

namespace expressions {

///////////////////////////////////////
/// Variable Set: unique_variables ///
/////////////////////////////////////
///
/// A specialized tuple-like type-set class for uniquely referencing the 
/// dependent variables in an expression.  The variables must be unique and 
/// sorted by their ID in the parameter pack of this class. 
///
///
///
/// Merging two unique_variables sets together keeps the resulting class'
/// template parameters in unique sorted order and is the only way new classes
/// of this template should be defined.
template< variable... Vars >
class unique_variables
{ static_assert( is_sorted_unique_seq_v< seq< var_id_v< Vars >... >>,
    "variables must be sorted by id and unique"); };
 
/// Case: Empty Set
///
template< >
class unique_variables< > {
public:
    static constexpr size_t size = 0;
    using scope_type = Scope< >;
    using variables_tuple = tuple< >;

    static constexpr variables_tuple as_tuple() 
    { return { }; }

    constexpr operator variables_tuple() const
    { return as_tuple(); }

protected:
    template< size_t I >
    struct Element
    { static_assert( I >= size, 
        "index into unique_variables must be less than size" ); }; 

public:
    template< size_t I >
    using element_t = void; // should never be instantiated
    
    template< size_t Id >
    using variable_t = void; // should never be instantiated

    constexpr scope_type make_scope() const;

    template< variable V >
    static consteval bool contains( V = {} )
    { return false; }

    static consteval bool contains_id( size_t Id )
    { return false; }

    template< variable V >
    static consteval size_t index_of( V = {} )
    { return 1; } // 1 > size

    constexpr unique_variables() = default;
    explicit constexpr unique_variables( tuple< >&& )
    { }
};

/// Case: One or More Dependent variables
///
template< variable First, variable... Rest >
requires( is_sorted_unique_seq_v< seq< var_id_v< First >, 
    var_id_v< Rest >... >> )
class unique_variables< First, Rest... >: unique_variables< Rest... > {
public:
    static constexpr size_t size = 1 + sizeof...( Rest );

    using scope_type = Scope< First, Rest... >;
    using values_tuple = tuple< var_value_t< First >,
        var_value_t< Rest >... >;
    using last_type = std::tuple_element_t< sizeof...( Rest ), 
        tuple< First, Rest... >>;
    using variables_tuple = tuple< First, Rest... >;
    
    using first_type = First;

    // returns this unique, sorted set of variables as a tuple
    constexpr variables_tuple as_tuple() const
    { return std::tuple_cat( tuple< first_type >{ first() },
        unique_variables< Rest... >::as_tuple() ); }

    // implicit conversion of this class into an std::tuple
    constexpr operator variables_tuple() const
    { return as_tuple(); }

protected:
    template< size_t I >
    struct Element
    { static_assert( I >= size, 
        "index into unique_variables must be less than size" ); }; 

    template< size_t I >
    requires( I == 0 )
    struct Element< I >
    { 
        using type = First; 
        static constexpr type value( unique_variables const& vars )
        { return vars.first(); }
    };

    template< size_t I >
    requires( 0 < I and I < size )
    struct Element< I >
    { 
        using type = unique_variables< Rest... >::template 
            Element< I - 1 >::type;
        static constexpr type value( unique_variables const& vars )
        { return unique_variables< Rest... >::template 
            Element< I - 1 >::value( vars ); }
    };

    template< size_t Id >
    struct Finder
    { static_assert( false, "variable Id not found in set" ); };

    template< size_t Id >
    requires( Id == var_id_v< first_type > ) 
    struct Finder< Id >
    { 
        using type = first_type;
        static constexpr type value( unique_variables const& vars )
        { return vars.first(); }
    };

    template< size_t Id >
    requires( is_greater( Id, var_id_v< first_type > ))
    struct Finder< Id >
    {
        using type = unique_variables< Rest... >::template 
            Finder< Id >::type;
        static constexpr type value( unique_variables const& vars )
        { return unique_variables< Rest... >::template 
            Finder< Id >::value( vars ); }
    };


public:
    // Ith dependent variable type
    template< size_t I >
    using element_t = Element< I >::type;

    // Ith variable in this collection
    template< size_t I >
    constexpr element_t< I >
    at() const 
    { return Element< I >::value( *this ); }

    // does this set contain a given variable?
    // TODO: this could be a binary search
    template< variable Var >
    static consteval bool contains()
    { return contains_id( var_id_v< Var > ); }

    static consteval bool contains_id( size_t Id )
    { 
        if( is_greater( Id, var_id_v< First > ))
            return unique_variables< Rest... >::contains_id( Id );
        
        return Id == var_id_v< First >;
    };

    template< variable Var >
    static consteval bool contains( Var&& var )
    { return contains< Var >(); }

    // index of the given variable in this set
    template< variable Var >
    static consteval size_t index_of()
    {
        if constexpr( is_greater( var_id_v< Var >, var_id_v< First > ))
            return 1 + unique_variables< Rest... >::template index_of< Var >();

        if constexpr( var_id_v< Var > == var_id_v< First > )
            return 0;

        return size;
    }

    template< variable Var >
    static consteval bool index_of( Var&& var )
    { return index_of< Var >(); }

    template< size_t Id >
    using variable_t = Finder< Id >::type;

    template< size_t Id >
    constexpr variable_t< Id >
    var() const
    { return Finder< Id >::value( *this ); }

    // Create the minimal scope that an expression with these dependent 
    // variables can be executed against.
    constexpr scope_type make_scope() const;

    // Checks if a scope contains the values necessary to evaluate an 
    // expression with these dependent variables
    template< variable... Vars >
    static consteval bool is_valid_scope( Scope< Vars... > scope = {} )
    { return unique_variables< Rest... >::is_valid_scope( scope ) and
        (( var_id_v< First > == var_id_v< Vars > ) or ... ); }

    // The first variable in this set
    constexpr first_type first() const
    { return { _first_name }; } 

    // The last variable in this set
    constexpr last_type last() const
    { return std::get< sizeof...( Rest )>( 
        operator tuple< first_type, Rest... >() ); }

    // The set of dependent variables except the first
    constexpr unique_variables< Rest... > rest() const
    { return *this; }

    constexpr unique_variables() = default;
    constexpr unique_variables( unique_variables const& ) = default;
//    constexpr unique_variables( first_type&& first, Rest&&... rest ):
//        unique_variables< Rest... >{ std::forward( rest )... }, 
//            _first_name{ first.name() } { }
    constexpr unique_variables( first_type const& first, Rest const&... rest ):
        unique_variables< Rest... >{ rest... }, _first_name{ first.name() }
    { }
    
    template< typename FirstName, typename... RestNames >
    requires( is_same_v< remove_cv_t< FirstName >, string > and 
        ( is_same_v< remove_cv_t< RestNames >, string > and ... ) and
            1 + sizeof...( RestNames ) == size )
    constexpr unique_variables( FirstName const& first, 
        RestNames const&... rest ): 
            unique_variables< RestNames... >( rest... ), _first_name{ first } 
    { }

private:
    // storage for the variable metadata
    std::string _first_name;
};


//////////////////////////////
/// Unique Var Operations ///
////////////////////////////
///
namespace detail {

/********************************************************************
 * DESIGN DECISION * Var Id Corresponds to a Unique value_type *
 ********************************************************************/

// DT: we are going to require that a variable id uniquely determines
//     the variable value type in a given expression, and remove the 
//     UniqueTypes class below

template< typename First, typename... Rest >
struct MergeValueTypes
{
    using type = First;
    static_assert(( std::is_same_v< First, Rest > and ...),
        "variables with the same id must have the same value_type" );
};

/// @brief detail for make_unique_variables_t
template< variable... Vars >
struct MakeUniqueVars;

/// terminal case
template< >
struct MakeUniqueVars< >
{
    using type = unique_variables< >;
    static constexpr type value( )
    { return { }; }
};

/// recursive case
template< variable First, variable... Rest >
struct MakeUniqueVars< First, Rest... >
{
    // (1)  recursively call MakeUniqueVars on the Rest...
    using rest_type = MakeUniqueVars< Rest... >::type;
    static constexpr size_t rest_size = rest_type::size;
    static constexpr rest_type rest_value( Rest const&... rest )
    { return MakeUniqueVars< Rest... >::value( rest... ); }

    static constexpr size_t first_id = var_id_v< First >;
    typedef make_seq< rest_size > for_rest;

    // (2)  determine the insert position (if it exists) for First and 
    //      reconstruct the unique_variables collection
    template< typename RestUnique > // unique_variables< >
    struct Inserter
    {
        using type = unique_variables< First >;
        static constexpr type
        value( First const& first )
        { return { first }; }
    };

    // (2a) terminal case for inserter: if rest is empty then the result will
    //      be a unique_variables set containing just First     
//    template< >
//    struct Inserter< unique_variables< >>
//    {
//        //static_assert( not is_substitution_expression_v< var_value_t< First >> ); // and tuple_size_v< tuple< Vars... >> == 1 );
//        using type = unique_variables< First >;
//        static constexpr type 
//        value( First const& first )
//        { return { first }; }
//    };

    // (2b) rest already contains a variable with the same id as First 
    template< typename... Vars >
    requires( unique_variables< Vars... >::contains_id( first_id ))
    struct Inserter< unique_variables< Vars... >>
    {
        static constexpr size_t index = unique_variables< Vars... >::
            template index_of< First >();
        
        using rest_variable = unique_variables< Vars... >::template 
            element_t< index >;
        using merged_variable = Var< var_id_v< First >,
            typename MergeValueTypes< var_value_t< First >, 
                var_value_t< rest_variable >>::type >;

        template< typename Seq >
        struct Enumerator;
        
        // enumerate the resultant unique_variables set, merging any value 
        // types and copy over the variable names from the original
        //
        // NOTE: differing variable types in currently not allowed for variables
        //       with the same id
        template< size_t... Is >
        struct Enumerator< seq< Is... >>
        {
            using type = unique_variables< std::conditional_t< Is == index,
                merged_variable, pack_element_t< Is, Vars... >>... >;
            static constexpr type value( First const& first, 
                rest_type const& rest )
            { return { ( Is == index ? first.name() : 
                rest.template at< Is >().name() )... }; }
        };

        using type = Enumerator< for_rest >::type;

        static constexpr type 
        value( First const& first, Rest const&... rest )
        { return Enumerator< for_rest >::
            value( first, rest_value( rest... )); }
    };

    // (2c) rest does not contain a variable with the same id as First
    template< typename... Vars >
    requires( not unique_variables< Vars... >::contains_id( first_id ))
    struct Inserter< unique_variables< Vars... >>
    {
        using unique_variables_type = unique_variables< Vars... >;

        // count how many variables will precede First
        static constexpr size_t index = 
            (( is_less( var_id_v< Vars >, first_id ) ? 1 : 0 ) + ... + 0 );

        typedef make_seq< 1 + unique_variables_type::size > for_spliced;

        template< typename Seq >
        struct Enumerator;

        // enumerate the new unique_variables set
        template< size_t... Is >
        struct Enumerator< seq< Is... >>
        {
            template< size_t I >
            struct Element;

            // if the Ith element comes before the index of the inserted element
            // then copy over the element from the original at I
            template< size_t I >
            requires( I < index )
            struct Element< I >
            {
                using type = unique_variables_type::template element_t< I >;
                static constexpr type value( First const&, 
                    rest_type const& rest )
                { return rest.template at< I >(); }
            };

            // insert First at index
            template< size_t I >
            requires( I == index )
            struct Element< I >
            {
                using type = First;
                static constexpr type value( First const& first, 
                    rest_type const& )
                { return first; }
            };

            // if the Ith element comes after the index of the inserted element
            // collect it from the original at the position I-1
            template< size_t I >
            requires( I > index )
            struct Element< I >
            {
                using type = unique_variables_type::template 
                    element_t< I - 1 >;
                static constexpr type value( First const&, 
                    rest_type const& rest )
                { return rest.template at< I - 1 >(); }
            };

            using type = unique_variables< typename Element< Is >::type... >;
            static constexpr type value( First const& first, 
                rest_type const& rest )
            { return { Element< Is >::value( first, rest )... }; }
        };

        using type = Enumerator< for_spliced >::type;
        static constexpr type
        value( First const& first, Rest const&... rest )
        { return Enumerator< for_spliced >::value( first, 
            rest_value( rest... )); }
    };

    using type = Inserter< rest_type >::type;
    static constexpr type
    value( First const& first, Rest const&... rest )
    { return Inserter< rest_type >::value( first, rest... ); }
};

template< typename... Sets >
struct MergeUniqueVars;

template< >
struct MergeUniqueVars< >
{ 
    using type = unique_variables< >;
    static constexpr type value( )
    { return { }; }
};

template< typename... Vars >
struct MergeUniqueVars< unique_variables< Vars... >>
{ 
    using type = unique_variables< Vars... >;
    static constexpr type value( unique_variables< Vars... > const& vars )
    { return vars; }
};

// for now just use the MakeUniqueVars implementation but
// TODO: this could be improved
template< typename... LeftVars, typename... RightVars >
struct MergeUniqueVars< unique_variables< LeftVars... >, 
    unique_variables< RightVars... >> 
{
private:
    using left_variable_set = unique_variables< LeftVars... >;
    using right_variable_set = unique_variables< RightVars... >;

    template< size_t I >
    struct Selector;

    template< size_t I >
    requires( is_less( I, left_variable_set::size ))
    struct Selector< I >
    { 
        static constexpr size_t index = I;
        using type = left_variable_set::template element_t< index >;
        static constexpr type
        value( left_variable_set const& left, 
            right_variable_set const& right )
        { return left.template at< index >(); }
    };

    template< size_t I >
    requires( not is_less( I, left_variable_set::size ))
    struct Selector< I >
    {
        static constexpr size_t index = I - left_variable_set::size;
        using type = right_variable_set::template element_t< index >;
        static constexpr type
        value( left_variable_set const& left, 
            right_variable_set const& right )
        { return right.template at< index >(); }
    };

    static constexpr size_t unmerged_size = 
        left_variable_set::size + right_variable_set::size;

    typedef make_seq< unmerged_size > for_unmerged;

    template< typename Seq >
    struct Maker;

    template< size_t... Is >
    struct Maker< seq< Is... >>
    { 
        using type = MakeUniqueVars< typename 
            Selector< Is >::type... >::type;
        static constexpr type
        value( left_variable_set const& left,
            right_variable_set const& right )
        { return MakeUniqueVars< typename Selector< Is >::type... >::
            value( Selector< Is >::value( left, right )... ); }
    };

public:
    using type = Maker< for_unmerged >::type;

    static constexpr type 
    value( unique_variables< LeftVars... > const& left,
        unique_variables< RightVars... > const& right )
    { return Maker< for_unmerged >::value( left, right ); } 
};

template< typename First, typename... Rest >
requires( is_greater( sizeof...( Rest ), 1 ))
struct MergeUniqueVars< First, Rest... >
{
    using rest_variable_set = MergeUniqueVars< Rest... >::type;

    using type = MergeUniqueVars< First, rest_variable_set >::type;
    static constexpr type value( First const& first, Rest const&... rest )
    { 
        rest_variable_set rest_set = 
            MergeUniqueVars< Rest... >:: value( rest... );
        return MergeUniqueVars< First, rest_variable_set >::
            value( first, rest_set );
    }
};

template< typename UniqueVars, typename RemoveVars >
struct SubtractUniqueVars;

template< typename... MinuendVars, typename... SubtrahendVars >
struct SubtractUniqueVars< unique_variables< MinuendVars... >,
    unique_variables< SubtrahendVars... >>
{
private:
    using minuend_type = unique_variables< MinuendVars... >;
    using subtrahend_type = unique_variables< SubtrahendVars... >;

    typedef make_seq< sizeof...( MinuendVars )> for_minuend;

    // (1) what elements from the first should be in our final set?
    template< size_t I >
    struct Pred: std::integral_constant< size_t, 
        ( subtrahend_type::template contains< pack_element_t< I, MinuendVars... >>() ? 0 : 1 )>
    { };

    template< typename Seq >
    struct Tester;

    template< size_t... Is >
    struct Tester< seq< Is... >>
    { 
        static constexpr size_t size = ( Pred< Is >::value + ... + 0 );
        using selection_seq = seq< Pred< Is >::value... >;
        using prefix_sum_seq = seq_prefix_sum_t< selection_seq >;
    };

    static constexpr size_t size = Tester< for_minuend >::size;
    typedef make_seq< size > for_elements;

    template< size_t I >
    struct Element
    {
        // find the output element index + 1 in the prefix sum
        static constexpr size_t index = index_of_seq_v< I + 1, 
            typename Tester< for_minuend >::prefix_sum_seq >;

        using type = minuend_type::template element_t< index >;
        static constexpr type value( minuend_type const& minuend, 
            subtrahend_type const& subtrahend )
        { return minuend.template at< index >(); }
    };

    template< typename Seq >
    struct Helper;

    template< size_t... Is >
    struct Helper< seq< Is... >>
    {
        using type = unique_variables< typename Element< Is >::type... >;
        static constexpr type value( minuend_type const& minuend,
            subtrahend_type const& subtrahend )
        { return { Element< Is >::value( minuend, subtrahend )... }; }
    };

public:
    using type = Helper< for_elements >::type;
    static constexpr type value( minuend_type const& minuend,
        subtrahend_type const& subtrahend )
    { return Helper< for_elements >::value( minuend, subtrahend ); }
};

static_assert( is_sorted_unique_seq_v< seq< 0 >> );
static_assert( requires{ typename unique_variables< >; } );
static_assert( requires{ typename unique_variables< Var< 0, int >>; } );
static_assert( unique_variables< Var< 0, int >>::template 
    contains< Var< 0, int >>());
static_assert( std::is_same_v< unique_variables< Var< 0, int >>,
    typename MakeUniqueVars< Var< 0, int >, Var< 0, int >>::type > );

} // namespace detail

template< typename... Ts >
using merge_unique_variables_t = detail::MergeUniqueVars< Ts... >::type;

template< typename... Ts >
using make_unique_variables_t = detail::MakeUniqueVars< Ts... >::type;

template< typename... Sets >
constexpr detail::MergeUniqueVars< Sets... >::type
merge_unique_variables( Sets const&... sets )
{ return detail::MergeUniqueVars< Sets... >::value( sets... ); }

template< typename... Vars >
constexpr make_unique_variables_t< Vars... >
make_unique_variables( Vars const&... vars )
{ return detail::MakeUniqueVars< Vars... >::value( vars... ); }

template< typename Vars, typename Removals >
using subtract_unique_variables_t = 
    detail::SubtractUniqueVars< Vars, Removals >::type;

template< typename Vars, typename Removals >
constexpr subtract_unique_variables_t< Vars, Removals >
subtract_unique_variables( Vars const& vars, Removals const& removals )
{ return detail::SubtractUniqueVars< Vars, Removals >::
    value( vars, removals ); }

template< typename UniqueA, typename UniqueB >
struct UniqueVarsOverlap;

template< variable A, variable... As, variable... Bs >
struct UniqueVarsOverlap< unique_variables< A, As... >,
    unique_variables< Bs... >>: std::integral_constant< bool,
        (( var_id_v< A > == var_id_v< Bs > ) or ... ) or 
            UniqueVarsOverlap< unique_variables< As... >, 
                unique_variables< Bs... >>::value > 
{ };

template< variable... Bs >
struct UniqueVarsOverlap< unique_variables< >, unique_variables< Bs... >>:
    std::false_type { };

template< typename UniqueA, typename UniqueB >
struct UniqueVarsDisjoint;

template< variable A, variable... As, variable... Bs >
struct UniqueVarsDisjoint< unique_variables< A, As... >,
    unique_variables< Bs... >>: std::integral_constant< bool,
        (( A::id != Bs::id ) and ... ) and UniqueVarsDisjoint<
            unique_variables< As... >, unique_variables< Bs... >>::value >
{ };

template< variable... Bs >
struct UniqueVarsDisjoint< unique_variables< >, unique_variables< Bs... >>:
    std::true_type { };



} // namespace expressions

///////////////////////////////////////
/// unique_variables is tuple-like ///
/////////////////////////////////////
///
namespace std {
/// @brief specialization of std::tuple_size
template< expressions::variable... Vars >
struct tuple_size< expressions::unique_variables< Vars... >>:
    integral_constant< size_t, sizeof...( Vars )> { };

/// @brief specialization of std::tuple_element_t for unique_variables
template< size_t I, expressions::variable... Vars >
struct tuple_element< I, expressions::unique_variables< Vars... >>
{ using type = pack_element_t< I, Vars... >; };

template< size_t I, expressions::variable... Vars >
struct tuple_element< I, const expressions::unique_variables< Vars... >>
{ using type = add_const< pack_element_t< I, Vars... >>::type; };

template< size_t I, expressions::variable... Vars >
struct tuple_element< I, volatile expressions::unique_variables< Vars... >>
{ using type = add_volatile< pack_element_t< I, Vars... >>::type; };

template< size_t I, expressions::variable... Vars >
struct tuple_element< I, const volatile expressions::unique_variables< 
    Vars... >>
{ using type = add_cv< pack_element_t< I, Vars... >>::type; };

/// @brief sepcialization of std::get for unique_variables
template< size_t I, expressions::variable... Vars >
constexpr tuple_element_t< I, expressions::unique_variables< Vars... >> 
get( expressions::unique_variables< Vars... >&& vars )
{ return vars.template at< I >(); }

/// @brief sepcialization of std::get for unique_variables
template< size_t I, expressions::variable... Vars >
constexpr tuple_element_t< I, expressions::unique_variables< Vars... >> const&
get( expressions::unique_variables< Vars... > const& vars )
{ return vars.template at< I >(); }

} // namespace std

#endif
