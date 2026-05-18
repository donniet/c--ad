#ifndef __SUBSTITUTION_HPP__
#define __SUBSTITUTION_HPP__

///////////////////////////
/// Tuple Substitution ///
/////////////////////////
///

#include <tuple>
#include <type_traits>

template< typename VarT, typename ExpressionT >
struct SubstitutionMatch
{
    using variable_type = VarT;
    using expression_type = ExpressionT;
};

/// @brief tests a list of variable matches to ensure that variables
/// across multiple branches of substitution checking match
template< typename... Matches >
struct CompatibleMatches;

// null case is true
template< >
struct CompatibleMatches< >: integral_constant< bool, true >
{ using type = tuple< >; };

// single match case is also true
template< typename VarT, typename ExprT >
struct CompatibleMatches< SubstitutionMatch< VarT, ExprT >>: 
    integral_constant< bool, true >
{ using type = SubstitutionMatch< VarIndex, ExprT >::type; };

template< typename FirstMatch, typename SecondMatch, typename Seq >
struct PairwiseCompatibleMatchesHelper;

/// @brief pairs of matches are always compatible if either matched no
/// variables
template< typename FirstMatch, typename SecondMatch >
requires( FirstMatch::size == 0 )
struct PairwiseCompatibleMatchesHelper< FirstMatch, SecondMatch, seq<> >:
    integral_constant< bool, SecondMatch::value >
{ using type = SecondMatch; };

/// @brief specialization in case second match was empty
template< typename FirstMatch, typename SecondMatch >
requires( FirstMatch::size != 0 and SecondMatch::size == 0 )
struct PairwiseCompatibleMatchesHelper< FirstMatch, SecondMatch, seq<> >:
{ using type = FirstMatch; };

/// @brief specialization in case the first match failed 
template< typename FirstMatch, typename SecondMatch, size_t... Is >
requires( isgreater( sizeof...( Is ), 0 ) and not FirstMatch::value )
struct PairwiseCompatibleMatchesHelper< FirstMatch, SecondMatch, 
    seq< Is... >>: FirstMatch
{ };

/// @brief specialization in case the second match failed
template< typename FirstMatch, typename SecondMatch, size_t... Is >
requires( isgreater( sizeof...( Is ), 0 ) and FirstMatch::value and
    not SecondMatch::value )
struct PairwiseCompatibleMatchesHelper< FirstMatch, SecondMatch, 
    seq< Is... >>: SecondMatch
{ };

/// @brief specialization in case both matches succeeded and were
/// not empty
template< typename FirstMatch, typename SecondMatch, size_t... Is >
requires( isgreater( sizeof...( Is ), 0 ) and FirstMatch::value and
    SecondMatch::value )
struct PairwiseCompatibleMatchesHelper< FirstMatch, SecondMatch,
    seq< Is... >>
{
    /// @brief helper to search MatchT's matches for the expression
    /// matching variable Var
    template< typename MatchT, typename Var, typename Seq >
    struct SelectCompatibleForVariableFromHelper;

    /// @brief null case is void
    template< typename MatchT, typename Var >
    struct SelectCompatibleForVariableFromHelper< MatchT, Var, seq< >>
    { using type = void; };

    /// @brief recursive case when our variable matches the Jth variable
    /// from MatchT. We return the Jth expression type in MatchT
    template< typename MatchT, typename Var, size_t J, size_t... Js >
    requires( Var::index == tuple_element_t< J, typename 
        MatchT::variables_tuple >::index )
    struct SelectCompatibleForVariableFromHelper< MatchT, Var, seq< J, Js... >>
    { using type = tuple_element_t< J, MatchT >::expression_type >; };

    /// @brief recursive case when our variable does not match the Jth variable
    /// from MatchT. We continue trying to match ...Js
    template< typename MatchT, typename Var, size_t J, size_t... Js >
    requires( Var::index != tuple_element_t< J, typename 
        MatchT::variables_tuple >::index )
    struct SelectCompatibleForVariableFromHelper< MatchT, Var, seq< J, Js... >>:
        SelectCompatibleForVariableFromHelper< MatchT, Var, seq< Js... >> { };

    /// @brief selects the match for Var from MatchT using a helper
    template< typename MatchT, typename Var >
    struct SelectCompatibleForVariableFrom:
        SelectCompatibleForVariableFromHelper< MatchT, Var, 
            make_seq< tuple_size_v< MatchT >>> { };

    /// @brief selects the match for Var from either FirstMatch or SecondMatch
    template< typename Var >
    struct SelectCompatibleForVariable:
        SelectCompatibleForVariableFrom< FirstMatch, Var >,
        SelectCompatibleForVariableFrom< SecondMatch, Var > { };

    /// @brief selects the matches for VariablesTuple from FirstMatch and SecondMatch
    template< typename VariablesTuple >
    struct SelectCompatibleExpressions;

    /// @brief implementation for match selection when given a tuple of variables
    template< typename... Vars >
    struct SelectCompatibleExpressions< tuple< Vars... >>
    { using type = tuple< typename SelectCompatibleForVariable< Vars >::type... >; };

    // match J from FirstMatch and match K from SecondMatch are compatible if
    // either the variables they matched were different, or if the expressions
    // they matched are the same
    template< size_t J, size_t K >
    struct IsCombinationCompatible: integral_constant< bool,
        ( tuple_element_t< J, FirstMatch >::variable_index !=
            tuple_element_t< K, SecondMatch >::variable_index ) or
        ( is_same_v< typename tuple_element_t< J, FirstMatch >::expression_type,
            typename tuple_element_t< K, SecondMatch >::expression_type >)> { };

    // the sequence ...Is represents all possible combinations of 
    // matches between first and second to check
    static constexpr bool value = ( IsCombinationCompatible< 
        Is / tuple_size_v< SecondMatch >, Is % tuple_size_v< SecondMatch >>::value and ... );

    using type = std::conditional_t< not value, tuple<>, 
        merge_unique_tuples_t< FirstMatch, SecondMatch >>;
};

// most of the work is done in the pairwise compatibility checker
template< typename FirstMatch, typename SecondMatch >
struct CompatibleMatches< FirstMatch, SecondMatch >:
    PairwiseCompatibleMatchesHelper< FirstMatch, SecondMatch, 
        make_seq< FirstMatch::size * SecondMatch::size >> 
{ };

// recursive case checks pairwise
template< typename First, typename... Rest >
requires( isgreater( sizeof...( Rest ), 1 ))
struct CompatibleMatches< First, Rest... >:
    CompatibleMatches< First, CompatibleMatches< Rest... >> { };



#endif // __SUBSTITUTION_HPP__
