#ifndef __EXPRESSIONS_HPP__
#define __EXPRESSIONS_HPP__

#include "utility.hpp"

#include <cmath>
#include <type_traits>
#include <tuple>
#include <map>
#include <any>

namespace expressions {

using std::size_t;
using std::tuple, std::make_tuple, std::tuple_element_t, std::get;
using std::map;
using std::any, std::any_cast;

/**
 * Depdendencies
 */
template< typename... Vars >
struct VariableSet;

template< typename FirstT, typename... Rest >
struct VariableSet< FirstT, Rest... >
{
    static constexpr size_t size = 1 + sizeof...( Rest );
    using first = FirstT;
    using rest = VariableSet< Rest... >;
};

template<>
struct VariableSet<>
{ static constexpr size_t size = 0; };

// parent class must define result_type and eval members
struct ExpressionTag 
{ 
    // using result_type...
    // using dependent_vars_type...
    // constexpr result_type eval( variable_values& ) const...
};

template< typename T >
struct is_expression
{ static constexpr bool value = std::is_base_of_v< ExpressionTag, T >; };

template< typename T >
static constexpr bool is_expression_v = is_expression< T >::value;

/**
 * storage for variable valuess
 */
struct variable_values 
{ 
    any operator[]( size_t I ) const 
    { 
        auto i = values.find( I );
        if( i == values.end() )
            return {};
        return i->second;
    }
    any& operator[]( size_t I ) { return values[I]; }
    map< size_t, any > values;
};


template< typename T >
struct Evaluator;

template< typename T >
requires( is_expression_v< T > )
struct Evaluator< T >
{ 
    using result_type = T::result_type;

    constexpr static result_type evaluate( T const& expr, 
        variable_values& vars )
    { return expr.eval( vars ); }
};

template< typename T >
requires( not is_expression_v< T > )
struct Evaluator< T >
{ 
    using result_type = T;
    constexpr static T evaluate( T const& expr, variable_values& )
    { return expr; }; 
};

template< typename T >
using result_t = Evaluator< T >::result_type;

/**
 * expressions can be evaluated, and by default just return the expression.
 */
template< typename T >
constexpr result_t< T > eval( T const& expr, 
    variable_values& vars)
{ return Evaluator< T >::evaluate( expr, vars ); }

template< typename T >
constexpr result_t< T > eval( T const& expr )
{ 
    variable_values _;
    return eval( expr, _ ); 
}

/**
 * expressions can have variables
 */
template< size_t I, typename T >
// DT: do we need this?
// requires( not is_expression_v< T > )
struct Variable : ExpressionTag
{
    using value_type = T;
    // using dependent_vars_type = VariableSet< Variable< I, T >>;
    using result_type = value_type;
    static constexpr size_t index = I;

    constexpr T eval( variable_values& vars ) const
    { return any_cast< T >( vars[ I ] ); }
};

/** 
 * Dependency Helpers
 */
template< size_t I, typename... Vars >
struct VariableTypeInList;

template< size_t I >
struct VariableTypeInList< I >
{ using type = void; };

template< size_t I, size_t J, typename T, typename... Rest >
requires( I == J )
struct VariableTypeInList< I, Variable< J, T >, Rest... >
{ using type = T; };

template< size_t I, size_t J, typename T, typename... Rest >
requires( I != J )
struct VariableTypeInList< I, Variable< J, T >, Rest... >
{ using type = VariableTypeInList< I, Rest... >::type; };

template< size_t I, typename... Vars >
using variable_type_in_list_t = VariableTypeInList< I, Vars... >::type;

static_assert( is_same_v< void, variable_type_in_list_t< 0 >> );
static_assert( is_same_v< void, variable_type_in_list_t< 0, Variable< 1, int >>> );
static_assert( is_same_v< int, variable_type_in_list_t< 1, Variable< 1, int >>> );
static_assert( is_same_v< int, variable_type_in_list_t< 1, Variable< 1, int >, Variable< 0, int >>> );

template< size_t I, typename TupleT >
struct VariableTypeInTuple;

template< size_t I, typename... Vars >
struct VariableTypeInTuple< I, tuple< Vars... >>
{ using type = variable_type_in_list_t< I, Vars... >; };

template< size_t I, typename TupleT >
using variable_type_in_tuple_t = VariableTypeInTuple< I, TupleT >::type;

template< typename... Vars >
struct LeastUpperVariableIndex
{ static constexpr size_t value = least_upper_bound_v< Vars::index... >; };

template< typename... Vars >
static constexpr size_t least_upper_variable_index_v = 
    LeastUpperVariableIndex< Vars... >::value;

static_assert( least_upper_variable_index_v< Variable< 0, int >> == 1 );

template< typename VarTuple, typename Seq >
struct MakeVariableSetHelper;

template< typename VarTuple, size_t... Is >
struct MakeVariableSetHelper< VarTuple, seq< Is... >>
{ using type = tuple_cat_t< 
    std::conditional_t< std::is_same_v< void, variable_type_in_tuple_t< Is, VarTuple >>,
        tuple<>,
        tuple< Variable< Is, variable_type_in_tuple_t< Is, VarTuple >>>>... >; };

template< typename TupleT >
struct TupleToVariableSet;

template< typename... Ts >
struct TupleToVariableSet< tuple< Ts... >>
{ using type = VariableSet< Ts... >; };

template< typename... Vars >
struct MakeVariableSet
{ 
    static constexpr size_t least_upper_variable_index = 
        least_upper_variable_index_v< Vars... >;

    using type = TupleToVariableSet< 
        typename MakeVariableSetHelper< tuple< Vars... >, 
            make_seq< least_upper_variable_index >>::type >::type; 
};

template< typename... Vars >
using make_variable_set_t = MakeVariableSet< Vars... >::type;

static_assert( is_same_v< VariableSet<>, make_variable_set_t<> > );
static_assert( is_same_v< VariableSet< Variable< 0, int >>, 
    make_variable_set_t< Variable< 0, int >>> );
static_assert( is_same_v< VariableSet< Variable< 0, int >>, 
    make_variable_set_t< Variable< 0, int >, Variable< 0, int >>> );
static_assert( is_same_v< VariableSet< Variable< 0, int >, Variable< 1, int >>, 
    make_variable_set_t< Variable< 1, int >, Variable< 0, int >>> );

template< typename VarT, typename VarU >
struct IsSameVariable;

template< size_t I, typename T, size_t J, typename U >
struct IsSameVariable< Variable< I, T >, Variable< J, U >>
{ static constexpr bool value = ( I == J ); };

template< typename VarT, typename VarU >
static constexpr bool is_same_variable_v = IsSameVariable< VarT, VarU >::value;

static_assert(     is_same_variable_v< Variable< 0, int >, Variable< 0, int >> );
static_assert( not is_same_variable_v< Variable< 0, int >, Variable< 1, int >> );
static_assert( not is_same_variable_v< Variable< 0, int >, Variable< 1, float >> );
static_assert(     is_same_variable_v< Variable< 0, int >, Variable< 0, float >> );
static_assert(     is_same_variable_v< Variable< 1, int >, Variable< 1, int >> );
static_assert(     is_same_variable_v< Variable< 2, int >, Variable< 2, int >> );

template< typename VarT, typename DepT >
struct IncludesVariable;

template< typename VarT, typename... Vars >
struct IncludesVariable< VarT, VariableSet< Vars... >>
{ static constexpr bool value = ( is_same_variable_v< VarT, Vars > or ... ); };

template< typename VarT, typename DepT >
static constexpr bool includes_variable_v = 
    IncludesVariable< VarT, DepT >::value;

static_assert( includes_variable_v< Variable< 0, int >, VariableSet< Variable< 0, int >>> );
static_assert( includes_variable_v< Variable< 0, int >, VariableSet< Variable< 0, int >, Variable< 1, int >>> );
static_assert( not includes_variable_v< Variable< 0, int >, VariableSet< Variable< 1, int >>> );
static_assert( not includes_variable_v< Variable< 0, int >, VariableSet< Variable< 1, int >, Variable< 2, int >>> );
static_assert( includes_variable_v< Variable< 1, int >, VariableSet< Variable< 1, int >>> );
static_assert( includes_variable_v< Variable< 1, int >, VariableSet< Variable< 0, int >, Variable< 1, int >>> );

template< typename VarT, typename DepT >
struct AddDependency;

template< typename VarT, typename... Vars >
requires( includes_variable_v< VarT, VariableSet< Vars... >> )
struct AddDependency< VarT, VariableSet< Vars... >>
{ using type = VariableSet< Vars... >; };

template< typename VarT, typename... Vars >
requires( not includes_variable_v< VarT, VariableSet< Vars... >> )
struct AddDependency< VarT, VariableSet< Vars... >>
{ using type = VariableSet< VarT, Vars... >; };

template< typename VarT, typename DepT >
using add_dependency_t = AddDependency< VarT, DepT >::type;

static_assert( includes_variable_v< Variable< 0, int >, add_dependency_t< Variable< 0, int >, VariableSet< >>> );
static_assert( not includes_variable_v< Variable< 0, int >, add_dependency_t< Variable< 1, int >, VariableSet< >>> );
static_assert( includes_variable_v< Variable< 0, int >, add_dependency_t< Variable< 0, int >, VariableSet< Variable< 1, int >>>> );
static_assert( includes_variable_v< Variable< 1, int >, add_dependency_t< Variable< 0, int >, VariableSet< Variable< 1, int >>>> );
static_assert( includes_variable_v< Variable< 1, int >, add_dependency_t< Variable< 1, int >, VariableSet< Variable< 1, int >>>> );
static_assert( not includes_variable_v< Variable< 0, int >, add_dependency_t< Variable< 1, int >, VariableSet< Variable< 1, int >>>> );

template< typename... Deps >
struct MergeVariableSet;

template< typename... FirstVars, typename... SecondVars >
struct MergeVariableSet< VariableSet< FirstVars... >, VariableSet< SecondVars... >>
{ using type = make_variable_set_t< FirstVars..., SecondVars... >; };

template< typename... Vars >
struct MergeVariableSet< VariableSet< Vars... >>
{ using type = make_variable_set_t< Vars... >; };

template<>
struct MergeVariableSet<>
{ using type = VariableSet<>; };

template< typename First, typename... Rest >
requires( std::isgreater( sizeof...( Rest ), 1 ))
struct MergeVariableSet< First, Rest... >
{ using type = MergeVariableSet< First, 
    typename MergeVariableSet< Rest... >::type >::type; };

template< typename... Ts >
using merge_variable_set_t = MergeVariableSet< Ts... >::type;

static_assert( std::is_same_v< VariableSet< Variable< 0, int >>, 
    merge_variable_set_t< 
        VariableSet< Variable< 0, int >>, VariableSet< Variable< 0, int >>>> );
static_assert( std::is_same_v< VariableSet< Variable< 0, int >, Variable< 1, int >>, 
    merge_variable_set_t< 
        VariableSet< Variable< 0, int >>, VariableSet< Variable< 1, int >>>> );
static_assert( std::is_same_v< VariableSet< Variable< 0, int >, Variable< 1, int >>,
    merge_variable_set_t< VariableSet< Variable< 1, int >>, VariableSet< Variable< 0, int >>, VariableSet< Variable< 1, int >>>> );
static_assert( std::is_same_v< VariableSet< Variable< 0, int >>, 
    merge_variable_set_t< VariableSet< Variable< 0, int >>>> );

template< typename T >
struct DependentVariables;

template< size_t I, typename T >
struct DependentVariables< Variable< I, T >>
{ using type = VariableSet< Variable< I, T >>; };

template< typename T >
requires( is_expression_v< T > )
struct DependentVariables< T >
{ using type = T::dependent_vars_type; };

template< typename T >
requires( not is_expression_v< T > )
struct DependentVariables< T >
{ using type = VariableSet<>; };

template< typename... T >
using dependent_variables_t = merge_variable_set_t< 
    typename DependentVariables< T >::type... >;

static_assert( std::is_same_v< 
    dependent_variables_t< >,
    VariableSet< >> );
static_assert( std::is_same_v< 
    dependent_variables_t< int >,
    VariableSet< >> );
// static_assert( std::is_same_v< 
//     Variable< 0, int >::dependent_vars_type,
//     VariableSet< Variable< 0, int >>> );
static_assert( std::is_same_v< 
    DependentVariables< Variable< 0, int >>::type,
    VariableSet< Variable< 0, int >>> );
static_assert( std::is_same_v< 
    dependent_variables_t< Variable< 0, int >>,
    VariableSet< Variable< 0, int >>> );

template< size_t I, typename ExprT >
static constexpr bool depends_on_v = includes_variable_v< Variable< I, int >, dependent_variables_t< ExprT >>;


/**
 * Wrapper to identify something as an expression
 */
template< typename T >
struct Expression : ExpressionTag
{ 
    using value_type = T;
    using dependent_vars_type = dependent_variables_t< T >;
    using result_type = result_t< T >;

    static constexpr size_t dependents_size = dependent_vars_type::size;

    static constexpr bool contains_expression = true;

    constexpr value_type const& get() const
    { return _value; }

    constexpr result_type eval( variable_values& vars ) const
    { return expressions::eval( get(), vars ); }

    explicit operator value_type() const
    { return get(); }
    constexpr Expression() = default;
    explicit constexpr Expression( value_type const& expr ): _value{ expr } { }

    value_type _value;
};

template< typename T >
consteval T strip( T const& t )
{ return t; }

template< typename T >
consteval T strip( Expression< T > const& t )
{ return t.get(); }

template< typename T >
struct strip_expression
{ using type = T; };

template< typename T >
struct strip_expression< Expression< T >>
{ using type = T; };

template< typename T >
using stripped_t = strip_expression< T >::type;

/**
 * A static expression with an unchanging value defined at run-time
 */
template< typename T >
Expression< T > static_expr( T const& value )
{ return { value }; }

template< size_t I, typename T >
using variable = Expression< Variable< I, T >>;

/**
 * Constant expression
 */
template< typename T, T Value >
struct Constant : ExpressionTag
{
    using value_type = T;
    using result_type = value_type;
    using dependent_vars_type = VariableSet<>;
    static constexpr T value = Value;
    constexpr value_type const& get() const { return value; };

    consteval result_type eval( variable_values& ) const
    { return value; }
};

template< typename T, T Value >
using constant = Expression< Constant< T, Value >>;

static constexpr constant< long double, 0.l > constant_zero = constant< long double, 0.l >{{}};
static constexpr constant< long double, 1.l > constant_one = constant< long double, 1.l >{{}};
static constexpr constant< bool, true > constant_true = constant< bool, true >{{}};
static constexpr constant< bool, false > constant_false = constant< bool, false >{{}};


/**
 * a unary expression
 */
template< typename T >
struct Unary : ExpressionTag
{
    using value_type = T;
    using dependent_vars_type = dependent_variables_t< T >;
    constexpr value_type get() const { return _arg; }
    constexpr Unary( value_type arg ): _arg{ arg } { }
    value_type _arg;
};

/**
 * a binary expression
 */
template< typename T, typename U >
struct Binary : ExpressionTag
{
    using left_value_type = T;
    using right_value_type = U;
    using dependent_vars_type = dependent_variables_t< T, U >;

    constexpr left_value_type get_left() const { return _left; }
    constexpr right_value_type get_right() const { return _right; }

    constexpr Binary( T left, U right ) : _left{ left }, _right{ right }
    { }

    left_value_type _left;
    right_value_type _right;
};

/**
 * an nary expression
 */
template< typename... Ts >
struct Nary : ExpressionTag
{
    using value_tuple_type = tuple< Ts... >;
    using dependent_vars_type = dependent_variables_t< Ts... >;

    template< size_t I >
    constexpr tuple_element_t< I, value_tuple_type > get() const
    { return std::get< I >( _values ); }

    constexpr Nary( Ts... ts ): _values{ ts... } {}

    value_tuple_type _values;
};

/**
 * a negation
 */
template< typename T >
struct Negation : Unary< T >
{ 
    using result_type = decltype( -result_t< T >{} );
    constexpr result_type eval( variable_values& vars ) const
    { return -eval( Unary< T >::get(), vars ); }

    constexpr Negation( T arg ): Unary< T >{ arg } {} 
};

/**
 * a sum
 */
template< typename T, typename U >
struct Sum : Binary< T, U >
{ 
    using result_type = decltype( result_t< T >{} + result_t< U >{} );
    constexpr auto eval( variable_values& vars ) const
    { return expressions::eval( Binary< T, U >::get_left(), vars ) + 
        expressions::eval( Binary< T, U >::get_right(), vars ); }

    constexpr Sum( T left, U right ): 
        Binary< T, U >{ left, right } { } 
};

/**
 * a difference
 */
template< typename T, typename U >
struct Difference : Binary< T, U >
{ 
    using result_type = decltype( result_t< T >{} - result_t< U >{} );
    constexpr result_type eval( variable_values& vars ) const
    { return expressions::eval( Binary< T, U >::get_left(), vars ) - 
        expressions::eval( Binary< T, U >::get_right(), vars ); }

    constexpr Difference( T left, U right ): 
        Binary< T, U >{ left, right } { } 
};

/**
 * a product
 */
template< typename T, typename U >
struct Product : Binary< T, U >
{ 
    using result_type = decltype( result_t< T >{} * result_t< U >{} );
    constexpr result_type eval( variable_values& vars ) const
    { return expressions::eval( Binary< T, U >::get_left(), vars ) *
        expressions::eval( Binary< T, U >::get_right(), vars ); }

    constexpr Product( T left, U right ): 
        Binary< T, U >{ left, right } { } 
};

static_assert( not depends_on_v< 0, Expression< Product< int, Constant< long double, 0.l >>>> );

/**
 * a quotent
 */
template< typename T, typename U >
struct Quotient : Binary< T, U >
{ 
    using result_type = decltype( result_t< T >{} / result_t< U >{} );
    constexpr auto eval( variable_values& vars ) const
    { return expressions::eval( Binary< T, U >::get_left(), vars ) / 
        expressions::eval( Binary< T, U >::get_right(), vars ); }

    constexpr Quotient( T numerator, U denominator ):
        Binary< T, U >{ numerator, denominator } { } 
};

/**
 * ElementOf expression
 */
template< size_t I, typename ArrayT >
struct ElementOf: Unary< ArrayT >
{ 
    using result_type = result_t< tuple_element_t< I, ArrayT >>;

    constexpr result_type eval( variable_values& vars ) const
    { return expressions::eval( get< I >( Unary< ArrayT >::get() ), vars ); }

    constexpr ElementOf( ArrayT arr ): Unary< ArrayT >{ arr } {} 
};

/**
 * ArrayOf expression
 */
template< typename... Ts >
struct ArrayOf: Nary< Ts... >
{ 
    using result_type = tuple< result_t< Ts >... >;

    template< size_t... Is >
    constexpr result_type eval_helper( variable_values& vars, seq< Is... > ) const
    { return make_tuple( expressions::eval( Nary< Ts... >::template get< Is >(), vars )... ); }
    constexpr auto eval( variable_values& vars ) const
    { return eval_helper( vars, make_seq< sizeof...( Ts )>{} ); }

    constexpr ArrayOf( Ts... ts ): Nary< Ts... >{ ts... } {} 
};

/**
 * ElementOf expression
 */
template< size_t I, typename... Ts >
struct ElementOf< I, ArrayOf< Ts... >>: Unary< ArrayOf< Ts... >>
{ 
    using result_type = result_t< tuple_element_t< I, tuple< Ts... >>>;

    constexpr result_type eval( variable_values& vars ) const
    { return expressions::eval( Unary< ArrayOf< Ts... >>::get().template get< I >(), vars ); }

    constexpr ElementOf( ArrayOf< Ts... > arr ): 
        Unary< ArrayOf< Ts... > >{ arr } {} 
};

/**
 * equality testing
 */
template< typename T, typename U >
struct Equals : Binary< T, U >
{ 
    using result_type = bool;

    constexpr result_type eval( variable_values& vars ) const
    { return expressions::eval( Binary< T, U >::get_left(), vars ) == 
        expressions::eval( Binary< T, U >::get_right(), vars ); }

    constexpr Equals( T left, T right ):
        Binary< T, U >{ left, right } { } 
};


/**
 * an expression transformer for the derivative
 * expression transformers expose a type and a static construct function 
 * for that type
 */
template< size_t I, typename ExprT >
struct Derivative;

template< size_t I, typename ExprT >
using derivative_t = Derivative< I, ExprT >::type;

template< size_t I, typename ExprT >
constexpr derivative_t< I, ExprT > d( ExprT const& expr )
{ return Derivative< I, ExprT >::construct( expr ); }

template< size_t I, typename ExprT >
requires( not depends_on_v< I, ExprT > )
struct Derivative< I, ExprT >
{
    using type = Constant< result_t< ExprT >, (result_t< ExprT >)0 >;
    static constexpr type construct( ExprT const& expr )
    { return {}; }
};

template< size_t I, typename T >
struct Derivative< I, Variable< I, T >>
{
    using type = Constant< T, (T)1 >;
    static constexpr type construct( Variable< I, T > const& expr )
    { return {}; }
};

template< size_t I, typename ExprT >
requires( depends_on_v< I, ExprT > )
struct Derivative< I, Expression< ExprT >>
{
    using type = Expression< derivative_t< I, ExprT >>;
    static constexpr type construct( Expression< ExprT > const& expr )
    { return type{ d< I >( expr.get() ) }; }
};

template< size_t I, typename ExprT >
requires( depends_on_v< I, ExprT > )
struct Derivative< I, Negation< ExprT >>
{
    using type = Negation< derivative_t< I, ExprT >>;
    static constexpr type construct( Negation< ExprT > const& expr )
    { return { d< I >( expr.get() ) }; }
};

template< size_t I, typename LeftT, typename RightT >
requires( depends_on_v< I, LeftT > and depends_on_v< I, RightT > )
struct Derivative< I, Sum< LeftT, RightT >>
{
    using type = Sum< derivative_t< I, LeftT >, derivative_t< I, RightT >>;
    static constexpr type construct( Sum< LeftT, RightT > const& expr )
    { return { d< I >( expr.get_left() ), d< I >( expr.get_right() ) }; }
};

template< size_t I, typename LeftT, typename RightT >
requires( depends_on_v< I, LeftT > and not depends_on_v< I, RightT > )
struct Derivative< I, Sum< LeftT, RightT >>
{
    using type = derivative_t< I, LeftT >;
    static constexpr type construct( Sum< LeftT, RightT > const& expr )
    { return d< I >( expr.get_left() ); }
};

template< size_t I, typename LeftT, typename RightT >
requires( not depends_on_v< I, LeftT > and depends_on_v< I, RightT > )
struct Derivative< I, Sum< LeftT, RightT >>
{
    using type = derivative_t< I, RightT >;
    static constexpr type construct( Sum< LeftT, RightT > const& expr )
    { return d< I >( expr.get_right() ); }
};

template< size_t I, typename LeftT, typename RightT >
requires( depends_on_v< I, LeftT > and depends_on_v< I, RightT > )
struct Derivative< I, Difference< LeftT, RightT >>
{
    using type = Difference< derivative_t< I, LeftT >, derivative_t< I, RightT >>;
    static constexpr type construct( Difference< LeftT, RightT > const& expr )
    { return { d< I >( expr.get_left() ), d< I >( expr.get_right() ) }; }
};

template< size_t I, typename LeftT, typename RightT >
requires( depends_on_v< I, LeftT > and not depends_on_v< I, RightT > )
struct Derivative< I, Difference< LeftT, RightT >>
{
    using type = derivative_t< I, LeftT >;
    static constexpr type construct( Difference< LeftT, RightT > const& expr )
    { return d< I >( expr.get_left() ); }
};

template< size_t I, typename LeftT, typename RightT >
requires( not depends_on_v< I, LeftT > and depends_on_v< I, RightT > )
struct Derivative< I, Difference< LeftT, RightT >>
{
    using type = Negation< derivative_t< I, RightT >>;
    static constexpr type construct( Difference< LeftT, RightT > const& expr )
    { return { d< I >( expr.get_left() ) }; }
};

template< size_t I, typename LeftT, typename RightT >
requires( depends_on_v< I, LeftT > and depends_on_v< I, RightT > )
struct Derivative< I, Product< LeftT, RightT >>
{
    using type = Sum< Product< derivative_t< I, LeftT >, RightT >, 
        Product< LeftT, derivative_t< I, RightT >>>;
    static constexpr type construct( Product< LeftT, RightT > const& expr )
    { return {{ d< I >( expr.get_left() ), expr.get_right() },
        { expr.get_left(), d< I >( expr.get_right() ) }}; }
};

template< size_t I, typename LeftT, typename RightT >
requires( depends_on_v< I, LeftT > and not depends_on_v< I, RightT > )
struct Derivative< I, Product< LeftT, RightT >>
{
    using type = Product< derivative_t< I, LeftT >, RightT >;

    static constexpr type construct( Product< LeftT, RightT > const& expr )
    { return { d< I >( expr.get_left() ), expr.get_right() }; }
};

template< size_t I, typename LeftT, typename RightT >
requires( not depends_on_v< I, LeftT > and depends_on_v< I, RightT > )
struct Derivative< I, Product< LeftT, RightT >>
{
    using type = Product< LeftT, derivative_t< I, RightT >>;
    static constexpr type construct( Product< LeftT, RightT > const& expr )
    { return { expr.get_left(), d< I >( expr.get_right() ) }; }
};

template< size_t I, typename NumT, typename DenT >
requires( depends_on_v< I, NumT > and depends_on_v< I, DenT > )
struct Derivative< I, Quotient< NumT, DenT >>
{
    using type = Quotient< Difference< 
        Product< derivative_t< I, NumT >, DenT >, Product< NumT, derivative_t< I, DenT >>>, 
        Product< DenT, DenT >>;

    static constexpr type construct( Quotient< NumT, DenT > const& expr )
    { return {{{ d< I >( expr.get_left() ), expr.get_right() }, { expr.get_left(), d< I >( expr.get_right() )}}, 
            { expr.get_right(), expr.get_right() }}; }
};

template< size_t I, typename NumT, typename DenT >
requires( not depends_on_v< I, NumT > and depends_on_v< I, DenT > )
struct Derivative< I, Quotient< NumT, DenT >>
{
    using type = Quotient< Negation< Product< NumT, derivative_t< I, DenT >>>, 
        Product< DenT, DenT >>;

    static constexpr type construct( Quotient< NumT, DenT > const& expr )
    { return {{{ expr.get_left(), d< I >( expr.get_right() )}}, 
            { expr.get_right(), expr.get_right() }}; }
};

template< size_t I, typename NumT, typename DenT >
requires( depends_on_v< I, NumT > and not depends_on_v< I, DenT > )
struct Derivative< I, Quotient< NumT, DenT >>
{
    using type = Quotient< derivative_t< I, NumT >, DenT >;

    static constexpr type construct( Quotient< NumT, DenT > const& expr )
    { return { d< I >( expr.get_left()), expr.get_right() }; }
};

/**
 * OPERATORS
 */

// arrays
template< typename... Ts >
constexpr auto array_of( Ts const&... ts )
{ return Expression< ArrayOf< stripped_t< Ts >... >>{{ strip( ts )... }}; }

// accessor
template< size_t I, typename T >
constexpr auto element_of( Expression< T > const& arg )
{ return Expression< ElementOf< I, T >>{ arg.get() }; }

template< size_t I, typename T >
constexpr auto element_of( T const& arg )
{ return Expression< ElementOf< I, T >>{ arg }; }

// negation
template< typename T >
constexpr auto operator-( Expression< T > const& arg )
{ return Expression< Negation< T >>{ arg.get() }; }

// addition
template< typename T, typename U >
constexpr auto operator+( Expression< T > const& left, 
    Expression< U > const& right )
{ return Expression< Sum< T, U >>{{  left.get(), right.get() }}; }

template< typename T, typename U >
constexpr auto operator+( Expression< T > const& left, 
    U const& right )
{ return Expression< Sum< T, U >>{{ left.get(), right }}; }

template< typename T, typename U >
constexpr auto operator+( T const& left, 
    Expression< U > const& right )
{ return Expression< Sum< T, U >>{{ left, right.get() }}; }

// subtraction
template< typename T, typename U >
constexpr auto operator-( Expression< T > const& left, Expression< U > const& right )
{ return Expression< Difference< T, U >>{{ left.get(), right.get() }}; }

template< typename T, typename U >
constexpr auto operator-( Expression< T > const& left, 
    U const& right )
{ return Expression< Difference< T, U >>{{ left.get(), right }}; }

template< typename T, typename U >
constexpr auto operator-( T const& left, 
    Expression< U > const& right )
{ return Expression< Difference< T, U >>{{ left, right.get() }}; }

// multiplication
template< typename T, typename U >
constexpr auto operator*( Expression< T > const& left, 
    Expression< U > const& right )
{ return Expression< Product< T, U >>{{ left.get(), right.get() }}; }

template< typename T, typename U >
constexpr auto operator*( T const& left, 
    Expression< U > const& right )
{ return Expression< Product< T, U >>{{ left, right.get() }}; }

template< typename T, typename U >
constexpr auto operator*( Expression< T > const& left, U const& right )
{ return Expression< Product< T, U >>{{ left.get(), right }}; }

// division
template< typename T, typename U >
constexpr auto operator/( Expression< T > const& left, U const& right )
{ return Expression< Quotient< T, U >>{{ left.get(), right }}; }

template< typename T, typename U >
constexpr auto operator/( T const& left, Expression< U > const& right )
{ return Expression< Quotient< T, U >>{{ left, right.get() }}; }

template< typename T, typename U >
constexpr auto operator/( Expression< T > const& left, 
    Expression< U > const& right )
{ return Expression< Quotient< T, U >>{{ left.get(), right.get() }}; }

// equality
template< typename T, typename U >
constexpr auto operator==( Expression< T > const& left, 
    Expression< U > const& right )
{ return Expression< Equals< T, U >>{{ left.get(), right.get() }}; }

} // namespace expressions


#endif