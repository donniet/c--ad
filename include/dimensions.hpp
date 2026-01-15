#ifndef __DIMENSIONS_HPP__
#define __DIMENSIONS_HPP__

#include "units.hpp"

#include <string>
#include <functional>
#include <concepts>
#include <type_traits>

using std::string;
using std::function;

/**
 * Dimensions store a tuple of functions which return type Unit...
 */
template< typename... Units >
struct Dimensions
{
    using tuple_type = std::tuple< Units... >;
    using evaluators_type = std::tuple< function< Units() >... >;
    static constexpr size_t size = sizeof...( Units ); 

    string to_string()
    { return to_string_helper( std::make_index_sequence< size >{} ); }

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
private:
    template< size_t I >
    struct dimension_helper { using type = std::tuple_element_t< I, tuple_type >; };

    template< size_t I >
    using dimensions_t = dimension_helper< I >::type;

    template< size_t I >
    struct evaluator_helper { using type = function< dimensions_t< I >() >; };

    template< size_t I >
    using evaluator_t = evaluator_helper< I >::type;



public:
    template< size_t I >
    units::Expression< units::Method< evaluator_t< I >>> expression()
    { return units::invocation_of( std::get< I >( evaluators )); }

    template< size_t I >
    dimensions_t< I > eval() { return std::get< I >( evaluators )(); }

private:
    template< size_t I, std::convertible_to< evaluator_t< I >> Expression >
    static auto expression_function( Expression expression )
    { return expression; }

    template< size_t I, std::convertible_to< dimensions_t< I >> Constant >
    static units::Constant< dimensions_t< I >> 
    expression_function( Constant constant )
    { return { constant }; }

    template< size_t... Is >
    string to_string_helper( std::index_sequence< Is... > )
    { return "{" + (( std::to_string( eval< Is >() ) + ( Is + 1 < size ? "," : "" )) + ... ) + "}"; }

    template< typename ExpressionTuple, size_t... Is >
    Dimensions( ExpressionTuple expressions, std::index_sequence< Is... > ) :
        evaluators{ expression_function< Is >( 
            std::get< Is >( expressions ))... }
    { }

public:
    evaluators_type evaluators;
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
{ using type = Dimensions< void >; };

template< typename Unit, size_t N >
using repeated_dim_t = RepeatedDimensions< Unit, N >::type;


#endif