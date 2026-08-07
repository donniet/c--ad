#ifndef __EXPRESSIONS_PREDICATE_HPP__
#define __EXPRESSIONS_PREDICATE_HPP__

#include "expressions/variable.hpp"

namespace expressions {

///////////////////////////////
/// Predicate Substitution ///
/////////////////////////////
///
/// Predicates used to identify parts of expressions to substitute.  
/// A predicate is a boolean valued trait consistent with 
/// std::integral_constant< bool, value >.  Typically they are nested inside
/// another templated class to allow the erasure of parameters to the
/// predicate.  
///
/// @brief expression manipulator that replaces any argument in an expression
/// with the given WithT argument.
///
/// @tparam Predicate resolves to an integral_constant< bool, ... > like
/// like object-type (ie: has a constexpr static bool value member) which
/// flags the expression passed as the template parameter as substitutable
///
///
/// Case: Default is idempotent
template< template< typename > class Predicate, typename ExprT, typename WithT >
struct PredicateSub
{
    using type = ExprT;
    static constexpr type value( ExprT const& expr, WithT const& )
    { return expr; }
};
 
/// Case: Predicate matched expression.  We resolve to our replacement
template< template< typename > class Predicate, typename ExprT, typename WithT >
requires( Predicate< ExprT >::value )
struct PredicateSub< Predicate, ExprT, WithT >
{
    using type = WithT;
    static constexpr type value( ExprT const& expr, WithT const& with )
    { return with; } 
};

/// Case: Unmatched non-compound expression is idempotent
template< template< typename > class Predicate, typename NonCompoundExprT, 
    typename WithT >
requires( not Predicate< NonCompoundExprT >::value and 
    not compound_expression< NonCompoundExprT > ) 
struct PredicateSub< Predicate, NonCompoundExprT, WithT >
{
    using type = NonCompoundExprT;
    static constexpr type value( NonCompoundExprT const& expr, WithT const& with )
    { return expr; }
};

/// Case: Unmatched compound expression recurses
template< template< typename > class Predicate, 
    template< typename... > class Op, typename... Args, typename WithT >
requires( compound_expression< Op< Args... >> and 
    not Predicate< Op< Args... >>::value )
struct PredicateSub< Predicate, Op< Args... >, WithT > {
private:
    template< typename ArgT >
    using ArgumentSub = PredicateSub< Predicate, ArgT, WithT >;

    template< typename ArgT >
    static constexpr typename ArgumentSub< ArgT >::type 
    sub_argument( ArgT const& arg, WithT const& with )
    { return ArgumentSub< ArgT >::value( arg, with ); }

public:
    using type = Op< typename ArgumentSub< Args >::type... >;

private:
    template< size_t... Is >
    static constexpr type 
    value_helper( Op< Args... > const& op, WithT const& with, seq< Is... > )
    { return { sub_argument( get_argument< Is >( op ), with )... }; }

public:
    static constexpr type 
    value( Op< Args... > const& op, WithT const& with )
    { return value_helper( op, with, make_seq< sizeof...( Args )>{} ); }
};

//////////////////////////
/// Predicate Matches ///
////////////////////////
///
/// @brief trait to reference a matched variable from an expression predicate
template< typename Var, typename ExprT >
struct match: std::tuple< Var, ExprT >
{
    using variable_type = Var;
    using expression_type = ExprT;

    constexpr variable_type const& var() const
    { return std::get< 0 >( *this ); }

    constexpr expression_type const& expression() const
    { return std::get< 1 >( *this ); }

    constexpr match() = default;
    constexpr match( match const& ) = default;
    constexpr match( variable_type const& var, expression_type const& expr ):
        std::tuple< Var, ExprT >{ var, expr } { }
};

struct no_match;

template< typename MatchT >
using match_variable_t = MatchT::variable_type;

template< typename MatchT >
using match_expression_t = MatchT::expression_type;

namespace detail {

template< typename MatchT >
struct IsMatch: integral_constant< bool, false > { };

template< typename Var, typename ExprT >
struct IsMatch< match< Var, ExprT >>: integral_constant< bool, true > { };

template< typename MatchT >
concept match = IsMatch< MatchT >::value;

// represents a Sub<...> that has been matched against an expression
//
template< typename ExprT, match... Matches >
struct SubMatches
{
    using expression_type = ExprT;
    using matches_tuple = tuple< Matches... >;

    typedef make_seq< sizeof...( Matches )> for_matches;

    constexpr expression_type const& expression() const
    { return _expr; }

    constexpr matches_tuple const& matches() const
    { return _matches; }

private:

    template< typename Var, typename Seq >
    struct MatchHelper;

    template< typename Var >
    struct MatchHelper< Var, seq< >>
    { static_assert( false, "variable was not matched" ); };

    template< variable Var, size_t J, size_t... Js >
    requires( var_id_v< Var > == 
        var_id_v< typename std::tuple_element_t< J, matches_tuple >::variable_type >)
    struct MatchHelper< Var, seq< J, Js... >>
    { 
        using type = std::tuple_element_t< J, matches_tuple >::expression_type;
        static constexpr type value( matches_tuple const& matches )
        { return std::get< J >( matches ).expression(); }
    };

    template< variable Var, size_t J, size_t... Js >
    requires( Var::id != std::tuple_element_t< J, matches_tuple >::variable_type::id )
    struct MatchHelper< Var, seq< J, Js... >>:
        MatchHelper< Var, seq< Js... >> { };

public:
    template< typename Var >
    constexpr typename MatchHelper< Var, for_matches >::type const&
    sub_for( Var var = {} ) const
    { return MatchHelper< Var, for_matches >::value( var ); }

    constexpr SubMatches() = default;
    constexpr SubMatches( SubMatches const& ) = default;
    constexpr SubMatches( expression_type const& expr, Matches const&... matches ):
        _expr{ expr }, _matches{ matches... } { }

private:
    expression_type _expr;
    matches_tuple _matches;
};

} // namespace detail
  
///////////////////////////////
/// Predicate: ForVar< I > ///
/////////////////////////////
///
/// @brief predicate class for the Ith variable id 
template< size_t I >
struct ForVar
{
    // default case
    template< typename TestT >
    struct Is: integral_constant< bool, false > { };

    template< variable Var >
    struct Is< Var >: integral_constant< bool, I == var_id_v< Var >> { };
};

//////////////////////////////////////////
/// Predicate: ForExpression< ExprT > ///
////////////////////////////////////////
/// 
/// Matches an expression of type ExprT including matching dependent variables
/// against sub-expressions and yielding them as match< Var, ExprU > in the
/// matches_type typedef.
///
template< typename ExprT >
struct ForExpression;
/// 
/// Case: No Dependent Vars in Expression to be matched
///
/// @brief predicate for matching an expression which contains no dependent
/// variables
template< typename ExprT >
requires( free_variables_t< ExprT >::size == 0 )
struct ForExpression< ExprT >
{
    // default case does not match
    template< typename TestT >
    struct Is: integral_constant< bool, false > 
    {  using matches_type = tuple<>; };

    // expression matches verbatim, so no variable matches need to be tracked
    template< >
    struct Is< ExprT >: integral_constant< bool, true > 
    { using matches_type = tuple<>; };
};
/// 
/// Case: Match a variable against any part of the expression whose result_type 
///       is the same as the variable's result_type
///
/// @brief specialization for individual variables
///
/// TODO: I believe this should be thee same logic as is_
template< variable Var > 
requires( variable< Var >)
struct ForExpression< Var >
{
    // variables match anything with the appropriate result type 
    template< typename TestT >
    struct Is: integral_constant< bool, 
        is_same_v< result_t< Var >, result_t< TestT >>> 
    { using matches_type = tuple< match< Var, TestT >>; };
};

/// @brief specialization for compound expressions with dependent variables
///
template< template< typename... > class Op, typename... Args >
requires( compound_expression< Op< Args... >> and 
    is_greater( free_variables_t< Op< Args... >>::size, 0 ))
struct ForExpression< Op< Args... >> {
private:
    using expression_type = Op< Args... >;
    using arguments_tuple = tuple< Args... >;

    // yields the first match from ...Matches with the variable id equal
    // to VarId
    template< size_t VarId, typename... Matches >
    struct ChooseMatchByVarId;

    // null case should never happen
    template< size_t VarId >
    struct ChooseMatchByVarId< VarId >
    { 
#ifndef NDEBUG
        static_assert( false, 
            "no expression found while searching for variable" ); 
#endif
        using type = no_match;
    };

    // if the first id matches yield the corresponding expression type
    template< size_t I, size_t J, typename U, typename ExprU, typename... Rest >
    requires( I == J )
    struct ChooseMatchByVarId< I, match< Var< J, U >, ExprU >, 
        Rest... >
    { using type = match< Var< J, U >, ExprU >; };

    // if the first id does not match test the remaining matches
    template< size_t I, size_t J, typename U, typename ExprU, typename... Rest >
    requires( I != J )
    struct ChooseMatchByVarId< I, match< Var< J, U >, ExprU >, 
        Rest... >: ChooseMatchByVarId< I, Rest... >
    { };

    // helpers to extract a tuple of unique variable matches from a list
    template< typename Seq, typename... Matches >
    struct UniqueMatchesHelper;

    template< size_t... Is, typename... Matches >
    struct UniqueMatchesHelper< seq< Is... >, Matches... >
    {
        // collect the unique variable indices from all the matches
        using unique_variable_id_seq = sort_unique_seq< seq< 
            match_variable_t< Matches...[ Is ]>::id... >>;

        // helper to find matched expression for each unique variable
        // indexed by Seq
        template< typename Seq >
        struct Helper;

        template< size_t... Js >
        struct Helper< seq< Js... >>
        { using matches_type = tuple< typename ChooseMatchByVarId< Js, 
            Matches... >::type... >; };

        using matches_type = Helper< unique_variable_id_seq >::matches_type;
    };
    
    template< typename... Matches >
    struct UniqueMatches: UniqueMatchesHelper< make_seq< sizeof...( Matches )>,
        Matches... >
    { };

    // helper to test the predicate matches from our arguments
    template< typename... >
    struct AreArgumentMatchesCompatible;

    template< >
    struct AreArgumentMatchesCompatible<>: integral_constant< bool, true >
    { using matches_type = tuple<>; };

    template< typename MatchesT >
    struct AreArgumentMatchesCompatible< MatchesT >: integral_constant< bool, 
        MatchesT::value >
    { using matches_type = MatchesT::matches_type; };

    // helper class for comparing two tuples of matched variables
    template< typename LeftTuple, typename RightTuple, typename Seq >
    struct CompatibilityHelper;

    // if one of the variable matches is empty we automatically succeed
    // and use the other list of matches
    template< typename... LeftMatches >
    struct CompatibilityHelper< tuple< LeftMatches... >, tuple< >, seq< >>:
        integral_constant< bool, true >
    { using matches_type = tuple< LeftMatches... >; };

    template< typename... RightMatches >
    requires( is_greater( sizeof...( RightMatches ), 0 )) // remove ambiguity
    struct CompatibilityHelper< tuple< >, tuple< RightMatches... >, seq< >>:
        integral_constant< bool, true >
    { using matches_type = tuple< RightMatches... >; };

    // neither side is empty
    template< typename LeftMatches, typename RightMatches, size_t... Is >
    requires( 
        is_greater( tuple_size_v< typename LeftMatches::matches_type >, 0 ) and
        is_greater( tuple_size_v< typename RightMatches::matches_type >, 0 ))
    struct CompatibilityHelper< LeftMatches, RightMatches, seq< Is... >>
    {
        static constexpr size_t left_matches_size = tuple_size_v< typename
            LeftMatches::matches_type >;

        static constexpr size_t right_matches_size = tuple_size_v< typename 
            RightMatches::matches_type >;

        template< size_t I >
        using left_match_t = tuple_element_t< I / right_matches_size, typename 
            LeftMatches::matches_type >;

        template< size_t I >
        using right_match_t = tuple_element_t< I % right_matches_size, typename
            RightMatches::matches_type >;

        // if the variable indices match the corresponding expression types must be equal
        static constexpr bool value = ((
            match_variable_t< left_match_t< Is >>::id != 
                match_variable_t< right_match_t< Is >>::id or is_same_v<
                    match_expression_t< left_match_t< Is >>,
                        match_expression_t< right_match_t< Is >>> ) and ... );

        template< typename SeqLeft, typename SeqRight >
        struct UniqueMatchesHelper;

        template< size_t... Js, size_t... Ks >
        struct UniqueMatchesHelper< seq< Js... >, seq< Ks... >>:
            UniqueMatches< left_match_t< Js >..., right_match_t< Ks >... >
        { };

        // our matches are the unique set of matches from the combined left and right matches
        using matches_type = UniqueMatchesHelper< make_seq< left_matches_size >, 
            make_seq< right_matches_size >>::matches_type;
    };

    template< typename LeftMatches, typename RightMatches >
    requires( not LeftMatches::value or not RightMatches::value )
    struct AreArgumentMatchesCompatible< LeftMatches, RightMatches >:
        integral_constant< bool, false >
    { using matches_type = tuple<>; };

    template< typename LeftMatches, typename RightMatches >
    requires( LeftMatches::value and RightMatches::value )
    struct AreArgumentMatchesCompatible< LeftMatches, RightMatches >:
        CompatibilityHelper< LeftMatches, RightMatches,
            make_seq< tuple_size_v< typename LeftMatches::matches_type > * 
                tuple_size_v< typename RightMatches::matches_type >>>
    { };

    // when we have more than two variable matching tuples to compare, check
    // the tail.  if that is compatible, compare those matches to the first
    template< typename First, typename... Rest >
    requires( is_greater( sizeof...( Rest ), 1 ) and 
        AreArgumentMatchesCompatible< Rest... >::value )
    struct AreArgumentMatchesCompatible< First, Rest... >:
        AreArgumentMatchesCompatible< First, AreArgumentMatchesCompatible< Rest... >>
    { };

    // when the tail is incompatible simply evaluate to false with an empty
    // tuple of matches
    template< typename First, typename... Rest >
    requires( is_greater( sizeof...( Rest ), 1 ) and not
        AreArgumentMatchesCompatible< Rest... >::value )
    struct AreArgumentMatchesCompatible< First, Rest... >:
        integral_constant< bool, false >
    { using matches_type = tuple<>; };
        
    // helper allows us to compare the arguments of this operation pairwise
    template< typename Seq, typename... TestArgs >
    struct Helper;

    // helper recurses when our operation matches the operation of the
    // expression being tested.  
    template< size_t... Js, typename... TestArgs >
    struct Helper< seq< Js... >, TestArgs... >: 
        AreArgumentMatchesCompatible< typename 
            ForExpression< std::tuple_element_t< Js, arguments_tuple >>::
                template Is< TestArgs...[ Js ]>... >
    { };

public:
    // by default we will not match TestT
    template< typename TestT >
    struct Is: integral_constant< bool, false > 
    { using matches_type = tuple<>; };

    // this predicate requires the operation of the compound expression to match
    // the helper will determine if the arguments match.
    template< typename... TestArgs >
    requires( std::tuple_size_v< arguments_tuple > == sizeof...( TestArgs ))
    struct Is< Op< TestArgs... >>:
        Helper< make_seq< sizeof...( Args )>, TestArgs... > 
    { };
};

} // namespace expressions

#endif // __EXPRESSIONS_PREDICATE_HPP__ 

