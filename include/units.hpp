#ifndef __UNITS_HPP__
#define __UNITS_HPP__

#include <cmath>
#include <string>
#include <chrono>
#include <functional>

namespace units 
{
    struct undefined_t { };

    static constexpr const undefined_t undefined = {};

    /**
     * Concepts
     */
    template< typename U >
    concept unit = requires ( U a, U b ) 
    {  
        a *= 1.0;
        a = b;
        a += b;
        a -= b;
        a = undefined;
    };

    namespace detail
    {
        template< typename T, typename U >
        auto span_expression( T, U );
    }

    /** Dimensions */
    struct Length 
    { 
        static constexpr const long double undefined_value = 
            std::numeric_limits< long double >::min();

        operator long double() const { return meters; }

        Length& operator-=( Length const& other )
        {
            meters -= other.meters;
            return *this;
        }
        Length operator-( Length const& other ) const
        {
            Length ret = *this;
            return ret -= other;
        }
        Length& operator+=( Length const& other )
        {
            meters += other.meters;
            return *this;
        }
        Length operator+( Length const& other ) const
        {
            Length ret = *this;
            return ret += other;
        }
        Length& operator*=( long double other )
        {
            meters *= other;
            return *this;
        }
        Length operator*( long double other )
        {
            Length ret = *this;
            return ret *= other;
        }
        Length& operator/=( long double other )
        {
            meters *= other;
            return *this;
        }
        Length operator/( long double other )
        {
            Length ret = *this;
            return ret /= other;
        }

        Length() : meters{ 0 } { }
        Length( undefined_t ) : meters{ undefined_value } { }
        Length( int value ) : meters{ (long double)value } { }
        Length( unsigned value ) : meters{ (long double)value } { }
        Length( unsigned long long value ) : meters{ (long double)value } { }
        Length( long long value ) : meters{ (long double)value } { }
        Length( long double value ) : meters{ (long double)value } { }

        long double meters; 
    };

    template< typename Unit >
    struct Interval : std::pair< Unit, Unit >
    { };

    template< typename Unit >
    Unit default_expression_for() { return {}; }


    /** Units */
    static constexpr long double meters_per_inch = 0.0254;
    static constexpr long double meters_per_foot = meters_per_inch / 12.;
    static constexpr long double meters_per_mile = meters_per_foot / 5280.;
    static constexpr long double meters_per_second = 299'792'458; // c

    namespace literals
    {
        using namespace std::chrono_literals;

        Length operator""_in(long double inches)
        { return { inches * meters_per_inch }; }
        Length operator""_in(unsigned long long inches)
        { return { (long double)inches * meters_per_inch }; }
        Length operator""_ft(long double feet)
        { return { feet * meters_per_foot }; }
        Length operator""_ft(unsigned long long feet)
        { return { (long double)feet * meters_per_foot }; }
        Length operator""_mi(long double miles)
        { return { miles * meters_per_mile }; }
        Length operator""_mi(unsigned long long miles)
        { return { (long double)miles * meters_per_mile }; }
        Length operator""_m(long double meters)
        { return { meters }; }
        Length operator""_m(unsigned long long meters)
        { return { (long double)meters }; }
        Length operator""_mm(long double millimeters)
        { return { 0.001 * millimeters }; }
        Length operator""_mm(unsigned long long millimeters)
        { return { 0.001 * (long double)millimeters }; }
        Length operator""_km(long double kilometers)
        { return { 1000. * kilometers }; }
        Length operator""_km(unsigned long long kilometers)
        { return { 1000. * (long double)kilometers }; } 
    }

    template< typename ToType, typename FromType >
    struct Projector;

    template< typename... UnitTypes >
    struct Space : std::tuple< UnitTypes... >
    { 
        template< size_t I >
        using element_t = std::tuple_element_t< I, std::tuple< UnitTypes... >>;

        Space& operator-=( Space const& other )
        {
            subtract_helper( other, 
                std::make_index_sequence< sizeof...( UnitTypes ) >{} );
            return *this;
        }

        Space operator-( Space const& other ) const
        { 
            Space ret = *this;
            return ret -= other;
        }

        static Space zero() 
        { return zero_helper( 
            std::make_index_sequence< sizeof...( UnitTypes )>{} ); }

        template< typename... UnitTypes2 >
        Space( Space< UnitTypes2... > const& projection )
        { copy_helper( projection, std::make_index_sequence<
            sizeof...( UnitTypes ) < sizeof...( UnitTypes2 ) ? 
                sizeof...( UnitTypes ) : sizeof...( UnitTypes2 )>{} ); }

        Space( std::tuple< UnitTypes... > units ) : 
            std::tuple< UnitTypes... >{ units }
        { }
        Space() : std::tuple< UnitTypes... >{ zero() } { }

    private:
        template< size_t... Is >
        static Space zero_helper( std::index_sequence< Is... > )
        { return { std::make_tuple( element_t< Is >{ 0 }... )}; }

        template< typename ProjectedType, size_t... Is >
        void copy_helper( ProjectedType const& other,
             std::index_sequence< Is... > )
        { (( std::get< Is >( *this ) = std::get< Is >( other )), ... ); }


        template< typename OtherType, size_t... Is >
        void subtract_helper( OtherType const& other, std::index_sequence< Is... > )
        { (( std::get< Is >( *this ) -= std::get< Is >( other )), ... ); }
    };

    template< typename UnitType >
    struct Dimensionality { static constexpr size_t value = 1; };

    template<>
    struct Dimensionality<void> { static constexpr size_t value = 0; };

    template< typename... UnitTypes >
    struct Dimensionality< Space< UnitTypes... >>
    { static constexpr size_t value = sizeof...( UnitTypes ); };

    template< typename UnitType >
    using dimensionality_of = Dimensionality< UnitType >::value;

    // template< typename UnitType >
    // static constexpr size_t dimensionality_of( UnitType )
    // { return Dimensionality< UnitType >::value; }

    template< > 
    struct Space< > 
    { 
        template< size_t I >
        using element_t = void; 

        bool operator==( Space< > const& ) const { return true; }
    };


    template< typename UnitType >
    struct Projector< UnitType, UnitType >
    { UnitType operator()( UnitType unit ) { return unit; } };

    template< typename... UnitTypes1, typename... UnitTypes2 >
    struct Projector< Space< UnitTypes1... >, Space< UnitTypes2... >>
    {
        Space< UnitTypes1... > operator()( Space< UnitTypes2... > other ) const
        { return project_helper< Space< UnitTypes1... >>( other, 
            std::make_index_sequence< 
                sizeof...( UnitTypes1 ) < sizeof...( UnitTypes2 ) ?
                    sizeof...( UnitTypes1 ) : sizeof...( UnitTypes2 ) >{} ); }

    private:
        template< typename ReturnType, size_t... Is >
        Space< typename Space< UnitTypes1... >::template element_t< Is >... > 
        project_helper( Space< UnitTypes2... > const& other, 
            std::index_sequence< Is... > ) const
        { return { std::get< Is >( other )... }; }
    };

    template< typename ToType, typename FromType >
    ToType project( FromType from )
    { return Projector< ToType, FromType >{}( from ); }

    template< typename... UnitTypes >
    struct ProductType;

    template< typename... UnitTypes >
    using product_of = ProductType< UnitTypes... >::type;

    template< typename Product1, typename Product2 >
    struct ProductConcatenate;

    template< typename... UnitTypes1, typename... UnitTypes2 >
    struct ProductConcatenate< Space< UnitTypes1... >, Space< UnitTypes2... > >
    { using type = Space< UnitTypes1..., UnitTypes2... >; };

    template< >
    struct ProductType< >
    { using type = Space< >; };

    // associativity
    template< typename... UnitTypes1, typename... UnitTypes2 >
    struct ProductType< Space< UnitTypes1... >, UnitTypes2... >
    { using type = ProductType< UnitTypes1..., UnitTypes2... >::type; };

    template< typename UnitType, typename... UnitTypes >
    struct ProductType< UnitType, UnitTypes... >
    { using type = ProductConcatenate< Space< UnitType >, typename ProductType< UnitTypes... >::type >::type; };

    template< typename UnitType >
    long double distance( UnitType x, UnitType y )
    { return y - x; }

    template< typename... UnitTypes >
    long double distance( Space< UnitTypes... > x, Space< UnitTypes... > y )
    { return distance_helper( x, y, 
        std::make_index_sequence< sizeof...( UnitTypes )>{}); }

    template< typename UnitType, size_t... Is >
    long double distance_helper( UnitType x, UnitType y, 
        std::index_sequence< Is... > )
    {
        UnitType u = y - x;
        long double val = (( std::get< Is >( u ) * std::get< Is >( u )) + ... );
        return std::sqrt(val);
    }

    static_assert( std::is_same_v< Space< Length, Length, Length >, ProductType< Space< Length, Length >, Length >::type > );
}

namespace std {
    using namespace units;

    string to_string( Length value )
    { return to_string( (long double)value ) + "m"; }

    template< size_t N >
    constexpr string repeat( string str )
    { return str + repeat<N-1>( str ); }

    template< >
    constexpr string repeat<0>( string str )
    { return ""; }

    // template< size_t I, size_t Dots >
    // string to_string( Variable<I,Dots> var )
    // { return var.variable_name + repeat<Dots>("'"); }

    template< size_t I, size_t Dots >
    string to_string( Expression< Variable<Dots> > unit );
    template< typename UnitType >
    string to_string( Expression< Scale< UnitType >> unit );
    template< typename UnitType >
    string to_string( Expression< Exp< UnitType >> unit );
    template< typename UnitType >
    string to_string( Expression< Log< UnitType >> unit );
    template< typename UnitType1, typename UnitType2 >
    string to_string( Expression< Product< UnitType1, UnitType2 >> unit );
    template< typename UnitType1, typename UnitType2 >
    string to_string( Expression< Quotient< UnitType1, UnitType2 >> unit );
    template< typename UnitType1, typename UnitType2 >
    string to_string( Expression< Sum< UnitType1, UnitType2 >> unit );


    template< size_t Dots >
    string to_string( Expression< Variable<Dots> > unit )
    { return unit.value.variable_name + repeat<Dots>("'"); }

    template< typename UnitType >
    string to_string( Expression< Scale< UnitType >> unit )
    { return to_string( unit.value.second ) + to_string( expression_of( unit.value.first )); }

    template< typename UnitType >
    string to_string( Expression< Exp< UnitType >> unit )
    { return "exp(" + to_string( expression_of( unit.value.operand )) + ")"; }

    template< typename UnitType >
    string to_string( Expression< Log< UnitType >> unit )
    { return "log(" + to_string( expression_of( unit.value.operand )) + ")"; }

    template< typename UnitType1, typename UnitType2 >
    string to_string( Expression< Sum< UnitType1, UnitType2 >> unit )
    { return "(" + to_string( expression_of( unit.value.first )) + " + " + to_string( expression_of( unit.value.second )) + ")"; }

    template< typename UnitType1, typename UnitType2 >
    string to_string( Expression< Product< UnitType1, UnitType2 >> unit )
    { return "(" + to_string( expression_of( unit.value.first )) + " * " + to_string( expression_of( unit.value.second )) + ")"; }

    template< typename UnitType1, typename UnitType2 >
    string to_string( Expression< Quotient< UnitType1, UnitType2 >> unit )
    { return "((" + to_string( expression_of( unit.value.first )) + ")/(" + to_string( expression_of( unit.value.second )) + "))"; }


    template< typename Tuple, size_t... Is >
    Tuple exp_helper( Tuple const& x, index_sequence< Is... > )
    { return { exp( get< Is >( x ))... }; }
    
    template< typename... Ts >
    tuple< Ts... > exp( tuple< Ts... > const& x )
    { return exp_helper( x, std::make_index_sequence< sizeof...( Ts )>{} ); }

    template< typename Tuple, size_t... Is >
    Tuple log_helper( Tuple const& x, index_sequence< Is... > )
    { return { log( get< Is >( x ))... }; }
    
    template< typename... Ts >
    tuple< Ts... > log( tuple< Ts... > const& x )
    { return log_helper( x, std::make_index_sequence< sizeof...( Ts )>{} ); }

}


#endif