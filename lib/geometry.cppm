module;

#include <bitset>
#include <array>
#include <tuple>
#include <ranges>
#include <numeric>

#include "disjoint_intervals.hpp"

export module geometry;

export class Grid
{ 
public:
    enum axis_type : int 
    {
        no_axis = 0b00,
        x_axis = 0b01,
        y_axis = 0b10,
        total_axis = 2
    };

    using unit_type = std::array<int, total_axis>;

    static constexpr unit_type x_unit = { 1, 0 };
    static constexpr unit_type y_unit = { 0, 1 };

    struct dimension_type 
    { 
        unit_type axis;
        double value;
    };

    struct cell
    {
    };

    struct index_type
    {
        bool operator==(index_type const& other) const
        { return index == other.index; }
        bool operator<(index_type const& other) const
        { return index < other.index; }
        index_type& operator++()
        { ++index; return *this; }

        long long x() const
        {
            unsigned long long t = index;
            bool negative = (t & 0b01);
            t >>= 2; // strip away the negative signifiers

            long long ret = 0;
            while(t > 0)
            {
                ret |= (t & 0b01);
                ret <<= 1;
                t >>= 2;
            }

            if(negative)
                return -ret;

            return ret;
        }
        long long y() const
        {
            unsigned long long t = index;
            bool negative = (t & 0b10);
            t >>= 3; // strip away the negative signifiers and the MSB of x

            long long ret = 0;
            while(t > 0)
            {
                ret |= (t & 0b01);
                ret <<= 1;
                t >>= 2;
            }

            if(negative)
                return -ret;

            return ret;
        }

        unsigned long long index;
    };


    struct cell_range
    {
        

        disjoint_intervals< index_type > intervals;
    };

    template< typename Predicate >
    cell_range select( Predicate&& pred )
    {
        using number_type = unsigned long long;
        using vector_type = std::vector< number_type >;
        using std::for_each;

        vector_type range(m_maximum.index);

        for_each( range.begin(), range.end(), 
        [&pred, &range]( number_type& value ) 
        { 
            // get the index of this cell
            number_type index = std::distance(&range[0], &value);
            index_type i = { index };
            // is this in the shape?
            value = pred(i.x(), i.y()) ? 1 : 0;
        });

        // prefix sum
        // find all the ranges and insert them into cell_range
    }

private:
    index_type m_maximum;
};

export Grid::dimension_type operator""_wide(unsigned long long value)
{ return { Grid::x_unit, (double)value }; }

export Grid::dimension_type operator""_high(unsigned long long value)
{ return { Grid::y_unit, (double)value }; }

export Grid::unit_type operator*(Grid::unit_type a, Grid::unit_type b)
{
    for(size_t i = 0; i < Grid::total_axis; ++i)
        a[i] += b[i];
    return a;
}

export Grid::dimension_type operator*(Grid::dimension_type const& a, Grid::dimension_type const& b)
{ return { a.axis * b.axis, a.value * b.value }; }


template<typename FloatType = double>
struct point2
{
    using value_type = FloatType;

    constexpr double& operator[](int i)
    { return i <= 0 ? x : y; }
    constexpr double operator[](int i) const
    { return i <= 0 ? x : y; }

    constexpr point2& operator=(point2 const&) = default;
    constexpr point2& operator=(point2&&) = default;
    constexpr point2() : x{0}, y{0} { }
    constexpr point2(value_type ax, value_type ay) : x{ax}, y{ay} { }
    constexpr point2(point2 const&) = default;
    constexpr point2(point2&&) = default;

    value_type x, y;
};

template<typename PointType = point2<>>
struct line
{
    using point_type = PointType;

    point_type p0, p1;
};

using point_type = point2<>;

enum direction_type : int
{
    no_direction     = -1,
    horizontal       = 0b0,
    vertical         = 0b1,
    total_directions = 2,
};

enum corner_type : int
{
    no_corner     = -1,
    bottom_left   = 0b00,
    bottom_right  = 0b01,
    top_left      = 0b10,
    top_right     = 0b11,
    total_corners = 4,
};

static constexpr point_type s_point_from[/* corner_type */]
{
    /* bottom_left  */ { 0., 0. },
    /* bottom_right */ { 1., 0. },
    /* top_left     */ { 0., 1. },
    /* top_right    */ { 1., 1. },
};

static constexpr point_type to_point(corner_type corner)
{
    if(corner == no_corner)
        return { 0., 0. };
    
    return s_point_from[(int)corner];
}

enum edge_type : int
{
    no_edge     = -1,
    bottom      = 0b00,
    top         = 0b01,
    left        = 0b10,
    right       = 0b11,
    total_edges = 4,
};

static constexpr std::pair<corner_type, corner_type> 
s_edge_corners[/* edge_type */] =
{
    // NOTE: should these be in numerical order or in CW or CCW order?
    { bottom_left, bottom_right },
    { top_left, top_right },
    { bottom_left, top_left },
    { bottom_right, top_right },
};

static constexpr std::pair<corner_type, corner_type> corners_of( edge_type edge )
{ 
    if( edge == no_edge )
        return { no_corner, no_corner };
    
    return s_edge_corners[(int)edge];
}

static constexpr direction_type s_edge_direction[] = 
{
    /* bottom */ horizontal,
    /* top    */ horizontal,
    /* left   */ vertical, 
    /* right  */ vertical,
};

static constexpr point_type interpolate(edge_type const& edge, double s)
{
    if(edge == no_edge)
        return { };

    direction_type direction = s_edge_direction[(int)edge];
    if(direction == horizontal)
        return { s, edge == top ? 1. : 0. };
    
    return { edge == right ? 1. : 0., s };
}

struct corner_classification
{

};


static constexpr corner_type s_corners[] = 
{ bottom_left, bottom_right, top_left, top_right };

struct corners_view : public std::ranges::view_interface<corners_view>
{
    constexpr auto begin() const
    { return m_begin; }
    constexpr auto end() const
    { return m_begin + m_count; }

    corners_view( corner_type const* begin = s_corners, 
        size_t count = total_corners ) :
        m_begin{begin}, m_count{count}
    { }

private:
    corner_type const* m_begin;
    size_t m_count;
};

static constexpr corners_view corners()
{ return {}; }

static constexpr edge_type s_edges[] =
{ bottom, top, left, right };

struct edges_view : public std::ranges::view_interface<edges_view>
{
    constexpr auto begin() const
    { return m_begin; }
    constexpr auto end() const
    { return m_begin + m_count; }

    edges_view( edge_type const* begin = s_edges, size_t count = total_edges ) :
        m_begin{begin}, m_count{count}
    { }
private:
    edge_type const* m_begin;
    size_t m_count;
};

static constexpr edges_view edges()
{ return {}; }

struct facet_edges : public std::pair<edge_type, edge_type>
{
    constexpr edge_type operator[](int i) const
    { return i <= 0 ? first : second; }

    constexpr facet_edges(edge_type edge0, edge_type edge1) :
        std::pair<edge_type, edge_type>(edge0, edge1)
    { }
    constexpr facet_edges(facet_edges const& other) :
        std::pair<edge_type, edge_type>{ other }
    { }
    constexpr facet_edges() { }
};

struct facet_pattern
{
    using value_type = facet_edges;
    using const_iterator = facet_edges const *;
private:
    int m_facet_count;
    facet_edges m_facet_edges[2];
public:

    constexpr facet_pattern& operator=(facet_pattern const&) = default;
    // constexpr facet_pattern& operator=(facet_pattern&&) = default;

    constexpr facet_pattern() : m_facet_count{0}, 
        m_facet_edges{{no_edge, no_edge}, {no_edge, no_edge}} 
    { }
    constexpr facet_pattern(facet_pattern const& other) :
        m_facet_count{other.m_facet_count}
    { 
        m_facet_edges[0] = other.m_facet_edges[0];
        m_facet_edges[1] = other.m_facet_edges[1];
    }
    // constexpr facet_pattern(facet_pattern&&) = default;
    constexpr facet_pattern(facet_edges const& edges) : 
        m_facet_count{1}, m_facet_edges{edges, {no_edge, no_edge}}
    { }
    constexpr facet_pattern(facet_edges const& edges0, 
        facet_edges const& edges1) : m_facet_count{2},
        m_facet_edges{edges0, edges1}
    { }

    constexpr int size() const 
    { return m_facet_count; }

    constexpr const_iterator begin() const
    { return &m_facet_edges[0]; }
    constexpr const_iterator end() const
    { return begin() + size(); }

    constexpr ~facet_pattern()
    { }


};

struct corner_values : std::array<double, total_corners>
{
    double constexpr operator[](corner_type corner) const
    { return std::array<double, total_corners>::operator[]((size_t)corner); }

    template<typename ValueView>
    constexpr corner_values(ValueView&& values) : 
        std::array<double, total_corners>{values}
    { }
};

struct edge_field_values : public std::array<double, total_edges>
{
    using base_type = std::array<double, total_edges>;

    double& operator[](edge_type edge)
    { return base_type::operator[]((size_t)edge); }
    double operator[](edge_type edge) const
    { return base_type::operator[]((size_t)edge); }
};

static constexpr edge_field_values interpolate( corner_values const& field_at )
{
    using std::views::transform, std::ranges::to;
    using std::abs;

    // assume the side length is 1 and find the distance along the edge where 
    // we'd expect the field value to be zero
    // f(t) = v0 + t * (v1 - v0) = 0
    //             t * (v1 - v0) = -v0
    //    t = - v0 / (v1 - v0)
    auto interpolate_between = [&field_at]( auto const& corners )
    {
        auto [ c0, c1 ] = corners;
        // TODO: check this math...
        return field_at[c0] / ( field_at[c0] - field_at[c1] );
    };

    edge_field_values ret;
    for(auto edge : edges())
        ret[edge] = interpolate_between( corners_of( edge ));
    
    return ret;
}

struct facet_view : public std::ranges::view_interface<facet_view>
{
    using value_type = line<>;

    struct const_iterator 
    {
        constexpr value_type operator*() const
        { 
            auto edges = *m_facet_iterator;
            return { 
                interpolate(edges[0], m_edge_values[edges[0]]),
                interpolate(edges[1], m_edge_values[edges[1]]) };
        }
        constexpr const_iterator& operator++()
        { ++m_facet_iterator; return *this; }
    private:
        facet_pattern::const_iterator m_facet_iterator;
        edge_field_values m_edge_values;     
    };

    facet_pattern m_facets;
    edge_field_values m_edge_values;
};

template<class Field>
struct field_value
{
    using point_type = point2< >;

    constexpr auto operator()(point_type const& point) const
    { return m_field(point.x, point.y); }

    field_value(Field const& field) : m_field{field} { }
private:
    Field const& m_field;
};

template<class RelativeDiscreteField>
static facet_view facets(RelativeDiscreteField const& field)
{
    using std::views::transform;

    corner_values field_at = corners() | transform( to_point ) | 
        transform( field_value{field} );

    
}

static constexpr facet_pattern __BLAH__ = {};

static constexpr facet_pattern s_facet_patterns_by[/* corner_bitset */] = 
{
    /* 0b0000 - 0 */ { },
    /* 0b0001 - ⤦ */ { { left, bottom } },
    /* 0b0010 - ⤥ */ { { bottom, right } },
    /* 0b0011 - ⤩ */ { { left, bottom }, { bottom, right } },
    /* 0b0100 - ⤣ */ { { top, left } },
    /* 0b0101 - ⤡ */ { { bottom, right }, { top, left } },
    /* 0b0110 - ⤪ */ { { left, bottom }, { top, left } },
    /* 0b0111 - ? */ { { top, right }, },
    /* 0b1000 - ⤤ */ { { right, top }, },
    /* 0b1001 - ⤢ */ { { left, bottom }, { right, top } },
    /* 0b1010 - ⤨ */ { { bottom, right }, { right, top } },
    /* 0b1011 - ⤰ */ { { left, top } },
    /* 0b1100 - ⤧ */ { { top, left }, { right, top } },
    /* 0b1101 - ? */ { { bottom, right } },
    /* 0b1110 - ⤯ */ { { bottom, left } }, 
    /* 0b1111 - ⤫ */ { }
};
