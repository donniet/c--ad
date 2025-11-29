module;

#include <vector>
#include <ranges>

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

    bool contains_surface( cell_type cell ) const;
    Set_of< triangle_type > triangulate( cell_type cell ) const;
    simplex_type surface() const;
};

template<typename PointType>
struct Grid<PointType>::corner_type
{ 
    enum value_type : int
    {
        front_top_right    = 0b000,
        front_top_left     = 0b001,
        front_bottom_right = 0b010,
        front_bottom_left  = 0b011,
        back_top_right     = 0b100,
        back_top_left      = 0b101,
        back_bottom_right  = 0b110,
        back_bottom_left   = 0b111,
        total_corners
    };

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
    /* key points */
    point_type front_top_right();
    point_type front_top_left();
    point_type front_bottom_right();
    point_type front_bottom_left();
    point_type back_top_right();
    point_type back_top_left();
    point_type back_bottom_right();
    point_type back_bottom_left();
    point_type center();

    point_type corner(corner_type corner)
    {
        switch(corner) {
        case corner_type::front_top_right:
            return front_top_right();
        case corner_type::front_top_left:
            return front_top_left();
        case corner_type::front_bottom_right:
            return front_bottom_right();
        case corner_type::front_bottom_left:
            return front_bottom_left();
        case corner_type::front_top_right:
            return front_top_right();
        case corner_type::front_top_left:
            return front_top_left();
        case corner_type::front_bottom_right:
            return front_bottom_right();
        case corner_type::front_bottom_left:
            return front_bottom_left();
        }

        return front_top_right();
    }

    corners_view corners();
};

template<typename PointType>
struct Grid<PointType>::const_iterator 
{
    cell_type operator*();
    cell_type* operator->();

    const_iterator& operator++();
    const_iterator& operator+=(int);
    const_iterator& operator--();
    const_iterator& operator-=(int);

    bool operator==(const_iterator const&) const;
    bool operator!=(const_iterator const&) const;
    bool operator<(const_iterator const&) const;
    bool operator<=(const_iterator const&) const;
    bool operator>(const_iterator const&) const;
    bool operator>=(const_iterator const&) const;
private:
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
Grid<PointType>::corners_view Grid<PointType>::cell_type::corners()
{ return { *this }; }

template<typename PointType>
Grid<PointType>::cell_view Grid<PointType>::cells() const
{ return { begin(), end() }; }

template<typename PointType>
Grid<PointType>::const_iterator Grid<PointType>::begin() const
{ }

template<typename PointType>
Grid<PointType>::const_iterator Grid<PointType>::end() const
{ }

template<typename PointType>
bool Grid<PointType>::contains_surface( 
    typename Grid<PointType>::cell_type cell ) const
{

}

template<typename PointType>
typename Grid<PointType>::template Set_of< 
    typename Grid<PointType>::triangle_type > 
Grid<PointType>::triangulate( typename Grid<PointType>::cell_type cell ) const
{

}

template<typename PointType>
typename Grid<PointType>::simplex_type 
Grid<PointType>::surface() const 
{   
    auto filter_if = std::views::filter; 
    auto transform_by = std::views::transform;
    auto contains_surface = [&](cell_type cell) 
        { return Grid::contains_surface(cell); };
    auto triangulate = [&](cell_type cell) 
        { return Grid::triangulate(cell); };

    return { cells() | 
                filter_if( contains_surface ) | 
                transform_by( triangulate ) };
}
