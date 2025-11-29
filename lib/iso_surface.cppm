module;

#include <vector>
#include <functional>
#include <ranges>
#include <array>

export module iso_surface;

template<typename FloatType = float>
union Point
{
    using float_type = FloatType;

    struct {
        float_type x, y, z;
    } p;

    float_type s[3];


};

template<typename FloatType = float>
struct Triangle
{
    using point_type = Point<FloatType>;

    point_type points[3];
};

template<typename FloatType = float>
class Simplex
{
    using triangle_type = Triangle<FloatType>;

    Simplex& operator+=(triangle_type const&);
};

/** Grid class represents a 3D grid of arbitrary size and density 
 * 
*/
template<typename PointType = Point<>>
class Grid
{
    static constexpr size_t inline dimensions = 3;
    using position_type = std::array<long, dimensions>;

    using point_type = Point<>;
    struct corner_type;
    struct cell_type;
    struct const_iterator;
    struct corners_view;
    struct cell_view;

    template<class T> using Set_of = std::vector<T>;
    using triangle_type = Triangle< >;
    using simplex_type = Simplex<triangle_type>;

    const_iterator begin() const;
    const_iterator end() const;

    cell_view cells() const;
    point_type at(position_type) const;

    bool contains_surface( cell_type cell ) const;
    Set_of< triangle_type > triangulate( cell_type cell ) const;
    simplex_type surface() const;

    template<size_t Dim> size_t stride_to() const;
    template<size_t Dim> size_t dimension() const;

    position_type position_begin() const;
    position_type position_end() const;
private:
    std::array<size_t, dimensions> m_bounds;
    point_type m_origin;
    point_type m_maximum;
};

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

template<typename PointType>
bool Grid<PointType>::contains_surface( 
    typename Grid<PointType>::cell_type cell ) const
{
    /* TODO: write contains_surface logic from rchandra CIsoSurface.cpp */
}

template<typename PointType>
typename Grid<PointType>::template Set_of< 
    typename Grid<PointType>::triangle_type > 
Grid<PointType>::triangulate( typename Grid<PointType>::cell_type cell ) const
{
    /* TODO: write triangulate logic from rchandra CIsoSurface.cpp */
}

template<typename PointType>
typename Grid<PointType>::simplex_type 
Grid<PointType>::surface() const 
{   
    auto filter_if = std::views::filter; 
    auto transform_by = std::views::transform;
    auto contains_surface = std::bind(&Grid::contains_surface, this);
    auto triangulate = std::bind(&Grid::triangulate, this);

    return { cells() | 
             filter_if( contains_surface ) | 
             transform_by( triangulate ) };
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

