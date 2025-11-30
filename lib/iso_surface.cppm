module;

#include <vector>
#include <functional>
#include <ranges>
#include <array>

export module iso_surface;

export template<typename FloatType = float>
struct Point : 
    public std::ranges::view_interface<Point<FloatType>>
{
    using float_type = FloatType;
    using value_type = float_type;

    float_type& operator[](int i);
    float_type const& operator[](int i) const;
    float_type& at(int i);
    float_type const& at(int i) const;

    struct iterator;
    struct const_iterator;

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;

    float_type x, y, z;
};

export template<typename FloatType = float>
struct Triangle
{
    using point_type = Point<FloatType>;

    point_type points[3];
};

export template<typename FloatType = float>
class Simplex
{
    using triangle_type = Triangle<FloatType>;

    Simplex& operator+=(triangle_type const&);
};

/** Grid class represents a 3D grid of arbitrary size and density 
 * 
*/
export template<typename PointType = Point<>>
class Grid
{
    static constexpr size_t inline dimensions = 3;
    using position_type = std::array<long, dimensions>;

    using point_type = Point<>;
    using edge_type = long;
    struct corner_type;
    struct cell_type;
    struct const_iterator;
    struct corners_view;
    struct cell_view;

    // template<class T> using Set_of = std::vector<T>;
    using triangle_type = Triangle< >;
    using simplex_type = Simplex<triangle_type>;

    const_iterator begin() const;
    const_iterator end() const;

    cell_view cells() const;
    point_type at(position_type) const;

    struct triangulation_range;

    // bool contains_surface( characterized_cell cell ) const;
    triangulation_range triangulate( Grid<PointType>::cell_view&& ) const;
    simplex_type surface() const;

    template<size_t Dim> size_t stride_to() const;
    template<size_t Dim> size_t dimension() const;

    position_type position_begin() const;
    position_type position_end() const;
protected:
    static triangle_type triangle_from(const_iterator, edge_type[3]);
    static size_t table_index(cell_type&& cell);
    static const unsigned int s_edge_table[256];
    static const int s_triangle_table[256][16];
private:
    std::array<size_t, dimensions> m_bounds;
    point_type m_origin;
    point_type m_maximum;
};


/**
 * Implementation 
 */

template<typename FloatType>
Point<FloatType>::float_type& Point<FloatType>::operator[](int i)
{ return at(i); }

template<typename FloatType>
Point<FloatType>::float_type const& Point<FloatType>::operator[](int i) const
{ return at(i); }

template<typename FloatType>
Point<FloatType>::float_type& Point<FloatType>::at(int i)
{
    if(i < 0) 
        return x;

    switch(x) {
    case 0: return x;
    case 1: return y;
    case 2: return z;
    }
    
    return z;
}

template<typename FloatType>
Point<FloatType>::float_type const& Point<FloatType>::at(int i) const
{
    if(i < 0) 
        return x;

    switch(x) {
    case 0: return x;
    case 1: return y;
    case 2: return z;
    }
    
    return z;
}


template<typename FloatType>
struct Point<FloatType>::iterator
{
    friend struct Point;

    float_type& operator*() const
    { return m_point->at(m_pos); }
    iterator& operator++()
    { ++m_pos; return *this; }
    iterator operator++(int)
    { iterator ret = *this; ++m_pos; return ret; }
    iterator& operator+=(int d)
    { m_pos += d; return *this; }
    iterator operator+(int d) const
    { iterator ret; return ret += d; }
    iterator& operator--()
    { --m_pos; return *this; }
    iterator operator--(int)
    { iterator ret = *this; --m_pos; return ret; }
    iterator& operator-=(int d)
    { m_pos -= d; return *this; }
    iterator operator-(int d) const
    { iterator ret; return ret -= d; }
    bool operator==(iterator const& other) const
    { return m_point == other.m_point && m_pos == other.m_pos;  }
    bool operator!=(iterator const& other) const
    { return !(*this == other); }
    bool operator==(std::default_sentinel_t) const
    { return m_pos == 3; }
    bool operator!=(std::default_sentinel_t) const
    { return m_pos != 3; }

    iterator() : m_point{nullptr}, m_pos{0} { }
    iterator(iterator&&) = default;
    iterator(iterator const&) = default;
    iterator& operator=(iterator&&) = default;
    iterator& operator=(iterator const&) = default;
protected:
    iterator(Point * point, int pos = 0) :
        m_point{point}, m_pos{pos}
    { }
private:
    Point * m_point;
    int m_pos;
};

template<typename FloatType>
struct Point<FloatType>::const_iterator
{
    friend struct Point;

    float_type const& operator*() const
    { return m_point->at(m_pos); }
    const_iterator& operator++()
    { ++m_pos; return *this; }
    const_iterator operator++(int)
    { const_iterator ret = *this; ++m_pos; return ret; }
    const_iterator& operator+=(int d)
    { m_pos += d; return *this; }
    const_iterator operator+(int d) const
    { const_iterator ret = *this; return ret += d; }
    const_iterator& operator--()
    { --m_pos; return *this; }
    const_iterator operator--(int)
    { const_iterator ret = *this; --m_pos; return ret; }
    const_iterator& operator-=(int d)
    { m_pos -= d; return *this; }
    const_iterator operator-(int d) const
    { const_iterator ret = *this; return ret -= d; }
    bool operator==(const_iterator const& other) const
    { return m_point == other.m_point && m_pos == other.m_pos;  }
    bool operator!=(const_iterator const& other) const
    { return !(*this == other); }
    bool operator==(std::default_sentinel_t) const
    { return m_pos == 3; }
    bool operator!=(std::default_sentinel_t) const
    { return m_pos != 3; }

    const_iterator() : m_point{nullptr}, m_pos{0} { }
    const_iterator(const_iterator&&) = default;
    const_iterator(const_iterator const&) = default;
    const_iterator& operator=(const_iterator&&) = default;
    const_iterator& operator=(const_iterator const&) = default;
protected:
    const_iterator(Point const* point, int pos = 0) :
        m_point{point}, m_pos{pos}
    { }
private:
    Point const* m_point;
    int m_pos;
};

template<typename FloatType>
Point<FloatType>::iterator Point<FloatType>::begin()
{ return { this, 0 }; }

template<typename FloatType>
Point<FloatType>::iterator Point<FloatType>::end()
{ return { this, 3 }; }

template<typename FloatType>
Point<FloatType>::const_iterator Point<FloatType>::begin() const
{ return { this, 0 }; }

template<typename FloatType>
Point<FloatType>::const_iterator Point<FloatType>::end() const
{ return { this, 3 }; }

template<typename FloatType>
Point<FloatType>::const_iterator Point<FloatType>::cbegin() const
{ return { this, 0 }; }

template<typename FloatType>
Point<FloatType>::const_iterator Point<FloatType>::cend() const
{ return { this, 3 }; }

template<typename PointType>
struct Grid<PointType>::corner_type
{ 
    enum value_type : int
    {
        back_bottom_left   = 0b000,
        back_bottom_right  = 0b001,
        back_top_left      = 0b010,
        back_top_right     = 0b011,
        front_bottom_left  = 0b100,
        front_bottom_right = 0b101,
        front_top_left     = 0b110,
        front_top_right    = 0b111,
        total_corners
    };

    template<size_t Dim> int displacement()
    { return (m_value & (0b1 << Dim)) ? 1 : 0; }

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

    corner_type& operator=(corner_type const&) = default;
    corner_type& operator=(corner_type&&) = default;

    corner_type(value_type value = front_top_right) :
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

template<typename PointType>
struct Grid<PointType>::cell_type
{
    friend struct const_iterator;

    cell_type& operator=(cell_type const&) = default;
    cell_type& operator=(cell_type&&) = default;

    cell_type(position_type position) :
        m_position{position}
    { }
    cell_type() { }
    cell_type(cell_type const&) = default;
    cell_type(cell_type&&) = default;

    template<size_t Dim> long position() const
    {
        if constexpr (Dim >= dimensions)
            return 0;
        
        return m_position[Dim];
    }

private:
    position_type m_position;
};

template<typename PointType>
struct Grid<PointType>::const_iterator 
{
    friend class Grid<PointType>;

    cell_type operator*() const
    { return m_cell; }
    cell_type const* operator->() const
    { return &m_cell; }

    const_iterator& operator++()
    { 
        increment_helper(1);
        return *this;
    }
    const_iterator& operator+=(long amount)
    { 
        increment_helper(amount);
        return *this;
    }
    const_iterator& operator--()
    { 
        increment_helper(-1);
        return *this;
    }
    const_iterator& operator-=(long amount)
    { 
        increment_helper(-amount);
        return *this;
    }

    template<typename IndexSequence>
    void increment_helper(long amount, 
        IndexSequence = std::make_index_sequence<Grid<PointType>::dimensions>{});

    template<size_t Dim, size_t ... Dims>
    void increment_helper(long amount, std::index_sequence<Dim, Dims...>)
    {
        auto stride = m_grid->stride_to<Dim>();
        m_cell.template position<Dim>() += (amount % stride);
        amount /= stride;

        // check for remainder
        if(m_cell.template position<Dim>() >= m_grid->dimension<Dim>())
            ++amount;

        // finish the other dimensions
        increment_helper(amount, std::index_sequence<Dims...>{});
    }

    template<>
    void increment_helper(long amount, std::index_sequence<>)
    { }

    bool is_valid() const
    {
        //TODO: turn this into variadic template
        return m_cell.template position<0>() >= 0 &&
               m_cell.template position<1>() >= 0 &&
               m_cell.template position<2>() >= 0 &&
               (size_t)m_cell.template position<0>() < m_grid->dimension<0>() &&
               (size_t)m_cell.template position<1>() < m_grid->dimension<1>() &&
               (size_t)m_cell.template position<2>() < m_grid->dimension<2>();
    }

    point_type corner(corner_type corner)
    { 
        auto dimensions_sequence = 
            std::make_index_sequence<Grid<PointType>::dimensions>{};
        return corner_helper(corner, dimensions_sequence);
    }

    corners_view corners(Grid<PointType> const* grid)
    { return { grid, *this }; }

    bool operator==(const_iterator const& other) const 
    { return m_cell == other.m_cell; };
    bool operator!=(const_iterator const& other) const 
    { return m_cell != other.m_cell; };
    bool operator==(std::default_sentinel_t) const
    { return !(bool)m_cell; }
    bool operator!=(std::default_sentinel_t) const
    { return (bool)m_cell; }
    bool operator<(const_iterator const& other) const
    { return m_cell < other.m_cell; }
    bool operator<=(const_iterator const& other) const
    { return m_cell <= other.m_cell; };
    bool operator>(const_iterator const& other) const 
    { return m_cell > other.m_cell; };
    bool operator>=(const_iterator const& other) const
    { return m_cell >= other.m_cell; };

    const_iterator(const_iterator&&) = default;
    const_iterator(const_iterator const&) = default;
    const_iterator& operator=(const_iterator&&) = default;
    const_iterator& operator=(const_iterator const&) = default;

protected:
    template<size_t ... Dims>
    point_type corner_helper(corner_type corner, std::index_sequence<Dims...>)
    {
        return m_grid->at({(static_cast<long>(m_grid->dimension<Dims>()) + 
            corner.template displacement<Dims>())... });
    }
    
    const_iterator(Grid<PointType> const* grid, cell_type cell = {}) :
        m_grid{grid}, m_cell{cell}
    { }
private:
    Grid<PointType> const* m_grid;
    cell_type m_cell;
};

template<typename PointType>
struct Grid<PointType>::corners_view : 
    public std::ranges::view_interface<corners_view>
{
    struct const_iterator
    {
        point_type operator*() 
        { return m_cell.corner(m_corner_id); }

        const_iterator& operator++();
        const_iterator& operator+=(int);
        const_iterator& operator--();
        const_iterator& operator-=(int);

        bool operator==(const_iterator const&) const;
        bool operator!=(const_iterator const&) const;
        bool operator==(std::default_sentinel_t) const;
        bool operator!=(std::default_sentinel_t) const;

        const_iterator(cell_type cell) : 
            m_cell{cell}, m_corner_id{0}
        { }
        const_iterator(const_iterator&&) = default;
        const_iterator(const_iterator const&) = default;
        const_iterator& operator=(const_iterator&&) = default;
        const_iterator& operator=(const_iterator const&) = default;

        cell_type m_cell;
        int m_corner_id;
    };

    const_iterator begin() { return m_begin; }
    std::default_sentinel_t end() { return {}; }

    corners_view(cell_type cell) : 
        m_begin{cell}
    { }

private:
    const_iterator m_begin;
};

template<typename PointType>
struct Grid<PointType>::cell_view : 
    public std::ranges::view_interface<cell_view>
{
    const_iterator begin()  { return m_begin; }
    const_iterator end()    { return m_end; }

    const_iterator m_begin, m_end;
    cell_view(const_iterator b, const_iterator e) :
        m_begin{b}, m_end{e}
    { }
};

template<typename PointType>
Grid<PointType>::cell_view Grid<PointType>::cells() const
{ return { begin(), end() }; }

template<typename PointType>
Grid<PointType>::const_iterator Grid<PointType>::begin() const
{ return {this, position_begin()}; }

template<typename PointType>
Grid<PointType>::const_iterator Grid<PointType>::end() const
{ return {this, position_end()}; }

// template<typename PointType>
// bool Grid<PointType>::contains_surface( 
//     typename Grid<PointType>::characterized_cell cell ) const
// {
//     /* TODO: write contains_surface logic from rchandra CIsoSurface.cpp */
//     return cell.contains_surface();
// }

template<typename PointType>
Grid<PointType>::triangle_type 
Grid<PointType>::triangle_from(Grid<PointType>::const_iterator, 
    Grid<PointType>::edge_type edges[3])
{
    //TODO: see GetEdgeID and GenerateSurface functions in CIsoSurface.cpp
}

template<typename PointType>
struct Grid<PointType>::triangulation_range :
    public std::ranges::view_interface<Grid<PointType>::triangulation_range>
{
    struct const_iterator
    {
        triangle_type operator*() const
        { 
            return triangle_from(m_current_cell, {
                s_triangle_table[m_table_index][m_triangle_index],
                s_triangle_table[m_table_index][m_triangle_index + 1],
                s_triangle_table[m_table_index][m_triangle_index + 2]
            });
        }
        const_iterator& operator++()
        {
            if(!m_current_cell.is_valid())
                return *this;
            
            m_triangle_index += 3;
            increment_until_valid_or_end();
            return *this;
        }
        bool operator==(const_iterator const& other) const
        { 
            return m_current_cell == other.m_current_cell &&
                   m_table_index == other.m_table_index &&
                   m_triangle_index == other.m_triangle_index;
        }
        bool operator!=(const_iterator const& other) const
        { return !(*this == other); }

        const_iterator(cell_view::const_iterator&& current_cell) :
            m_current_cell{current_cell}, 
            m_table_index{table_index(*m_current_cell)}, 
            m_triangle_index{0}
        { increment_until_valid_or_end(); }
    protected:
        void next_cell()
        {
            ++m_current_cell;
            m_table_index = table_index(*m_current_cell);
            m_triangle_index = 0;
        }

        void increment_until_valid_or_end()
        {
            while(m_current_cell.is_valid() && 
                  s_triangle_table[m_table_index][m_triangle_index] == -1)
                next_cell();
        }
    private:
        cell_view::const_iterator m_current_cell;
        size_t m_table_index;
        size_t m_triangle_index;
    };

    const_iterator begin() const { return { m_cells.begin() }; }
    std::default_sentinel_t end() const { return {}; }

    triangulation_range& operator=(triangulation_range&&) = default;
    triangulation_range& operator=(triangulation_range const&) = default;
    triangulation_range( cell_view&& cells ) : m_cells{cells} { }
    triangulation_range() : m_cells{} { }
    triangulation_range(triangulation_range&&) = default;
    triangulation_range(triangulation_range const&) = default;
private:
    cell_view m_cells;
};

template<typename PointType>
Grid<PointType>::triangulation_range 
Grid<PointType>::triangulate( Grid<PointType>::cell_view&& cell_range ) const
{ return { cell_range }; }

template<typename PointType>
typename Grid<PointType>::simplex_type 
Grid<PointType>::surface() const 
{   
    auto triangulate = std::bind(&Grid::triangulate, this);

    return { cells() | triangulate };
}

template<typename PointType>
template<size_t Dim> 
size_t Grid<PointType>::stride_to() const
{ 
    if constexpr (Dim >= dimensions - 1)
        return 1;

    return m_bounds[Dim] * stride_of<Dim+1>();
}

template<typename PointType>
template<size_t Dim> 
size_t Grid<PointType>::dimension() const
{ return m_bounds[Dim]; }

template<typename PointType>
Grid<PointType>::position_type Grid<PointType>::position_begin() const
{ return { 0, 0, 0 }; }

template<typename PointType>
Grid<PointType>::position_type Grid<PointType>::position_end() const
{ return { m_bounds[0], m_bounds[1], m_bounds[2] }; }



/** from here:
 * 
 * File Name: CIsoSurface.cpp
 * Last Modified: 5/8/2000
 * Author: Raghavendra Chandrashekara (based on source code provided
 * by Paul Bourke and Cory Gene Bloyd)
 * Email: rc99@doc.ic.ac.uk, rchandrashekara@hotmail.com
 * 
 */
template<typename PointType> 
const unsigned int Grid<PointType>::s_edge_table[256] = {
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

template<typename PointType> 
const int Grid<PointType>::s_triangle_table[256][16] = {
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

