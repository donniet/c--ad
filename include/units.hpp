#ifndef __UNITS_HPP__
#define __UNITS_HPP__

#include <cmath>
#include <string>
#include <chrono>
#include <functional>

namespace units 
{
    using std::ratio;

    namespace detail
    {
        template< typename T, typename U >
        auto span_expression( T, U );
    }

    /** Dimensions */
    struct Length 
    { 
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

    /** Expressions */
    template< typename Unit >
    struct Constant
    { 
        using value_type = Unit;
        value_type operator()() const { return value; }
        value_type value; 
    };

    struct Zero 
    { 
        using value_type = long double;
        operator long double() const { return 0; } 
        value_type operator()() const { return 0; }
    };

    template< typename T >
    struct Unary
    { T operand; };

    template< typename T, typename U >
    struct Binary
    { T first; U second; };

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

    template< typename ReturnType, typename... ArgTypes >
    Method< std::function< ReturnType(ArgTypes...) >, ArgTypes...  > 
    invocation_of( std::function< ReturnType( ArgTypes... )> method, 
        ArgTypes... args )
    { return { method, args... }; }

    template< typename T, typename U >
    struct Product : public Binary< T, U >
    { 
        using value_type = decltype( T{}() * U{}() );
        value_type operator()() const 
        { return Binary<T,U>::first() * Binary<T,U>::second(); } 
    };

    template< typename T, typename U >
    struct Quotient : public Binary< T, U >
    { 
        using value_type = decltype( T{}() / U{}() );
        value_type operator()() const 
        { return Binary<T,U>::first() / Binary<T,U>::second(); } 
    };

    template< typename T, typename U >
    struct Sum : Binary<T,U>
    { 
        using value_type = decltype( T{}() + U{}() );
        value_type operator()() const 
        { return Binary<T,U>::first() + Binary<T,U>::second(); } 
    };

    template< typename T >
    struct Exp : Unary<T>
    { 
        using value_type = decltype( std::exp( T{}() ));
        value_type operator()() const 
        { return std::exp( Unary<T>::operand() ); } 
    };

    template< typename T >
    struct Log : Unary<T>
    { 
        using value_type = decltype( std::log( T{}() ));
        auto operator()() const 
        { return std::log( Unary<T>::operand() ); } 
    };

    template< typename T >
    struct Scale : Binary< T, long double >
    { 
        using value_type = decltype( T{}() * (long double)0 );
        using base_type = Binary< T, long double >;
        auto operator()() const 
        { return base_type::first() * base_type::second; } 
    };

    template< typename T, size_t I >
    struct ElementOf : Unary< T >
    { 
        static constexpr size_t index = I;
        auto operator()() const  
        { return std::get< index >( Unary< T >::operand() ); }
    };

    template< typename T, typename U >
    struct Cartesian : Binary< T, U >
    { 
        auto operator()() const 
        { return detail::span_expression( Binary<T,U>::first(), 
            Binary<T,U>::second() ); } 
    };

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

    namespace variables 
    {
        auto _x = Variable<0>::named("x");
        auto _y = Variable<0>::named("y");
    }

    template< typename T >
    struct Expression
    {
        using operation_type = T;
        using value_type = typename operation_type::value_type;

        template< size_t I >
        constexpr Expression< ElementOf< T, I >> element() const
        { return {{ *this }}; }

        template< typename U >
        Expression< Cartesian< T, U >> cartesian( Expression< U > other ) const
        { return { *this, other }; }

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

    // scaling of a cartesian product is the product of the scale operation
    // template< typename T >
    // Expression< Cartesian<


    template< typename Unit >
    Expression< Constant< Unit >> constant( Unit value )
    { return {{ value }}; }

    /**
     * Tensors
     */
    namespace detail 
    {
        struct contravariant_components_type { };
        struct covariant_components_type { };

        template< typename T >
        concept contravariant =
        requires { typename std::is_same< 
            typename T::components_type,
            contravariant_components_type >; };

        template< typename T >
        concept covariant =
        requires { typename std::is_same<
            typename T::components_type,
            covariant_components_type >; };

        template< typename Operation, typename TupleType, size_t... Is >
        auto tuple_op_helper(Operation op, TupleType const& left, 
            TupleType const& right, std::index_sequence< Is... > )
        { 
            using std::get, std::make_tuple;
            return make_tuple( op( get< Is >( left ), get< Is >( right ))... );
        }

        template< typename Operation, typename TupleType >
        auto tuple_op( Operation op, TupleType const& left, TupleType const& right )
        { return tuple_op_helper( op, left, right ); }

        template< typename Operation, typename TupleType, size_t... Is >
        auto tuple_op_helper(Operation op, TupleType const& operand, 
            std::index_sequence< Is... > )
        { 
            using std::get, std::make_tuple;
            return make_tuple( op( get< Is >( operand ))... );
        }

        template< typename Operation, typename TupleType >
        auto tuple_op( Operation op, TupleType const& operand )
        { return tuple_op_helper( op, operand ); }

        template< typename T, typename U >
        struct tuple_span
        {
            using type = std::tuple< T, U >;
            static type apply( T a, U b ) 
            { return std::make_tuple( a, b ); }
        };

        template< typename T, typename... Ts >
        struct tuple_span< T, std::tuple< Ts... >>
        {
            using type = std::tuple< T, Ts... >;
            static type apply( T a, std::tuple< Ts... > b )
            { return std::tuple_cat( std::make_tuple( a ), b ); }
        };

        template< typename... Ts, typename T >
        struct tuple_span< std::tuple< Ts... >, T >
        {
            using type = std::tuple< Ts..., T >;
            static type apply( std::tuple< Ts... > a, T b )
            { return std::tuple_cat( a, std::make_tuple( b ) ); }
        };

        template< typename... Ts, typename... Us >
        struct tuple_span< std::tuple< Ts... >, std::tuple< Us... >>
        {
            using type = std::tuple< Ts..., Us... >;
            static type apply( std::tuple< Ts... > a, std::tuple< Us... > b )
            { return std::tuple_cat( a, b ); }
        };

        template< typename T, typename U >
        auto span_expression( T t, U u )
        { return tuple_span< T, U >::apply( t, u ); }
    }

    template< size_t I, typename T >
    auto component( T& tup ) { return std::get< I >( tup ); }

    template< typename... UnitTypes >
    struct Components : public std::tuple< UnitTypes... >
    {
        using tuple_type = std::tuple< UnitTypes... >;
        static constexpr size_t size = sizeof...( UnitTypes );

        Components operator-( Components const& other ) const
        { return detail::tuple_op( std::minus<>{}, *this, other ); }

        Components operator+( Components const& other ) const
        { return detail::tuple_op( std::plus<>{}, *this, other ); }

        template< typename... UnitTypes2 >
        Components< UnitTypes..., UnitTypes2... > operator*( 
            Components< UnitTypes2... > const& other ) const
        { 
            using other_type = Components< UnitTypes2... >;
            static constexpr size_t other_size = other_type::size;
            return mult_helper< Components< UnitTypes..., UnitTypes2... >>( 
                other, std::make_index_sequence< size + other_size >{} ); 
        }
        
    private:
        template< typename ReturnType, typename OtherType, size_t... Is >
        ReturnType mult_helper( OtherType const& other,
            std::index_sequence< Is... > ) const
        { return { component<( Is < size ? Is : Is - size )>( 
            Is < size ? *this : other )... }; }
    };

    template< typename... UnitTypes >
    struct Contravariant : public Components< UnitTypes... >
    { using components_type = detail::contravariant_components_type; };

    template< typename... UnitTypes >
    Contravariant< UnitTypes... > contravariant_of( UnitTypes... components )
    { return { components... }; }

    template< typename... UnitTypes >
    struct Covariant : public Components< UnitTypes... >
    { using components_type = detail::covariant_components_type; };

    template< typename... UnitTypes >
    Covariant< UnitTypes... > covariant_of( UnitTypes... components )
    { return { components... }; }

    template< typename, typename >
    struct Tensor;

    template< typename Contra, typename Covar >
    Tensor< Contra, Covar > make_tensor( Contra const& contra, 
        Covar const& covar );

    template< typename ContravariantComponents, typename CovariantComponents >
    struct Tensor 
    { 
        Tensor operator-( Tensor const& other ) const
        { return { contravariant - other.contravariant, 
            covariant - other.covariant }; }
        
        Tensor operator+( Tensor const& other ) const
        { return { contravariant + other.contravariant, 
            covariant + other.covariant }; }

        template< typename Contra2, typename Cov2 >
        auto operator*( Tensor< Contra2, Cov2 > const& other ) const
        { return make_tensor( contravariant * other.contravariant, 
            covariant * other.covariant ); }

        ContravariantComponents contravariant;
        CovariantComponents covariant;
    };

    template< typename Contra, typename Covar >
    Tensor< Contra, Covar > make_tensor( Contra const& contra, 
        Covar const& covar )
    { return { contra, covar }; }

    template< typename... UnitTypes >
    using Vector = Tensor< Contravariant< UnitTypes... >, Covariant< >>;

    template< typename... UnitTypes >
    using Bivector = Tensor< Contravariant<>, Covariant< UnitTypes... >>;

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
        auto d( Zero ) { return Zero{}; }

        template< size_t Dots >
        auto d( Expression< Variable< Dots >> var )
        { return Expression< Variable< Dots+1 >>{ 
            Variable< Dots+1 >::named( var.value.variable_name )}; }
        
        template< size_t Dots >
        auto s( Expression< Variable< Dots >> var )
        { return Expression< Variable< Dots-1 >>{ 
            Variable< Dots-1 >::named( var.value.variable_name )}; }
        
        

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
    }

    template< typename UnitType >
    Expression< UnitType > expression_of( UnitType value ) { return { value }; }

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