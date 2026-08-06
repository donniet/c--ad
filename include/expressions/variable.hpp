////////////////////////////////////
/// Variables and Substitutions ///
//////////////////////////////////
/// 
/// substitute( expr, subs... ) ->
///     (1) identify free variables in expr -> free_vars
///     (2) pair each free variable with the appropriate ...subs -> binding
///     (3) sort the bindings by dependencies -> sorted_bindings
///     sub_for( expr sorted_bindings )... ->
///         
///          (i) if first order variable: predicate replace
///         (ii) if second order variable: 
///
/// second and higher order variables:
///
/// auto g = f(x,y);  // f,x,y free; no bound
/// auto h = g(z,z);  // g,z free; f <- g, x <- z, y <- z
/// auto l = h(3);    // h free; f <- g <- h, x <- z <- 3, y <- z <- 3
/// auto val = l(x*y) // no free; f <- g <- h <- (x*y), x <- z <- 3, y <- z <- 3
///                   // -> expression evaluated here
///
/// assert( 9 == val 
///     and 9 == l(x*y) 
///     and 9 == h(3)(x*y) 
///     and 9 == g(z,z)(3)(x*y) 
///     and 9 == f(x,y)(z,z)(3)(x*y) );
///
///
/// Applying a substitution is done variable-by-variable yet the substituted
/// arguments may themselves contain variables.
///
///     ````
///     using Var = Var;
///     usinv Sub = Sub;
///     using v1 = Var< 1, int >; 
///     using v2 = Var< 2, int >;
///     using v3 = Var< 3, int >;
///
///     v1 x; v2 y; v3 f;
///
///     auto g = f(x, y);
///     auto h = f(y, x);
///
///     assert( g( 4, 2 )( x / y ) == 2 and
///             g( 4, 2 )( x / y ) == h( 2, 4 )( x / y ) );
///     //...
///     ```
///
/// This creates additional problems since unique_variables are sorted by id
/// and that order is used to identify the matching substitution arguement. 
/// But were we to apply the substitutions in that naive order we would see
/// inconsistent results of expressions.
///
///     h( 2, 4 ); // implies 2 is substituted for y, yet we said we apply 
///                // substitutions in variable id order, and x is lower than
///                // y!
///
/// To resolve this we will abuse the notation. Sub expressions will
/// be constructed in a way that identifies the variable being substituted, then
/// executes it in order of id. 

/*******************************************************
 * DESIGN DECISION * Direct and Indirect Substitutions *
 *******************************************************
 * Substituting into an expression via operator() is a DIRECT SUBSTITUTION.
 * When evaluating a direct substitution variables are replaced by expressions.
 * These replacements are called INDIRECT SUBSTITUTIONs.
 **/

/***********************************************************
 * DESIGN DECISION * Tuples of Expressions are Expressions *
 ***********************************************************/

/************************************************************
 * DESIGN DECISION * Tensors of Expressions are Expressions *
 ************************************************************/

/************************************
 * DESIGN DECISION * Free Vars *
 ************************************
 * Vars may be "free" or "bound".
 * 
 * Free Var:
 * - Var< id, value_type > is free by default.
 * - Sub< ExprT, Subs... > binds free variables in ExprT with
 *   ...Subs in FreeVars::id order.
 * - No other operation may bind a variable.
 * - A variable that is not free is bound.
 **/

/******************************************
 * DESIGN DECISION * Var Subs Are Allowed *
 ******************************************
 * Ids for second-order variables correspond to substitution argument order,
 * but are re-ordered in resultant substitution expression by the nested
 * variable Id 
 * 
 * Second Order Var: Var< I, Var< J, T >>
 *          temporary variable id --^            ^
 *          original variable id  ---------------
 *
 * 
 *
 **/

/**********************************************
 * DESIGN DECISION * Partial Subs Are Allowed *
 **********************************************
 * Partial substitutions are allowed.  Any unbound variables are FREE in 
 * resultant expression.  Over-substitutions are also allowed.  Unmatched
 * substitution arguments are ignored.
 **/

/****************************************************************
 * DESIGN DECISION * substitute re-recurses until terminal case *
 ****************************************************************/

///
/// auto g = f( x, y ); // Sub< Var< 6, v3 >, Var< 4, v1 >, Var< 5, v2 >>
/// auto h = f( y, x ); // Sub< Var< 6, v3 >, Var< 5, v1 >, Var< 4, v2 >>
///
/// auto p = g( 4, 2 ); // Sub< Sub< Var< 6, v3 >, Var< 4, v1 >, Var< 5, v2 >>,
///                     //     int, int >{{ "f", "x", "y" }, 4, 2 };
///                         
/// auto q = h( 2, 4 ); // Sub< Sub< Var< 6, v3 >, Var< 5, v1 >, Var< 4, v2 >>,
///                     //     int, int >{{ "f", "y", "x" }, 2, 4 };
///
/// int vg = p( x / y ); 
/// int vq = q( x / y );
///
///  ┌─ Sub is constructed and evaluated:
///  │
///  │  Sub< Sub< Sub< Var< 6, v3 >, Var< 4, v1 >, Var< 5, v2 >>, int, int >, 
///  │      Quotient< v1, v2 >>{{{ "f", "x", "y" }, 4, 2 }, { "x", "y" }}()
///  │  
///  ├─ Evaluation recurses and second-order variables are replaced by
///  │  substitutions.
///  │                        
///  │  Sub< Sub< Var< 6, v3 >, Sub< v1, int >, Sub< v2, int >>,
///  │      Quotient< v1, v2 >>{{ "f", { "x", 4 }, { "y", 2 }}, { "x", "y" }}()
///  │  Sub< Quotient< v1, v2 >, Sub< v1, int >, Sub< v2, int >>
///  │      {{ "x", "y" }, { "x", 4 }, { "y", 2 }}()
///  │  Sub< Quotient< v1, v2 >, int, int >
///  │      {{ "x", "y" }, 4, 2 }()
///  │  Quotient< int, int >{ 4, 2 }()
///  │  2
///  │ 
///  │  Sub< Sub< Sub< Var< 6, v3 >, Var< 5, v1 >, Var< 4, v2 >>, int, int >,
///  │      Quotient< v1, v2 >>{{{ "g", "x", "y" }, 2, 4 }, { "x", "y" }}()
///  │  Sub< Sub< Var< 6, v3 >, Sub< v1, int >, Sub< v2, int >>,
///  │      Quotient< v1, v2 >>{{ "g", { "x", 4 }, { "y", 2 }}, { "x", "y" }}()
///  │  Sub< Quotient< v1, v2 >, int, int >
///  │      {{ "x", "y" }, 4, 2 }()
///  │  Quotient< int, int >{ 4, 2 }()
///  │  2
///  │       
///  │  
///
///  *** SPECIAL CASES ***
///  We will forbid substituting expressions directly into variables:
///
///  auto _ = f( y + x, x ); // not allowed because l( 2, 4 ) is now unclearlly defined
///
///  auto _ = f( x, x );     // not allowed because l( 2, 4 ) is malformed 



#ifndef __EXPRESSIONS_VARIABLE_HPP__
#define __EXPRESSIONS_VARIABLE_HPP__

#include "expressions/forward_decl.hpp"
#include "expressions/unique_variables.hpp"
#include "expressions/constant.hpp"
#include "expressions/static_value.hpp"

namespace expressions {

////////////////////
/// Substituter ///
//////////////////
/// 
/// helper class to evaluate a substitution 
template< typename ExprT, typename... Subs >
struct Substituter;

template< typename ExprT, typename... Args >
using substitute_t = Substituter< ExprT, Args... >::type;

/// @brief substitute method invokes the Substituter expression transformer
template< typename ExprT, typename... Args >
constexpr substitute_t< make_expression_t< ExprT >, 
    make_expression_t< Args >... >
substitute( ExprT expr, Args... args )
{ return Substituter< make_expression_t< ExprT >, 
    make_expression_t< Args >... >::value( make_expression( expr ), 
        make_expression( args )... ); }

template< typename ExprT >
struct BoundVars;

/// try just using bound_vars
template< typename ExprT, typename... Ss >
constexpr bool is_compatible_substitution_v = 
    BoundVars< Sub< make_expression_t< ExprT >, make_expression_t< Ss >... >>::
        is_compatible;

/////////////
/// Func ///
///////////
///
/// @brief unbound function specialization
template< typename ExprT, variable... Vars >
struct Func: std::tuple< ExprT, Vars... >
{
    using formula_type = ExprT;
    using arguments_tuple = std::tuple< formula_type, Vars... >;
    using this_type = Func< formula_type, Vars... >;

private:
    static constexpr size_t vars_size = sizeof...( Vars );
    typedef make_seq< vars_size > for_vars;

    template< typename... Args >
    struct VarsSub
    { static constexpr bool is_compatible = false; };

    template< typename... Args >
    requires( sizeof...( Args ) == vars_size )
    struct VarsSub< Args... >
    {
        template< typename Seq >
        struct Helper;

        template< size_t... Is >
        struct Helper< seq< Is... >>
        {
            // we are compatible if each ...Args is substitutible into the 
            // corresponding ...Vars as an expression
            static constexpr bool
            is_compatible = ( is_compatible_substitution_v< 
                Vars...[ Is ], Args...[ Is ]> and ... and true );

            using type = Sub< this_type, Args... >;
            static constexpr type 
            value( Func const& func, Args const&... args )
            { return { func, args... }; }
        };

        static constexpr bool 
        is_compatible = Helper< for_vars >::is_compatible;

        using type = Helper< for_vars >::type;

        static constexpr type
        value( Func const& func, Args const&... args )
        { return Helper< for_vars >::value( func, args... ); }
    };

public:
    constexpr formula_type const&
    formula() const
    { return std::get< 0 >( *this ); }

    template< size_t K >
    constexpr Vars...[ K ]
    var() const
    { return std::get< 1 + K >( *this ); }

    // substitutes make_expression( args )... in for vars...
    template< typename... Args >
    requires( VarsSub< make_expression_t< Args >... >::is_compatible )
    constexpr typename VarsSub< make_expression_t< Args >... >::type
    operator ()( Args const&... args ) const
    { return VarsSub< make_expression_t< Args >... >::value( *this, 
        make_expression( args )... ); }

    constexpr Func( formula_type const& func, 
        Vars const&... vars ): arguments_tuple{ func, vars... } 
    { };
    constexpr Func( Func const& ) = default;
    constexpr Func() = default;
};

////////////
/// Var ///
//////////
///
/// @brief a placeholder in an expression whose value can change
/// @tparam I is the id in the declared variables to this variable
/// @tparam T is the value_type of this variable
///
/// Two variables with the same identifier I are considered equal and MUST
/// have the same value_type T. The identifier I is also used to sort a
/// list of free variables when substituting.
///
/// The value_type T must a literal type and may be an expression type, in 
/// which case the result_type of the variable will be the result_t< T >. 
/// If the value_type is not an expression, then any substitution into the
/// variable must only match result_types with the type T.  If the value_type
/// is an expression itself then the value being substituted must also be
/// substitutable into the value_type.
///
/// is_valid_substitution< Var< I, T >, ExprT >:
/// - is_convertible_v< result_t< ExprT >, result_t< T >> and
/// - 
///
/// The result_type of a variable is the result_type of the value_type of
/// the variable. If the value_type is not an expression then it is the 
/// result type. If the value_type is an expression then it is a template
/// for what is substituted in for the variable.
///
/// f(x)(x+1) =>
///
/// Sub( Var< 0, Func< Var< 0, int >, Var< 1, int >>>{}, 
///     Plus< Var< 1, int >, Const< 2 >>>{} )
///
/// @brief variable implementation
template< size_t I, typename T >
struct Var
{ 
    using value_type = T;
    static constexpr size_t id = I;

private:
    using this_type = Var< id, value_type >;

public:
    /// @brief operator= is overriden to construct a SetVarValue 
    /// expression
    /// HACK: this is not typical of C++ classes and may cause problems
    constexpr SetVarValue< Var > 
    operator=( value_type const& other ) const
    { return { *this, other }; }

    constexpr string const& 
    name() const 
    { return _name; }

    constexpr void 
    set_name( string const& new_name )
    { _name = new_name; }

    // DT: consider including a value again.
    //constexpr value_type const& 
    //value() const
    //{ return _value; }

    //constexpr void
    //set_value( value_type const& val )
    //{ _value = val; }

    /// @brief substitution into a variable creates a function 
    template< variable First, variable... Rest > 
    requires( is_non_repeating_v< seq< var_id_v< First >,
        var_id_v< Rest >... >> ) 
    constexpr Func< increase_var_order_t< this_type >, First, Rest... >
    operator ()( First first, Rest... rest ) const
    { return { name(), first, rest... }; }

    //constexpr Var( string const& name = "var", value_type const& value ): 
    //    _name{ name }, _value{ value }
    //{ }
    constexpr Var( string const& name = "var", value_type const& value = {} ): 
        _name{ name } { }
    constexpr Var( Var const& ) = default; 

private:
    string _name;
    //value_type _value;
};

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

//////////////////
/// Free Vars ///
////////////////
///
/// @brief trait to for free variables in an expression. The default 
///        implementation is empty to handle non-expressions and terminal
///        cases. This class will be specialized
template< typename ExprT >
struct GetFreeVars
{ 
    using type = unique_variables< >; 
    static constexpr type value( ExprT const& )
    { return {}; }
};

/// @brief unique_variable list of free variables in an expression
template< typename ExprT >
using free_variables_t = 
    GetFreeVars< std::remove_cv_t< ExprT >>::type;

template< typename ExprT >
constexpr free_variables_t< ExprT > 
get_free_variables( ExprT const& expr )
{ return GetFreeVars< ExprT >::value( expr ); }

///////////////////
/// Bound Vars ///
/////////////////
///
///
/// @brief container of bound variables and the substitution arguments
///        that will replace them (referents)
template< typename ExprT >
struct BoundVars
{ 
    using expression_type = ExprT;

    // number of bound variables
    static constexpr size_t size = 0;

    static constexpr bool is_compatible = true;

    // sorted list of variable types
    using variable_set_type = unique_variables< >; 

    static constexpr variable_set_type
    variable_set( expression_type const& )
    { return {}; }

    // parallel tuple to zip with referents
    using variable_tuple = tuple< >;

    // type of a tuple that will contain the values to-be-substituted
    // (referents)
    using referent_tuple = tuple< >;

    // sorted index sequence of binding order
    using for_binding_order = seq< >;

    static constexpr variable_tuple
    variables( expression_type const& )
    { return {}; }

    // method to find the values that will be substituted for the
    // variables in variable_set
    static constexpr referent_tuple
    referents( expression_type const& )
    { return {}; }
};

template< typename T >
using bound_variables_t = BoundVars< T >;

template< typename ExprT >
constexpr bound_variables_t< ExprT >
bound_variables( ExprT const& expr )
{ return { expr }; }

////////////////////////////////////
/// Open and Closed Expressions ///
//////////////////////////////////
///
/// @brief an expression is closed if it contains no free variables
template< typename T >
struct IsClosedExpression: IsExpression< T > { };

template< expression T >
struct IsClosedExpression< T >: std::integral_constant< bool,
    ( free_variables_t< T >::size == 0 )> { };

/// @brief A closed expression contains no free (unbound) variables
template< typename T >
concept closed_expression = IsClosedExpression< T >::value; 

/// @brief an open expression contains free (unbound) variables.
template< typename T >
concept open_expression = not closed_expression< T >;

///////////////////////////////////
/// Substitution Compatibility ///
/////////////////////////////////
///
/// Trait to verify whether a set of substitution arguments may bind the free
/// variables in an expression.
///
namespace detail {
template< typename ExprT, typename... Subs >
struct IsCompatibleSub;

template< size_t Id, typename ExprT, typename Sub >
struct IsCompatibleVarSub: std::false_type { };

/// @brief for first order variables we are compatible if the result of our
///        sub's type is convertible to the variable's non-expression type
template< size_t Id, typename T, typename S >
requires( not expression< T > )
struct IsCompatibleVarSub< Id, Var< Id, T >, S >: 
    std::integral_constant< bool, std::is_convertible_v< T, result_t< S >>> 
{ }; 

/// @brief for higher-order variables, we are compatible if we are compatible
///        to sub into the nested expression type
template< size_t Id, expression ExprT, typename S >
struct IsCompatibleVarSub< Id, Var< Id, ExprT >, S >:
    std::integral_constant< bool, IsCompatibleVarSub< Id, ExprT, S >::value >
{ };

/// @brief we are compatible to sub into a function if we can sub into the 
///        function's formula or one of it's variables
template< size_t Id, typename FormulaT, variable... Vars, typename S >
struct IsCompatibleVarSub< Id, Func< FormulaT, Vars... >, S >:
    std::integral_constant< bool, IsCompatibleVarSub< Id, FormulaT, S >::value 
        or (( IsCompatibleVarSub< Id, Vars, S >::value ) or ... or false )>
{ };

/// @brief we are compatible to sub into a tuple of expressions if we are
///        compatible with at least one element
template< size_t Id, typename... Exprs, typename S >
struct IsCompatibleVarSub< Id, std::tuple< Exprs... >, S >:
    std::integral_constant< bool, 
        (( IsCompatibleVarSub< Id, Exprs, S >::value ) or ... )>
{ };

/// @brief we are compatible to sub into a substitution expression if we can
///        sub into the evaluated expression
template< size_t Id, typename FormulaT, typename... Args, typename S >
struct IsCompatibleVarSub< Id, Sub< FormulaT, Args... >, S >:
    IsCompatibleVarSub< Id, substitute_t< FormulaT, Args... >, S >
{ };

/// @brief we are compatible to sub into a compound expression if we are
///        compatible with at least one arg (same as tuples)
template< size_t Id, template< typename... > class Op, typename... Args, 
    typename S >
requires( compound_expression< Op< Args... >> and not
    is_substitution_expression_v< Op< Args... >> )
struct IsCompatibleVarSub< Id, Op< Args... >, S >:
    IsCompatibleVarSub< Id, std::tuple< Args... >, S > 
{ };

/// @brief we are compatible to sub into a tensor if we are compatible with
///        at least one element (same as tuples)
template< size_t Id, shape Shp, typename... Ts, typename S >
struct IsCompatibleVarSub< Id, Tensor< Shp, Ts... >, S >:
    IsCompatibleVarSub< Id, std::tuple< Ts... >, S >
{ };

} // namespace detail


//////////////////////////////
/// Complete Substitution ///
////////////////////////////
///
/// @brief trait to identify whether a substitution uses all the original free 
/// variables in an expression
template< typename ExprT, typename... Subs >
struct IsCompleteSub: std::integral_constant< bool,
    bound_variables_t< Sub< ExprT, Subs... >>::is_complete > 
{ };

template< typename ExprT, typename... Subs >
constexpr bool is_complete_substitution_v = 
    IsCompleteSub< ExprT, Subs... >::value;

//////////////////////
/// SubstituteFor ///
////////////////////
///
/// @brief Expression representing the substitution of variable I in ExprT with
///        SubU
template< size_t Id, typename ExprT, typename SubU >
struct SubFor {
private:
    using variable_set = free_variables_t< ExprT >;
    using expression_type = ExprT;
    using referent_type = SubU;
    static constexpr size_t id = Id;

    template< typename T >
    struct Parser
    { 
        using type = T;
        static constexpr type
        value( T const& expr, referent_type const& )
        { return expr; }
    };

    //template< auto Value >
    //struct Parser< Constant< Value >>
    //{ 
    //    using type = std::remove_cv_t< decltype( Value )>;
    //    static constexpr type
    //    value( Constant< Value > const&, referent_type const& )
    //    { return Value; }
    //};

    //template< typename T >
    //struct Parser< StaticValue< T >>
    //{
    //    using type = T;
    //    static constexpr type
    //    value( StaticValue< T > const& expr, referent_type const& )
    //    { return expr.get_value(); }
    //};

    template< variable Var >
    requires( var_id_v< Var > != id )
    struct Parser< Var >
    { 
        using type = Var;
        static constexpr type
        value( Var const& var, referent_type const& )
        { return var; }
    };

    template< variable Var >
    requires( var_id_v< Var > == id ) // and var_order_v< Var > == 1 )
    struct Parser< Var >
    {
        using type = referent_type;
        static constexpr type
        value( Var const&, referent_type const& sub )
        { return sub; }
    };

    /// Case: higher-order variables recurse
//    template< variable Var >
//    requires( var_id_v< Var > == id and is_greater( var_order_v< Var >, 1 ))
//    struct Parser< Var >
//    {
//        using type = SubFor< id, var_value_t< Var >, referent_type >::type;
//        static constexpr type
//        value( Var const& var, referent_type const& sub )
//        { return SubFor< id, var_value_t< Var >, referent_type >::
//            value( var.get_value(), sub ); }
//    };

    /// Case: functions: we only sub into the formula
    template< expression U, variable... Args >
    struct Parser< Func< U, Args... >>
    {
        using type = Parser< U >::type;
        static constexpr type
        value( Func< U, Args... > const& func, referent_type const& sub )
        { return Parser< U >::value( func.formula(), sub ); }
    };

    // do we need a case for Sub<...> expressions?
    template< template< typename... > class Op, typename... Args >
    requires( compound_expression< Op< Args... >> )
    struct Parser< Op< Args... >>
    {
        static constexpr size_t size = sizeof...( Args );
        typedef make_seq< size > for_args;

        template< typename Seq >
        struct Helper;

        template< size_t... Is >
        struct Helper< seq< Is... >>
        {
            using type = Op< typename Parser< Args...[ Is ]>::type... >;
            static constexpr type
            value( Op< Args... > const& expr, referent_type const& sub )
            { return { Parser< Args...[ Is ]>::value( 
                get_argument< Is >( expr ), sub )... }; }
        };

        using type = Helper< for_args >::type;
        static constexpr type
        value( Op< Args... > const& expr, referent_type const& sub )
        { return Helper< for_args >::value( expr, sub ); }
    };

public:
    using type = Parser< expression_type >::type;
    static constexpr type
    value( expression_type const& expr, referent_type const& sub )
    { return Parser< expression_type >::value( expr, sub ); }
};

// helper function
template< size_t Id, typename ExprT, typename SubU >
constexpr typename SubFor< Id, make_expression_t< ExprT >, 
    make_expression_t< SubU >>::type
sub_for( ExprT const& expr, SubU const& sub )
{ return SubFor< Id, make_expression_t< ExprT >, 
    make_expression_t< SubU >>::value( make_expression( expr ), 
        make_expression( sub )); }

/// @brief expression transformer that replaces a list of variable ids with 
///        ...Subs in order
template< typename ExprT, typename IdSeq, typename... Subs >
struct SubstituteForIdSeq;

/// @brief terminal case for non-expressions and open expressions is idempotent
template< typename ExprT, size_t... Ids, typename... Subs >
requires( not expression< ExprT > or ( open_expression< ExprT > and 
    not ( free_variables_t< ExprT >::contains_id( Ids ) or ... or false )))
struct SubstituteForIdSeq< ExprT, seq< Ids... >, Subs... >
{
    using type = ExprT;
    static constexpr type
    value( ExprT const& expr, Subs const&... )
    { return expr; }
};

/// @brief terminal case for closed expressions returns the result
template< typename ExprT, size_t... Ids, typename... Subs >
requires( closed_expression< ExprT > )
struct SubstituteForIdSeq< ExprT, seq< Ids... >, Subs... >
{
    using type = result_t< ExprT >;
    static constexpr type
    value( ExprT const& expr, Subs const&... )
    { return expr(); }
};

/// @brief recursive case for open expressions that contain the first Id as 
///        a free variable
template< typename ExprT, size_t FirstId, size_t... RestIds, 
    typename FirstSub, typename... RestSubs >
requires( open_expression< ExprT > and 
    free_variables_t< ExprT >::contains_id( FirstId ))
struct SubstituteForIdSeq< ExprT, seq< FirstId, RestIds... >, 
    FirstSub, RestSubs... >
{
    using type = SubstituteForIdSeq< typename
        SubFor< FirstId, ExprT, FirstSub >::type, 
            seq< RestIds... >, RestSubs... >::type;

    static constexpr type
    value( ExprT const& expr, FirstSub const& first, RestSubs const&... rest )
    { return SubstituteForIdSeq< typename 
        SubFor< FirstId, ExprT, FirstSub >::type, 
            seq< RestIds... >, RestSubs... >::value(
                SubFor< FirstId, ExprT, FirstSub >::value( expr, first ),
                    rest... ); }
};

/// @brief recursive case for open expressions that does not contain the first 
///        Id as a free variable
template< typename ExprT, size_t FirstId, size_t... RestIds, 
    typename FirstSub, typename... RestSubs >
requires( open_expression< ExprT > and 
    not free_variables_t< ExprT >::contains_id( FirstId ))
struct SubstituteForIdSeq< ExprT, seq< FirstId, RestIds... >, 
    FirstSub, RestSubs... >
{
    using type = SubstituteForIdSeq< ExprT, 
        seq< RestIds... >, RestSubs... >::type;

    static constexpr type
    value( ExprT const& expr, FirstSub const& first, RestSubs const&... rest )
    { return SubstituteForIdSeq< ExprT, 
        seq< RestIds... >, RestSubs... >::value( expr, rest... ); }
};

//////////////////////////////////////
/// Referent to Variable Matching ///
////////////////////////////////////
/// 
/// In the default case we match referents to variables in increasing variable
/// id order, which is how they are indexed in unique_variables
template< typename ExprT >
struct DirectSubOrder
{
    using formula_type = ExprT;

    using type = GetFreeVars< formula_type >::type::variables_tuple;
    static constexpr type
    value( formula_type const& expr )
    { return GetFreeVars< formula_type >::value( expr ).as_tuple(); }
};

/// @brief functions match referents in the order of Vars... 
template< typename ExprT, variable... Vars >
struct DirectSubOrder< Func< ExprT, Vars... >>
{
    using formula_type = Func< ExprT, Vars... >;

    using type = std::tuple< Vars... >;
    static constexpr type
    value( formula_type const& expr )
    {
        static constexpr make_seq< sizeof...( Vars )> for_vars;
        
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
        { return { expr.template var< Is >()... }; };

        return helper( for_vars );
    }
};

/// @brief container for bound variables in a substitution expression
template< typename ExprT, typename... Subs >
//requires( not is_function_v< ExprT > )
struct BoundVars< Sub< ExprT, Subs... >> {
private:
    using expression_type = Sub< ExprT, Subs... >;
    using formula_expression = ExprT;

    // determine the order we should match variables to substituted expressions
    using formula_variable_tuple = DirectSubOrder< formula_expression >::type;
    static constexpr size_t formula_variables_size = 
        std::tuple_size_v< formula_variable_tuple >; 

    static constexpr formula_variable_tuple
    formula_variables( expression_type const& expr )
    { return DirectSubOrder< formula_expression >::value( 
        std::get< 0 >( expr )); }

    using variable_sub_order = DirectSubOrder< ExprT >;

public:
    static constexpr size_t size = std::min( 
        formula_variables_size, sizeof...( Subs ));
    static constexpr bool is_complete = 
        formula_variables_size == size;



private:
    // TODO: we need to extract a binding order from the expression here
    //       If it's a function then it gives a different binding order than
    //       the simple trimmer will yield.
    //
    // Maybe we need another trait: binding_order_seq< ExprT >?
    //
    // (1) identify variables and substitution expressions that are bound
    typedef make_seq< size > for_bindings;

    template< typename Seq >
    struct Trimmer;

    // trims the variables and substitutions to just those that were matched
    template< size_t... Is >
    struct Trimmer< seq< Is... >>
    { 
        using variable_set_type = make_unique_variables_t< 
            std::tuple_element_t< Is, formula_variable_tuple >... >;
        using variable_tuple = std::tuple<
            std::tuple_element_t< Is, formula_variable_tuple >... >;
        using referent_tuple = std::tuple< Subs...[ Is ]... >;

        // this is useful in our GetFreeVars implementation
        static constexpr variable_set_type
        variable_set( expression_type const& expr )
        {
            formula_variable_tuple formula_variables( expr );
            return make_unique_variables( 
                std::get< Is >( formula_variables )... );
        }
    };

    using trimmed_variable_tuple = Trimmer< for_bindings >::variable_tuple;
    using trimmed_referent_tuple = Trimmer< for_bindings >::referent_tuple;

    // (2) sort dependencies of bound expressions
    // 
    // Vars and Subs are paired.
    // 
    // logic: given two variable/expression pairs A and B 
    // - if expression(B) depends on variable(A) but expression(A) does not 
    //   depend on variable(B) we substitute B first
    // - otherwise we substitue in id order
    //
    // Truth Table:
    //                                              not(B dep A) 
    //  id(A) < id(B) | A dep B | B dep A | A < B | or (A dep B) | Undirected
    //  ---------------------------------------------------------------------
    //  True          | True    | True    | True  | True         | True 
    //  True          | True    | False   | True  | True         | False
    //  True          | False   | True    | False | False        | False
    //  True          | False   | False   | True  | True         | False
    //  False         | True    | True    | False | False        | True
    //  False         | True    | False   | True  | True         | False
    //  False         | False   | True    | False | False        | False
    //  False         | False   | False   | False | True**       | False
    //  
    // Note: Evaluation order should be the reverse of substitution order

    template< size_t I >
    using bound_variable_t = std::tuple_element_t< I, trimmed_variable_tuple >;

    template< size_t I >
    using referent_free_variables_t = GetFreeVars< 
        std::tuple_element_t< I, trimmed_referent_tuple >>::type;

    template< size_t I, size_t J >
    struct IsDependent;

    // I is dependent on J if the Ith referent's free variables contain the Jth
    // bound variable, or if the Ith variable is higher-order and it's nested 
    // expression contains an unbound instance of the Jth variable.
    template< size_t I, size_t J >
    struct IsDependent: std::integral_constant< bool, 
        referent_free_variables_t< I >::template 
            contains< bound_variable_t< J >>() or
        free_variables_t< var_value_t< bound_variable_t< I >>>::template
            contains< bound_variable_t< J >>()> 
    { };

    // handles all cases except the last exception (**)
    template< size_t I, size_t J >
    struct BindsBefore: std::integral_constant< bool,
        IsDependent< I, J >::value or not IsDependent< J, I >::value > 
    { 
        static constexpr bool is_circular = IsDependent< I, J >::value and
            IsDependent< J, I >::value; 

        //NOTE: do we care if there are circular references?
        //static_assert( not is_circular, "circular dependency detected" );
    };

    // handles the exception case (**)
    template< size_t I, size_t J >
    requires( is_greater( I, J )) 
    struct BindsBefore< I, J >: std::integral_constant< bool,
        not BindsBefore< J, I >::value > 
    { };

    template< size_t I >
    struct PreceedingCount
    {
        template< typename Seq >
        struct Helper;

        template< size_t... Is >
        struct Helper< seq< Is... >>
        { static constexpr size_t value = 
            (( BindsBefore< Is, I >::value ? 1 : 0 ) + ... ); };

        static constexpr size_t value = Helper< for_bindings >::value;
    };

    template< typename Seq >
    struct SortedIndexSeq;

    template< size_t... Is >
    struct SortedIndexSeq< seq< Is... >>
    { 
        using position_seq = seq< PreceedingCount< Is >::value... >;
        using type = sort_index_seq< position_seq >;
    };

    // (3) expose the variables and their referents in parallel tuples, and a 
    //     sequence that determines the order each variable should be replaced
    //     to preserve dependencies
    //
    // this is the sequence that must be used when replacing variables with 
    // their referents when applying a substitution expression
    using for_binding_order = SortedIndexSeq< for_bindings >::type;

public:
    // we use for_bindings here since the variable_set is always sorted by 
    // variable id anyway, and the binding_order just resorts the parallel
    // variable and referent tuples to ensure they stay parallel.
    using variable_set_type = Trimmer< for_bindings >::variable_set_type;

    using variable_tuple = Trimmer< for_binding_order >::variable_tuple;
    using referent_tuple = Trimmer< for_binding_order >::referent_tuple;

private:
    template< typename Seq >
    struct CompatibleHelper;

    template< size_t... Is >
    struct CompatibleHelper< seq< Is... >>: 
        std::integral_constant< bool, ( detail::IsCompatibleVarSub< 
            var_id_v< std::tuple_element_t< Is, variable_tuple >>,
            formula_expression, 
            std::tuple_element_t< Is, referent_tuple >>::value and 
                ... and true )>
    { };

public:
    static constexpr bool is_compatible = 
        CompatibleHelper< for_bindings >::value;

    static constexpr variable_set_type
    variable_set( expression_type const& expr )
    { return Trimmer< for_bindings >::variable_set( expr ); }

    static constexpr variable_tuple
    variables( expression_type const& expr ) 
    {
        formula_variable_tuple vars = formula_variables( expr );

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr ->
            variable_tuple
        { return { vars.template at< Is >()... }; };

        return helper( for_binding_order{} );
    }

    static constexpr referent_tuple
    referents( expression_type const& expr ) 
    {
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr ->
            referent_tuple
        { return { std::get< 1 + Is >( expr )... }; };

        return helper( for_binding_order{} );
    }
};

///////////////////////////////////
/// Substituter Implementation ///
/////////////////////////////////
///
/// Constructs new expressions by substituting ...Subs into free variables
/// of ExprT.
template< typename ExprT, typename... Subs >
struct Substituter {
//private:
    typedef make_seq< sizeof...( Subs )> for_subs;

    using formula_type = ExprT;
    using substitution_type = Sub< ExprT, Subs... >;
    using bound_variables_type = bound_variables_t< substitution_type >;
    using bound_variable_tuple = bound_variables_type::variable_tuple;
    using referent_tuple = bound_variables_type::referent_tuple;

    template< size_t N >
    using bound_variable_t = std::tuple_element_t< N, bound_variable_tuple >;

    template< size_t N >
    static constexpr size_t var_id_of = var_id_v< bound_variable_t< N >>;

    template< size_t N >
    using referent_t = std::tuple_element_t< N, referent_tuple >;

    template< size_t N >
    static constexpr bound_variable_t< N >
    bound_var( formula_type const& expr, Subs const&... subs ) 
    { return std::get< N >( bound_variables_type::variables(
        substitution_type{ expr, subs... })); }

    template< size_t N >
    static constexpr referent_t< N >
    referent( formula_type const& expr, Subs const&... subs )
    { return std::get< N >( bound_variables_type::referents(
        substitution_type{ expr, subs... })); }

    typedef make_seq< bound_variables_type::size > for_bindings;

    template< typename Seq >
    struct Helper;

    template< size_t... Is >
    struct Helper< seq< Is... >>
    {
        using subber = SubstituteForIdSeq< formula_type, 
            seq< var_id_of< Is >... >, referent_t< Is >... >;

        using type = subber::type;

        static constexpr type
        value( formula_type const& expr, Subs const&... subs ) 
        { return subber::value( expr, referent< Is >( expr, subs... )... ); }
    };

public:
    // Type Manipulator Requirements //
    using type = Helper< for_bindings >::type;

    static constexpr type value( formula_type const& expr, 
        Subs const&... subs )
    { return Helper< for_bindings >::value( expr, subs... ); }
    // //
};

/// @brief substituting into a non-expresssion, a closed expression is, or an
///        open expression with no substitution arguments is idempotent
template< typename T, typename... Ss >
requires( not expression< T > or 
    ( open_expression< T > and sizeof...( Ss ) == 0 ))
struct Substituter< T, Ss... >
{
    using type = T;
    static constexpr type
    value( T const& expr, Ss const&... )
    { return expr; }
};

//////////////////
/// Free Vars ///
////////////////
///
/// @brief trait to identify free variables in a tuple of expressions. 
template< typename... Ts >
struct GetFreeVars< tuple< Ts... >>
{ 
    using type = merge_unique_variables_t< typename 
        GetFreeVars< Ts >::type... >; 
    static constexpr type value( tuple< Ts... > const& expr )
    { 
        static constexpr make_seq< sizeof...( Ts )> for_elements;

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
        { return merge_unique_variables( GetFreeVars< Ts >::value( 
            std::get< Is >( expr ))... ); };

        return helper( for_elements );
    }
};

/// @brief trait to identify free variables in a compound expression.  We reduce
///        to the tuple-case.
template< template< typename... > class Op, typename... Args >
requires( compound_expression< Op< Args... >> and 
    not is_substitution_expression_v< Op< Args... >> )
struct GetFreeVars< Op< Args... >>: GetFreeVars< tuple< Args... >> 
{ };

/// @brief variables have a single free variable plus any variables in the
///        the nested type T
template< size_t I, typename T >
struct GetFreeVars< Var< I, T >> {
private:
    // get the free variables of the value type
    using value_variable_set = GetFreeVars< T >::type;
    using this_variable_set = unique_variables< Var< I, T >>;

    // subtract out Variable I since the value_types would not match
    using subtracted_variable_set = subtract_unique_variables_t<
        value_variable_set, this_variable_set >;

public:
    // remerge with variable I with the correct value type
    using type = merge_unique_variables_t<
        subtracted_variable_set, this_variable_set >;
       
    static constexpr type
    value( Var< I, T > const& var )
    {
        static constexpr this_variable_set this_variable;

        return merge_unique_variables(
            subtract_unique_variables(
                GetFreeVars< T >::value( var.get_value() ),
                this_variable ),
            this_variable );
    }
};

/// @brief trait to identify free variables in an unbound function
template< size_t I, typename T, variable... Vars >
struct GetFreeVars< Func< Var< I, Var< I, T >>,
    Vars... >>
{
    using formula_variable_type = Var< I, Func< Var< I, T >, Vars... >>;

    using type = make_unique_variables_t< formula_variable_type, Vars... >;

    static constexpr type 
    value( Func< Var< I, Var< I, T >>, Vars... > const& func )
    {
        static constexpr make_seq< sizeof...( Vars )> for_vars;

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
        { return make_unique_variables( std::get< 0 >( func ).name(),
            std::get< 1 + Is >( func )... ); };

        return helper( for_vars ); 
    }
};

/// @brief trait to identify free variables in a substituted, unbound
///        function expression
//template< size_t I, typename T, variable... Vars, typename... Args >
//struct GetFreeVars< 
//    Sub< Func< Var< I, Var< I, T >>, Vars... >, Args... >>
//{
//    using formula_variable_type = Var< I, Func< Var< I, T >, Vars... >>;
//
//    using type = merge_unique_variables_t<
//        unique_variables< formula_variable_type >,
//        typename GetFreeVars< Args >::type... >;
//    static constexpr type
//    value( Sub< Func< 
//        Var< I, Var< I, T >>, Vars... >, Args... > const& expr )
//    { 
//        static constexpr make_seq< sizeof...( Args )> for_args;
//
//        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
//        { return merge_unique_variables(
//            unique_variables< formula_variable_type >{ std::get< 0 >( std::get< 0 >( expr )) },
//            GetFreeVars< Args >::value( std::get< 1 + Is >( expr ))... ); };
//
//        return helper( for_args );
//    }
//};

/// @brief trait to identify free variables in a bound function
template< typename ExprT, variable... Vars >
struct GetFreeVars< Func< ExprT, Vars... >>
{
    using type = merge_unique_variables_t<
        typename GetFreeVars< ExprT >::type,
        make_unique_variables_t< Vars... >>;
    
    static constexpr type
    value( Func< ExprT, Vars... > const& func )
    { 
        static constexpr make_seq< sizeof...( Vars )> for_vars;

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
        { return merge_unique_variables( 
            GetFreeVars< ExprT >::value( func.formula() ),
            make_unique_variables( func.template var< Is >()... )); };

        return helper( for_vars );
    }
};

/// @brief free variables in a substitution expression are the free variables 
///        in the formula that are not bound. 
template< typename ExprT, typename... Subs >
struct GetFreeVars< Sub< ExprT, Subs... >> { 
private:
    using expression_type = Sub< ExprT, Subs... >;
    using formula_type = ExprT;
    using formula_variable_set_type = GetFreeVars< ExprT >::type;

    using bound_variables_type = BoundVars< expression_type >;
    using bound_variable_set_type = bound_variables_type::variable_set_type;

public:
    using type = subtract_unique_variables_t<
        formula_variable_set_type, bound_variable_set_type >;

    static constexpr type
    value( Sub< ExprT, Subs... > const& expr )
    { return subtract_unique_variables(
        GetFreeVars< ExprT >::value( expr.formula() ),
        bound_variables_type::variable_set( expr )); }
};

//////////////////////////////
/// Higher-Order Variable ///
////////////////////////////
///
/// @brief specialization of higher-order variables. We exclude
///        `Var< I, Var< I, T >>` cases which are covered by the default
///        implementation.
///
/// The expression value_type is synonymous with substitution. In other words,
/// an indirect substitution into a higher-order variable `Var< I, ExprT >` is 
/// the same as a direct substitution of variable I into ExprT:
/// 
/// ```
/// static_assert( is_same_v< 
///     substitute_t< tuple< Var< 0, Expr0 >, Var< 1, Expr1 >>, T, U >,
///     tuple< substitute_for_t< 0, Expr0, T >, 
///         substitute_for_t< 1, Expr1, U >> );
/// ```
///
template< size_t I, expression ExprT >
struct Var< I, ExprT >
{
    using value_type = ExprT;
    static constexpr size_t id = I;

    // we are holding a place if we contain a reference to ourself in the
    // the nested type
    static constexpr bool is_free = 
        free_variables_t< ExprT >::contains_id( id );

    // we represent an evaluated variable if we do not contain a reference to 
    // ourselves
    //
    // This occurs, for example, when determining free variables 
    static constexpr bool is_bound = not is_free;

    // we signal a substitution stage if our nested expression is simply the 
    // lower-order version of ourselves
    static constexpr bool is_substitution_stage = 
        not is_same_v< Var< I, ExprT >, increase_var_order_t< ExprT >>;

private:
    using this_type = Var< I, value_type >;

public:
    constexpr string const& 
    name() const
    { return _name; }

    constexpr void 
    set_name( string const& new_name )
    { _name = new_name; }

    constexpr value_type const& 
    value() const
    { return _expr; }

    constexpr void
    set_value( value_type const& new_expr )
    { _expr = new_expr; }

    // return an unbound function using the stored expression
    template< variable First, variable... Rest >
    requires( is_non_repeating_v< seq< var_id_v< First >,
        var_id_v< Rest >... >> and is_free )
    constexpr Func< this_type, First, Rest... >
    operator ()( First first, Rest... rest ) const
    { return { *this, first, rest... }; } 

    // return a bound function if we have been evlauated
    template< variable First, variable... Rest >
    requires( is_non_repeating_v< seq< var_id_v< First >,
        var_id_v< Rest >... >> and is_bound )
    constexpr Func< value_type, First, Rest... >
    operator ()( First first, Rest... rest ) const
    { return { value(), first, rest... }; } 

    constexpr Var( string const& name = "var", value_type const& expr = {} ): 
        _name{ name }, _expr{ expr }
    { }
    constexpr Var( Var const& ) = default;

private:
    string _name;

    // NOTE: `value_type` is a misnomer since for higher-order variables the
    // value is an expression. Storing it here does not allow the unbound
    // evaluation of this variable since it is guaranteed that the nested
    // expression contains itself a lower-order instance of variable I.
    //
    // We store the expression to (1) allow for values in StaticValue<...> and
    // Var< J, ExprU > nested types to be passed through the evaluation of 
    // substitutions, 
    value_type _expr;
};
//
//template< typename FuncT, typename... Args >
//struct FuncalSub;
//
//template< typename ExprT, variable... Vars, typename... Args >
//struct FuncalSub< Func< ExprT, Vars... >, Args... >
//{
//    using index_seq = sort_index_seq< seq< var_id_v< Vars >... >>;
//    typedef make_seq< sizeof...( Args )> for_args;
//
//    // resorts the arguments to line up to vars IDs
//    template< typename Seq >
//    struct Helper;
//
//    template< size_t... Is >
//    struct Helper< seq< Is... >>
//    { 
//        using type = substitute_t< ExprT, Args...[ Is ]... >;
//        static constexpr type 
//        value( Func< ExprT, Vars... > const& func, Args const&... args )
//        { return substitute( std::get< 0 >( func ), args...[ Is ]... ); }
//    };
//
//    using type = Helper< for_args >::type;
//    static constexpr type 
//    value( Func< ExprT, Vars... > const& func, Args const&... args )
//    { return Helper< for_args >::value( func, args... ); }
//};
//
//template< typename FuncT, typename... Args >
//using functional_substitution_t = FuncalSub< FuncT, Args... >::type;
//
//template< typename FuncT, typename... Args >
//constexpr functional_substitution_t< FuncT, Args... >
//functional_substitution( FuncT const& func, Args const&... args )
//{ return FuncalSub< FuncT, Args... >::value( func, args... ); }
//
//////////////////////////////////
/// Funcal Sub Specialization ///
////////////////////////////////
///
//template< size_t I, typename T, variable... Vars, typename... Subs >
//struct Sub< Func< Var< I, Var< I, T >>, Vars... >, Subs... >:
//    std::tuple< Func< Var< I, Var< I, T >>, Vars... >, Subs... >
//{
//    typedef make_seq< sizeof...( Subs )> for_subs;
//    using formula_type = Func< Var< I, Var< I, T >>, Vars... >;
//    
//    constexpr formula_type
//    formula() const
//    { return std::get< 0 >( *this ); }
//
//    template< size_t K >
//    constexpr Subs...[ K ]
//    arg() const 
//    { return std::get< 1 + K >( *this ); }
//
//    constexpr std::tuple< Subs... >
//    subs() const
//    { 
//        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr ->
//            std::tuple< Subs... >
//        { return { arg< Is >()... }; };
//
//        return helper( for_subs{} );
//    };
//
//    constexpr Sub( formula_type const& formula, Subs const&... subs ):
//        std::tuple< formula_type, Subs... >{ formula, subs... }    
//    { }
//    constexpr Sub() = default;
//    constexpr Sub( Sub const& ) = default;
//
//};

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

//////////////////////////////////////////////////////
/// Arguments Base Class for Compound Expressions ///
////////////////////////////////////////////////////
///
/// Provides storage for expression arguments and the implementation of the
/// evaluation and substitution operator().
///
/// @brief default case is a tuple that calls the parent's value method upon
///        evaluation
///
/// @pre   Op< Args... > implementation MUST define a static value method
///        taking the ...Args as parameters
///
template< template< typename... > class Op, typename... Args >
struct Arguments: tuple< Args... > 
{
    using expression_type = Op< Args... >;
    using arguments_tuple = tuple< Args... >;

    static constexpr size_t arguments_size = sizeof...( Args );
    static constexpr make_seq< arguments_size > for_args;

    constexpr arguments_tuple const&
    args() const
    { return *this; }

    // calls Op< Args... >::value static method to calculate the result of the
    // operation
    constexpr auto
    operator ()() const
    {
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
        { return expression_type::value( get< Is >( args() )... ); };

        return helper( for_args );
    }

    constexpr Arguments( Args const&... args ): tuple< Args... >{ args... } { }
    constexpr Arguments( Arguments const& ) = default;
    constexpr Arguments() = default;
};

/// @brief specialization of Arguments when there is at least one expression
///        argument. We allow substitutions via operator() with at least one
///        parameter.  We calculate our result type from the nullary
///        operator() of the default implementation (which in turn calls the
///        Op< Args... >::value( Args... ) static method)
template< template< typename... > class Op, typename... Args >
requires(( expression< Args > or ... or false ))
struct Arguments< Op, Args... >: tuple< Args... > 
{
    using expression_type = Op< Args... >;
    using arguments_tuple = tuple< Args... >;
    using result_type = std::remove_cvref_t< decltype(
//        Op< result_t< Args >... >::value( result_t< Args >{}... )) >;
        Arguments< Op, result_t< Args >... >{}() )>;

    static constexpr size_t arguments_size = sizeof...( Args );
    static constexpr make_seq< arguments_size > for_args;

    constexpr arguments_tuple const& 
    args() const
    { return *this; }

private:
    // DT: is it ok to static_cast this to a derived class pointer?
    // DT: seems so: https://en.cppreference.com/cpp/language/static_cast
    constexpr expression_type const& 
    expr() const
    { return *static_cast< expression_type const* >( this ); }

private:
    template< size_t I >
    struct ArgEvaluator
    { 
        using type = Args...[ I ];
        static constexpr type
        value( arguments_tuple const& tup )
        { return std::get< I >( tup ); }
    };

    // NOTE: this does not guard against open expression idempotency on 
    //       Args...[I]::operator()** because Arguments< Op, Args... >::
    //       operator()*** guards itself so there be no circular logic.
    template< size_t I >
    requires( expression< Args...[ I ]> )
    struct ArgEvaluator< I >
    {
        using type = result_t< Args...[ I ]>;
        static constexpr type
        value( arguments_tuple const& tup )
        { return std::get< I >( tup )(); } // ** the invocation operator()
    }; 

public:

    // non-substitutions evalaute depth first
    constexpr result_type 
    operator ()() const 
    //requires(( closed_expression< Args > and ... and true ))
    requires( closed_expression< expression_type > and
        not is_substitution_expression_v< expression_type > )
    {
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> 
            result_type
        { return Op< typename ArgEvaluator< Is >::type... >{ 
            ArgEvaluator< Is >::value( args() )... }(); };

        return helper( for_args );
    }

    // substitutions evaluate breadth first
    constexpr result_type
    operator ()() const
    requires( closed_expression< expression_type > and
        is_substitution_expression_v< expression_type > )
    { return expr()(); }

    constexpr expression_type const&
    operator ()() const
    //requires(( open_expression< Args > or ...  or false ))
    requires( open_expression< expression_type > )
    { return expr(); }

    /// @brief Sub via invocation operator
    template< typename First, typename... Rest >
    requires( is_compatible_substitution_v< expression_type, First, Rest... > )
    constexpr substitute_t< expression_type, make_expression_t< First >, 
        make_expression_t< Rest >... >
    operator ()( First first, Rest... rest ) const
    { return substitute( expr(), make_expression( first ), 
        make_expression( rest )... ); }

    // DEBUG:
//    template< typename First, typename... Rest >
//    constexpr int debug_sub( First const& first, Rest const&... rest )
//    //{ static_assert( is_same_v< void, free_variables_t< expression_type >> ); }
//    { static_assert( is_same_v< void, std::tuple< 
//        substitute_t< expression_type, First, Rest... >,
//        Sub< expression_type, First, Rest... >>> ); }
//    //{ static_assert( is_same_v< void, Sub< expression_type, First, Rest... >> ); }

    constexpr Arguments( Args const&... args ): tuple< Args... >{ args... } { }
    constexpr Arguments( Arguments const& ) = default;
    constexpr Arguments() = default; 
};

////////////
/// Sub ///
//////////
/// 
template< typename ExprT, typename... Ss >
//requires( is_compatible_substitution_v< ExprT, Ss... >)
struct Sub: Arguments< Sub, ExprT, Ss... >
{
    static constexpr substitute_t< ExprT, Ss... >
    value( ExprT const& formula, Ss const&... subs )
    { return substitute( formula, subs... ); }

    using formula_type = ExprT;

    constexpr formula_type
    formula() const
    { return std::get< 0 >( *this ); }

    template< size_t I >
    constexpr Ss...[ I ]
    arg() const
    { return std::get< 1 + I >( *this ); }

    using Arguments< Sub, ExprT, Ss... >::Arguments;
};

/////////////
/// Func ///
///////////
/// 
/// @brief definition of a function expression
//template< typename ExprT, variable... Vars >
//requires( expression< ExprT > ) 
//struct Func< ExprT, Vars... >: std::tuple< ExprT, Vars... >
//{
//    using arguments_tuple = std::tuple< ExprT, Vars... >;
//    using formula_type = ExprT;
//    using this_type = Func< ExprT, Vars... >;
//
//    constexpr formula_type const&
//    formula() const
//    { return std::get< 0 >( *this ); }   
//
//    template< size_t I >
//    constexpr Vars...[ I ]
//    var() const
//    { return std::get< 1 + I >( *this ); }
//
//    // substitutes make_expression( args )... in for vars...
//    template< typename... Args >
//    requires( is_compatible_substitution_v< this_type, Args... > )
//    constexpr substitute_t< Func< ExprT, Vars... >, Args... >
//    operator ()( Args const&... args ) const
//    { return substitute( *this, args... ); }
//
//    constexpr Func( ExprT const& expr, Vars const&... vars ):
//        arguments_tuple{ expr, vars... } { };
//    constexpr Func( Func const& ) = default;
//    constexpr Func() = default;
//};
//



} // namespace expressions 

#endif

