/**
 * expressions.hpp are lazily evaluated mathematical expressions of units
 */

#ifndef __EXPRESSIONS_HPP__
#define __EXPRESSIONS_HPP__

#include <functional>

namespace expressions {

/**
 * Constant represents an unchanging expression value
 */
template< typename Unit >
struct Constant
{ 
    using value_type = Unit;
    value_type operator()() const { return value; }
    value_type value; 
};

/**
 * Zero is an identically zero, unitless, and dimensionless
 */
struct Zero 
{ 
    using value_type = long double;
    operator long double() const { return 0; } 
    value_type operator()() const { return 0; }
};

// expression base classes 

/**
 * Unary is the base of a unary expression
 */
template< typename T >
struct Unary
{ T operand; };

/**
 * Binary is the base of a binary expression
 */
template< typename T, typename U >
struct Binary
{ T first; U second; };

/**
 * Nary is the base of an N-ary expression
 */
template< typename T, typename... Ts >
struct Nary : std::tuple< T, Ts... >
{ 
    using tuple_type = std::tuple< T, Ts... >;

    T& get_first() { return std::get< 0 >( *this ); }
    Nary< Ts... > get_rest() 
    { return get_rest_helper( std::make_index_sequence< sizeof...( Ts )>{} ); }

    Nary& operator=( Nary const& ) = default;
    Nary& operator=( Nary&& ) = default;

    Nary( T first, Ts... rest ) : tuple_type{ first, rest... } { }
    Nary() : tuple_type{} { }
    Nary( Nary const& ) = default;
    Nary( Nary&& ) = default;
private:
    template< size_t... Is >
    Nary< Ts... > get_rest_helper( std::index_sequence< Is... > )
    { return { std::get< Is+1 >( *this )... }; }
};

/**
 * Method is a Nary expression that wraps an std::function object
 */
template< typename T, typename... ArgTypes >
struct Method : protected Nary< T, ArgTypes... >
{
    using value_type = decltype( T{}( ArgTypes{}()... ) );
    using function_type = std::function< value_type( ArgTypes... )>;
    using nary_type = Nary< T, ArgTypes... >;

    value_type operator()() const
    { return exec_helper( std::make_index_sequence< sizeof...( ArgTypes )>{} ); }

    T& function() { return nary_type::get_first(); }
private:
    template< size_t... Is >
    value_type exec_helper( std::index_sequence< Is... > )
    { return function()( std::get< 1+Is >( *this )... ); }
};

/**
 * invocation_of is a helper method that returns a Method expression for the
 * given std::function object
 */
template< typename ReturnType, typename... ArgTypes >
Method< std::function< ReturnType(ArgTypes...) >, ArgTypes...  > 
invocation_of( std::function< ReturnType( ArgTypes... )> method, 
    ArgTypes... args )
{ return { method, args... }; }

/**
 * Product is the binary multiplication expression
 */
template< typename T, typename U >
struct Product : public Binary< T, U >
{ 
    using value_type = decltype( T{}() * U{}() );
    value_type operator()() const 
    { return Binary<T,U>::first() * Binary<T,U>::second(); } 
};

/**
 * Quotient is the binary division expresssion
 */
template< typename T, typename U >
struct Quotient : public Binary< T, U >
{ 
    using value_type = decltype( T{}() / U{}() );
    value_type operator()() const 
    { return Binary<T,U>::first() / Binary<T,U>::second(); } 
};

/**
 * Sum is the binary addition expression
 */
template< typename T, typename U >
struct Sum : Binary<T,U>
{ 
    using value_type = decltype( T{}() + U{}() );
    value_type operator()() const 
    { return Binary<T,U>::first() + Binary<T,U>::second(); } 
};

/**
 * Exp is the unary exponentiation expression e^x where e is euler's constant
 */
template< typename T >
struct Exp : Unary<T>
{ 
    using value_type = decltype( std::exp( T{}() ));
    value_type operator()() const 
    { return std::exp( Unary<T>::operand() ); } 
};

/**
 * Log is the unary logarithm expression ln(x) where ln is the log base e or 
 * natural logarithm
 */
template< typename T >
struct Log : Unary<T>
{ 
    using value_type = decltype( std::log( T{}() ));
    auto operator()() const 
    { return std::log( Unary<T>::operand() ); } 
};

/**
 * Scale is a binary expression representing the scaling of another expression
 * by a constant
 */
template< typename T >
struct Scale : Binary< T, long double >
{ 
    using value_type = decltype( T{}() * (long double)0 );
    using base_type = Binary< T, long double >;
    auto operator()() const 
    { return base_type::first() * base_type::second; } 
};

/**
 * ElementOf represents the extraction of an element from a tuple-type object
 * using a compile-time index
 */
template< typename T, size_t I >
struct ElementOf : Unary< T >
{ 
    static constexpr size_t index = I;
    auto operator()() const  
    { return std::get< index >( Unary< T >::operand() ); }
};

// template< typename T, typename U >
// struct Cartesian : Binary< T, U >
// { 
//     auto operator()() const 
//     { return detail::span_expression( Binary<T,U>::first(), 
//         Binary<T,U>::second() ); } 
// };

/**
 * Variable represents an unknown in an expression including the absolute
 * derivatives (Dots > 0)
 */
template< size_t Dots >
struct Variable 
{ 
    // TODO: needs constructor and to live with Unit and Operands
    static Variable named( std::string const& name ) 
    { return Variable( name ); }

    Variable( Variable const& ) = default;

    std::string variable_name; 
private:
    Variable( std::string const& name ) : variable_name{ name } { }
};

// namespace variables 
// {
//     auto _x = Variable<0>::named("x");
//     auto _y = Variable<0>::named("y");
// }

/**
 * Expression is a wrapper that allows for lazy evaluation of any type
 */
template< typename T >
struct Expression
{
    using operation_type = T;
    using value_type = typename operation_type::value_type;

    template< size_t I >
    constexpr Expression< ElementOf< T, I >> element() const
    { return {{ *this }}; }

    // template< typename U >
    // Expression< Cartesian< T, U >> cartesian( Expression< U > other ) const
    // { return { *this, other }; }

    Expression< Scale< T >> scale( long double s ) const
    { return {{ value, s }}; }

    Expression< Exp< T >> exp() const
    { return {{ value }}; }

    Expression< Log< T >> log() const
    { return {{ value }}; }

    Expression< Exp< Scale< Log< T >>>> sqrt() const
    { return {{{{ value }, 0.5 }}}; }

    Expression< Exp< Scale< Log< T >>>> pow( long double exponent ) const
    { 
        Log< T > l = { value };
        Scale< Log< T >> s = { value , exponent };
        return {{{ value , exponent }}}; 
    }

    template< typename U >
    Expression< Product< T, U >> product( Expression< U > other ) const
    { return {{ value, other.value }}; }

    template< typename U >
    Expression< Quotient< T, U >> quotient( Expression< U > other ) const
    { return {{ value, other.value }}; }

    template< typename U >
    Expression< Sum< T, U >> sum( Expression< U > other ) const
    { return {{ value, other.value }}; }

    template< typename U >
    Expression< Sum< T, Scale< U >>> difference( Expression< U > other ) const
    { return {{ value, { other.value, -1 }}}; }

    value_type operator()() const { return value(); }

    T value;
};

template< typename Unit >
Expression< Constant< Unit >> constant( Unit value )
{ return {{ value }}; }

/**
 * Values is a tuple of Units with helpers for concatenating and slicing
 */
template< unit... Ts >
struct Values : std::tuple< Ts... >
{
    using tuple_type = std::tuple< Ts... >;
    // using dimensions_type = Dimensions< Ts... >;
    static constexpr const size_t size = sizeof...( Ts );

    template< size_t I >
    using element_t = std::tuple_element_t< I, tuple_type >;

    // append elements
    template< unit... Us >
    Values< Ts..., Us... > concat( Values< Us... > const& other ) const
    { return std::tuple_cat( *this, other ); }

    // cast-away some elements
    template< unit... Us >
    operator Values< Us... >()
    { return cast_helper< Values< Us... >>( 
        std::make_index_sequence< sizeof...( Us ) >{} ); }

    // getters
    template< size_t I >
    element_t< I >& at() { return std::get< I >( *this ); }
    template< size_t I >
    element_t< I > const& at() const { return std::get< I >( *this ); }
    // element_t< 0 >& first() { return at<0>(); }
    // element_t< 0 > const& first() const { return at<0>(); }
    // element_t< size-1 >& last() { return at< size-1 >(); }
    // element_t< size-1 > const& last() const { return at< size-1 >(); }


    // arithemetic, this counts as a unit itself
    Values& operator+=( Values const& other )
    { return plus_equal_helper( other, std::make_index_sequence< size >{} ); }
    Values& operator-=( Values const& other )
    { return minus_equal_helper( other, std::make_index_sequence< size >{} ); }
    Values& operator*=( long double scalar )
    { return scale_helper( scalar, std::make_index_sequence< size >{} ); }

    Values() : tuple_type{ Ts{ 0 }... } { }

private:
    template< typename CastType, size_t... Is >
    CastType cast_helper( std::index_sequence< Is... > )
    { return { std::get< Is >( *this )... }; }

    template< size_t... Is >
    Values& plus_equal_helper( Values const& other, 
        std::index_sequence< Is... > )
    {
        (( std::get< Is >( *this ) += std::get< Is >( other )), ... );
        return *this;
    }

    template< size_t... Is >
    Values& minus_equal_helper( Values const& other, 
        std::index_sequence< Is... > )
    {
        (( std::get< Is >( *this ) -= std::get< Is >( other )), ... );
        return *this;
    }

    template< size_t... Is >
    Values& scale_helper( long double scalar, std::index_sequence< Is... > )
    {
        (( std::get< Is >( *this ) *= scalar ), ... );
        return *this;
    }
};


/**
 * operators namespace includes the c++ operators on expresssions
 */
namespace operators
{
    template< typename T >
    auto operator*( long double s, Expression< T > const& expr )
    { return expr.scale( s ); }

    template< typename T >
    auto operator*( Expression< T > const& expr, long double s )
    { return expr.scale( s ); }

    template< typename T, typename U >
    auto operator+( Expression< T > const& expr, Expression< U > const& other )
    { return expr.sum( other ); }

    template< typename T, typename U >
    auto operator-( Expression< T > const& expr, Expression< U > const& other )
    { return expr.difference( other ); }

    template< typename T, typename U >
    auto operator*( Expression< T > const& expr, Expression< U > const& other )
    { return expr.product( other ); }

    template< typename T, typename U >
    auto operator/( Expression< T > const& expr, Expression< U > const& other )
    { return expr.quotient( other ); }

    template< typename T >
    auto exp( Expression< T > const& expr )
    { return expr.exp(); }

    template< typename T >
    auto log( Expression< T > const& expr )
    { return expr.log(); }

    template< typename T >
    auto sqrt( Expression< T > const& expr )
    { return expr.sqrt(); }

    template< typename T >
    auto pow( Expression< T > const& expr, long double exponent )
    { return expr.pow( exponent ); }


    auto d( long double ) { return Zero{}; }
    
    // TODO: do we need to differentiate constants?
    // template< typename UnitType >
    // auto d( Expression< Constant< UnitType >> ) 
    // { return Expression< Constant< UnitType >>{ 0 }; }

    auto d( Zero ) { return Zero{}; }

    // template< size_t Dots >
    // auto d( Expression< Variable< Dots >> var )
    // { return Expression< Variable< Dots+1 >>{ 
    //     Variable< Dots+1 >::named( var.value.variable_name )}; }
    
    // template< size_t Dots >
    // auto s( Expression< Variable< Dots >> var )
    // { return Expression< Variable< Dots-1 >>{ 
    //     Variable< Dots-1 >::named( var.value.variable_name )}; }
    
    

    // template< typename Contra, typename Covar >
    // auto d( Expression< Tensor< Contra, Covar >> var )
    // { return Expression< Tensor< Contra, Covar >>{ var.contravariant, 
    //     var.covariant }; }

    template< typename UnitType >
    auto d( Expression< Scale< UnitType >> unit )
    { return d( expression_of( unit.value.first )).scale( unit.value.second ); }

    template< typename UnitType >
    auto d( Expression< Exp< UnitType >> unit )
    { return d( expression_of( unit.value.operand )).product( unit ); }

    template< typename UnitType >
    auto d( Expression< Log< UnitType >> unit )
    { return d( expression_of( unit.value.operand )).quotient( 
        expression_of( unit.value.operand )); }

    template< typename U, typename T >
    auto d( Expression< Product< U, T >> unit )
    { 
        auto left = expression_of( unit.value.first ).product( 
            d( expression_of( unit.value.second )));

        auto right = d( expression_of( unit.value.first )).product(
            expression_of( unit.value.secon ));

        return left.sum( right );
    }

    template< typename U, typename T >
    auto d( Expression< Quotient< U, T >> unit )
    { 
        auto left = d( expression_of( unit.value.first )).product( 
            expression_of( unit.value.second ));

        auto right = expression_of( unit.value.first ).product( 
            d( expression_of( unit.value.second )));

        auto denom = expression_of( unit.value.second ).product( 
            expression_of( unit.value.second ));

        return left.difference( right ).quotient( denom ); 
    }
} // namespace operators

/**
 * expression_of is a helper method that turns any unit-type object into an 
 * expression which can be lazily evaluated and differentiated
 */
template< typename UnitType >
Expression< UnitType > expression_of( UnitType value ) { return { value }; }

} // namespace expressions

#endif