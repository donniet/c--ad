#ifndef __EXPRESSIONS_SCOPE_HPP__
#define __EXPRESSIONS_SCOPE_HPP__

#include "expressions/forward_decl.hpp"
#include "expressions/unique_variables.hpp"
#include "expressions/constant.hpp"
#include "expressions/static_value.hpp"

namespace expressions {

/////////////////////////////
/// Scope Implementation ///
///////////////////////////
///
/// @brief container and factory for Vars.  Scope is a manipulator and
/// application of scope via operator| evaluates dependent variables against
/// scoped values.
///
template< variable... Vars >
struct Scope: tuple< Vars... > 
{
    using values_tuple_type = tuple< var_value_t< Vars >... >;
    using dirty_tuple_type = std::array< bool, sizeof...( Vars )>;
    using variables_tuple_type = tuple< Vars... >;
    
    using variables_set_type = make_unique_variables_t< Vars... >;
    static constexpr size_t size = sizeof...( Vars );

protected:
    template< size_t I, typename Seq >
    struct Helper;

    template< size_t I, size_t J, size_t... Js >
    requires( I == var_id_v< pack_element_t< J, Vars... >> and 
        1 == var_order_v< pack_element_t< J, Vars... >> )
    struct Helper< I, seq< J, Js... >>
    { 
        using variable_type = pack_element_t< J, Vars... >;

        static constexpr tuple_element_t< J, values_tuple_type > 
        get( values_tuple_type const& vals )
        { return std::get< J >( vals ); }

        static constexpr tuple_element_t< J, values_tuple_type >
        set( values_tuple_type& vals, dirty_tuple_type& flags, 
            tuple_element_t< J, values_tuple_type > const& val )
        { 
            std::get< J >( vals ) = val; 
            std::get< J >( flags ) = true;
            return val;
        }

        static constexpr bool
        is_dirty( dirty_tuple_type const& flags )
        { return std::get< J >( flags ); }

        static constexpr void
        wash( dirty_tuple_type& flags )
        { std::get< J >( flags ) = false; }

        static constexpr bool has_value = true; 
    };

    template< size_t I, size_t J, size_t... Js >
    requires( I == var_id_v< pack_element_t< J, Vars... >> and
        1 != var_order_v< pack_element_t< J, Vars... >> )
    struct Helper< I, seq< J, Js... >>
    { 
        // should never be instantiated
        using variable_type = void;

        static constexpr bool 
        has_value = false; 
    };

    template< size_t I, size_t J, size_t... Js >
    requires( I != var_id_v< pack_element_t< J, Vars... >> )
    struct Helper< I, seq< J, Js... >>:
        Helper< I, seq< Js... >>
    { };

    template< size_t I >
    struct Helper< I, seq<>>
    { static constexpr bool 
        has_value = false; };

    template< variable Var >
    using helper_for = Helper< var_id_v< Var >, make_seq< size >>;

    template< size_t Id >
    using helper_for_id = Helper< Id, make_seq< size >>;

    template< size_t... Js >
    constexpr void initialize_flags( seq< Js... > )
    {(( std::get< Js >( _flags ) = false ), ... ); }

public:
    static consteval bool
    contains_id( size_t Id )
    { return variables_set_type::contains_id( Id ); }

    template< size_t Id >
    using variable_t = helper_for_id< Id >::variable_type;

    /// @brief determines if the scope has a value for variable I
    template< variable Var >
    static constexpr bool 
    has_value_v = Helper< var_id_v< Var >, 
        make_seq< size >>::has_value;

    template< variable Var >
    static consteval bool 
    has_value( Var ) 
    { return has_value_v< Var >; }

    /// @brief retrieves the value of variable Var in this scope
    template< variable Var >
    constexpr typename Var::value_type 
    get_value( Var = {} ) const 
    { return helper_for< Var >::get( _values ); }

    /// @brief assigns other to the scoped value of Var
    /// TODO: should we use result_type here instead of value_type?
    template< variable Var >
    constexpr typename Var::value_type 
    set_value( typename Var::value_type const& other, Var var = {} ) 
    { return helper_for< Var >::set( _values, _flags, other ); }

    /// @brief has a variable's value been assigned by set_value?
    template< variable Var >
    constexpr bool
    is_dirty( Var = {} ) const
    { return helper_for< Var >::is_dirty( _flags ); }

    /// @brief force our flag to false for Var
    template< variable Var >
    constexpr void
    wash( Var = {} ) const
    { helper_for< Var >::wash( _flags ); }

    /// @brief take values and flags from another 
    template< typename ScopeU >
    constexpr void
    take_from( ScopeU const& other )
    {
        auto take_value_if_dirty = [&]< variable Var >( Var var ) constexpr 
        { if( other.is_dirty( var ))
                set_value( other.get_value( var ), var ); };

        auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
        {( take_value_if_dirty( pack_element_t< Is, Vars... >{} ), ... ); }; 

        helper( make_seq< sizeof...( Vars )>{} );
    }

    /// @brief returns a tuple of scoped variables
    constexpr tuple< Vars... > variables() const
    { return { Vars{}... }; }

    /// @brief invocation against a constant will return the constant's value
    template< auto Value > 
    constexpr decltype( Value ) 
    operator ()( Constant< Value > ) const
    { return Value; }

    /// @brief invocation against a static will return the static's value
    template< typename T >
    constexpr T 
    operator ()( StaticValue< T > const& static_value ) const
    { return static_cast< T >( static_value ); }

    template< typename T >
    requires( not expression< T > )
    constexpr T 
    operator ()( T const& value ) const
    { return value; }

    // @brief sets the value of a variable in this scope
    // TODO: update this for a more generic SetVar
    template< size_t Id, auto Value >
    requires( contains_id( Id ))
    constexpr var_value_t< variable_t< Id >>
    operator ()( SetVar< Id, Constant< Value >> const& setter )
    { return set_value< variable_t< Id >>( Value ); }

    template< size_t Id, typename T >
    requires( contains_id( Id ))
    constexpr var_value_t< variable_t< Id >>
    operator ()( SetVar< Id, StaticValue< T >> const& setter )
    { return set_value< variable_t< Id >>( 
        std::get< 0 >( setter ).get_value() ); }

    template< size_t Id, typename T >
    requires( contains_id( Id ) and not expression< T > )
    constexpr var_value_t< variable_t< Id >>
    operator ()( SetVar< Id, T > const& setter )
    { return set_value< variable_t< Id >>( std::get< 0 >( setter )); }

    // @brief invoking with a parameter list will evaluate
    // the comma operator on the invocation of each argument
    template< typename... Args >
    requires( is_greater( sizeof...( Args ), 1 ))
    constexpr auto 
    operator ()( Args const&... args )
    { return ( operator ()( args ), ... ); }

    // NOTE: we shouldn't need this anymore as the applier will parse expressions
    // @brief invoking on a compound expression
//    template< compound_expression ExprT >
//    //requires( scope_contains_unique_variables_v< ExprT, Scope< Vars... >> )
//    constexpr result_t< ExprT > 
//    operator ()( ExprT const& expr ) const;

    /// @brief invocation against a scoped variable will return the
    /// scoped value
    template< variable Var >
    requires( scope_contains_variable_v< var_id_v< Var >, 
        Scope< Vars... >> )
    constexpr var_value_t< Var >
    operator ()( Var const& var ) const 
    { return helper_for< Var >::get( _values ); }

    constexpr Scope& operator =( Scope const& other )
    {
        take_from( other );
        return *this;
    }

    explicit constexpr Scope( tuple< Vars... > const& vars ):
        tuple< Vars... >{ vars } 
    { initialize_flags( make_seq< size >{} ); }

    constexpr Scope( Vars&&... vars ) 
        requires( is_greater( sizeof...( Vars ), 0 )): 
            tuple< Vars... >{ vars... } 
    { initialize_flags( make_seq< size >{} ); }

    constexpr Scope( Scope const& other ) = default;

    constexpr Scope(): tuple< Vars... >{}, _values{}
    { initialize_flags( make_seq< size >{} ); }

private:
    values_tuple_type _values;
    dirty_tuple_type _flags;
};

/////////////////////////
/// Scope Comparison ///
///////////////////////
///
/// Utilities to compare a scope's breadth, values and flags
template< variable... VarsA, typename ScopeB >
consteval bool 
is_sub_scope( Scope< VarsA... > const&, ScopeB const& )
{ return ( ScopeB::template has_value_v< VarsA > and ... ); }

#ifndef NDEBUG
template< variable... Vars >
constexpr std::string ScopeString( Scope< Vars... > const& scope )
{
    std::string ret = "";
    ret += (( std::to_string( Vars::id ) + "==" + 
        std::to_string( scope.get_value( Vars{} )) + 
            "(" + std::to_string( scope.is_dirty( Vars{} )) + "), " ) + ... );
    return ret;
}
#endif // DEBUG

template< variable... VarsA, typename ScopeB >
constexpr bool 
compatible_scopes( Scope< VarsA... > const& left, 
    ScopeB const& right )
{
#ifndef NDEBUG
    std::string avars = ScopeString( left );
    std::string bvars = ScopeString( right );

    std::println( "checking scope compatibility:\n{}\n{}", avars, bvars ); 
    std::println( "subscopes: {} and {}", is_sub_scope( left, right ), 
        is_sub_scope( right, left ));
#endif // DEBUG
    // scopes must store the same variables...
    if( not is_sub_scope( left, right ) or not is_sub_scope( right, left ))
        return false;

    // ... and if they are both flagged they must have the same value.
    return (( not ( left.is_dirty( VarsA{} ) and right.is_dirty( VarsA{} )) or 
        left.get_value( VarsA{} ) == right.get_value( VarsA{} )) and ... );
}

template< variable... VarsA, typename... OtherScopes >
requires( is_greater( sizeof...( OtherScopes ), 1 ))
constexpr bool 
compatible_scopes( Scope< VarsA... > const& left, 
    OtherScopes const&... others )
{ return ( compatible_scopes( left, others ) and ... ); }

template< variable... VarsA, typename... OtherScopes >
constexpr bool compatible_scopes( 
    tuple< Scope< VarsA... >, OtherScopes... > const& tup )
{ 
    auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr
    { return compatible_scopes( std::get< 0 >( tup ), 
        std::get< 1 + Is >( tup )... ); };

    return helper( make_seq< sizeof...( OtherScopes )>{} );
}

template< variable... VarsA, size_t Size >
constexpr bool compatible_scopes( 
    std::array< Scope< VarsA... >, Size > const& tup )
{
    auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr
    { return compatible_scopes( std::make_tuple( 
        std::get< Is >( tup )... )); };

    return helper( make_seq< Size >{} );
}

/// @brief Constructs a new scope by adopting values of un-flaged variables
/// from the flagged values in other scopes
///
/// @pre assumes scopes are compatible
/// @returns A scope compatible with left and right whose variables are dirty
/// if and only if left or right's variable was dirty
///
template< variable... VarsA, typename ScopeB >
constexpr Scope< VarsA... > 
merge_compatible_scopes( Scope< VarsA... > const& left, ScopeB const& right )
{
    Scope< VarsA... > scope;
#ifndef NDEBUG
    if( not compatible_scopes( left, right ))
        throw std::logic_error( "incompatible scopes cannot be merged." );
#endif
    auto set_variable_value = [&]< variable Var >( Var var ) constexpr 
    {
        // if the right scope is dirty then it's value must be equal
        // to left by the compatibility assumption, so we use it.
        if( right.is_dirty( var ))
            scope.set_value( right.get_value( var ), var );
        
        // otherwise if the left scope is dirty we use it's value
        else if( left.is_dirty( var ))
            scope.set_value( left.get_value( var ), var );

        // neither scope contains a dirty Var so we do nothing
        // so that the returned scope also has a clean Var and is
        // therefore compatible with left and right.
    };

    auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
    {( set_variable_value( pack_element_t< Is, VarsA... >{} ), ... ); }; 

    helper( make_seq< sizeof...( VarsA )>{} );
    return scope;
}

template< variable... VarsA >
constexpr Scope< VarsA... > const&
merge_compatible_scopes( Scope< VarsA... > const& only )
{ return only; }

template< variable... VarsA, typename... OtherScopes >
requires( is_greater( sizeof...( OtherScopes ), 1 ))
constexpr Scope< VarsA... >
merge_compatible_scopes( Scope< VarsA... > const& left, 
    OtherScopes const&... others )
{
    Scope< VarsA... > scope = left;
    return (( scope = merge_compatible_scopes( scope, others )), ... );
}

template< variable... VarsA, typename... OtherScopes >
constexpr Scope< VarsA... >
merge_compatible_scopes( 
    std::tuple< Scope< VarsA... >, OtherScopes... > const& tup )
{
    auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr 
    { return merge_compatible_scopes( std::get< 0 >( tup ),
        std::get< 1 + Is >( tup )... ); };

    return helper( make_seq< sizeof...( OtherScopes )>{} );
}

template< variable... VarsA, size_t Size >
constexpr Scope< VarsA... >
merge_compatible_scopes(
    std::array< Scope< VarsA... >, Size > const& scopes )
{
    auto helper = [&]< size_t... Is >( seq< Is... > ) constexpr
    { return merge_compatible_scopes( std::make_tuple( 
        std::get< Is >( scopes )... )); };

    return helper( make_seq< Size >{} );
};

////////////////////
/// SimpleScope ///
//////////////////
///
template< typename Seq, typename... Values >
struct SimpleScopeHelper;

template< size_t... Is, typename... Values >
struct SimpleScopeHelper< seq< Is... >, Values... >
{ 
    using type = Scope< Var< Is, pack_element_t< Is, Values... >>... >;

    template< typename... Names >
    requires( sizeof...( Names ) == sizeof...( Is ))
    static constexpr type from_names( Names const&... names )
    { return { Var< Is, pack_element_t< Is, Values... >>{ 
        pack_element< Is >( names... )}... }; }

    static constexpr type from_values( Values const&... values )
    { 
        type scope;
        ( set_value< Var< Is, pack_element_t< Is, Values... >>>( scope, 
            pack_element< Is >( values... )), ... );
        return scope;
    }
};

template< typename... Values >
constexpr typename SimpleScopeHelper< make_seq< sizeof...( Values )>, 
    Values... >::type
simple_scope( Values const&... values )
{ return SimpleScopeHelper< make_seq< sizeof...( Values )>, Values... >::
    from_values( values... ); }

/// @brief method to declare a set of variables to be used in expressions
/// @tparam ...Decls 
/// @param ...decls 
/// @return 
template< typename... Decls >
typename SimpleScopeHelper< make_seq< sizeof...( Decls )>, 
    typename Decls::value_type... >::type
constexpr declare_variables( Decls... decls )
{ return SimpleScopeHelper< make_seq< sizeof...( Decls )>, typename
    Decls::value_type... >::from_names( decls.name()... ); }

/////////////////////
/// Buffer Scope ///
///////////////////
///
/// Reads of variables come from the scope ScopeT, but writes are all made to
/// a buffer. When destroyed the buffered values are copied back to the 
/// referenced scope
///
/// DT: can you buffer a buffer?
template< scope ScopeT >
struct Buffer: ScopeT
{
    using scope_type = ScopeT;

    constexpr scope_type const&
    read_scope() const
    {
        if( _read_scope_ptr == nullptr )
            throw std::logic_error( "reading from a moved buffer" );

        return *_read_scope_ptr; 
    }

    // read from the _read_scope
    template< typename ExprT >
    requires( std::is_invocable_v< scope_type, ExprT > and 
        not is_set_expression_v< ExprT > )
    constexpr auto 
    operator ()( ExprT const& expr ) const
    { return read_scope()( expr ); }

    constexpr Buffer() = delete;
    constexpr Buffer( Buffer const& ) = default;

    // moving requires unsetting the buffered read scope ptr to prevent
    // premature copying on destruction
    constexpr Buffer( Buffer&& other ): Buffer( other )
    { other._read_scope_ptr = nullptr; }
    
    // construction from the buffered scope type saves a pointer to the 
    // buffered scope and copies the scope values using the copy constructor
    constexpr Buffer( scope_type& read_scope ): 
        _read_scope_ptr{ &read_scope }, scope_type{ read_scope }
    { }

    // destruction copies the values from our buffer back into the referenced
    // scope we used for reads, but does nothing if we were moved
    constexpr ~Buffer()
    { 
        if( _read_scope_ptr != nullptr ) 
        {
            *_read_scope_ptr = *(scope_type*)this; 
            _read_scope_ptr = nullptr;
        }
    }

private:
    scope_type* _read_scope_ptr;
};

// DT: iteration tests went into an infinite processor loop (memory never moved)

// a buffer is a scope
template< scope ScopeT >
struct IsScope< Buffer< ScopeT >>: true_type { };

template< scope ScopeT >
constexpr Buffer< ScopeT >
buffer( ScopeT& scope )
{ return { scope }; }

// if the scope is already buffered, don't rebuffer it...
template< scope ScopeT >
constexpr Buffer< ScopeT >&
buffer( Buffer< ScopeT >& buffered_scope )
{ return buffered_scope; }

template< scope ScopeT >
struct BufferType
{ using type = Buffer< ScopeT >; };

template< scope ScopeT >
struct BufferType< Buffer< ScopeT >>
{ using type = Buffer< ScopeT >; };

template< typename ScopeT >
using buffer_t = BufferType< ScopeT >::type;

// and we can't buffer a constant scope so don't bother
template< scope ScopeT >
constexpr ScopeT const&
buffer( ScopeT const& scope )
{ return scope; }


} // namespace expressions

#endif // __EXPRESSIONS_SCOPE_HPP__
