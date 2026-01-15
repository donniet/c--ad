
#include <iostream>
#include <ranges>
#include <functional>
#include <bitset>

import iso_surface;
import linalg;

template<typename IntType = int>
struct Grid 
{
    using integer_type = IntType;

    using coordinate_predicate = std::function< bool(double x, double y) >;

    struct cell_type
    {
        /**
         * from_ullong converts a bitfield into a cell
         * |high ------------------------------------------ low|
         * |Y[0]|X[0]|...|Y[N-1]|X[N-1]|Y[N]|X[N]|NEG(Y)|NEG(X)|
         */
        static cell_type from_ullong(unsigned long long bits)
        {
            bool negative_x = bits & 0b01;
            bool negative_y = bits & 0b10;
            bits >>= 2;

            integer_type x, y;

            for(int i = 0; i < sizeof(bits) - 2; i += 2)
            {
                x = (x << 1) | bits & 0b01;
                y = (y << 1) | ((bits & 0b10) >> 1);
                bits >>= 2;
            }

            return {
                negative_x ? -x : x,
                negative_y ? -y : y };
        }
        
        integer_type x, y;
    };

    enum corner_type : int
    {
        no_corner = -1,
        lower_left = 0b00,
        lower_right = 0b01,
        upper_left = 0b10,
        upper_right = 0b11,
        total_corners = 4,
    };

    struct selected_cell
    {
        cell_type cell;
        std::bitset<total_corners> corner_selected;
    };

    /**
     * cell_view is an enumeration of 2D cells in a grid identified by 
     * two integers x and y representing the corner closest to the origin
     * and two bits representing the quadrant of the cartisian space
     * this cell is in.
     */
    struct cell_view : std::ranges::view_interface<cell_view>
    {
        using range_type = disjoint_intervals<unsigned long long>;
        using value_type = cell_type;

        range_type range;

        struct const_iterator 
        {
            value_type operator*() const
            { return cell_type::from_ullong(*range_iterator); }
            const_iterator& operator++()
            { ++range_iterator; return *this; }

            // TODO: make this iterator random access
            // const_iterator& operator+=();
            range_type::const_iterator range_iterator;
        };

        const_iterator begin() const
        { return { range.begin() }; }
        const_iterator end() const
        { return { range.end() }; }
    };

    struct selected_cell_view :
        std::ranges::view_interface<selected_cell_view>
    {

    };

    struct cell_range_adapter : 
        std::ranges::range_adaptor_closure<cell_range_adapter>
    {
        constexpr selected_cell_view operator()( cell_view cells ) const
        {

        }
    };

    cell_range_adapter select_if( coordinate_predicate& predicate )
    {

    }
};

template<typename PointType>
struct TriangleView
{

};



template<typename CellType>
struct TriangularMarch : std::ranges::range_adapter_closure<TriangularMarch>
{
    using cell_type = CellType;
    using point_type = typename cell_type::point_type;
    using triangle_view = TriangleView<point_type>;

    point_type minimum, maximum;
    
    template<typename CellView>
    constexpr triangle_view operator()( CellView&& cell_view ) const
    {
        return { triangulate_cells(cell_view) };
    }

};

auto triangular_march()
{

}


template<typename FloatType>
std::ostream& operator<<(std::ostream& os, Point<FloatType> const& point)
{ return os << point.x << " " << point.y << " " << point.z; } 

int main(int ac, char* av[])
{
    Grid canvas;

    auto inside_ellipse = [](double x, double y) -> bool
    { return 0.05 * x * x + 0.02 * y * y <= 100; };

    auto simplex = canvas.select_if( inside_ellipse ) | triangular_march;
    auto model_out = make_stl_writer( std::cout );

    model_out << simplex;

    return 0;
}