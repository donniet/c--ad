module;

#include <vector>
#include <functional>
#include <ranges>
#include <array>
#include <numeric>
#include <bitset>

export module iso_surface;

import linalg;

template<typename ValueType>
struct array_view : 
    public std::ranges::view_interface<array_view>
{
    using value_type = ValueType;
    using const_iterator = value_type const*;

    const_iterator begin() const
    { return m_begin; }

    const_iterator end() const
    { return m_end; }

    array_view(const_iterator b, const_iterator e) :
        m_begin(b), m_end(e)
    { }

    const_iterator m_begin, m_end;
};


/** Grid class represents a 3D grid of arbitrary size and density 
 * 
*/
class Grid
{
    enum dimension_names : size_t
    {
        left_right = 0,
        bottom_top = 1,
        back_front = 2,
        dimensions = 3
    };

    struct position_type;
    struct corner_type;
    struct edge_type;
    struct cell_type;

    struct cell_iterator;

    struct corners_view;
    struct positions_view;
    struct cell_view;

    template<typename Float>
    struct density_grid_type : public std::vector<Float> 
    { 

        coordinate_type m_bounds;
    };

    struct cell_grid_type;

    using coordinate_type = std::array<long, 3>;

    friend struct position_type;

    template<typename Float>
    cell_grid_type cells(density_grid_type<Float> const&) const;
    // point_type at(position_type) const;

    // bool contains_surface( characterized_cell cell ) const;
    triangle_grid_type triangulate( Grid::cell_view&& );

    template<typename SurfaceFunction>
    simplex_type surface(SurfaceFunction&&) const;

    position_type origin() const;

    coordinate_type const& bounds() const { return m_bounds; }

    positions_view positions() const;

    position_type corner_of(cell_type, corner_type);
    
    long vertex_id(position_type const&) const;
    long vertex_id_along(position_type const&, edge_type const&) const;
// protected:

    static const unsigned int s_edge_table[256];
    static const int s_triangle_table[256][16];
    static const unsigned int s_edge_table_corner_bitmask[8];
// private:
    coordinate_type m_bounds;
};

/**
 * Implementation 
 */
struct Grid::position_type
{
    position_type operator+(corner_type const&) const;
    position_type operator+(coordinate_type const& other) const
    {
        auto other_position = position_type{other, m_bounds};
        auto ret = *this;
        ret.m_index += other_position.m_index;
        return ret;
    }
    position_type& operator++();
    position_type operator++(int);
    position_type& operator--();
    position_type operator--(int);
    position_type& operator+=(long);
    position_type& operator-=(long);
    long operator-(position_type const&);

    bool operator==(position_type const& other) const
    { return m_index == other.m_index; }    
    bool operator!=(position_type const& other) const
    { return m_index != other.m_index; }
    bool operator>(position_type const& other) const
    { return m_index > other.m_index; }
    bool operator>=(position_type const& other) const
    { return m_index >= other.m_index; }
    bool operator<(position_type const& other) const
    { return m_index < other.m_index; }
    bool operator<=(position_type const& other) const
    { return m_index <= other.m_index; }

    long operator[](int i) const
    { return at(i); }

    long at(int i) const
    { 
        if(i < 0 || i >= 3)
            return 0;
        
        long ret = m_index;
        for(int j = 0; j < i; ++j)
            ret /= m_bounds[j];

        return ret % m_bounds[i];
    }

    long index() const
    { return m_index; }

    bool is_valid() const
    { return m_index >= 0 && m_index < index_upper_bound(); }

    long index_upper_bound() const
    { return m_bounds[0] * m_bounds[1] * m_bounds[2]; }

    position_type position_upper_bound() const
    { return { index_upper_bound(), m_bounds }; }

    position_type(long index, coordinate_type const& bounds) :
        m_index{index}, m_bounds{bounds}
    { }
    position_type(coordinate_type const& coordinates, coordinate_type const& bounds) :
        m_index{coordinates[0] + bounds[0] * 
            (coordinates[1] + bounds[1] * coordinates[2]) }, 
        m_bounds{bounds}
    { }

    long m_index;
    coordinate_type m_bounds;
};

struct Grid::positions_view :
    std::ranges::view_interface<Grid::positions_view>
{
    struct const_iterator
    {
        position_type operator*() const
        { return m_position; }

        position_type operator[](long amount) const
        { return *(*this + amount); }

        const_iterator& operator++()
        { ++m_position; return *this; }
        const_iterator& operator--()
        { --m_position; return *this; }
        const_iterator& operator+=(long amount)
        { m_position += amount; return *this; }
        const_iterator& operator-=(long amount)
        { m_position -= amount; return *this; }
        const_iterator operator+(long amount) const
        { auto ret = *this; ret.m_position += amount; return ret; }
        const_iterator operator-(long amount) const
        { auto ret = *this; ret.m_position -= amount; return ret; }
        long operator-(const_iterator const& other) const
        { auto ret = *this; ret.m_position - other.m_position; return ret; }
        
        bool operator==(const_iterator const& other) const
        { return m_position == other.m_position; }
        bool operator!=(const_iterator const& other) const
        { return m_position != other.m_position; }
        bool operator<(const_iterator const& other) const
        { return m_position < other.m_position; }
        bool operator<=(const_iterator const& other) const
        { return m_position <= other.m_position; }
        bool operator>(const_iterator const& other) const
        { return m_position > other.m_position; }
        bool operator>=(const_iterator const& other) const
        { return m_position >= other.m_position; }

        const_iterator& operator=(const_iterator&&) = default;
        const_iterator& operator=(const_iterator const&) = default;

        operator bool() const
        { return m_position.is_valid(); }
    
        // DT: do we need a default constructor?
        const_iterator(position_type const& position) : 
            m_position{position}
        { }
        const_iterator(const_iterator&&) = default;
        const_iterator(const_iterator const&) = default;

        position_type m_position;
    };

    const_iterator begin() const
    { return { m_grid->origin() }; }
    const_iterator end() const
    { return { m_grid->origin().position_upper_bound() }; }

    //positions_view() : m_grid{nullptr} { }

    positions_view& operator=(positions_view&&) = default;
    positions_view& operator=(positions_view const&) = default;

    positions_view(Grid const& grid) : m_grid(&grid) { }
    positions_view(positions_view const&) = default;
    positions_view(positions_view&&) = default;
    
// private:
    Grid const * m_grid;
};


struct Grid::corner_type
{ 
    enum value_type : int
    {
        back_bottom_left   = 0b000,
        back_bottom_right  = 0b001,
        back_top_right     = 0b010,
        back_top_left      = 0b011,
        front_bottom_left  = 0b100,
        front_bottom_right = 0b101,
        front_top_right    = 0b110,
        front_top_left     = 0b111,
        total_corners      = 8,
    };

    long at(int x) const
    {
        if(x < 0 || x >= dimensions) 
            return 0;
        
        return (m_value & (0b1 << x)) ? 1 : 0;
    }

    int operator[](int x) const
    { return at(x); }

    bool operator==(corner_type other) const
    { return m_value == other.m_value; }
    bool operator!=(corner_type other) const
    { return m_value != other.m_value; }
    bool operator<(corner_type other) const
    { return m_value < other.m_value; }
    bool operator<=(corner_type other) const
    { return m_value <= other.m_value; }
    bool operator>(corner_type other) const
    { return m_value > other.m_value; }
    bool operator>=(corner_type other) const
    { return m_value >= other.m_value; }

    corner_type& operator++() 
    { ++m_value; return *this; }
    corner_type& operator--() 
    { --m_value; return *this; }
    corner_type operator++(int) 
    { corner_type ret = *this; ++m_value; return ret; }
    corner_type operator--(int)
    { corner_type ret = *this; --m_value; return ret; }

    corner_type& operator+=(int i) 
    { m_value += i; return *this; }
    corner_type& operator-=(int i) 
    { m_value -= i; return *this; }
    corner_type operator+(int i) 
    { return { m_value + i }; }
    corner_type operator-(int i) 
    { return { m_value - i }; }

    corner_type& operator+=(corner_type other) 
    { m_value += other.m_value; return *this; }
    corner_type& operator-=(corner_type other) 
    { m_value -= other.m_value; return *this; }
    corner_type operator+(corner_type other) 
    { return { m_value + other.m_value }; }
    corner_type operator-(corner_type other) 
    { return { m_value - other.m_value }; }

    position_type relative_to(position_type const& position) const
    { return position + *this; }

    corner_type& operator=(corner_type const&) = default;
    corner_type& operator=(corner_type&&) = default;

    corner_type(value_type value = back_bottom_left) :
        m_value{value}
    { }
    corner_type(int value) : 
        m_value{value} 
    { }

    corner_type(corner_type const&) = default;
    corner_type(corner_type&&) = default;
    
private:
    int m_value;
};

struct Grid::corners_view : 
    public std::ranges::view_interface<Grid::corners_view>
{
    struct const_iterator
    {       
        using value_type = corner_type;

        value_type operator*() const 
        { return m_current; }
        const_iterator& operator++() 
        { ++m_current; return *this; }
        bool operator==(const_iterator const& other) const
        { return m_current == other.m_current; }
        bool operator!=(const_iterator const& other) const
        { return m_current != other.m_current; }

        const_iterator(value_type corner) : m_current{corner} { }

        value_type m_current;
    };

    const_iterator begin() const { return { 0 }; }
    const_iterator end() const { return { corner_type::total_corners }; }
};

struct Grid::edge_type 
{
    enum value_type : int 
    {
        back_bottom,
        bottom_right,
        front_bottom,
        bottom_left,

        back_top, 
        top_right,
        top_front, 
        top_left,

        back_left, 
        back_right, 
        front_right, 
        front_left,

        total_edges
    };

    long relative_vertex_id(Grid const& grid, position_type const& p)
    {
        using rel = coordinate_type;

        switch(m_value) 
        {
        case edge_type::back_bottom:
            return grid.vertex_id(p + rel{0,0,0}) + 1;
        case edge_type::bottom_right:
            return grid.vertex_id(p + rel{0,1,0}) + 0;
        case edge_type::front_bottom:
            return grid.vertex_id(p + rel{1,0,0}) + 1;
        case edge_type::bottom_left:
            return grid.vertex_id(p + rel{0,0,0}) + 0;

        case edge_type::back_top: 
            return grid.vertex_id(p + rel{0,0,1}) + 1;
        case edge_type::top_right: 
            return grid.vertex_id(p + rel{0,1,1}) + 0;
        case edge_type::top_front: 
            return grid.vertex_id(p + rel{1,0,1}) + 1;
        case edge_type::top_left: 
            return grid.vertex_id(p + rel{0,0,1}) + 0;

        case edge_type::back_left: 
            return grid.vertex_id(p + rel{0,0,0}) + 2;
        case edge_type::back_right: 
            return grid.vertex_id(p + rel{0,1,0}) + 2;
        case edge_type::front_right: 
            return grid.vertex_id(p + rel{1,1,0}) + 2;
        case edge_type::front_left: 
            return grid.vertex_id(p + rel{1,0,0}) + 2;
        }

        return -1;
    }

private:
    float m_distance; /* [0,1] */
    int m_value;
};

template<typename T>
concept view_of = std::ranges::view<T>;


struct Grid::cell_type {
    friend struct cell_iterator;
    
    using triangle_pattern = tetrahedral_triangulation<triangle_type>;

    // from CIsoSurface.cpp

    static constexpr corners_view corners()
    { return {}; }

    // this_is_a( MovableType )
    cell_type& operator=(cell_type const&) = default;
    cell_type& operator=(cell_type&&) = default;

    cell_type() : m_field{}, m_is_rightmost{false}, m_is_topmost{false}, 
        m_is_frontmost{false}, m_triangles{}
    { }
    cell_type(scalar_field_iterator back_bottom_left_corner) :
        m_field{back_bottom_left_corner}
    {
        auto right_side = m_field[1,0,0];
        auto top_side   = m_field[0,1,0];
        auto front_side = m_field[0,0,1];
        auto extents = m_field.extents();

        m_is_rightmost = ( right_side == extents[0] - 1 );
        m_is_topmost   = ( top_side   == extents[1] - 1 );
        m_is_frontmost = ( front_side == extents[2] - 1 );

        m_triangles = triangle_pattern::from_corners
        ( corners() | mapped_to( m_field ) );
    }

    cell_type(cell_type const&) = default;
    cell_type(cell_type&&) = default;

    int triangles_found() const;
    bool is_rightmost() const { return m_is_rightmost; }
    bool is_topmost() const   { return m_is_topmost;   }
    bool is_frontmost() const { return m_is_frontmost; }



private:
    bool m_is_rightmost, m_is_topmost, m_is_frontmost;
    scalar_field_iterator m_field;
    triangle_pattern m_triangles;
};

struct Grid::cell_iterator 
{
    friend class Grid;

    cell_type operator*() const { return m_cell; }
    cell_type const* operator->() const { return &m_cell; }

    cell_iterator& operator++()
    { increment_helper(1); return *this; }
    cell_iterator operator++(int)
    { auto ret = *this; increment_helper(1); return ret; }
    cell_iterator& operator+=(long amount)
    { increment_helper(amount); return *this; }
    cell_iterator& operator--()
    { increment_helper(-1); return *this; }
    cell_iterator operator--(int)
    { auto ret = *this; increment_helper(-1); return ret; }
    cell_iterator& operator-=(long amount)
    { increment_helper(-amount); return *this; }

    cell_iterator operator+(long amount)
    { auto ret = *this; ret.increment_helper(amount); return ret; }
    cell_iterator operator-(long amount)
    { auto ret = *this; ret.increment_helper(-amount); return ret; }

    void increment_helper(long amount)
    {
        amount += m_cell[0];
        m_cell[0] = amount % m_grid->bounds(0);
        amount /= m_grid->bounds(0);

        amount += m_cell[1];
        m_cell[1] = amount % m_grid->bounds(1);
        amount /= m_grid->bounds(1);

        m_cell[2] += amount;
    }

    bool is_valid() const
    {
        return m_cell[0] >= 0 && m_cell[0] < m_grid->bounds(0) &&
               m_cell[1] >= 0 && m_cell[1] < m_grid->bounds(1) &&
               m_cell[2] >= 0 && m_cell[2] < m_grid->bounds(2);
    }


    corners_view corners() const;

    auto edge_position(edge_type const&) const
    { 
        // TODO: return the linear interpolated position along this edge
    }

    bool operator==(cell_iterator const& other) const 
    { return m_cell == other.m_cell; };
    bool operator!=(cell_iterator const& other) const 
    { return m_cell != other.m_cell; };
    bool operator==(std::default_sentinel_t) const
    { return m_cell < cell_type::origin() || m_cell >= m_grid->m_bounds; }
    bool operator!=(std::default_sentinel_t) const
    { return m_cell >= cell_type::origin() && m_cell < m_grid->m_bounds; }
    bool operator<(cell_iterator const& other) const
    { return m_cell < other.m_cell; }
    bool operator<=(cell_iterator const& other) const
    { return m_cell <= other.m_cell; };
    bool operator>(cell_iterator const& other) const 
    { return m_cell > other.m_cell; };
    bool operator>=(cell_iterator const& other) const
    { return m_cell >= other.m_cell; };

    cell_iterator(cell_iterator&&) = default;
    cell_iterator(cell_iterator const&) = default;
    cell_iterator& operator=(cell_iterator&&) = default;
    cell_iterator& operator=(cell_iterator const&) = default;

protected:
    cell_iterator(Grid const* grid, cell_type cell = {}) :
        m_grid{grid}, m_cell{cell}
    { }
private:
    Grid const* m_grid;
    cell_type m_cell;
};



struct Grid::cell_view : 
    public std::ranges::view_interface<cell_view>
{
    cell_iterator begin()  { return m_begin; }
    cell_iterator end()    { return m_end; }

    cell_iterator m_begin, m_end;
    cell_view(cell_iterator b, cell_iterator e) :
        m_begin{b}, m_end{e}
    { }
};

/**
 * table_index calculates the index into the edge table used to construct facets
 * pred is a function f(p : position_type) -> float_type and the surface is 
 * defined where pred(p) == 0
 */
template<typename SurfaceFunction>
size_t Grid::table_index(
    Grid::cell_iterator&& this_cell, SurfaceFunction&& pred)
{
    using std::views::filter,
          std::get,
          // DT: not implemented in libc++ yet (#105251) 
          //     https://github.com/llvm/llvm-project/issues/105251)
          // std::views::enumerate,
          std::views::values,
          std::views::zip,
          std::reduce;

    static auto combine_masks = std::bit_or< >{};

    auto in_volume = [&pred]( auto const& pair ) 
    { return pred(get<0>(pair)); };

    auto valid_corners = 
        zip(cell.corners(), s_edge_table_corner_bitmask) |
        filter( in_volume ) | values; 

    return reduce(valid_corners.begin(), valid_corners.end(), combine_masks); 
}

long Grid::vertex_id(Grid::position_type const& p) const
{ 
    long index = ( p[0] + ( m_bounds[0] + 1 ) * ( p[1] + ( m_bounds[1] + 1 ) * p[2] ) ); 

    return 3 * index; // leave room for interpolation
}

Grid::triangle_type 
Grid::triangle_from(Grid::cell_iterator this_cell, Grid::edge_type edges[3])
{
    //TODO: see GetEdgeID and GenerateSurface functions in CIsoSurface.cpp
    return { this_cell.edge_position(edges[0]), 
             this_cell.edge_position(edges[1]), 
             this_cell.edge_position(edges[2]) };
}

Grid::positions_view Grid::positions() const
{ return { *this }; }

position_type Grid::origin() const
{ return { 0, m_bounds }; }

long Grid::bounds(int dim) const
{ return m_bounds[dim]; }

Grid::triangulation_range 
Grid::triangulate( Grid::cell_view&& cell_range )
{ return { cell_range }; }

template<typename SurfaceFunction>
Grid::simplex_type Grid::surface(SurfaceFunction&& surf) const 
{
    auto triangulate = std::bind(&Grid::interpolate, this);

    // evaluate the function at each of the grid points
    density_grid_type density = positions() | surf;

    // create a set of cells using the density grid
    cell_grid_type cell_grid = cells(density);

    // triangluate the cells
    return cell_grid | triangulate;
}




template< typename TriangleType, typename FieldPredicate = greater_than_zero >
struct tetrahedral_triangulation
{
    static constexpr int maximum_triangles = 5;
    static constexpr size_t total_corners = 8;

    static constexpr FieldPredicate predicate{};

    using triangle_type = TriangleType;
    using triangles_view = array_view< triangle_type >;
    using triangles_array = std::array< triangle_type, maximum_triangles >;
    using index_type = size_t;

    operator bool() const { return triangle_count() > 0; }
    int triangle_count() const { return m_triangle_count; }
    triangles_view triangles() const 
    { return { m_triangles.begin(), m_triangles.end() }; }

    template<view_of CornerValues>
    static tetrahedral_triangulation from_corners(CornerValues&& field_at) 
    { 
        auto triangle_pattern = lookup_from(field_at); // corners
        return { triangle_pattern.size(), triangle_pattern };
    }

    tetrahedral_triangulation() : m_triangle_count{0}, m_triangles{} 
    { }

protected:
    // ASSUMPTION: corners are always in the < order of corner_type
    template<view_of FieldValues>
    static index_type lookup_index(FieldValues&& corner_values)
    { 
        using corner_bitset = std::bitset<total_corners>;

        auto bits = corner_values | predicate | std::ranges::to<corner_bitset>;

        return bits.to_ulong();
    }

    template<view_of CornerValues>
    static std::vector< triangle_type > lookup_from(CornerValues&& field_at)
    {
        using std::ranges::values,
              std::ranges::keys;
        using to_lookup_index = std::bind(&)

        auto archetype = s_lookup_table[field_at | values | to_lookup_index];
        
        return archetype | relative_to( field_at | keys );
    }

    template< view_of Triangles >
    tetrahedral_triangulation( int triangle_count, Triangles&& triangles ) :
        m_triangle_count{ triangle_count }, 
        m_triangles{ triangles | std::ranges::to< triangles_array >() }
    { }

private:
    int m_triangle_count;
    // reserve space for the maximum number
    triangles_array m_triangles;
};

const unsigned int Grid::s_edge_table_corner_bitmask[8] = 
{ 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80 };

/** from here:
 * 
 * File Name: CIsoSurface.cpp
 * Last Modified: 5/8/2000
 * Author: Raghavendra Chandrashekara (based on source code provided
 * by Paul Bourke and Cory Gene Bloyd)
 * Email: rc99@doc.ic.ac.uk, rchandrashekara@hotmail.com
 * 
 */
 
const unsigned int Grid::s_edge_table[256] = {
	0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
	0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
	0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
	0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
	0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
	0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
	0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
	0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
	0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
	0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
	0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
	0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
	0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
	0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
	0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
	0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
	0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
	0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
	0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
	0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
	0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
	0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
	0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
	0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
	0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
	0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
	0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
	0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
	0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
	0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
	0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
	0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0
};

 
const int Grid::s_triangle_table[256][16] = {
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
	{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
	{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
	{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
	{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
	{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
	{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
	{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
	{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
	{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
	{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
	{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
	{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
	{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
	{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
	{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
	{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
	{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
	{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
	{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
	{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
	{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
	{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
	{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
	{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
	{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
	{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
	{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
	{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
	{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
	{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
	{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
	{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
	{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
	{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
	{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
	{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
	{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
	{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
	{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
	{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
	{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
	{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
	{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
	{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
	{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
	{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
	{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
	{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
	{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
	{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
	{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
	{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
	{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
	{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
	{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
	{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
	{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
	{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
	{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
	{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
	{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
	{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
	{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
	{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
	{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
	{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
	{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
	{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
	{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
	{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
	{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
	{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
	{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
	{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
	{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
	{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
	{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
	{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
	{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
	{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
	{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
	{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
	{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
	{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
	{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
	{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
	{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
	{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
	{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
	{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
	{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
	{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
	{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
	{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
	{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
	{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
	{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
	{6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
	{9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
	{1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
	{4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
	{7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
	{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
	{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
	{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
	{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
	{0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
	{11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
	{6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
	{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
	{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
	{1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
	{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
	{10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
	{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
	{5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
	{10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
	{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
	{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
	{7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
	{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
	{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
	{9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
	{9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
	{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
	{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
	{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
	{0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
	{10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
	{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
	{0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
	{0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
	{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
	{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
	{3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
	{5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
	{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
	{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
	{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
	{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
	{3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
	{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
	{9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
	{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
	{11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
	{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
	{9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
	{3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
	{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
	{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
	{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
	{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
	{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
};

