#ifndef __EXPRESSIONS_HPP__
#define __EXPRESSIONS_HPP__

#include "tensor.v1.2.hpp"
#include "utility.hpp"

#include <cmath>
#include <type_traits>
#include <tuple>
#include <map>
#include <any>
#include <string>

#ifndef NO_EXPRESSION_PRINTING
#include <format>
#endif

// #include <string_view>

namespace expressions {

using std::size_t;
using std::tuple, std::make_tuple, std::tuple_element_t, std::get;
using std::map;
using std::any, std::any_cast;

using namespace tensor;


/// @brief base class of any expression
///
struct ExpressionTag 
{ 
    // using result_type...
    // using dependent_vars_type...
    // constexpr result_type eval( variable_values& ) const...
};

/// @brief trait to identify a type as an expression type
/// @tparam T is the type to be checked
template< typename T >
struct is_expression
{ static constexpr bool value = std::is_base_of_v< ExpressionTag, T >; };

/// @brief is true if T is an expression type
/// @tparam T is the type to be checked
///
template< typename T >
static constexpr bool is_expression_v = is_expression< T >::value;

template< typename T >
concept expression = is_expression_v< T >;

/// @brief storage for values of variables
///
struct variable_values 
{ 
    /// @brief access the value of variable I without modifying the container
    /// @param I is the index of the variable
    /// @return an std::any containing the value of the variable, if it exists
    ///
    any operator[]( size_t I ) const 
    { 
        auto i = values.find( I );
        if( i == values.end() )
            return {};
        return i->second;
    }

    /// @brief access the value of variable I creating a new entry if this value
    ///        does not exist
    /// @param I is the index of the variable
    /// @return an std::any reference containing the value of the variable
    ///
    any& operator[]( size_t I ) { return values[I]; }

    /// map of the variable values
    map< size_t, any > values;
};

/// @brief extracts a typed variable value from a values container
/// @tparam T the type to be returned
/// @tparam I the index of the variable
/// @param vals the variable values
/// @return the value of variable I cast to type T
///
template< size_t I, typename T >
constexpr T get_variable_value( variable_values const& vals )
{ return any_cast< T >( vals[I] ); }

namespace detail {
/// @brief evaluates a type T
/// @tparam T the type to be evaluated
///
template< typename T >
struct Evaluator;

/// @brief evaluator for expressions of type T
/// @tparam T the type of the expression
///
template< typename T >
requires( is_expression_v< T > )
struct Evaluator< T >
{ 
    using result_type = T::result_type;

    constexpr static result_type evaluate( T const& expr, 
        variable_values& vars )
    { return expr.eval( vars ); }
};

/// @brief evaluator for non-expression types
/// @tparam T the type of the non-expression
///
template< typename T >
requires( not is_expression_v< T > )
struct Evaluator< T >
{ 
    using result_type = T;

    // always return the value itselv
    constexpr static T evaluate( T const& value, variable_values& )
    { return value; }; 
};

} // namespace detail

/// @brief trait for the result_type of an expression
/// @tparam T the tested type
///
template< typename T >
using result_t = detail::Evaluator< T >::result_type;

/// @brief evaluates an expression given a set of variable values
/// @tparam T the expression type
/// @param expr the expression to be evaluated
/// @param vars the values of variables in the expression
/// @return the result of evaluating expression expr using values stored in vars
///
template< typename T >
constexpr result_t< T > eval( T const& expr, 
    variable_values& vars)
{ return detail::Evaluator< T >::evaluate( expr, vars ); }


/// @brief a depnedent variable
/// @tparam T the type the variable represents
/// @tparam I the unique index of this variable 
/// 
/// variables are uniquely identified only by their index.  Undefined behavior
/// will occur if two variables with the same index but different types are 
/// used in the same expression
///
template< size_t I, typename T >
// DT: do we need this?
// requires( not is_expression_v< T > )
struct Variable : ExpressionTag
{
    using value_type = T;
    // using dependent_vars_type = VariableSet< Variable< I, T >>;
    using result_type = value_type;
    static constexpr size_t index = I;

    constexpr std::string name() const
    { return _name; }

    constexpr T eval( variable_values& vars ) const
    { return any_cast< T >( vars[ I ] ); }

    std::string _name = "var";

#ifndef NO_EXPRESSION_PRINTING
    constexpr Variable( std::string format ): 
        _name{ std::format( std::runtime_format( format ), I ) } {}
    constexpr Variable(): _name{ std::format( "var_{}", I ) } {}
#endif
};


/// @brief Set of variables used in an expression
/// @tparam ...Vars are the variable types
///
template< typename... Vars >
struct VariableSet;

/// @brief a set containing at least one variable
/// @tparam FirstT is the first variable type
/// @tparam ...Rest are the remaining variable types
///
template< typename FirstT, typename... Rest >
struct VariableSet< FirstT, Rest... >
{
    static constexpr size_t size() { return 1 + sizeof...( Rest ); }
    using first = FirstT;
    using rest = VariableSet< Rest... >;
};

/// @brief an empty set of variables
///
template<>
struct VariableSet<>
{ static constexpr size_t size() { return 0; } };


/// depdency details
namespace detail {

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

template< typename... Ts >
struct DependentVariables< tuple< Ts... >>
{ using type = merge_variable_set_t< 
    typename DependentVariables< Ts >::type... >; };

template< typename ShapeT, typename... Ts >
struct DependentVariables< Tensor< ShapeT, Ts... >>
{ using type = merge_variable_set_t< 
    typename DependentVariables< Ts >::type... >; };

} // namespace detail

/// @brief resolve the dependent variables in an expression
/// @tparam ...T the expressions
///
template< typename... T >
using dependent_variables_t = detail::merge_variable_set_t< 
    typename detail::DependentVariables< T >::type... >;


static_assert( std::is_same_v< 
    dependent_variables_t< >,
    VariableSet< >> );
static_assert( std::is_same_v< 
    dependent_variables_t< int >,
    VariableSet< >> );
static_assert( std::is_same_v< 
    detail::DependentVariables< Variable< 0, int >>::type,
    VariableSet< Variable< 0, int >>> );
static_assert( std::is_same_v< 
    dependent_variables_t< Variable< 0, int >>,
    VariableSet< Variable< 0, int >>> );

template< size_t I, typename ExprT >
static constexpr bool depends_on_v = 
    detail::includes_variable_v< Variable< I, int >, 
        dependent_variables_t< ExprT >>;

template< typename... Ts >
consteval dependent_variables_t< Ts... > depends( Ts const&... )
{ return {}; }

/// @brief evaluates an expression that does not have any depdendent variables
/// @tparam T the expression type
/// @param expr the expression to be evaluated
/// @return the result of evaluating expr
///
template< typename T >
requires( dependent_variables_t< T >::size() == 0 )
constexpr result_t< T > eval( T const& expr )
{ 
    variable_values _;
    return eval( expr, _ ); 
}

/// @brief wrapper to turn any type into an expression
/// @tparam T the wrapped type
///
template< typename T >
requires( not is_expression_v< T > ) // not sure if we need this.. meta expressions?
struct Expression : ExpressionTag
{ 
    using value_type = T;
    using dependent_vars_type = dependent_variables_t< T >;
    using result_type = result_t< T >;

    static constexpr size_t dependents_size = dependent_vars_type::size();

    static constexpr bool contains_expression = true;

    constexpr value_type get() const
    { return _value; }

    constexpr result_type eval( variable_values& vars ) const
    { return expressions::eval( get(), vars ); }

    // casting to and from an expression should be explicit
    explicit operator value_type() const
    { return get(); } 
    explicit constexpr Expression( value_type const& expr ): _value{ expr } { }

    constexpr Expression() = default;

    // template< typename Arg, typename... Args >
    // explicit constexpr Expression( Arg&& arg, Args&&... args ): _value{ arg, args... } {}

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

/// @brief helper to create expressions with unchanging values
/// @tparam T the type of this expression
/// @param value the value of this expression
/// @return returns an expression that will always evaluate to value
///
template< typename T >
requires( not is_expression_v< T > )
Expression< T > static_expr( T const& value )
{ return Expression< T >{ value }; }


template< size_t I, typename T >
// using variable = Expression< Variable< I, T >>;
using variable = Variable< I, T >;

template< size_t I, typename T >
constexpr variable< I, T > make_variable( std::string name_format = "var{}" )
{ return {{ name_format }}; }

/**
 * Constant expression
 */
template< typename T, T Value >
struct Constant : ExpressionTag
{
    using value_type = T;
    using result_type = value_type;
    using dependent_vars_type = VariableSet<>;
    static consteval size_t size() { return 1; }
    static constexpr T value = Value;

    constexpr operator T() const
    { return value; }

    constexpr value_type get() const { return value; };

    consteval result_type eval( variable_values& ) const
    { return value; }
};

template< typename T, T Value >
using constant = Constant< T, Value >;

static constexpr constant< long double, 0.l > constant_zero = constant< long double, 0.l >{{}};
static constexpr constant< long double, 1.l > constant_one = constant< long double, 1.l >{{}};
static constexpr constant< bool, true > constant_true = constant< bool, true >{{}};
static constexpr constant< bool, false > constant_false = constant< bool, false >{{}};


/**
 * a negation
 */
template< typename T >
struct Negation: ExpressionTag
{ 
    using result_type = decltype( -result_t< T >{} );
    using dependent_vars_type = dependent_variables_t< T >;

    constexpr T arg() const { return _arg; }

    constexpr result_type eval( variable_values& vars ) const
    { return -expressions::eval( arg(), vars ); }

    constexpr Negation( T arg ): _arg{ arg } {} 

    T _arg;
};

/**
 * a sum
 */
template< typename T, typename U >
struct Sum: ExpressionTag
{ 
    using result_type = decltype( result_t< T >{} + result_t< U >{} );
    using dependent_vars_type = dependent_variables_t< T, U >;

    constexpr T left_arg() const { return _left; }
    constexpr U right_arg() const { return _right; }

    constexpr auto eval( variable_values& vars ) const
    { return expressions::eval( left_arg(), vars ) + 
        expressions::eval( right_arg(), vars ); }

    constexpr Sum( T left, U right ): 
        _left{ left }, _right{ right } { } 
    
    T _left;
    U _right;
};

/**
 * a difference
 */
template< typename T, typename U >
struct Difference: ExpressionTag
{ 
    using result_type = decltype( result_t< T >{} - result_t< U >{} );
    using dependent_vars_type = dependent_variables_t< T, U >;

    constexpr T left_arg() const { return _left; }
    constexpr U right_arg() const { return _right; }

    constexpr result_type eval( variable_values& vars ) const
    { return expressions::eval( left_arg(), vars ) - 
        expressions::eval( right_arg(), vars ); }

    constexpr Difference( T left, U right ): 
        _left{ left }, _right{ right } { } 

    T _left;
    U _right;
};

/**
 * a product
 */
template< typename T, typename U >
struct Product: ExpressionTag
{ 
    using result_type = decltype( result_t< T >{} * result_t< U >{} );
    using dependent_vars_type = dependent_variables_t< T, U >;

    constexpr T left_arg() const { return _left; }
    constexpr U right_arg() const { return _right; }

    constexpr result_type eval( variable_values& vars ) const
    { return expressions::eval( left_arg(), vars ) *
        expressions::eval( right_arg(), vars ); }

    constexpr Product( T left, U right ): 
        _left{ left }, _right{ right } { } 

    T _left;
    U _right;
};

static_assert( not depends_on_v< 0, Product< int, Constant< long double, 0.l >>> );

/**
 * a quotient
 */
template< typename T, typename U >
struct Quotient: ExpressionTag
{ 
    using result_type = decltype( result_t< T >{} / result_t< U >{} );
    using dependent_vars_type = dependent_variables_t< T, U >;

    constexpr T numerator_arg() const { return _numerator; }
    constexpr U denominator_arg() const { return _denominator; }

    constexpr result_type eval( variable_values& vars ) const
    { return expressions::eval( numerator_arg(), vars ) / 
        expressions::eval( denominator_arg(), vars ); }

    constexpr Quotient( T numerator, U denominator ):
        _numerator{ numerator }, _denominator{ denominator } { } 

    T _numerator;
    U _denominator;
};

/**
 * square root
 */
template< typename T >
struct SquareRoot: ExpressionTag
{
    using result_type = decltype( std::sqrt( result_t< T >{} ));
    using dependent_vars_type = dependent_variables_t< T >;

    constexpr T arg() const { return _arg; }

    constexpr result_type eval( variable_values& vars ) const
    { return std::sqrt( expressions::eval( arg() )); }

    constexpr SquareRoot( T arg ): _arg{ arg } {} 

    T _arg;
};

/**
 * power
 */
template< int Exp, typename T >
struct Power: ExpressionTag
{
    using result_type = decltype( std::pow< Exp >( result_t< T >{} ));
    using dependent_vars_type = dependent_variables_t< T >;

    constexpr T arg() const { return _arg; }

    constexpr result_type eval( variable_values& vars ) const
    { return std::pow< Exp >( expressions::eval( arg(), vars )); }

    constexpr Power( T arg ): _arg{ arg } {} 

    T _arg;
};

/**
 * Element expression
 */
template< size_t I, typename ArrayT >
struct Element: tuple_element_t< I, ArrayT >
{  
    constexpr Element( ArrayT arr ): 
        tuple_element_t< I, ArrayT >{ std::get< I >( arr ) }
    { }
};

/// @brief represents the contraction of a tensor 
/// @tparam TensorT is the type of the tensor
/// @tparam I is the first index of contraction
/// @tparam J is the second index of contraction
///
template< size_t I, size_t J, typename TensorT >
struct Contraction: ExpressionTag
{
    using result_type = decltype( contract< I, J>( result_t< TensorT >{} ));
    using dependent_vars_type = dependent_variables_t< TensorT >;

    constexpr TensorT arg() const { return _arg; }

    constexpr result_type eval( variable_values& vars ) const
    { return contract< I, J >( eval( arg(), vars )); }

    constexpr Contraction( TensorT arg ): _arg{ arg } { }

    TensorT _arg;
};

/**
 * Array expression
 */
template< typename... Ts >
struct Array: tuple< Ts... >, ExpressionTag
{ 
    static consteval size_t size() { return sizeof...( Ts ); }
    using shape_type = Shape< size() >;
    using result_type = tuple< result_t< Ts >... >;
    using dependent_vars_type = dependent_variables_t< Ts... >;


    template< size_t... Is >
    constexpr result_type eval_helper( variable_values& vars, 
        seq< Is... > ) const
    { return make_tuple( expressions::eval( 
        std::get< Is >( *this ), vars )... ); }

    constexpr auto eval( variable_values& vars ) const
    { return eval_helper( vars, make_seq< sizeof...( Ts )>{} ); }

    constexpr Array( Ts... ts ): tuple< Ts... >{ ts... } { } 
};

template< typename ShapeT, typename... Ts >
requires( ShapeT::size() == sizeof...( Ts ))
struct ShapedArray : ExpressionTag
{
    using shape_type = ShapeT;
    static consteval size_t size() { return sizeof...( Ts ); }

    
};


// template< size_t I, typename... Ts >
// struct ElementOf< I, Array< Ts... >>: Unary< Ts...[I] >
// {
//     using result_type = result_t< Ts...[I] >;

//     using Unary< Ts...[I] >::get;

//     constexpr auto eval( variable_values& vars ) const
//     { return eval( get(), vars ); }

//     constexpr ElementOf( Array< Ts... > expr ): 
//         Unary< Ts...[I] >{ std::get< I >( expr ) }
//     { }
// };

// template< size_t I, typename T, typename U >
// struct ElementOf< I, Product< T, U >>:
//     Product< ElementOf< I / U::size(), T >, ElementOf< I % U::size(), U >>
// {
//     constexpr ElementOf( Product< T, U >> expr ):
//         Product< ElementOf< I / U::size(), T >, ElementOf< I % U::size(), U >>
//         { expr.left_arg(), expr.right_arg() }
//     { }
// };

/**
 * equality testing
 */
template< typename T, typename U >
struct Equals : ExpressionTag
{ 
    using result_type = bool;
    using dependent_vars_type = dependent_variables_t< T, U >;

    constexpr T left_arg() const { return _left; }
    constexpr U right_arg() const { return _right; }

    constexpr result_type eval( variable_values& vars ) const
    { return expressions::eval( left_arg(), vars ) == 
        expressions::eval( right_arg(), vars ); }

    constexpr Equals( T left, U right ): 
        _left{ left }, _right{ right } { } 
    
    T _left;
    U _right;
};

/**
 * logical operations
 */
template< typename T, typename U >
struct Conjunction: ExpressionTag
{
    using result_type = bool;
    using dependent_vars_type = dependent_variables_t< T, U >;

    constexpr T left_arg() const { return _left; }
    constexpr U right_arg() const { return _right; }

    constexpr result_type eval( variable_values& vars ) const
    { return expressions::eval( left_arg(), vars ) and
        expressions::eval( right_arg(), vars ); }

    constexpr Conjunction( T left, U right ): 
        _left{ left }, _right{ right } { } 
    
    T _left;
    U _right;
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
    { return { d< I >( expr.left_arg() ), d< I >( expr.right_arg() ) }; }
};

template< size_t I, typename LeftT, typename RightT >
requires( depends_on_v< I, LeftT > and not depends_on_v< I, RightT > )
struct Derivative< I, Sum< LeftT, RightT >>
{
    using type = derivative_t< I, LeftT >;
    static constexpr type construct( Sum< LeftT, RightT > const& expr )
    { return d< I >( expr.left_arg() ); }
};

template< size_t I, typename LeftT, typename RightT >
requires( not depends_on_v< I, LeftT > and depends_on_v< I, RightT > )
struct Derivative< I, Sum< LeftT, RightT >>
{
    using type = derivative_t< I, RightT >;
    static constexpr type construct( Sum< LeftT, RightT > const& expr )
    { return d< I >( expr.right_arg() ); }
};

template< size_t I, typename LeftT, typename RightT >
requires( depends_on_v< I, LeftT > and depends_on_v< I, RightT > )
struct Derivative< I, Difference< LeftT, RightT >>
{
    using type = Difference< derivative_t< I, LeftT >, derivative_t< I, RightT >>;
    static constexpr type construct( Difference< LeftT, RightT > const& expr )
    { return { d< I >( expr.left_arg() ), d< I >( expr.right_arg() ) }; }
};

template< size_t I, typename LeftT, typename RightT >
requires( depends_on_v< I, LeftT > and not depends_on_v< I, RightT > )
struct Derivative< I, Difference< LeftT, RightT >>
{
    using type = derivative_t< I, LeftT >;
    static constexpr type construct( Difference< LeftT, RightT > const& expr )
    { return d< I >( expr.left_arg() ); }
};

template< size_t I, typename LeftT, typename RightT >
requires( not depends_on_v< I, LeftT > and depends_on_v< I, RightT > )
struct Derivative< I, Difference< LeftT, RightT >>
{
    using type = Negation< derivative_t< I, RightT >>;
    static constexpr type construct( Difference< LeftT, RightT > const& expr )
    { return { d< I >( expr.left_arg() ) }; }
};

template< size_t I, typename LeftT, typename RightT >
requires( depends_on_v< I, LeftT > and depends_on_v< I, RightT > )
struct Derivative< I, Product< LeftT, RightT >>
{
    using type = Sum< Product< derivative_t< I, LeftT >, RightT >, 
        Product< LeftT, derivative_t< I, RightT >>>;
    static constexpr type construct( Product< LeftT, RightT > const& expr )
    { return {{ d< I >( expr.left_arg() ), expr.right_arg() },
        { expr.left_arg(), d< I >( expr.right_arg() ) }}; }
};

template< size_t I, typename LeftT, typename RightT >
requires( depends_on_v< I, LeftT > and not depends_on_v< I, RightT > )
struct Derivative< I, Product< LeftT, RightT >>
{
    using type = Product< derivative_t< I, LeftT >, RightT >;

    static constexpr type construct( Product< LeftT, RightT > const& expr )
    { return { d< I >( expr.left_arg() ), expr.right_arg() }; }
};

template< size_t I, typename LeftT, typename RightT >
requires( not depends_on_v< I, LeftT > and depends_on_v< I, RightT > )
struct Derivative< I, Product< LeftT, RightT >>
{
    using type = Product< LeftT, derivative_t< I, RightT >>;
    static constexpr type construct( Product< LeftT, RightT > const& expr )
    { return { expr.left_arg(), d< I >( expr.right_arg() ) }; }
};

template< size_t I, typename NumT, typename DenT >
requires( depends_on_v< I, NumT > and depends_on_v< I, DenT > )
struct Derivative< I, Quotient< NumT, DenT >>
{
    using type = Quotient< Difference< 
        Product< derivative_t< I, NumT >, DenT >, Product< NumT, derivative_t< I, DenT >>>, 
        Product< DenT, DenT >>;

    static constexpr type construct( Quotient< NumT, DenT > const& expr )
    { return {{{ d< I >( expr.left_arg() ), expr.right_arg() }, { expr.left_arg(), d< I >( expr.right_arg() )}}, 
            { expr.right_arg(), expr.right_arg() }}; }
};

template< size_t I, typename NumT, typename DenT >
requires( not depends_on_v< I, NumT > and depends_on_v< I, DenT > )
struct Derivative< I, Quotient< NumT, DenT >>
{
    using type = Quotient< Negation< Product< NumT, derivative_t< I, DenT >>>, 
        Product< DenT, DenT >>;

    static constexpr type construct( Quotient< NumT, DenT > const& expr )
    { return {{{ expr.left_arg(), d< I >( expr.right_arg() )}}, 
            { expr.right_arg(), expr.right_arg() }}; }
};

template< size_t I, typename NumT, typename DenT >
requires( depends_on_v< I, NumT > and not depends_on_v< I, DenT > )
struct Derivative< I, Quotient< NumT, DenT >>
{
    using type = Quotient< derivative_t< I, NumT >, DenT >;

    static constexpr type construct( Quotient< NumT, DenT > const& expr )
    { return { d< I >( expr.numerator_arg()), expr.denominator_arg() }; }
};

template< size_t I, typename ExprT >
requires( depends_on_v< I, ExprT > )
struct Derivative< I, SquareRoot< ExprT >>
{
    using type = Product< Constant< long double, -0.5l >, 
        Quotient< derivative_t< I, ExprT >, SquareRoot< ExprT >>>;
    static constexpr type construct( SquareRoot< ExprT > const& expr )
    { return {{}, { d< I >( expr.get() ), expr }}; }
};

template< size_t I, int Exp, typename ExprT >
requires( depends_on_v< I, ExprT > )
struct Derivative< I, Power< Exp, ExprT >>
{
    using type = Product< Constant< int, Exp >, 
        Product< Power< Exp-1, ExprT >, derivative_t< I, ExprT >>>;

    static constexpr type construct( Power< Exp, ExprT > const& expr )
    { return {{}, { expr, d< I >( expr.get() ) }}; }
};

/**
 * OPERATORS
 */

// arrays
// template< typename... Ts >
// constexpr auto array_of( Ts const&... ts )
// { return Expression< Array< stripped_t< Ts >... >>{{ strip( ts )... }}; }

// // accessor
// template< size_t I, typename T >
// constexpr auto element_of( Expression< T > const& arg )
// { return Expression< ElementOf< I, T >>{ arg.get() }; }

// template< size_t I, typename T >
// constexpr auto element_of( T const& arg )
// { return Expression< ElementOf< I, T >>{ arg }; }

// negation
template< expression T >
constexpr auto operator -( T const& arg )
{ return Negation< T >{ arg }; }

// addition
template< expression T, expression U >
constexpr auto operator +( T const& left, U const& right )
{ return Sum< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator +( T const& left, U const& right )
{ return Sum< T, Expression< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator +( T const& left, U const& right )
{ return Sum< Expression< T >, U >{ static_expr( left ), right }; }

// subtraction
template< expression T, expression U >
constexpr auto operator -( T const& left, U const& right )
{ return Difference< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator -( T const& left, U const& right )
{ return Difference< T, Expression< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator -( T const& left, U const& right )
{ return Difference< Expression< T >, U >{ static_expr( left ), right }; }

// multiplication
template< expression T, expression U >
constexpr auto operator *( T const& left, U const& right )
{ return Product< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator *( T const& left, U const& right )
{ return Product< T, Expression< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator *( T const& left, U const& right )
{ return Product< Expression< T >, U >{ static_expr( left ), right }; }

// division
template< expression T, expression U >
constexpr auto operator /( T const& left, U const& right )
{ return Quotient< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator /( T const& left, U const& right )
{ return Quotient< T, Expression< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator /( T const& left, U const& right )
{ return Quotient< Expression< T >, U >{ static_expr( left ), right }; }

// sqrt
template< expression T >
constexpr auto sqrt( T const& arg )
{ return SquareRoot< T >{ arg }; }

// pow
template< int Exp, expression T >
constexpr auto pow( T const& arg )
{ return Power< Exp, T >{ arg }; }

// equality
template< expression T, expression U >
constexpr auto operator ==( T const& left, U const& right )
{ return Equals< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator ==( T const& left, U const& right )
{ return Equals< T, Expression< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator ==( T const& left, U const& right )
{ return Equals< Expression< T >, U >{ static_expr( left ), right }; }

template< typename TupleT, typename TupleU, size_t... Is >
constexpr auto tuple_equals_helper( TupleT const& left, TupleU const& right,
    seq< Is... > )
{ return (( get< Is >( left ) == get< Is >( right )) and ... ); }

template< typename TensorT, typename TensorU, size_t... Is >
constexpr auto tensor_equals_helper( TensorT const& left, TensorU const& right,
    seq< Is... > )
{ return (( tensor_get< Is >( left ) == get< Is >( right )) and ... ); }

template< typename... Ts, typename... Us >
requires( sizeof...( Ts ) == sizeof...( Us ) and 
    (( expression< Ts > or ... ) or ( expression< Us > or ... )))
constexpr auto operator==( tuple< Ts... > const& left, 
    tuple< Us... > const& right )
{ return tuple_equals_helper( left, right, make_seq< sizeof...( Ts )>{} ); }

template< typename ShapeT, typename... Ts, typename... Us >
requires( (( expression< Ts > or ... ) or ( expression< Us > or ... )))
constexpr auto operator==( Tensor< ShapeT, Ts... > const& left,
    Tensor< ShapeT, Us... > const& right )
{ return tensor_equals_helper( left, right, make_seq< sizeof...( Ts )>{} ); }

// logical operations
template< expression T, expression U >
constexpr auto operator and( T const& left, U const& right )
{ return Conjunction< T, U >{ left, right }; }

template< expression T, typename U >
requires( not expression< U > )
constexpr auto operator and( T const& left, U const& right )
{ return Conjunction< T, Expression< U >>{ left, static_expr( right )}; }

template< typename T, expression U >
requires( not expression< T > )
constexpr auto operator and( T const& left, U const& right )
{ return Conjunction< Expression< T >, U >{ static_expr( left ), right }; }

/**
 * solver
 */
template< typename ExprT >
struct Solver;

template< typename SolverT, typename ExprT >
constexpr variable_values solve( ExprT const& expr, SolverT&& solver = {} )
{
    // get the dependent variables for this expression

    // construct a container for the variable values
    variable_values values;

    initialize_variables( values, expr, solver );
    // 
};

} // namespace expressions


#ifndef NO_EXPRESSION_PRINTING

namespace std {

// template< size_t I, typename... Ts >
// Ts...[I] get( expressions::Array< Ts... > const& arr )
// { return get< I >( arr.values() ); }


template< typename ExprT >
struct formatter< expressions::Expression< ExprT >, char >: 
    formatter< ExprT >
{
    template< typename FormatContext >
    FormatContext::iterator format( expressions::Expression< ExprT > expr, 
        FormatContext& ctx ) const
    { return formatter< ExprT >::format( expr.get(), ctx ); }
};

template< typename T, T Value >
struct formatter< expressions::Constant< T, Value >, char >: 
    formatter< T >
{
    template< typename FormatContext >
    FormatContext::iterator format( expressions::Constant< T, Value > expr, 
        FormatContext& ctx ) const
    { return formatter< T >::format( Value, ctx ); }
};

template< size_t I, typename T >
struct formatter< expressions::Variable< I, T >, char >:
    formatter< std::string >
{
    string format_string;

    template< typename FormatContext >
    FormatContext::iterator format( expressions::Variable< I, T > expr, 
        FormatContext& ctx ) const
    { return formatter< std::string >::format( expr.name(), ctx ); }
};

template< typename LeftT, typename RightT >
struct formatter< expressions::Sum< LeftT, RightT >, char >:
    formatter< std::string >
{   
    template< typename FormatContext >
    FormatContext::iterator format( expressions::Sum< LeftT, RightT > expr, 
        FormatContext& ctx ) const
    { 
        auto str = std::format( "({}+{})", expr.left_arg(), expr.right_arg() );
        return formatter< std::string >::format( str, ctx );
    }
};

template< typename LeftT, typename RightT >
struct formatter< expressions::Product< LeftT, RightT >, char >:
    formatter< std::string >
{   
    template< typename FormatContext >
    FormatContext::iterator format( expressions::Product< LeftT, RightT > expr, 
        FormatContext& ctx ) const
    { 
        auto str = std::format( "({}*{})", expr.left_arg(), expr.right_arg() );
        return formatter< std::string >::format( str, ctx );
    }
};

template< typename LeftT, typename RightT >
struct formatter< expressions::Quotient< LeftT, RightT >, char >:
    formatter< std::string >
{   
    template< typename FormatContext >
    FormatContext::iterator format( expressions::Quotient< LeftT, RightT > expr, 
        FormatContext& ctx ) const
    { 
        auto str = std::format( "({}/{})", expr.numerator_arg(), expr.denominator_arg() );
        return formatter< std::string >::format( str, ctx );
    }
};

template< typename LeftT, typename RightT >
struct formatter< expressions::Difference< LeftT, RightT >, char >:
    formatter< std::string >
{   
    template< typename FormatContext >
    FormatContext::iterator format( expressions::Difference< LeftT, RightT > expr, 
        FormatContext& ctx ) const
    { 
        auto str = std::format( "({}-{})", expr.left_arg(), expr.right_arg() );
        return formatter< std::string >::format( str, ctx );
    }
};

template< typename ExprT >
struct formatter< expressions::Negation< ExprT >, char >:
    formatter< ExprT >
{
    template< typename FormatContext >
    FormatContext::iterator format( expressions::Negation< ExprT > expr, 
        FormatContext& ctx ) const
    { 
        auto i = ctx.out();
        *i++ = '-';
        ctx.advance_to(i);
        return formatter< ExprT >::format( expr.arg(), ctx );
    }
};

template< typename LeftT, typename RightT >
struct formatter< expressions::Equals< LeftT, RightT >, char >:
    formatter< std::string >
{   
    template< typename FormatContext >
    FormatContext::iterator format( expressions::Equals< LeftT, RightT > expr, 
        FormatContext& ctx ) const
    { 
        auto str = std::format( "({}=={})", expr.left_arg(), expr.right_arg() );
        return formatter< std::string >::format( str, ctx );
    }
};

template< typename LeftT, typename RightT >
struct formatter< expressions::Conjunction< LeftT, RightT >, char >:
    formatter< std::string >
{   
    template< typename FormatContext >
    FormatContext::iterator format( expressions::Conjunction< LeftT, RightT > expr, 
        FormatContext& ctx ) const
    { 
        auto str = std::format( "({}and{})", expr.left_arg(), expr.right_arg() );
        return formatter< std::string >::format( str, ctx );
    }
};


} // namespace std

#endif

#endif