#ifndef __DIMENSIONS_HPP__
#define __DIMENSIONS_HPP__

#include "units.hpp"

#include <string>
#include <functional>
#include <concepts>
#include <type_traits>

using std::string;
using std::function;

using units::unit;


template< unit... >
struct Dimensions;

template< unit... >
struct Values;

/**
 * Concepts
 */

// expression types are invocable and return a tuple of unit types
template< typename T, typename Unit >
concept expression_of = std::is_invocable_r_v< Unit, T >;

// DT: not sure if dimensions should all support differentiation
//     I think perhaps Dimensions are only affected by constraints and at the
//     bottom-most level of the processing hierarchy.
//     in other words, the differentiation should only take place on the 
//     expressions, and once those expressions have been tied to dimensions
//     then the constraints can use the differentiation to find values which
//     meet the constraints, or error because no such values exist.  See the README
//      
// template< typename Dim, typename Unit >
// concept dimension = expression_of< Dim, Unit > && requires ( Dim dim,
//     std::function< Unit() > f )
// {
//     dim = f;
//     units::operators::d( dim );
// };

template< unit Unit >
struct Dimension;

struct ConstraintExpression;

using constraint_expr_ptr = std::shared_ptr< ConstraintExpression >;

struct ConstraintExpression
{
    enum operation_type : size_t 
    {
        constant, expression,
        equals, conjunction, disjunction, negation,
        // greater_than, greater_than_or_equal,
        // less_than, less_than_or_equal,
    };

    operation_type op;
    ConstraintExpression* first,* second;
    long double value;
};

constraint_expr_ptr make_constraint_expression(
    ConstraintExpression::operation_type operation, 
    constraint_expr_ptr first, constraint_expr_ptr second,
    long double value)
{ return std::make_shared< ConstraintExpression >( operation, first, second, value ); }

template< unit Unit >
struct Constraint
{
    using unit_type = Unit;
    using operation_type = ConstraintExpression::operation_type;

    // could be undefined or not unique
    unit_type allowed_value();

    // true iff defined and unique
    bool has_unique_value();

    // true if not undefined
    bool has_value()
    {
        if( expr == nullptr )
            return false;
        
        if( expr->op == operation_type::constant )
            return true;
        
        if( expr->op == operation_type::equals )
            return Constraint{ expr->argument }.has_value();

        if( expr->op == operation_type::conjunction )
            if(  )
    }

    Constraint operator&&( Constraint& other )
    { return { make_constraint_expression( operation_type::conjunction, expr, other.expr, 0 )}; }
    Constraint operator||( Constraint& other )
    { return { make_constraint_expression( operation_type::disjunction, expr, other.expr, 0 )}; }
    Constraint operator!()
    { return { make_constraint_expression( operation_type::negation, expr, nullptr, 0 )}; }

    Constraint() : expr{ nullptr } { }

    constraint_expr_ptr expr;
};

/**
 * Dimension is a physical property of an object. 
 * - [can be called via `Unit operator()`]
 * - [can be differentiated `via units::operators::d`]
 * - can be used in Expressions
 * - can be be assigned to any other Expression
 */
template< unit Unit > //, size_t Dots = 0 >
struct Dimension 
{
    using unit_type = Unit;

    // determine the value of this dimension at run time
    unit_type operator()()
    { return constraint.allowed_value(); }

    // template< expression_of< unit_type > ExprType >
    // Dimension& operator=( ExprType other );
    // Dimension& operator=( Dimension const& other );

    Constraint< unit_type > operator==( Dimension& other );
    template< expression_of< unit_type > ExprType >
    Constraint< unit_type > operator==( ExprType& other );

    // Dimension< unit_type, Dots+1 > derivative();

    Dimension();

    bool is_uniquely_constrained()
    { return constraint.has_unique_value(); }

private:
    Constraint< unit_type > constraint;
};

/**
 * dimension_element extracts an element from the unit_type of DimType
 */
template< size_t I, typename DimType >
auto dimension_element( DimType& );

template< size_t I, typename... Ts >
auto dimension_element( Dimension< Values< Ts... >>& dim )
{

}

/**
 * DimensionAlias is a new name for a dimensional member of the object.
 */
template< unit Unit >
struct DimensionAlias
{
    // can be constructed with a reference to a dimension
    DimensionAlias( Dimension< Unit >& dimension ) : dimension{ &dimension }
    { }

    // can be constructed with a pointer to an alias member
    DimensionAlias( DimensionAlias< Unit > const& alias ) :
        dimensions{ alias.dimension }
    { }

    DimensionAlias() : dimensions{ nullptr } { }

    Dimension< Unit >* dimension;
};

/**
 * Constraint
 */
template< unit Unit >
struct Constraint
{

};

/**
 * Dimensions store a tuple of functions which return type Unit...
 */
template< unit... Units >
struct Dimensions
{
    using values_type = Values< Units... >;
    using evaluators_type = std::tuple< function< Units() >... >;
    static constexpr size_t size = sizeof...( Units ); 

private:
    template< size_t I >
    struct dimension_helper 
    { using type = typename values_type::template element_t< I >; };

    template< size_t I >
    using dimension_t = dimension_helper< I >::type;

    template< size_t I >
    struct evaluator_helper { using type = function< dimension_t< I >() >; };

    template< size_t I >
    using evaluator_t = evaluator_helper< I >::type;

public:
    template< size_t I >
    struct element_proxy;

    string to_string()
    { return to_string_helper( std::make_index_sequence< size >{} ); }

    template< size_t I, typename Expression > 
    void set( Expression expr )
    { std::get< I >( evaluators ) = expression_function< I >( expr ); }

    template< size_t I > 
    auto& get()
    { return std::get< I >( evaluators ); }

    template< size_t I >
    element_proxy< I > at()
    { return { *this }; }

    values_type operator()()
    { return evaluate_helper( std::make_index_sequence< size >{} ); }

    template< typename ExpressionTuple >
    Dimensions& operator=( ExpressionTuple const& expressions )
    { return *this = Dimensions{ expressions }; }

    Dimensions& operator=( Dimensions const& ) = default;
    Dimensions& operator=( Dimensions&& ) = default;

    Dimensions() : evaluators{ units::default_expression_for< Units >... } { }
    Dimensions( Dimensions const& ) = default;
    Dimensions( Dimensions&& ) = default;

    template< typename... Expressions >
    Dimensions( Expressions... expressions ) : 
        Dimensions{ std::make_tuple( expressions... ), 
            std::make_index_sequence< size >{} }
    { }

public:
    template< size_t I >
    units::Expression< units::Method< evaluator_t< I >>> expression()
    { return units::invocation_of( std::get< I >( evaluators )); }

    template< size_t I >
    dimension_t< I > eval() { return std::get< I >( evaluators )(); }

    template< size_t I, std::convertible_to< evaluator_t< I >> Expression >
    static auto expression_function( Expression expression )
    { return expression; }

    template< size_t I, std::convertible_to< dimension_t< I >> Constant >
    static units::Constant< dimension_t< I >> 
    expression_function( Constant constant )
    { return { constant }; }

private:
    template< size_t... Is >
    string to_string_helper( std::index_sequence< Is... > )
    { return "{" + (( std::to_string( eval< Is >() ) + ( Is + 1 < size ? "," : "" )) + ... ) + "}"; }

    template< size_t... Is >
    values_type evaluate_helper( std::index_sequence< Is... > )
    { return { eval<Is>()... }; }

    template< typename ExpressionTuple, size_t... Is >
    Dimensions( ExpressionTuple expressions, std::index_sequence< Is... > ) :
        evaluators{ expression_function< Is >( 
            std::get< Is >( expressions ))... }
    { }

public:
    evaluators_type evaluators;
};

template< size_t I, typename Dimensions >
auto reference_to( Dimensions& dimensions )
{ return dimensions.template expression< I >(); }


namespace detail
{
    template< typename DimensionsType, size_t... Is >
    auto reference_to_helper( DimensionsType& dimensions, 
        std::index_sequence< Is... > )
    { return Dimensions{ reference_to< Is >( dimensions )... }; }
}

template< unit... Units >
Dimensions< Units... > reference_to( Dimensions< Units... >& dimensions )
{ return detail::reference_to_helper( dimensions, 
    std::make_index_sequence< sizeof...( Units )>{} ); }


template< unit Domain >
using dimensions_for = Domain::dimensions_type;

// Dimensions<...>::element_proxy helps us treat each element of a dimension
// as an expression, allows operator(), and supports assignment
template< unit... Units >
template< size_t I >
struct Dimensions< Units... >::element_proxy
{
    friend class Dimensions;
    
    template< expression_of< dimension_t< I >> Expression >
    element_proxy& operator=( Expression expr )
    {
        std::get< I >( self->evaluators ) = 
            Dimensions::expression_function< I >( expr );
        return *this;
    }

    operator units::Expression< evaluator_t< I >>()
    { return { std::get< I >( self->evaluators )}; }

    dimension_t< I > operator()() const
    { return self->eval< I >(); }

    element_proxy& operator=( element_proxy const& ) = default;
    element_proxy& operator=( element_proxy&& ) = default;

    element_proxy() = delete;
    element_proxy( element_proxy const& ) = default;
    element_proxy( element_proxy&& ) = default;
    element_proxy( Dimensions& from ) : self{ &from } { }

private:
    Dimensions* self;
};

template< size_t I, typename DimensionsType >
using element_t = typename DimensionsType::template element_proxy< I >;



template< size_t N, typename... Types >
using nth = std::tuple_element_t< N, std::tuple< Types... >>;

template< size_t I, typename DimensionsType >
struct DimensionAlias
{
    DimensionAlias( DimensionsType& dimensions );
    DimensionAlias( DimensionAlias const& other );

};

// template< size_t I, typename DimensionsType >
// auto get_dim( DimensionsType& dimensions )
// { return }

template< typename DimensionsType, typename... Units >
struct ExtendDimensions;

template< typename... Units, typename... Units2 >
struct ExtendDimensions< Dimensions< Units... >, Units2... >
{ using type = Dimensions< Units..., Units2... >; };

template< typename Unit, size_t N >
struct RepeatedDimensions
{ using type = ExtendDimensions< typename RepeatedDimensions< Unit, N-1 >::type, 
    Unit >::type; };

template< typename Unit >
struct RepeatedDimensions< Unit, 0 >
{ using type = Dimensions< >; };

template< typename Unit, size_t N >
using repeated_dim_t = RepeatedDimensions< Unit, N >::type;


#endif