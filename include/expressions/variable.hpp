////////////////////////////////////
/// Variables and Substitutions ///
//////////////////////////////////
///
/// Expressions are nested templated classes, optionally with placeholders 
/// called "variables" which can be parsed by the template pattern matching 
/// system in c++. This allows for compile-time analysis of arithmetic, 
/// geometric, and logical formulae and the re-use of common analysis and 
/// solution methods across problem spaces. The central tool to evaluating, 
/// solving, and construction of complex expressions are variables and 
/// substitutions, the definition of which are defined in this header file.
///
/// Variables are `Var< id, T >` types where `id` is a `size_t` integer that
/// uniquely identifies this variable, and `T` is a type that determines the
/// structure of valid substitutions and the ultimate result-type of the 
/// variable.  Substitutions into `Var< id, T >` are "compatible" if the 
/// expression type S being substituted, called the "referent type":
///
///     Compatiblity Rules:
///     (a) have the same result type as T
///     (b) could be substituted for the variable identified by id into T if
///         T itself is an expression type (see: higher-order variables)
///     
/// Substitutions happen one variable at a time. The order is calculated in 
/// BoundVars by looking through the dependencies of the variables and the
/// expressions they will be substituted for (referents). The `SubFor<...>` 
/// class template executes the substitution of a single variable with id Id 
/// into ExprT replacing it with the referent expression SubU.
///
/// Substitutions automatically evaluate static and closed expressions. See the 
/// evaluation logic in the intermediate class SubstituteForIdSeq.
///
////////////////////
/// Definitions ///
//////////////////
///
/// - A "formula" is an expression containing placeholder variables and the
///   first parameter in a call to `substitute(...)`
/// - "Referents" are expressions that will replace variables in a 
///   substitution. They are often represented as `...Subs` or `...Ss` in the 
///   code.
/// - "Partial" or "incomplete" substitutions are calls to 
///   `substitute( expr, ...subs )` which do not pair all free variables in the 
///   formula expression `expr`. 
/// - "Free" variables are `Var< I, T >` types that are unbound to a referent
/// - "Bound" variables are `Var< I, T>` types that are paired with a referent
///   during a substitution.
/// - A "closed" expression is one with no free variables and
/// - An "open" expression is one with free variables.
/// - A "static" expression is one that contains no variables at all, free nor
///   bound.  Note that all static expressions are also closed.
/// - The "order" or "variable order" of an expression is the minimum number of
///   substitution operations that must be done before the expression becomes
///   static and therefore evaluatable.
/// - a "function" in the context of this expression library is a pseudo-
///   expression which defines the substitution order into a second-order
///   variable, called the formula variable. Functions are higher-order than 
///   their arguments, and require at least one additional substitution to 
///   become static.
/// - "Direct" substitutions are calls to `substitute(...)`, including calls
///   to `operator()` with at least one argument on compound expressions (see
///   the `Arguments<...>` template in `expressions.hpp` for details).
/// - "Indirect" substitutions are the resulting "sub_for<id>(...)" calls which
///   are made while processing direct substitutions. 
/// - A referent is "dependent" upon a variable with identifier I if it itself 
///   contains a variable with the same identifier I.
///
/// Using these terms we can define the substitution operation itself:
///
/////////////////////////////////////
/// Definition of a Substitution ///
///////////////////////////////////
///
/// Calling `substitute( formula, ...subs )` will bind and replace 
/// `sizeof...(subs)` free variables in `formula` with the `...subs` referents. 
/// The binding order is either the order prescribed by formula using functions 
/// or defaulting to the numerical order of the free variables ids.  Once bound 
/// substitution takes place one variable at a time. These indirect 
/// substitutions are prioritized by dependencies between the `...subs` 
/// referents and the free variables themselves.
///
////////////////////////////////
/// Substitution Expression ///
//////////////////////////////
///
/// Since we allow for partial substitutions, a substitution itself is an 
/// expression, represented by `Sub< FormulaT, ...Subs >`. `Sub<...>` is a 
/// pseudo-compound expression bootstrapped in the code below to act the 
/// same as any other compound expression, but much of the compound expression
/// logic must be duplicated to enable this behaviour without circular
/// references. The use of the term "bootstrap" in this header file indicates
/// logic enabling the compound-expression logic for partial and incomplete
/// substitutions.
///
////////////////////////
/// Class Templates ///
//////////////////////
/// 
/// - `Var< Id, Expr >` expression class representing a placeholder
/// - `Sub< Expr, ...Subs >` compound expression class representing partial 
///   substitutions
/// - `Func< Expr, ...Vars >` expression class that specifies the order of 
///   variable-to-referent matching
/// - `SubFor< Id, Expr, Sub >` transformer class that replaces variables with
///   id == Id in Expr with Sub
/// - `SubstituteForIdSeq< Expr, seq< ...Ids >, ...Subs >` intermediate 
///   transformer class which enumerates the individual variable substitutions
///   and evaluates any closed expressions (including nested Sub<...> 
///   expressions
/// - `DirectSubOrder< Expr >` utility method class to determine the how free 
///   variables in an expression are matched to the substitution arguments 
/// - `BoundVars< Sub< Expr, ...Subs >>` utility method class that contains all 
///   necessary details to implement a substitution:
///   * is_compatible flag to ensure the substitution is compatible with the
///     free variables in Expr
///   * variable_set( Sub<...> ): method to return a unique_variable<...> set
///     of the bound variables in the Sub<...> expression
///   * variables( Sub<...> ): method to return a tuple of the variables in 
///     variable_set(...) in the proper substitution order
///   * referents( Sub<...> ): method to return a tuple of the substitution
///     arguments (referents) in parallel order with their matched variables
/// - `GetFreeVars< Expr >` utility method to return a unique_variable<...> set
///   of variables which are free in Expr
/// - `Substituter< Expr, ...Subs >` utility method class that executes the
///   specified substitution using the classes above
///
/////////////////////////
/// Method Templates ///
///////////////////////
///
/// The classes above are the bodies of the following methods and traits which 
/// should be used instead of the classes themselves outside of this header:
/// - `get_free_variables( expr )` returns a unique_variables set of free
///   variables in expr.
/// - `bound_variables( expr )` returns a BoundVars< Expr > object
/// - `closed_expression< Expr >` concept to identify expressions with no 
///   free variables (including substitutions)
/// - `open_expression< Expr >` concept is the negation of `closed_expression`
/// - `sub_for< Id >( expr, sub )` replaces all instances of variable 
///   identified by Id in expr with sub via `SubFor<Id,Expr,Sub>`
/// - `substitute( expr, ...subs )` executes a substitution of ...subs into the
///   free variables of expr.
///
/// The `substitute( expr, ...subs )` method is the intended entry point for
/// all substitutions outside of this header file.
///   
///////////////////////////////////////
/// Substitution Algorithm Details ///
/////////////////////////////////////
///
/// The `substitute(expr, ...subs)` method is the entry point for repacing 
/// free variables in the formula expression `expr` with compatible substitute
/// expressions `...subs`.  It's implementation is the bulk of the code in this
/// header file. It operates in stages:
///
///   (i) Identify free variables in formula expression
///  (ii) Sort the free variables into a binding order and pair with the
///       referent arguments
/// (iii) Sort the paired free variables by dependencies between the referents
///       and substitute each free variable one-by-one
///

/// NOTES:
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

using std::true_type, std::false_type;

namespace expressions {

/////////////////////////////
/// Forward Declarations ///
///////////////////////////
/// 
/// @brief forward declaration the expression transformer which evaluates a 
///        substitution 
template< typename ExprT, typename... Subs >
struct Substituter;

/// @brief trait to determine the return type of a substitution
template< typename ExprT, typename... Args >
using substitute_t = Substituter< ExprT, Args... >::type;

///////////////////////////////////////////////////
/// substitute( formula, ...referents ) method ///
/////////////////////////////////////////////////
///
/// @brief substitute method invokes the Substituter expression transformer
template< typename ExprT, typename... Args >
constexpr substitute_t< make_expression_t< ExprT >, 
    make_expression_t< Args >... >
substitute( ExprT expr, Args... args )
{ return Substituter< make_expression_t< ExprT >, 
    make_expression_t< Args >... >::value( make_expression( expr ), 
        make_expression( args )... ); }

/// @brief forward declaration of a helper trait class that determines the
///        variables that will be bound by a substitution
template< typename ExprT >
struct BoundVars;

/// @brief trait to determine if a given substitution is compatible 
template< typename ExprT, typename... Ss >
constexpr bool is_compatible_substitution_v = 
    BoundVars< Sub< make_expression_t< ExprT >, make_expression_t< Ss >... >>::
        is_compatible;

////////////
/// Sub ///
//////////
///
/// A partial or incomplete substitution is itself an expression given by this
/// template class.
///
/// @brief bootstrapped Sub expression is a compound expression
template< typename ExprT, typename... Ss >
//requires( is_compatible_substitution_v< ExprT, Ss... >)
struct Sub: tuple< ExprT, Ss... >
{
    using formula_type = ExprT;
    using expression_type = Sub< ExprT, Ss... >;
    using arguments_tuple = tuple< ExprT, Ss... >;

    static constexpr size_t arguments_size = 1 + sizeof...( Ss );
    static constexpr make_seq< arguments_size > for_args;

private:
    static constexpr size_t subs_size = sizeof...( Ss );
    static constexpr make_seq< subs_size > for_subs;

public:
    constexpr arguments_tuple const&
    args() const
    { return *this; }

    // we manually call substitute here since there would be a circular ref
    // if we used the default implementation of Arguments<... >
    constexpr substitute_t< ExprT, Ss... >
    operator ()() const
    {
        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr ->
            substitute_t< ExprT, Ss... >
        { return substitute( formula(), arg< Is >()... ); };

        return helper( for_subs );
    }

    // this logic is duplicated from Arguments<...> to avoid circular logic
    template< typename First, typename... Rest >
    requires( is_compatible_substitution_v< expression_type, First, Rest... > )
    constexpr substitute_t< expression_type, make_expression_t< First >, 
        make_expression_t< Rest >... >
    operator ()( First first, Rest... rest ) const
    { return substitute( *this, make_expression( first ), 
        make_expression( rest )... ); }

    constexpr formula_type
    formula() const
    { return std::get< 0 >( *this ); }

    template< size_t I >
    constexpr Ss...[ I ]
    arg() const
    { return std::get< 1 + I >( *this ); }

    constexpr Sub( ExprT const& formula, Ss const&... subs ):
        tuple< ExprT, Ss... >{ formula, subs... }
    { }
    constexpr Sub( Sub const& ) = default;
    constexpr Sub() = default;
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
/// Represents the variables bound by a substitution operation, and is returned
/// by `bound_variables( expr )`. Non-substitution expressions cannot have any
/// bound variables, and so the default specialization is empty and also shows
/// the necessary fields and type definitions that will be calculated.
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

/// @brief trait to determine the return type of `bound_variables(...)`
template< typename T >
using bound_variables_t = BoundVars< T >;

///////////////////////////////////////
/// bound_variables( expr ) method ///
/////////////////////////////////////
/// 
/// @brief method to cacluate the variables bound by a substitution and pair
///        them with the referents they will be replaced by.
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
    expression< T > and ( free_variables_t< T >::size == 0 )> { };

/// @brief A closed expression contains no free (unbound) variables
template< typename T >
concept closed_expression = IsClosedExpression< T >::value; 

/// @brief an open expression contains free (unbound) variables.
template< typename T >
concept open_expression = expression< T > and not closed_expression< T >;

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

// compatibility with SetVar
template< size_t Id, size_t Jd, typename T, typename S >
struct IsCompatibleVarSub< Id, SetVar< Jd, T >, S >:
    IsCompatibleVarSub< Id, T, S >
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

/// @brief we are compatible to sub into a compound expression if we are
///        compatible with at least one arg (same as tuples)
template< size_t Id, template< auto, typename... > class Op, auto Discriminator,
    typename... Args, typename S >
requires( compound_expression< Op< Discriminator, Args... >> and not
    is_substitution_expression_v< Op< Discriminator, Args... >> )
struct IsCompatibleVarSub< Id, Op< Discriminator, Args... >, S >:
    IsCompatibleVarSub< Id, std::tuple< Args... >, S > 
{ };

/// @brief we are compatible to sub into a tensor if we are compatible with
///        at least one element (same as tuples)
template< size_t Id, shape Shp, typename... Ts, typename S >
struct IsCompatibleVarSub< Id, Tensor< Shp, Ts... >, S >:
    IsCompatibleVarSub< Id, std::tuple< Ts... >, S >
{ };

} // namespace detail

//////////////////////
/// SubstituteFor ///
////////////////////
///
/// Parses, replaces, and reconstructs expressions that are the results of a
/// single variable substitution
///
/// @brief Expression representing the substitution of variable I in ExprT with
///        SubU
/// @pre:  Id is free in ExprT (guarded by Substituter<...>)
template< size_t Id, typename ExprT, typename SubU >
struct SubFor {
private:
    using variable_set = free_variables_t< ExprT >;
    using expression_type = ExprT;
    using referent_type = SubU;
    static constexpr size_t id = Id;
    
    // default case is idempotent
    template< typename T >
    struct Parser
    { 
        using type = T;
        static constexpr type
        value( T const& expr, referent_type const& )
        { return expr; }
    };

    // if our variables don't match, return the variable as is
    template< variable Var >
    requires( var_id_v< Var > != id )
    struct Parser< Var >
    { 
        using type = Var;
        static constexpr type
        value( Var const& var, referent_type const& )
        { return var; }
    };

    // if our variables match, return the substitute
    template< variable Var >
    requires( var_id_v< Var > == id ) // and var_order_v< Var > == 1 )
    struct Parser< Var >
    {
        using type = referent_type;
        static constexpr type
        value( Var const&, referent_type const& sub )
        { return sub; }
    };

    // we cannot substitute for the variable being assigned in a set expression
    template< size_t Jd, typename ExprJ >
    requires( Jd != Id )
    struct Parser< SetVar< Jd, ExprJ >>
    {
        using type = SetVar< Jd, typename Parser< ExprJ >::type >;
        static constexpr type
        value( SetVar< Jd, ExprJ > const& expr, referent_type const& sub )
        { return { Parser< ExprJ >::value( std::get< 0 >( expr ), sub ) }; }
    };

    // parser for func expressions
    template< typename FuncExpr >
    struct FuncParser;

    // if our formula for this function will not be static after the
    // substitution then recreate the function expression around the 
    // substituted formula
    template< expression U, variable... Vars >
    requires( not static_expression< typename Parser< U >::type > )
    struct FuncParser< Func< U, Vars... >> 
    {
        using type = Func< typename Parser< U >::type, Vars... >;
        static constexpr type
        value( Func< U, Vars... > const& func, referent_type const& sub )
        {
            static constexpr make_seq< sizeof...( Vars )> for_vars;

            auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
            { return { Parser< U >::value( func.formula(), sub ), 
                func.template var< Is >()... }; };

            return helper( for_vars );
        }
    };

    // if our formula will be static we unwrap the function from it and just
    // return the static formula.
    template< expression U, variable... Vars >
    requires( static_expression< typename Parser< U >::type > )
    struct FuncParser< Func< U, Vars... >>
    {
        using type = typename Parser< U >::type;
        static constexpr type
        value( Func< U, Vars... > const& func, referent_type const& sub )
        { return Parser< U >::value( func.formula(), sub ); }
    };

    // we inherit from our specialized funcparser in the case of functions
    template< expression U, variable... Args >
    struct Parser< Func< U, Args... >>: FuncParser< Func< U, Args... >>
    { };

    // parser or compound expressions
    template< typename CompoundExpr >
    struct CompoundParser;

    // if a compound expression is not a substitution expression then recurse
    // the parser into the args and reconstruct the expression
    template< template< typename... > class Op, typename... Args >
    requires( not is_substitution_expression_v< Op< Args... >> )
    struct CompoundParser< Op< Args... >>
    {
        typedef make_seq< sizeof...( Args )> for_args;

        template< typename Seq >
        struct Helper;

        template< size_t... Is >
        struct Helper< seq< Is... >>
        {
            using type = Op< typename Parser< Args...[ Is ]>::type... >;
            static constexpr type
            value( Op< Args... > const& expr, referent_type const& sub )
            { return { Parser< Args...[ Is ]>::value(
                get_argument< Is >( expr ), sub )... }; };
        };

        using type = Helper< for_args >::type;
        static constexpr type
        value( Op< Args... > const& expr, referent_type const& sub )
        { return Helper< for_args >::value( expr, sub ); }
    };

    //  recurse into compound discriminated expressions
    template< template< auto, typename... > class Op, auto Discriminator,
        typename... Args >
    struct CompoundParser< Op< Discriminator, Args... >>
    {
        typedef make_seq< sizeof...( Args )> for_args;

        template< typename Seq >
        struct Helper;

        template< size_t... Is >
        struct Helper< seq< Is... >>
        {
            using type = Op< Discriminator, typename Parser< Args...[ Is ]>::
                type... >;

            static constexpr type
            value( Op< Discriminator, Args... > const& expr, 
                referent_type const& sub )
            { return { Parser< Args...[ Is ]>::value(
                get_argument< Is >( expr ), sub )... }; };
        };

        using type = Helper< for_args >::type;
        static constexpr type
        value( Op< Discriminator, Args... > const& expr, 
            referent_type const& sub )
        { return Helper< for_args >::value( expr, sub ); }

    };

    // parser for substitution expressions
    template< typename SubExpr >
    struct SubstitutionParser;

    // default case of a substitution parser duplicates the logic of a compound
    // parser to avoid circular references
    template< template< typename... > class Op, typename... Args >
    struct SubstitutionParser< Op< Args... >>
    {
        typedef make_seq< sizeof...( Args )> for_args;

        template< typename Seq >
        struct Helper;

        template< size_t... Is >
        struct Helper< seq< Is... >>
        {
            using type = Op< typename Parser< Args...[ Is ]>::type... >;
            static constexpr type
            value( Op< Args... > const& expr, referent_type const& sub )
            { return { Parser< Args...[ Is ]>::value(
                get_argument< Is >( expr ), sub )... }; };
        };

        using type = Helper< for_args >::type;
        static constexpr type
        value( Op< Args... > const& expr, referent_type const& sub )
        { return Helper< for_args >::value( expr, sub ); }
    };

    // if the nested substitution expression is closed we evaluate the 
    // substitution, then pass the result back into the parser
    template< typename ExprU, typename... Ss >
    requires( closed_expression< Sub< ExprU, Ss... >> )
    struct SubstitutionParser< Sub< ExprU, Ss... >>
    {
        using type = Parser< substitute_t< ExprU, Ss... >>::type;

        static constexpr type
        value( Sub< ExprU, Ss... > const& expr, referent_type const& sub )
        { 
            static constexpr make_seq< sizeof...( Ss )> for_subs;

            auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
            { return Parser< substitute_t< ExprU, Ss... >>::value( 
                substitute( expr.formula(), expr.template arg< Is >()... ), 
                    sub ); };

            return helper( for_subs );
        }
    };

    // Our substitution parser is a special case of our compound expression
    // parser.
    template< typename ExprU, typename... Ss >
    struct CompoundParser< Sub< ExprU, Ss... >>: 
        SubstitutionParser< Sub< ExprU, Ss... >>
    { };

    // and our compount expression parser is a special case of our parser.
    template< compound_expression CompoundExpr >
    struct Parser< CompoundExpr >: CompoundParser< CompoundExpr > { };
   
       
public:
    using type = Parser< expression_type >::type;

    static constexpr type
    value( expression_type const& expr, referent_type const& sub )
    { return Parser< expression_type >::value( expr, sub ); }
};

//////////////////////////////////////////////////
/// sub_for< id >( formula, referent ) method ///
////////////////////////////////////////////////
///
/// @brief method to evaluate the substitution of a variable identified by Id
///        with SubU into expression ExprT. Calls the expression transformer
///        SubFor<...> and guards against non-expression arguments.
template< size_t Id, typename ExprT, typename SubU >
constexpr typename SubFor< Id, make_expression_t< ExprT >, 
    make_expression_t< SubU >>::type
sub_for( ExprT const& expr, SubU const& sub )
{ return SubFor< Id, make_expression_t< ExprT >, 
    make_expression_t< SubU >>::value( make_expression( expr ), 
        make_expression( sub )); }

///////////////////////////
/// SubstituteForIdSeq ///
/////////////////////////
/// 
/// Intermediate expression transformer which enumerates the individual 
/// variable replacements performed by SubFor<...>. This template class also
/// defines the depth-first logic of the SubFor<...> operation (indirect 
/// substitutions) and the breadth-first logic of direct substitutions
///
/// @brief expression transformer that replaces a list of variable ids with 
///        ...Subs in order.  This is an intermediate step of substitute(...) 
///        and not intended to be invoked directly. See Substituter<...> for
///        invocation details.
template< typename IdSeq, typename ExprT, typename... Subs >
struct SubstituteForIdSeq;

/// @brief substitution into a non-expression is idempotent
template< typename IdSeq, typename ExprT, typename... Subs >
requires( not expression< ExprT >)
struct SubstituteForIdSeq< IdSeq, ExprT, Subs... >
{
    using type = ExprT;
    static constexpr type
    value( ExprT const& expr, Subs const&... )
    { return expr; }
};

/// @brief we evaluate substitution expressions breadth-first.
template< size_t... Ids, typename ExprT, typename... Ss, typename... Subs >
requires( closed_expression< Sub< ExprT, Ss... >> )
struct SubstituteForIdSeq< seq< Ids... >, Sub< ExprT, Ss... >, Subs... >
{
    // process subs in breadth first order
    using type = SubstituteForIdSeq< seq< Ids... >, 
        substitute_t< ExprT, Ss... >, Subs... >::type;

    static constexpr type
    value( Sub< ExprT, Ss... > const& expr, Subs const&... subs )
    { 
        static constexpr make_seq< sizeof...( Ss )> for_subsubs;

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr -> type
        { return SubstituteForIdSeq< seq< Ids... >, 
            substitute_t< ExprT, Ss... >, Subs... >::value( substitute( expr.formula(),
                expr.template arg< Is >()... ), subs... ); };

        return helper( for_subsubs );
    }
};

/// @brief if our expression has none of the ...Ids as free then
///        substitution is idempotent
template< size_t... Ids, typename ExprT, typename... Subs >
requires( open_expression< ExprT > and 
    not ( free_variables_t< ExprT >::contains_id( Ids ) or ... or false ))
struct SubstituteForIdSeq< seq< Ids... >, ExprT, Subs... >
{
    using type = ExprT;
    static constexpr type
    value( ExprT const& expr, Subs const&... )
    { return expr; }
};

/// @brief terminal case for closed expressions that is not a substitution
///        expression itself returns the result_type
template< size_t... Ids, typename ExprT, typename... Subs >
requires( not is_substitution_expression_v< ExprT > and
    not is_function_v< ExprT > and
        closed_expression< ExprT > )
struct SubstituteForIdSeq< seq< Ids... >, ExprT, Subs... >
{
    using type = result_t< ExprT >;
    static constexpr type
    value( ExprT const& expr, Subs const&... )
    { return expr(); }
};

/// DT: Do we need this? 
/// DT: shouldn't it evaluate the expression?
template< size_t... Ids, typename ExprT, variable... Vars, typename... Subs >
requires( closed_expression< Func< ExprT, Vars... >> )
struct SubstituteForIdSeq< seq< Ids... >, Func< ExprT, Vars... >, Subs... >
{
    using type = Func< ExprT, Vars... >;
    static constexpr type
    value( Func< ExprT, Vars... > const& expr, Subs const&... )
    { return expr; }
};

/// @brief recursive case for open expressions that contain the first Id as 
///        a free variable
///
/// NOTE: if ExprT is substitution it will report free and bound variables
///       properly.
/// NOTE: there is a chance of a circular dependency:
///       FreeVars< Sub<...>> -> BoundVars< Sub<...>> ->
template< size_t FirstId, size_t... RestIds, typename ExprT, 
    typename FirstSub, typename... RestSubs >
requires( free_variables_t< ExprT >::contains_id( FirstId )) 
struct SubstituteForIdSeq< seq< FirstId, RestIds... >, ExprT, 
    FirstSub, RestSubs... >
{
    using type = SubstituteForIdSeq< seq< RestIds... >, typename
        SubFor< FirstId, ExprT, FirstSub >::type, RestSubs... >::type;

    static constexpr type
    value( ExprT const& expr, FirstSub const& first, RestSubs const&... rest )
    { return SubstituteForIdSeq< seq< RestIds... >, typename 
        SubFor< FirstId, ExprT, FirstSub >::type, RestSubs... >::value(
            SubFor< FirstId, ExprT, FirstSub >::value( expr, first ),
                rest... ); }
};

/// @brief recursive case for open expressions that does not contain the first 
///        Id as a free variable
template< size_t FirstId, size_t... RestIds, typename ExprT, 
    typename FirstSub, typename... RestSubs >
requires( open_expression< ExprT > and 
    not free_variables_t< ExprT >::contains_id( FirstId ))
struct SubstituteForIdSeq< seq< FirstId, RestIds... >, ExprT, 
    FirstSub, RestSubs... >
{
    using type = SubstituteForIdSeq< seq< RestIds... >, ExprT, 
        RestSubs... >::type;

    static constexpr type
    value( ExprT const& expr, FirstSub const& first, RestSubs const&... rest )
    { return SubstituteForIdSeq< seq< RestIds... >, ExprT, RestSubs... >::
        value( expr, rest... ); }
};

template< typename Seq, typename ExprT, typename... Subs >
using substitute_for_id_seq_t = SubstituteForIdSeq< Seq, ExprT, Subs... >::type;

template< typename Seq, typename ExprT, typename... Subs >
constexpr substitute_for_id_seq_t< Seq, ExprT, Subs... >
substitute_for_id_seq( ExprT const& expr, Subs const&... subs )
{ return SubstituteForIdSeq< Seq, ExprT, Subs... >::value( expr, subs... ); }

///////////////////////////////////////////////
/// Binding Order for Direct Substitutions /// 
/////////////////////////////////////////////
/// 
/// Helper class to distiguish between the default variable binding order
/// defined by the numeric sorting of the free variable's id, and the binding 
/// order defined by substituting directly into a function with prescribed
/// variable parameters.
///
/// @brief In the default case we match referents to variables in increasing 
///        variable id order, which is how they are indexed in unique_variables
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

//////////////////////////////////////////
/// Bound Variables of a Substitution ///
////////////////////////////////////////
///
/// Critical specialization of BoundVars for Substitutions. This class template
/// calculates the free variables in the formula expression and binds them
/// to the referents.
///
/// @brief container for bound variables in a substitution expression
template< typename ExprT, typename... Subs >
//requires( not is_function_v< ExprT > )
struct BoundVars< Sub< ExprT, Subs... >> {
public:
    // we are friends with any Id seq substituter for bootstrapping
    template< typename Seq, typename ExprU, typename... SubSubs >
    friend struct SubstituteForIdSeq;

// private:
    using expression_type = Sub< ExprT, Subs... >;
    using formula_expression = ExprT;

    // determine the order we should match variables to substituted expressions
    using formula_variable_tuple = 
        DirectSubOrder< formula_expression >::type;
    static constexpr size_t formula_variables_size = 
        std::tuple_size_v< formula_variable_tuple >; 

    // returns a tuple of the unique free variables in the formula expression
    // in the appropriate binding order
    static constexpr formula_variable_tuple
    formula_variables( expression_type const& expr )
    { return DirectSubOrder< formula_expression >::value( 
        std::get< 0 >( expr )); }

    using variable_sub_order = DirectSubOrder< ExprT >;

public:
    // the number of variables that will be bound is the minimum of the number
    // of free variables, and the number of referents
    static constexpr size_t size = std::min( 
        formula_variables_size, sizeof...( Subs ));
    static constexpr bool is_complete = 
        formula_variables_size == size;

private:
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
    // PRE: Vars and Subs are paired.
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

    // trait for the variable type of the Ith variable matched with a referent
    template< size_t I >
    using bound_variable_t = std::tuple_element_t< I, trimmed_variable_tuple >;

    // trait for the free variables in the Ith referent
    template< size_t I >
    using referent_free_variables_t = GetFreeVars< 
        std::tuple_element_t< I, trimmed_referent_tuple >>::type;

    // trait to determine whether the Ith match is dependent on the Jth
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
        // flag to identify circular dependencies in this substitution
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

    // how many bindings should precede the current Ith one when executing this
    // substitution?
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

    // trait to sort the current bindings by their dependencies
    template< typename Seq >
    struct SortedIndexSeq;

    // sort the bound variables by the count of bindings which should preceed it
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

    // resort the bound variables by the order in which they will be
    // substituted
    using variable_tuple = Trimmer< for_binding_order >::variable_tuple;

    // resort the referents to keep the two tuples parallel
    using referent_tuple = Trimmer< for_binding_order >::referent_tuple;

private:
    // helper to determine compatibility of this substitution
    template< typename Seq >
    struct CompatibleHelper;

    // an substitution is compatible if each variable is compatible with it's 
    // bound referent.
    template< size_t... Is >
    struct CompatibleHelper< seq< Is... >>: 
        std::integral_constant< bool, ( detail::IsCompatibleVarSub< 
            var_id_v< std::tuple_element_t< Is, variable_tuple >>,
            formula_expression, 
            std::tuple_element_t< Is, referent_tuple >>::value and 
                ... and true )>
    { };

public:
    // flag to determine compatibliity of this substitution
    static constexpr bool is_compatible = 
        CompatibleHelper< for_bindings >::value;

    // method to return a unique_variable set of the bound variables
    static constexpr variable_set_type
    variable_set( expression_type const& expr )
    { return Trimmer< for_bindings >::variable_set( expr ); }

    // method to return a tuple of bound variables in substitution order
    static constexpr variable_tuple
    variables( expression_type const& expr ) 
    {
        formula_variable_tuple vars = formula_variables( expr );

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr ->
            variable_tuple
        { return { vars.template at< Is >()... }; };

        return helper( for_binding_order{} );
    }

    // method to return the parallel tuple of referents for the bound variables
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

    // trait for the Nth variable which will be substituted
    template< size_t N >
    using bound_variable_t = std::tuple_element_t< N, bound_variable_tuple >;

    // trait for the Nth substituted variable's identifier
    template< size_t N >
    static constexpr size_t var_id_of = var_id_v< bound_variable_t< N >>;

    // trait for the Nth substituted variables referent
    template< size_t N >
    using referent_t = std::tuple_element_t< N, referent_tuple >;

    // method to return the value of the Nth substituted variable
    template< size_t N >
    static constexpr bound_variable_t< N >
    bound_var( formula_type const& expr, Subs const&... subs ) 
    { return std::get< N >( bound_variables_type::variables(
        substitution_type{ expr, subs... })); }

    // method to return the value of the Nth substituted variable's referent
    template< size_t N >
    static constexpr referent_t< N >
    referent( formula_type const& expr, Subs const&... subs )
    { return std::get< N >( bound_variables_type::referents(
        substitution_type{ expr, subs... })); }

    typedef make_seq< bound_variables_type::size > for_bindings;

    template< typename T >
    static constexpr bool open_or_non_expression_v = not expression< T > or 
        open_expression< T >;

    // helper to implement the substitution operation via SubstituteForIdSeq
    template< typename Seq >
    struct Helper;

    // we enumerate the bound variable ids and their referents in substitution
    // order for the intermediate helper class, SubstituteForIdSeq
    //
    // case: the resulting expression after the substitution is closed, 
    //       evaluate it.
    template< size_t... Is >
    requires( closed_expression< substitute_for_id_seq_t< seq< var_id_of< Is >... >, 
        formula_type, referent_t< Is >... >> )
    struct Helper< seq< Is... >>
    {
        using type = result_t< substitute_for_id_seq_t< 
            seq< var_id_of< Is >... >, formula_type, referent_t< Is >... >>;

        static constexpr type
        value( formula_type const& expr, Subs const&... subs )
        { return substitute_for_id_seq< seq< var_id_of< Is >... >>(
            expr, referent_t< Is >( expr, subs... )... )(); }
    };

    // case: the resultant expression will be open, return it
    template< size_t... Is >
    requires( open_or_non_expression_v< substitute_for_id_seq_t< 
        seq< var_id_of< Is >... >, formula_type, referent_t< Is >... >> )
    struct Helper< seq< Is... >>
    {
        using type = substitute_for_id_seq_t< 
            seq< var_id_of< Is >... >, formula_type, referent_t< Is >... >;

        static constexpr type
        value( formula_type const& expr, Subs const&... subs ) 
        { return substitute_for_id_seq< seq< var_id_of< Is >... >>( 
            expr, referent< Is >( expr, subs... )... ); }
    };

public:
    // Type Manipulator Requirements //
    using type = Helper< for_bindings >::type;

    static constexpr type value( formula_type const& expr, 
        Subs const&... subs )
    { return Helper< for_bindings >::value( expr, subs... ); }
    // //
};

/// DT: we need a case for Substituter< Func<...>> which corresponds to the
///     direct substitution into a function.  This would result in a 
///     Sub< Func<...>> if the substitution was incomplete, and a 
///     substitute_for_id_seq_t<...> if it was.
/// DT: do we? I accidentally included a "closed_expression<..>" condition
///     on the specialization below.  I think the handles above may be working

/// @brief substituting into a non-expresssion, a closed expression is, or an
///        open expression with no substitution arguments is idempotent
template< typename T, typename... Ss >
requires( not expression< T > or // circular logic? 
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

/// @brief trait to identify free variables in a compound expression.  We 
///        reduce to the tuple-case.
template< template< typename... > class Op, typename... Args >
requires( compound_expression< Op< Args... >> and 
    not is_substitution_expression_v< Op< Args... >> )
struct GetFreeVars< Op< Args... >>: GetFreeVars< tuple< Args... >> 
{ };

template< template< auto, typename... > class Op, auto Discriminator, 
    typename... Args >
requires( compound_expression< Op< Discriminator, Args... >> and 
    not is_substitution_expression_v< Op< Discriminator, Args... >> )
struct GetFreeVars< Op< Discriminator, Args... >>: 
    GetFreeVars< tuple< Args... >>
{ };

/// @brief variables have a single free variable plus any variables in the
///        the nested type T
template< size_t I, typename T >
requires( expression< T > )
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

template< size_t I, typename T >
requires( not expression< T > )
struct GetFreeVars< Var< I, T >> 
{
    using type = unique_variables< Var< I, T >>;
    static constexpr type
    value( Var< I, T > const& var )
    { return { var }; }
};

//variable Id is not considered free in the expression T
//template< size_t Id, typename T >
//requires( free_variables_t< T >::contains_id( Id ))
//struct GetFreeVars< SetVar< Id, T >> {
//private:
//    using value_variable_set = GetFreeVars< T >::type;
//    using value_type = var_value_t< typename value_variable_set::template 
//        variable_t< Id >>;
//    using this_variables_set = unique_variables< Var< Id, value_type >>;
//
//public:
//    using type = subtract_unique_variables_t< 
//        value_variable_set, this_variables_set >;
//
//    static constexpr type
//    value( SetVar< Id, T > const& expr )
//    {
//        static constexpr this_variables_set this_variable;
//        return subtract_unique_variables(
//            GetFreeVars< T >::value( std::get< 0 >( expr )),
//            this_variable );
//    }
//};

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

/// @brief trait to identify free variables in a bound function
template< typename ExprT, variable... Vars >
requires( var_order_v< ExprT > == 1 )
struct GetFreeVars< Func< ExprT, Vars... >>
{
    using type = GetFreeVars< ExprT >::type;
    
    static constexpr type
    value( Func< ExprT, Vars... > const& func )
    { return GetFreeVars< ExprT >::value( func.formula() ); };
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

            //using type = Sub< this_type, Args... >;
            //using type = substitute_for_id_seq_t< 
            //    seq< var_id_v< Vars...[ Is ]>... >, ExprT, Args...[ Is ]... >;
            using type = substitute_t< Sub< this_type, Args... >>;

            static constexpr type 
            value( Func const& func, Args const&... args )
            { return substitute( Sub< this_type, Args... >{ func, args... }); }
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

    // attempting to evaluate a function is the same as evaluating the formula
//    constexpr result_t< formula_type >
//    operator ()() const
//    { return formula()(); }

    constexpr Func( formula_type const& func, 
        Vars const&... vars ): arguments_tuple{ func, vars... } 
    { };
    constexpr Func( Func const& ) = default;
    constexpr Func() = default;
};

template< typename ExprT, variable... Vars >
constexpr Func< ExprT, Vars... >
func( ExprT const& expr, Vars const&... vars )
{ return { expr, vars... }; }


/// @brief helper function to construct SetVarValue expressions
template< variable VarT, typename T >
constexpr SetVar< var_id_v< VarT >, make_expression_t< T >> 
set_variable( T const& val, VarT var = {} )
{ return { var, make_expression( val )}; }

/// @brief implementation of operator= from Var<...>
//template< size_t I, typename T >
//template< typename U >
//constexpr SetVar< I, make_expression_t< U >> 
//Var< I, T >::operator =( U const& expr ) const
//{ return { make_expression( expr )}; }

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
/// list of free variables when substituting (ie: binding order).
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
    /// NOTE: it already did, by creating circular definitions with SetVar in 
    /// stdc++lib
    template< typename U >
    constexpr SetVar< I, make_expression_t< U >> 
    operator =( U const& expr ) const
    { return { make_expression( expr )}; }

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

    /// @brief direct substitution into a variable creates a function 
    template< variable First, variable... Rest > 
    requires( is_non_repeating_v< seq< var_id_v< First >,
        var_id_v< Rest >... >> ) 
    constexpr Func< increase_var_order_t< this_type >, First, Rest... >
    operator ()( First first, Rest... rest ) const
    { return { name(), first, rest... }; }

    // TODO:
    // operator++()
    // operator--()
    // operator+=(auto)
    // operator-=(auto)
    // operator*=(auto)
    // operator/=(auto)
    // operator%=(auto)

    template< typename U >
    constexpr Chain< Var< I, T >, U >
    operator ,( U const& next ) const;

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

    template< typename U >
    constexpr Chain< Var< I, ExprT >, U >
    operator ,( U const& next ) const;

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

//////////////////////////////////
/// Closed Sub Specialization ///
////////////////////////////////
///
/// Represents a substitution into an expression with no free variables
template< closed_expression ExprT, typename... Ss >
struct Sub< ExprT, Ss... >: tuple< ExprT, Ss... >
{
    using formula_type = ExprT;
    using expression_type = Sub< ExprT, Ss... >;
    using arguments_tuple = tuple< ExprT, Ss... >;

    static constexpr size_t arguments_size = 1 + sizeof...( Ss );
    static constexpr make_seq< arguments_size > for_args;

    constexpr arguments_tuple const&
    args() const
    { return *this; }

    constexpr formula_type
    formula() const
    { return std::get< 0 >( args() ); }

    template< size_t I >
    constexpr Ss...[ I ]
    arg() const
    { return std::get< 1 + I >( args() ); }

    template< typename... Ts >
    constexpr result_t< ExprT >
    operator ()( Ts const&... ) const
    { return formula()(); }

    constexpr Sub( ExprT const& formula, Ss const&... subs ):
        tuple< ExprT, Ss... >{ formula, subs... }
    { }
    constexpr Sub( Sub const& ) = default;
    constexpr Sub() = default;
};

///////////////////////////////////
/// Closed Func Specialization ///
/////////////////////////////////
///
template< closed_expression ExprT, variable... Vars >
struct Func< ExprT, Vars... >: std::tuple< ExprT, Vars... >
{
    using formula_type = ExprT;
    using arguments_tuple = std::tuple< formula_type, Vars... >;
    using this_type = Func< formula_type, Vars... >;

    constexpr formula_type const&
    formula() const
    { return std::get< 0 >( *this ); }

    template< size_t K >
    constexpr Vars...[ K ]
    var() const
    { return std::get< 1 + K >( *this ); }

    template< typename... Args >
    requires( sizeof...( Args ) == sizeof...( Vars ))
    constexpr result_t< formula_type >
    operator ()( Args const&... ) const
    { return formula()(); }

    // attempting to evaluate a function is the same as evaluating the formula
    constexpr result_t< formula_type >
    operator ()() const
    { return formula()(); }

    constexpr Func( formula_type const& func, 
        Vars const&... vars ): arguments_tuple{ func, vars... } 
    { };
    constexpr Func( Func const& ) = default;
    constexpr Func() = default;

};

} // namespace expressions 

#endif

