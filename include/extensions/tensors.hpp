/**
 * NO ifndef check since this is meant to be included in-place
 */

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
