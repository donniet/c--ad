module;

#ifdef PARALLEL
#include <execution>
#define PAR_UNSEQ std::execution::par_unseq,
#else
#define PAR_UNSEQ 
#endif

#include <stddef.h>
#include <vector>
#include <cstdint>
#include <iterator>
#include <functional>
#include <numeric>
#include <cmath>
#include <algorithm>

export module volume; 


import linalg;


class Grid {
public:
    struct node 
    {
        
    };

    template<typename PredicateType, typename RangeType>
    Grid from_predicate(PredicateType&& pred, RangeType const& range)
    {
        using coordinate_type = typename RangeType::value_type;
        auto index_of = [&range](coordinate_type const& x) -> size_t
        { return std::distance(&(*range.begin()), &x); };

        for(coordinate_type const& x : range)
        {
            size_t i = index_of(x);
            
        }
    }
private:

};


export template<typename PointType>
class SpaceTree {
public:
    using point_type = PointType;
    using point_value_type = point_traits<point_type>::value_type;
    
    enum class axial_direction_type : bool 
    { positive = true, negative = false };
    static constexpr inline size_t dimensions = 
        point_traits<PointType>::dimension();

    struct direction_type {
        // fixed at max 16 dimensions
        using bitmask_type = std::uint16_t;

        template<typename Vector>
        static constexpr bitmask_type bitmask_from_vector(Vector v)
        {
            auto b = vector_traits<Vector>::values_begin(v);
            auto e = vector_traits<Vector>::values_end(v);
            std::vector<point_value_type> values{b, e};

            auto axis_mask = [&values](auto& a) -> bitmask_type
            { 
                // std::vectors are just arrays
                size_t dim = std::distance(&values[0], &a);
                // positive == 1, negative == 0
                return (a > 0 ? 0x1 : 0x0) << dim;
            };

            return std::transform_reduce(PAR_UNSEQ
                values.begin(), values.end(), bitmask_type{0},
                /* reduce with    */ std::bit_or<>{}, 
                /* transform with */ axis_mask);
        }
        point_type relative_point(point_type p, point_value_type dist)
        {
            using traits_type = point_traits<point_type>;

            auto ref = typename traits_type::reference_view{p};
            std::vector<bitmask_type> masks{traits_type::dimension(), 0};

            // create masks by shifting 1 right by the dimension 
            std::for_each(PAR_UNSEQ masks.begin(), masks.end(), [&masks](auto& m)
            { m = (0x1 << std::distance(&masks[0], &m)); });

            std::transform(PAR_UNSEQ ref.begin(), ref.end(), masks.begin(), 
                ref.begin(), [&](auto const& x, auto const& mask)
            { return mask & m_mask ? x + dist : x; });

            return p;
        }
        
        axial_direction_type operator[](int axis) const
        { return static_cast<axial_direction_type>(m_mask & (0x1 << axis)); }

        direction_type& operator++()
        { ++m_mask; return *this; }

        direction_type operator++(int)
        { 
            auto ret = *this;
            ++(*this);
            return ret;
        }

        direction_type& operator--()
        { --m_mask; return *this; }

        direction_type& operator--(int)
        { 
            auto ret = *this;
            --(*this);
            return ret;
        }

        bool operator<(direction_type const& other) const
        { return m_mask < other.m_mask; }

        bool operator==(direction_type const& other) const
        { return m_mask == other.m_mask; }

        direction_type() : m_mask{0} { }
        direction_type(bitmask_type mask) : m_mask{mask} { }

        template<typename Vector>
        static direction_type from_vector(Vector const& v) 
        { return direction_type{bitmask_from_vector(v)}; }

        static direction_type diagonal()
        { return direction_type{(bitmask_type)((1 << dimensions) - 1)}; } // all 1s
        
        direction_type(direction_type&&) = default;
        direction_type(direction_type const&) = default;
        direction_type& operator=(direction_type&&) = default;
        direction_type& operator=(direction_type const&) = default;

        bitmask_type mask() const 
        { return m_mask; }

        operator size_t() const
        { return mask(); }

        bitmask_type m_mask;

    };

    static constexpr direction_type direction_end()
    { 
        using bitmask_type = direction_type::bitmask_type;
        return direction_type{(bitmask_type)0x1 << dimensions}; 
    }

    struct space_iterator;

    struct node {
        friend class SpaceTree;
        friend struct space_iterator;

        node* create(direction_type dir)
        {
            if(m_subspace[dir] == nullptr)
            {
                m_subspace[dir] = new node(this, dir);
                ++m_occupied;
            }
            return m_subspace[dir];
        }
        bool erase(direction_type dir)
        {
            if(m_subspace[dir] == nullptr)
                return false;

            m_subspace[dir]->delete_children();
            delete m_subspace[dir];
            m_subspace[dir] = nullptr;
            --m_occupied;
            return true;
        }
        node* operator[](direction_type dir) const
        { return m_subspace[dir]; }

        void fill() { delete_children(); }
        node * parent() const { return m_parent; }
        direction_type parent_direction() const { return m_parent_direction; }
        bool is_leaf() const { return m_occupied == 0; }
        bool is_full() const { return m_occupied == m_subspace.size(); }
        auto occupied() const { return m_occupied; }
        

    protected:
        node(node* parent = nullptr, 
             direction_type parent_direction = direction_end()) : 
            m_parent{parent}, m_parent_direction{parent_direction}, 
            m_occupied{0}
        { m_subspace.fill(nullptr); }

    protected:
        void delete_children()
        {
            for(node *& child : m_subspace)
            {
                if(child == nullptr)
                    continue;
                    
                // TODO: remove recurse
                // TODO: add paralellism
                child->delete_children();
                delete child;
                child = nullptr;
                --m_occupied;
            }
        }

    private:
        node* m_parent;
        direction_type m_parent_direction;
        std::array<node*, 1 << dimensions> m_subspace;
        unsigned m_occupied;
    };

    struct space_iterator {
        bool is_leaf() const
        { return m_node->is_leaf(); }
        bool is_full() const
        { return m_node->is_full(); }
        operator bool() const
        { return m_node != nullptr; }
        
        space_iterator operator[](direction_type dir) const
        { return { *this, dir }; }

        point_type min_corner() const
        { return m_min_corner; }
        point_type center() const
        { 
            auto diag = direction_type::diagonal();
            return diag.relative_point(min_corner(), side() / 2.);
        }
        point_value_type side() const
        { return std::pow(2, -(int)m_depth); }
        size_t depth() const
        { return m_depth; }
        direction_type direction_to(point_type const& p) const
        { return direction_type::from_vector(p - min_corner()); }

        // fills this area
        void fill()
        { m_node->fill(); }


        space_iterator() : 
            m_node{nullptr}, m_depth{0}, m_min_corner{} 
        { }
        space_iterator(node* root) : m_node{root}, m_depth{0}, m_min_corner{}
        { }
        space_iterator(space_iterator const& parent, 
            direction_type dir) : m_node{(*parent.m_node)[dir]}, 
            m_depth{parent.m_depth + 1},
            m_min_corner{dir.relative_point(parent.min_corner(), side())}
        { }

        space_iterator(space_iterator&&) = default;
        space_iterator(space_iterator const&) = default;
        space_iterator& operator=(space_iterator&&) = default;
        space_iterator& operator=(space_iterator const&) = default;

        node * m_node;
        unsigned m_depth;
        point_type m_min_corner;
    };

    
    static bool erase(space_iterator const& i)
    {
        if(i.m_node == nullptr)
            return false;
        
        i.m_node->parent()->erase(i.m_node->parent_direction());
        return true;
    }

    bool operator()(point_type const& p) const
    {
        auto cur = root();

        while(cur && !cur.is_leaf())
            cur = cur[cur.direction_to(p)];

        return (bool)cur;        
    }

    space_iterator root() const
    { return {m_root}; }

    SpaceTree intersect(SpaceTree<point_type> const& other) const
    {
        if(!root() || !other.root())
            return {}; // nothing
        
        SpaceTree<point_type> ret; // empty
        
        using element_type = std::tuple<
            space_iterator, space_iterator, space_iterator>;
        using stack_type = std::vector<element_type>;

        space_iterator r, cur, oth;
        stack_type stack;

        auto push = [&stack](space_iterator const& r, 
            space_iterator const& a, space_iterator const& b) 
        { stack.push_back({r, a, b}); };

        auto pop = [&stack](space_iterator& r, 
            space_iterator& a, space_iterator& b)
        { std::tie(r, a, b) = stack.back(); stack.pop_back(); };

        push(ret.root(), root(), other.root());
        while(!stack.empty())
        {
            pop(r, cur, oth);
            r.fill(); // fill this space and carve it away below

            for(direction_type dir{}; dir != direction_end(); ++dir)
            {
                if(!cur[dir] || !oth[dir])
                    continue;
                
                push(r[dir], cur[dir], oth[dir]);
            }
        }

        return ret;
    }

    /* union */
    SpaceTree merge(SpaceTree<point_type> const& other) const
    {
        if(!root() && !other.root())
            return {}; // nothing

        SpaceTree<point_type> ret; // empty
        
        using element_type = std::tuple<
            space_iterator, space_iterator, space_iterator>;
        using stack_type = std::vector<element_type>;

        space_iterator r, cur, oth;
        stack_type stack;

        auto push = [&stack](space_iterator const& r, 
            space_iterator const& a, space_iterator const& b) 
        { stack.push_back({r, a, b}); };

        auto pop = [&stack](space_iterator& r, 
            space_iterator& a, space_iterator& b)
        { std::tie(r, a, b) = stack.back(); stack.pop_back(); };

        push(ret.root(), root(), other.root());
        while(!stack.empty())
        {
            pop(r, cur, oth);
            r.fill();

            for(direction_type dir{}; dir != direction_end(); ++dir)
            {
                space_iterator cur1 = cur ? cur[dir] : cur;
                space_iterator oth1 = oth ? oth[dir] : oth;

                if(!cur1 && !oth1)
                    continue;
                
                push(r[dir], cur[dir], oth[dir]);
            }
        }

        return ret;
    }

    static SpaceTree everything()
    {
        SpaceTree<point_type> ret;
        ret.m_root = new node;
        return ret;
    }

    template<typename PointPredicate>
    static SpaceTree from_predicate(PointPredicate&& pred, size_t max_depth);

    template<typename PointPredicate>
    static SpaceTree from_predicate(PointPredicate&& pred, double eps = 1.)
    {
        using frame_type = std::tuple<space_iterator, point_type, direction_type>;
        using stack_type = std::vector<frame_type>;

        stack_type stack;
        auto push = [&stack](space_iterator const& cur, point_type point, 
            direction_type dir)
        { stack.emplace_back(cur, point, dir); };
        
        auto pop = [&stack](space_iterator& cur, point_type& point, 
            direction_type& dir) -> bool
        { 
            using std::get;
            if(stack.empty())
                return false;
            auto tup = stack.back(); 
            stack.pop_back(); 
            cur = get<0>(tup);
            point = get<1>(tup);
            dir = get<2>(tup);
            return true;
        };

        auto ret = SpaceTree<point_type>::everything();
        
        push(ret.root(), point_traits<point_type>::origin(), direction_type{});

        space_iterator cur;
        point_type point;
        direction_type dir;

        while(pop(cur, point, dir))
        {
            if(dir == direction_end())
            {
                // we've tested all directions from this space
                if(cur.is_full())
                    cur.fill(); // if it's full

                continue;
            }

            auto side = cur.side();
            auto subspace = cur[dir++];

            // push the next direction onto the stack
            push(cur, point, dir);

            // if we are at the bottom and the center of this voxel is out-
            // side the predicate, then erase this subspace
            if(side <= eps)
            {
                ///// Step 2: erase things at the leaf level 
                if(!pred(cur.center()))
                    erase(cur); 

                continue;
            }

            // not at the bottom so push the subspace onto the stack
            auto next_point = dir.relative_point(point, side);
            push(subspace, next_point, direction_type{});
        }

        return ret;
    }

    SpaceTree() : m_root{nullptr} { }

private:
    node * m_root;
};



