module;

#include <stdexcept>
#include <numeric>
#include <cmath>
#include <tuple>
#include <ranges>
#include <functional>
#include <iostream>

#ifdef PARALLEL
#include <execution>
#define PAR_UNSEQ std::execution::par_unseq,
#else
#define PAR_UNSEQ 
#endif

export module linalg;

template<class T>
concept point3 = requires(T a)
{
    a.x;
    a.y;
    a.z;
};



template<typename T = void>
struct square {
    T operator()(T const& a) const
    { return a * a; }
};

template<>
struct square<void> {
    auto operator()(auto const & a) const
    { return a * a; }
};


export template<typename PointType>
struct point_traits {
    using point_type = PointType;
    using value_type = PointType::value_type;

    static constexpr size_t dimension() 
    { return std::tuple_size<point_type>{}; }
    static constexpr point_type origin()
    { return {}; }

    static auto values_begin(point_type const& v)
    { return v.begin(); }
    static auto values_end(point_type const& v)
    { return v.end(); }
    static auto values_begin(point_type & v)
    { return v.begin(); }
    static auto values_end(point_type & v)
    { return v.end(); }

    struct reference_view : public std::ranges::view_interface<reference_view>
    {
        auto begin() const { return values_begin(*m_point); }
        auto end() const   { return values_end(*m_point);   }
        reference_view(point_type & point) : m_point{&point} { }
        point_type * m_point;
    };

    struct const_reference_view : 
        public std::ranges::view_interface<const_reference_view>
    {
        auto begin() const { return values_begin(*m_point); }
        auto end() const   { return values_end(*m_point);   }
        const_reference_view(point_type const& point) : m_point{&point} { }
        point_type const * m_point;
    };
    
};

export template<typename VectorType>
struct vector_traits {
    static constexpr size_t dimensions() 
    { return std::tuple_size<VectorType>{}; }
    using value_type = VectorType::value_type;

    value_type length2(VectorType v)
    {
        return std::transform_reduce(PAR_UNSEQ 
            values_begin(v), values_end(v), value_type{0},
            /* reduce */ std::plus<>{},
            /* transform */ square<>{});
    }
    value_type length(VectorType v)
    { return std::sqrt(length2(v)); }

    static auto values_begin(VectorType const& v) 
    { return v.begin(); }

    static auto values_end(VectorType const& v) 
    { return v.end(); }

    struct reference_view : public std::ranges::view_interface<reference_view>
    {
        auto begin() const { return values_begin(*m_vector); }
        auto end()   const { return values_end(*m_vector); }
        reference_view(VectorType const& vector) : m_vector{&vector} { }

        VectorType const * m_vector;
    };
};

export template<class Float = float>
class Point
{
public:
    using value_type = Float;

    
    Float x, y, z;

    constexpr Float & operator[](int i) 
    {
        switch(i) {
        case 0: return x;
        case 1: return y;
        case 2: return z;
        }

        throw std::out_of_range("index to Point must be 0, 1 or 2");
    }

    constexpr Float const & operator[](int i) const
    {
        switch(i) {
        case 0: return x;
        case 1: return y;
        case 2: return z;
        }

        throw std::out_of_range("index to Point must be 0, 1 or 2");
    }

    Point() : x{0}, y{0}, z{0} { }
    constexpr Point(value_type x, value_type y, value_type z) : 
        x{x}, y{y}, z{z} { }
    Point(Point<value_type>&&) = default;
    Point(Point<value_type> const&) = default;
    Point& operator=(Point<value_type>&&) = default;
    Point& operator=(Point<value_type> const&) = default;
};



template<typename Float> 
struct point_traits<Point<Float>> {
    static constexpr size_t dimension() 
    { return 3; }
    using value_type = Float;
    using point_type = Point<Float>;

    static constexpr point_type origin()
    { return {}; }

    static auto values_begin(point_type const& v)
    { return &v.x; }
    static auto values_end(point_type const& v)
    { return &v.x + dimension(); }
    static auto values_begin(point_type & v)
    { return &v.x; }
    static auto values_end(point_type & v)
    { return &v.x + dimension(); }

    struct reference_view : public std::ranges::view_interface<reference_view>
    {
        auto begin() const { return values_begin(*m_point); }
        auto end() const   { return values_end(*m_point);   }
        reference_view(point_type & point) : m_point{&point} { }
        point_type * m_point;
    };

    struct const_reference_view : 
        public std::ranges::view_interface<const_reference_view>
    {
        auto begin() const { return values_begin(*m_point); }
        auto end() const   { return values_end(*m_point);   }
        const_reference_view(point_type const& point) : m_point{&point} { }
        point_type const * m_point;
    };  
};

export template<class Float = float>
class Vector : public Point<Float> { 
public:
    Vector(Float x, Float y, Float z) : Point<Float>{x, y, z} { }
};


export template<typename Float>
struct vector_traits<Vector<Float>> {
    static constexpr size_t dimensions() 
    { return 3; }
    using value_type = Float;

    value_type length2(Vector<Float> v)
    {
        return std::transform_reduce(PAR_UNSEQ 
            values_begin(v), values_end(v), value_type{0},
            /* reduce */ std::plus<>{},
            /* transform */ square<>{});
    }
    value_type length(Vector<Float> v)
    { return std::sqrt(length2(v)); }

    static auto values_begin(Vector<Float> const& v) 
    { return &v.x; }

    static auto values_end(Vector<Float> const& v) 
    { return &v.x + 3; }

    struct reference_view : public std::ranges::view_interface<reference_view>
    {
        auto begin() const { return values_begin(*m_vector); }
        auto end()   const { return values_end(*m_vector); }
        reference_view(Vector<Float> const& vector) : m_vector{&vector} { }

        Vector<Float> const * m_vector;
    };
};


export template<class Float>
Vector<Float> operator+(Point<Float> const & a, Point<Float> const & b)
{
    return Vector<Float>{a.x + b.x, a.y + b.y, a.z + b.z};
}
export template<class Float>
Vector<Float> operator-(Point<Float> const & a, Point<Float> const & b)
{
    return Vector<Float>{a.x - b.x, a.y - b.y, a.z - b.z};
}
export template<class Float>
Vector<Float> operator/(Point<Float> const & a, Float const & s)
{
    return Vector<Float>{a.x / s, a.y / s, a.z / s};
}
export template<class Float>
Vector<Float> operator*(Point<Float> const & a, Float const & s)
{
    return Vector<Float>{a.x * s, a.y * s, a.z * s};
}
export template<class Float>
Vector<Float> operator*(Float const & s, Point<Float> const & a)
{
    return Vector<Float>{a.x * s, a.y * s, a.z * s};
}

export template<class Float>
Vector<Float> cross(Vector<Float> const & a, Vector<Float> const & b)
{
    return Vector<Float>{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

export template<class Float>
Float length(Vector<Float> const & a)
{
    return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

export template<class Float>
Float length(Point<Float> const & a)
{
    return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

export template<class Float>
Vector<Float> normalize(Vector<Float> const & a)
{
    return a / length(a);
}


export template< typename PointType = Point<> >
struct Triangle
{
    using point_type = PointType;

    point_type points[3];
};

export template< typename TriangleType = Triangle<> >
class Simplex
{
    using triangle_type = TriangleType;

    Simplex& operator+=(triangle_type const&);
};





