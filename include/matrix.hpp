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

template< size_t... Is >
using seq = ::std::index_sequence< Is... >;

template< size_t Size >
using make_seq = ::std::make_index_sequence< Size >;

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
    template< typename Matrix >
    struct TransposeHelper;

    template< typename MatrixType, typename IndexSequence >
    struct TransposeTypeHelper;

    template< typename MatrixType, size_t... Is >
    struct TransposeTypeHelper< MatrixType, seq< Is... >>
    { 
        static constexpr size_t stride = MatrixType::stride;

        static constexpr size_t transposed_index( size_t I )
        { return row_of( I, stride ) + col_of( I, stride ) * stride; }

        using type = SquareMatrix< 
            std::tuple_element_t< transposed_index( Is ), 
                typename MatrixType::tuple_type >... >;
    };

    template< typename... Ts >
    struct TransposeHelper< SquareMatrix< Ts... >>
    {
        using matrix_type = SquareMatrix< Ts... >;
        static constexpr size_t stride = matrix_type::stride;
        static constexpr size_t size = sizeof...( Ts );

        using type = TransposeTypeHelper< matrix_type, 
            std::make_index_sequence< size >>::type;
         
        static type transpose( matrix_type const& mat )
        { return transpose_helper( mat, 
            std::make_index_sequence< sizeof...( Ts )>{} ); }

        template< size_t... Is >
        static type transpose_helper( matrix_type const& mat, 
            seq< Is... > )
        {
            type ret;
            (( ret.template at< row_of( Is, stride ), col_of( Is, stride ) >() = 
               mat.template at< col_of( Is, stride ), row_of( Is, stride ) >()), 
              ... );
            return ret;
        }
    };

    template< typename Matrix >
    using transpose_t = TransposeHelper< Matrix >::type;

    template< typename Matrix >
    transpose_t< Matrix > transpose( Matrix const& mat )
    { return TransposeHelper< Matrix >::transpose( mat ); }

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
                    mat.template at< 0, Is >() * // value of 0th row, Is col
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

    static constexpr shape_type row_col_of( size_t I )
    { return { I / columns, I % columns }; }

    template< size_t Row, size_t Col >
    using element_t = helper_type::template element_t< Row, Col >;

    template< size_t Row, size_t Col >
    element_t< Row, Col >& at() 
    { return std::get< Row * columns + Col >( *this ); }

    template< size_t Row, size_t Col >
    element_t< Row, Col > const& at() const
    { return std::get< Row * columns + Col >( *this ); }

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


    SquareMatrix() = default;
    SquareMatrix( Ts... ts ) : tuple_type{ ts... } { }
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

} // namespac matrix

#endif