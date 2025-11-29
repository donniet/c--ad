
#include <stdexcept>
#include <array>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <concepts>
#include <utility>
#include <vector>
#include <cstdint>

#ifdef PARALLEL
#include <execution>
#define PAR_UNSEQ std::execution::par_unseq,
#else
#define PAR_UNSEQ 
#endif

import volume;
import linalg;

class Nothing { };


template<class Point>
class Triangle
{
public:
    using point_type = Point;

    point_type points[3];

    friend class iterator;

    class const_iterator 
    {
        friend class Triangle;
    private:
        Triangle * _p;
        int _i;
    
        const_iterator(Triangle * p) : _p(p), _i(0) { }
        const_iterator() : _p(nullptr), _i(0) { }
    public:
        using value_type = point_type;

        const_iterator(const_iterator const &) = default;
        const_iterator(const_iterator &&) = default;
        const_iterator & operator=(const_iterator const &) = default;
        const_iterator & operator=(const_iterator &&) = default;

        const_iterator & operator++() { ++_i; return *this; }
        Point const & operator*() { return _p->points[_i]; }
        Point const * operator->() { return &_p->points[_i]; }
        bool operator==(const_iterator const & other) const
        {
            if(_p == nullptr && other._p == nullptr)
                return true;
            if(_p == nullptr && other._i >= 3)
                return true;
            if(other._p == nullptr && _i >= 3)
                return true;
            
            return _p == other._p && _i == other._i;
        }
        bool operator!=(const_iterator const & other) const
        {
            return !(*this == other);
        }
    };

    const_iterator begin() const { return const_iterator{this}; }
    const_iterator end() const { return const_iterator{}; }

    point_type & operator[](int i) { return points[i]; }
    point_type const & operator[](int i) const{ return points[i]; }

};



template<size_t Vertices, class Index = int>
class TriangleFan;

template<size_t Vertices, class Index>
class FaceIndices
{
public:
    Index vertex_offset[Vertices];

    constexpr FaceIndices(std::initializer_list<Index> indices)
    { std::copy(indices.begin(), indices.end(), vertex_offset); }

    constexpr Index const & operator[](size_t i) const { return vertex_offset[i]; }
};

template<size_t Vertices, class Index>
class TriangleFan : public FaceIndices<Vertices, Index>
{
public:
    constexpr TriangleFan(std::initializer_list<Index> indices) : 
        FaceIndices<Vertices, Index>(indices) { }
};


template<class T>
concept simplex = requires(T a)
{
    typename T::triangle_type;

    (*a.begin())[0];
    (*a.begin())[1];
    (*a.begin())[2];

    (*a.begin())[0].x;
    (*a.begin())[0].y;
    (*a.begin())[0].z;

    a.end() == a.begin() || a.end() != a.begin();
};

template<class T>
concept transformation = requires(T a)
{
    typename T::point_type;

    { a(typename T::point_type{}) } -> std::convertible_to<typename T::point_type>;
};

template<class T, class U, size_t ... Is>
constexpr void copy_indices(T & t, U const & u, std::index_sequence<Is...>)
{
    ((t[Is] = u[Is]), ...);
}


template<class Float = float>
class Mat4
{
public:
    using value_type = Float;
    using point_type = Point<Float>;
private:
    value_type _m[12];

    static constexpr size_t c[][3] = {
        {  0,  4,  8 },
        {  1,  5,  9 },
        {  2,  6, 10 },
        {  3,  7, 11 },
    };

public:
    static constexpr Mat4 identity() 
    {
        return Mat4(
            1., 0., 0., 0.,
            0., 1., 0., 0.,            
            0., 0., 1., 0.
        );
    }

    constexpr Mat4() { *this = identity(); }
    // constexpr Mat4(std::initializer_list<value_type> const & values)
    // {
    //     copy_indices(_m, values, std::make_index_sequence<12>{});
    // }
    constexpr Mat4(
        value_type m00, value_type m10, value_type m20, value_type m30,
        value_type m01, value_type m11, value_type m21, value_type m31,
        value_type m02, value_type m12, value_type m22, value_type m32) :
        _m{m00, m10, m20, m30, m01, m11, m21, m31, m02, m12, m22, m32}
    { }
    constexpr Mat4(Mat4 const & other)
    {
        copy_indices(_m, other._m, std::make_index_sequence<12>{});
    }
    constexpr Mat4(Mat4 && other) = delete;
    constexpr Mat4 & operator=(Mat4 const & other)
    {
        if(this != &other)
            copy_indices(_m, other._m, std::make_index_sequence<12>{});
        return *this;
    }

    Point<Float> operator()(Point<Float> const & p) const
    {
        return Point<Float>{
            p.x * _m[c[0][0]] + p.y * _m[c[1][0]] + p.z * _m[c[2][0]] + _m[c[3][0]],
            p.x * _m[c[0][1]] + p.y * _m[c[1][1]] + p.z * _m[c[2][1]] + _m[c[3][1]],
            p.x * _m[c[0][2]] + p.y * _m[c[1][2]] + p.z * _m[c[2][2]] + _m[c[3][2]]
        };
    }
    Vector<Float> operator()(Vector<Float> const & p) const
    {
        return Vector<Float>{
            p.x * _m[c[0][0]] + p.y * _m[c[1][0]] + p.z * _m[c[2][0]],
            p.x * _m[c[0][2]] + p.y * _m[c[1][2]] + p.z * _m[c[2][2]],
            p.x * _m[c[0][1]] + p.y * _m[c[1][1]] + p.z * _m[c[2][1]]
        };
    }
};



template<typename Float = float, typename Index = int>
class Cube
{
public:
    using point_type = Point<Float>;
    using face_type = TriangleFan<4, Index>;
    using float_type = Float;
    using index_type = Index;
    using triangle_type = Triangle<point_type>;

    enum vertex_reference {
        center = 0,
        right_front_top,
        right_front_bottom,
        right_back_top,
        right_back_bottom,
        left_front_top,
        left_front_bottom,
        left_back_top,
        left_back_bottom,
        total_points,
    };

    static constexpr point_type vertices[] = {
        {  0,  0,  0 }, // center of mass
        {  1,  1,  1 }, 
        {  1,  1, -1 },
        {  1, -1,  1 },
        {  1, -1, -1 },
        { -1,  1,  1 },
        { -1,  1, -1 },
        { -1, -1,  1 },
        { -1, -1, -1 },
    };

    enum face_reference {
        right = 0,
        front,
        top,
        left, 
        back,
        bottom,
        total_faces,
    };

    static constexpr face_type faces[] = {
        face_type{ right_front_bottom, right_back_bottom, right_back_top, right_front_top }, // right
        face_type{ right_front_top, left_front_top, left_front_bottom, right_front_bottom }, // front
        face_type{ right_front_top, right_back_top, left_back_top, left_front_top         }, // top
        face_type{ left_back_top, left_back_bottom, left_front_bottom, left_front_top     }, // left
        face_type{ right_back_top, right_back_bottom, left_back_bottom, left_back_top     }, // back,
        face_type{ right_front_bottom, left_front_bottom, left_back_bottom, right_back_bottom } // bottom
    };

    friend class const_iterator;

    class const_iterator
    {
        friend class Cube;
    private:
        Cube const * _cube;
        int _face;
        int _tri;

        const_iterator(Cube const * cube) : _cube(cube), _face(0), _tri(0) { }

    public:
        const_iterator() : _cube(nullptr), _face(0), _tri(0) { }
        const_iterator(const_iterator const &) = default;
        const_iterator(const_iterator &&) = default;
        const_iterator & operator=(const_iterator const &) = default;
        const_iterator & operator=(const_iterator &&) = default;

        const_iterator & operator++() 
        {
            _tri = (_tri + 1) % 2;

            if(_tri == 0) 
                ++_face;

            return *this;
        }
        triangle_type operator*() const
        {
            auto const & face = faces[_face];

            return triangle_type{ 
                vertices[face[0]], 
                vertices[face[_tri+1]], 
                vertices[face[_tri+2]] 
            };
        }

        bool operator==(const_iterator const & other) const
        {
            if(_cube == nullptr && other._cube == nullptr)
                return true;
            if(_cube == nullptr && other._face >= total_faces)
                return true;
            if(other._cube == nullptr && _face >= total_faces)
                return true;
            
            return _cube == other._cube && _face == other._face && _tri == other._tri;
        }
        bool operator!=(const_iterator const & other) const
        {
            return !(*this == other);
        }
    };

    const_iterator begin() const { return const_iterator{this}; }
    const_iterator end() const { return const_iterator{}; }

};


template<transformation M, simplex T>
class Transformed
{
private:
    T const * _p;
    M _m;
public:
    Transformed(M const & m, T const & mesh) : _p(&mesh), _m(m) { }
    Transformed() : _p(nullptr), _m() { }

    Transformed(Transformed const &) = default;
    Transformed(Transformed &&) = default;
    Transformed & operator=(Transformed const &) = default;
    Transformed & operator=(Transformed &&) = default;

    using point_type = typename T::point_type;
    using triangle_type = T::triangle_type;

    friend class const_iterator;

    class const_iterator 
    {
        friend class Transformed;
    private:
        Transformed const * _t;
        typename T::const_iterator _i;

        const_iterator() : _t(nullptr), _i() {}
        const_iterator(Transformed const * t) : _t(t), _i(t->_p->begin()) { }
    public:
        const_iterator(const_iterator const &) = default;
        const_iterator(const_iterator &&) = default;
        const_iterator & operator=(const_iterator const &) = default;
        const_iterator & operator=(const_iterator &&) = default;

        const_iterator & operator++() { ++_i; return *this; }
        bool operator==(const_iterator const & other) const
        {
            if(_t == nullptr && other._t == nullptr)
                return true;
            if(_t == nullptr && other._i == other._t->_p->end())
                return true;
            if(other._t == nullptr && _i == _t->_p->end())
                return true;

            return _t == other._t && _i == other._i;
        }
        bool operator!=(const_iterator const & other) const
        {
            return !(*this == other);
        }

        triangle_type operator*() const
        {
            triangle_type tri = *_i;

            return triangle_type{
                _t->_m(tri[0]),
                _t->_m(tri[1]),
                _t->_m(tri[2])
            };
        }
    };

    const_iterator begin() const { return const_iterator(this); }
    const_iterator end() const { return const_iterator(); }

};

template<transformation M, simplex T>
auto transform(M const & m, T const & t)
{
    return Transformed{m, t};
}

template<simplex T>
auto scale(typename T::float_type scale, T const & m)
{
    using float_type = typename T::float_type;
    using mat4_type = Mat4<float_type>;

    return Transformed(mat4_type{
        scale, 0., 0., 0., 
        0., scale, 0., 0.,
        0., 0., scale, 0.
    }, m);
}

template<simplex T, class CharT>
void to_stl(T const & triangles, std::basic_ostream<CharT> & os, std::string solid_name = "")
{
    os << "solid " << solid_name << "\n";
    for(auto const & tri : triangles)
    {
        auto v0 = tri[1] - tri[0];
        auto v1 = tri[2] - tri[0];

        auto n = cross(v0, v1);

        os << "\tfacet normal " << n.x << " " << n.y << " " << n.z << "\n";
        os << "\t\touter loop\n";

        for(int j = 0; j < 3; j++)
            os << "\t\t\tvertex " << tri[j].x << " " << tri[j].y << " " << tri[j].z << "\n";
        
        os << "\t\tendloop\n";
        os << "\tendfacet\n";
    }
    os << "endsolid " << solid_name << "\n";
}


template<class Float = float>
class Ray
{
private:

public:
    Point<Float> origin;
    Vector<Float> direction;
};

template<class A, class B>
auto fuse(A const & a, B const & b);

template<typename Float>
std::ostream& operator<<(std::ostream& os, Point<Float> const& p)
{ return os << p.x << " " << p.y << " " << p.z; }

int main(int ac, char * av[])
{
    auto c1 = Cube{};
    auto c2 = Cube{};

    // auto f = fuse(translate(c1, {0.5, 0., 0.}), c2);

    // to_stl(scale(3, c1), std::cout);
    // to_stl(c1, std::cout);

    auto is_inside_unit_sphere = [](auto const& p) -> bool
    { return length(p) < 1; };

    auto sphere = 
        SpaceTree<Point<float>>::from_predicate(is_inside_unit_sphere);

    Point<float> inside{-1.1, 0.1, 0.1};

    std::cout << "point " << inside << " of volume is " << sphere(inside) << std::endl;
    

    return 0;
}

