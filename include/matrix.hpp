/**
 * Matrix library for expressions
 */

#ifndef __MATRIX_HPP__
#define __MATRIX_HPP__

#include "utility.hpp"

#include <utility>
#include <tuple>
#include <array>
#include <iostream>

using std::size_t;

/**
 * Matrices of expressions
 */
namespace matrix {

using std::get;

// TODO: Vector or Formulae?  Any?  All?
template< typename... Ts >
struct Vector : std::tuple< Ts... > 
{ };

template< typename... Ts >
requires is_square< sizeof...( Ts )>::value
struct SquareMatrix;

template< typename... Ts >
requires is_square< sizeof...( Ts )>::value
SquareMatrix< Ts... > from( Ts... ts );

namespace detail {
    static constexpr size_t row_of( size_t I, size_t stride ) 
    { return I / stride; }
    static constexpr size_t col_of( size_t I, size_t stride ) 
    { return I % stride; }

    template< typename Matrix >
    struct SquareMatrixHelper;

    template< typename... Ts >
    struct SquareMatrixHelper< SquareMatrix< Ts... >>
    {
        using type = SquareMatrix< Ts... >;
        using tuple_type = std::tuple< Ts... >;
        using shape_type = std::tuple< size_t, size_t >;
        static constexpr size_t size = sizeof...( Ts );
        static constexpr size_t stride = square_root< size >::value;
        static constexpr size_t rows = stride; 
        static constexpr size_t columns = stride;
        static constexpr shape_type shape = { rows, columns };

        template< size_t Row, size_t Col >
        using element_t = std::tuple_element_t< Row * columns + Col, tuple_type >;
    };

    /**
     * Transpose helpers
     */
    template< typename Matrix, size_t... Is >
    auto transpose_helper( Matrix const& mat, seq< Is... > )
    { 
        static constexpr size_t stride = Matrix::stride;
        return from( mat.template at< col_of( Is, stride ), row_of( Is, stride )>()... );
    }

    template< typename Matrix >
    auto transpose( Matrix const& mat );

    template< typename... Ts >
    auto transpose( SquareMatrix< Ts... > const& mat )
    { return transpose_helper( mat, make_seq< sizeof...( Ts )>{} ); }

    template< typename Matrix >
    using transpose_t = decltype( transpose( Matrix{} ));

    /**
     * Submatrix utilities
     */
    template< typename Matrix, size_t Row, size_t Col, typename OutIndexSequence >
    struct SubMatrixResultHelper;

    template< typename Matrix, size_t Row, size_t Col, size_t... Is >
    struct SubMatrixResultHelper< Matrix, Row, Col, seq< Is... >>
    {
        static constexpr size_t size = sizeof...( Is );
        static constexpr size_t stride = square_root< size >::value;
        static constexpr size_t input_row_of( size_t I ) 
        { 
            size_t r = row_of( I, stride );
            return r < Row ? r : r + 1;
        }
        static constexpr size_t input_col_of( size_t I ) 
        { 
            size_t c = col_of( I, stride );
            return c < Col ? c : c + 1;
        }

        template< size_t InRow, size_t InCol >
        using input_element_t = SquareMatrixHelper< Matrix >::
            template element_t< InRow, InCol >;

        template< size_t I >
        using sub_index_t = input_element_t< 
            input_row_of( I ), input_col_of( I ) >;

        using type = SquareMatrix< sub_index_t< Is >... >;

        // TODO: do we need to move this after the definition of SquareMatrix?
        static type sub( Matrix const& mat )
        { return { mat.template at< input_row_of( Is ), input_col_of( Is )>()... }; }
    };

    template< typename Matrix, size_t Row, size_t Col >
    struct SubMatrixHelper;

    template< size_t Row, size_t Col, typename... Ts >
    struct SubMatrixHelper< SquareMatrix< Ts... >, Row, Col>
    {
        using input_type = SquareMatrix< Ts... >;
        static constexpr size_t input_size = input_type::size;
        static constexpr size_t input_stride = input_type::stride;
        
        static constexpr size_t output_stride = input_stride - 1;
        static constexpr size_t output_size = ( output_stride * output_stride );
        using helper_type = SubMatrixResultHelper< input_type, Row, Col, 
            std::make_index_sequence< output_size >>;

        using type = helper_type::type;
        static type sub( input_type const& mat )
        { return helper_type::sub( mat ); }
    };

    template< typename Matrix, size_t Row, size_t Col >
    using sub_matrix_t = SubMatrixHelper< Matrix, Row, Col >::type;

    template< size_t Row, size_t Col, typename Matrix >
    sub_matrix_t< Matrix, Row, Col > sub_matrix( Matrix const& mat )
    { return SubMatrixHelper< Matrix, Row, Col >::sub( mat ); }

    /**
     * Determinant Calculation
     */
    template< typename Matrix >
    auto determinant( Matrix const& mat );

    // determinant of the null matrix is 1 
    // TODO: is that true? Probably, it seems like a math thing...
    template< >
    auto determinant( SquareMatrix< > const& mat ) { return 1.; }

    // determinant of a 1x1 matrix is the value of the matrix
    template< typename T >
    auto determinant( SquareMatrix< T > const& mat )
    { return mat.template at< 0, 0 >(); }

    template< typename Matrix, typename ColumnSequence >
    struct DeterminantHelper;

    template< typename Matrix, size_t... Is >
    struct DeterminantHelper< Matrix, seq< Is... >>
    {
        static constexpr size_t stride = Matrix::stride;
        static auto calculate( Matrix const& mat );
    };

    template< typename... Ts >
    requires ( sizeof...( Ts ) > 1 )
    auto determinant( SquareMatrix< Ts... > const& mat )
    { return DeterminantHelper< SquareMatrix< Ts... >, 
        make_seq< SquareMatrix< Ts... >::stride >>::calculate( mat ); }
    
    template< typename Matrix, size_t... Is >
    auto DeterminantHelper< Matrix, seq< Is... >>::calculate( Matrix const& mat )
    { 
        return ((( Is % 2 == 0 ? 1. : -1. ) *    // alternate permutations
                    mat.template at< 0, Is >() * // value of 0th row, Is-th col
                    determinant(                 // determinant of submatrix
                        sub_matrix< 0, Is >( mat ))) + ... );
    }

    template< size_t Row, size_t Col, typename Matrix >
    auto minor( Matrix const& mat )
    { return determinant( sub_matrix< Row, Col >( mat )); }

    template< size_t Row, size_t Col, typename Matrix >
    auto cofactor( Matrix const& mat )
    { return (( Row + Col ) % 2 == 0 ? 1. : -1 ) * minor< Row, Col >( mat ); }

    template< typename Matrix >
    auto inverse( Matrix const& mat );

    template< typename Matrix, typename Seq >
    auto inverse_helper( Matrix const&, Seq );

    // DT: this can be much more efficient, but we just want it to work first
    template< typename Matrix, size_t... Is >
    auto inverse_helper( Matrix const& mat, std::index_sequence< Is... > )
    {
        static constexpr size_t stride = Matrix::stride;
        auto det = determinant( mat );
        return transpose( from( 
            cofactor< row_of( Is, stride ), col_of( Is, stride ) >( mat )... ));
    }

    template< typename... Ts >
    auto inverse( SquareMatrix< Ts... > const& mat )
    { return inverse_helper( mat, make_seq< sizeof...( Ts ) >{} ); }

    /**
     * Matrix Multiplication
     */
    template< typename Scheme, typename LeftType, typename RightType >
    auto do_multiply( LeftType const& left, RightType const& right );

    template< size_t... Is, typename LeftType, typename RightType >
    auto do_multiply< seq< Is... >>( LeftType const& left, RightType const& right )
    { 
        return from( dot( row< left_row( Is )>( left ), column< right_col( Is )>( right ))... );
    }

    template< typename LeftType, typename RightType >
    struct Multiplier;

    template< typename... Ts, typename... Us >
    requires sizeof...( Ts ) == sizeof...( Us )
    struct Multiplier< SquareMatrix< Ts... >, SquareMatrix< Us... >> 
    {
        using left_type = SquareMatrix< Ts... >;
        using right_type = SquareMatrix< Us... >;
        using output_size = sizeof...( Ts );
        using output_stride = left_type::stride;

        static auto mulitply( left_type const& left, right_type const& right )
        { return do_multiply( left, right, /* ... */ ); }
    };

} // namespace detail

template< typename... Ts >
requires is_square< sizeof...( Ts )>::value
struct SquareMatrix : std::tuple< Ts... >
{ 
    using this_type = SquareMatrix< Ts... >;
    using tuple_type = std::tuple< Ts... >;
    using helper_type = detail::SquareMatrixHelper< this_type >;
    using shape_type = std::tuple< size_t, size_t >;
    static constexpr size_t size = sizeof...( Ts );
    static constexpr size_t stride = square_root< size >::value;
    static constexpr size_t columns = stride, rows = stride;
    static constexpr shape_type shape = { rows, columns };
    using element_seq = make_seq< size >;
    using stride_seq = make_seq< stride >;
    static constexpr element_seq all_elements = {}; 
    static constexpr stride_seq stride_elements = {};

    static constexpr shape_type row_col_of( size_t I )
    { return { I / columns, I % columns }; }

    template< size_t Row, size_t Col >
    using element_t = helper_type::template element_t< Row, Col >;

    template< size_t Row, size_t Col >
    element_t< Row, Col >& at() 
    { return get< Row * columns + Col >( *this ); }

    template< size_t Row, size_t Col >
    element_t< Row, Col > const& at() const
    { return get< Row * columns + Col >( *this ); }

    auto transpose() const
    { return detail::transpose( *this ); }

    template< size_t Row, size_t Col >
    requires ( Row < rows and Col < columns )
    auto sub() const
    { return detail::sub_matrix< Row, Col >( *this ); }

    template< size_t Row, size_t Col >
    requires ( Row < rows and Col < columns )
    auto minor() const
    { return detail::minor< Row, Col >( *this ); }

    template< size_t Row, size_t Col >
    requires ( Row < rows and Col < columns )
    auto cofactor() const
    { return detail::cofactor< Row, Col >( *this ); }
    
    auto det() const
    { return detail::determinant( *this ); }

    auto inverse() const
    { return detail::inverse( *this ); }

    // TODO: this is going to be a pain! have to parameter pack both the params
    //       and the elements.
    // Option 1: wrap the params in a tuple, unpack the elements and pass to
    //           another helper to unpack the tuple of params which finally
    //           invokes the object stored in the matrix. wrap that in a from
    template< typename... Us >
    auto operator()( Us... );

    template< >
    auto operator()() 
    { return invoke_helper( all_elements ); }

    template< typename T >
    auto operator()( T t ) 
    { return invoke_helper( t, all_elements ); }

    template< typename... Us >
    requires ( sizeof...( Us ) == size )
    auto operator+( SquareMatrix< Us... > const& other ) const
    { return element_op_helper( std::plus<>{}, other, all_elements ); }

    template< typename... Us >
    requires ( sizeof...( Us ) == size )
    auto operator-( SquareMatrix< Us... > const& other ) const
    { return element_op_helper( std::minus<>{}, other, all_elements ); }

    template< typename... Us >
    requires ( sizeof...( Us ) == size )
    auto operator*( SquareMatrix< Us... > const& other ) const
    { return multiplies_helper( other, all_elements ); }

    template< typename... Us >
    requires ( sizeof...( Us ) == size )
    auto operator/( SquareMatrix< Us... > const& other ) const
    { return multiplies_helper( other.inverse(), all_elements ); }

    SquareMatrix() = default;
    SquareMatrix( Ts... ts ) : tuple_type{ ts... } { }

private:
    template< size_t... Is >
    auto invoke_helper( seq< Is... > )
    { return from( get< Is >( *this )()... ); }

    template< typename T, size_t... Is >
    auto invoke_helper( T t, seq< Is... > )
    { return from( get< Is >( *this )( t )... ); }

    template< typename OpType, typename OtherType, size_t... Is >
    auto element_op_helper( 
        OpType op, OtherType const& other, seq< Is... > ) const
    { return from( op( get< Is >( *this ), get< Is >( other ))... ); }

    template< typename OtherType, size_t... Is >
    auto multiplies_helper( OtherType const& other, seq< Is... > ) const
    { return from( multiplies_element_helper< 
        detail::row_of( Is, stride ), detail::col_of( Is, stride )>( 
            other, stride_elements )... ); }

    template< size_t Row, size_t Col, typename OtherType, size_t... Is >
    auto multiplies_element_helper( OtherType const& other, seq< Is... > ) const
    { return (( at< Row, Is >() * other.template at< Is, Col >() ) + ... ); }
};

template< typename... Ts >
requires is_square< sizeof...( Ts )>::value
SquareMatrix< Ts... > from( Ts... ts )
{ return { ts... }; }

namespace detail {

template< typename T, size_t I > using always_t = T;

template< size_t I, typename T >
static constexpr T always_value( T value ) { return value; }

template< typename T, size_t... Is >
auto uniform_helper( T value, std::index_sequence< Is... > )
{ return SquareMatrix< always_t< T, Is >... >{ 
    always_value< Is >( value )... }; }

} // namespace detail

template< size_t Side, typename T >
auto uniform( T value = T{} )
{ return detail::uniform_helper( value, make_seq< Side * Side >{} ); }

namespace tests {

    using mat3x3 = SquareMatrix< bool, char,  int,
                                 float, double, long,
                                 unsigned, long long, long double >;

    static_assert( std::is_same_v< mat3x3::element_t< 0, 0 >, bool  > );
    static_assert( std::is_same_v< mat3x3::element_t< 0, 1 >, char  > );
    static_assert( std::is_same_v< mat3x3::element_t< 0, 2 >,  int  > );
    static_assert( std::is_same_v< mat3x3::element_t< 1, 0 >, float > );
    static_assert( std::is_same_v< mat3x3::element_t< 1, 1 >, double  > );
    static_assert( std::is_same_v< mat3x3::element_t< 1, 2 >, long  > );
    static_assert( std::is_same_v< mat3x3::element_t< 2, 0 >,  unsigned  > );
    static_assert( std::is_same_v< mat3x3::element_t< 2, 1 >, long long > );
    static_assert( std::is_same_v< mat3x3::element_t< 2, 2 >, long double > );

    using mat3x3_t = detail::transpose_t< mat3x3 >;

    static_assert( std::is_same_v< mat3x3_t::element_t< 0, 0 >, bool  > );
    static_assert( std::is_same_v< mat3x3_t::element_t< 0, 1 >, float  > );
    static_assert( std::is_same_v< mat3x3_t::element_t< 0, 2 >,  unsigned  > );
    static_assert( std::is_same_v< mat3x3_t::element_t< 1, 0 >, char > );
    static_assert( std::is_same_v< mat3x3_t::element_t< 1, 1 >, double  > );
    static_assert( std::is_same_v< mat3x3_t::element_t< 1, 2 >, long long  > );
    static_assert( std::is_same_v< mat3x3_t::element_t< 2, 0 >,  int  > );
    static_assert( std::is_same_v< mat3x3_t::element_t< 2, 1 >, long > );
    static_assert( std::is_same_v< mat3x3_t::element_t< 2, 2 >, long double > );

    using sub_t = detail::sub_matrix_t< mat3x3, 0, 1 >;

    static_assert( std::is_same_v< sub_t::element_t< 0, 0 >, float > );
    static_assert( std::is_same_v< sub_t::element_t< 0, 1 >, long > );
    static_assert( std::is_same_v< sub_t::element_t< 1, 0 >, unsigned > );
    static_assert( std::is_same_v< sub_t::element_t< 1, 1 >, long double > );

    using mat3_t = decltype( uniform< 3 >( 0. ));

    static_assert( std::is_same_v< decltype( mat3_t{}.det() ), double > );

} // namespace tests



/**
 * Details of printing matrices
 */
namespace detail {

template< size_t Row, typename Matrix, size_t... Columns >
void print_row_elements( std::ostream& os, Matrix const& mat, seq< Columns... > )
{ (( os << mat.template at< Row, Columns >() << " " ), ... ); }

template< typename Matrix, size_t... Rows >
void print_rows( std::ostream& os, Matrix const& mat, seq< Rows... > )
{ (( print_row_elements< Rows >( os, mat, make_seq< Matrix::columns >{} ), 
    os << "\n" ), ... ); }

} // namespace detail


template< typename... Ts >
std::ostream& operator<<( std::ostream& os, SquareMatrix< Ts... > const& mat )
{ 
    detail::print_rows( os, mat, make_seq< SquareMatrix< Ts... >::rows >{} );
    return os;
}

} // namespace matrix

#endif