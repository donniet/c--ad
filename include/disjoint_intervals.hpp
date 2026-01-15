#ifndef __DISJOINT_INTERVALS_HPP__
#define __DISJOINT_INTERVALS_HPP__

#include <set>
#include <tuple>
#include <algorithm>
#include <functional>

template<typename T>
struct pre_increment
{
    static constexpr T& operator()(T& value)
    { return ++value; }
};

// TODO: conceptualize T as a comparable type that supports operator++()
template< typename T = long long, class LessThan = std::less< >, 
    class PreIncrement = pre_increment<T> >
class disjoint_intervals {
public:
    using value_type = T;
    using compare_type = LessThan;
    using pre_increment_type = PreIncrement;

    struct interval_type : std::pair< value_type, value_type >
    {
        using base_type = std::pair< value_type, value_type >;

        value_type const& lowest_inclusive() const
        { return base_type::first; }

        value_type const& highest_exclusive() const
        { return base_type::second; }

        bool operator<( interval_type const& other ) const
        { return lowest_inclusive() < other.lowest_inclusive(); }

        interval_type( value_type const& b, value_type const& e ) :
            std::pair< value_type, value_type >{ b, e }
        { }

        interval_type() : std::pair< value_type, value_type >{} { }
    };

    using container_type = std::set< interval_type >;

    struct const_iterator
    {
        using value_type = value_type;
        using pointer = value_type const*;
        using reference = value_type const&;
        // TODO: bidirectional at least? maybe random access?
        using iterator_category = std::forward_iterator_tag;
        using difference_type = size_t;

        constexpr reference operator*() const
        { return value; }

        constexpr pointer operator->() const
        { return &value; }

        constexpr const_iterator& operator++()
        {
            pre_increment( value );
            if( is_less_than( value, current->highest_exclusive() ))
                return *this;

            ++current;
            if( current == end )
                return *this;
            
            value = current->lowest_inclusive();
            return *this;
        }

        constexpr bool operator==(const_iterator const& other) const
        { 
            if( current != other.current )
                return false;
            
            if( current == end )
                return true;

            return value == other.value; 
        }
        constexpr bool operator!=(const_iterator const& other) const
        { return !(*this == other); }

        const_iterator( container_type::const_iterator b, 
            container_type::const_iterator e ) : current{b}, end{e}
        {
            if( current != end )
                value = current->lowest_inclusive();
        }

        container_type::const_iterator current;
        container_type::const_iterator end;
        value_type value;
    };

    const_iterator insert( value_type begins_with, value_type ends_with )
    {
        // filter out invalid intervals
        if( ends_with <= begins_with )
            return end();

        auto interval = interval_type{ begins_with, ends_with };

        // if we are empty just insert and return
        if( m_intervals.size() > 0 )
        {
            auto lo = m_intervals.lower_bound({ begins_with, begins_with });

            if( lo != m_intervals.begin() )
                --lo;
            
            auto up = m_intervals.upper_bound({ ends_with, ends_with });

            // remove intervals that overlap this one while modifying this
            // one to include the values already in our collection
            while( lo != up )
            {
                auto c = lo;
                ++lo;

                // does c intersect our new interval?
                if( intervals_intersect( interval, *c ))
                {
                    if( c->lowest_inclusive() < begins_with )
                        begins_with = c->lowest_inclusive();

                    if( ends_with < c->highest_exclusive() )
                        ends_with = c->highest_exclusive();

                    m_intervals.erase(c);
                }
            }  
        }
        
        auto [ inserted_at, _ ] = m_intervals.insert( interval );
        return { inserted_at, m_intervals.end() };
    }

    const_iterator begin() const
    { return { m_intervals.begin(), m_intervals.end() }; }

    const_iterator end() const
    { return { m_intervals.end(), m_intervals.end() }; }



private:
    container_type m_intervals;
    // TODO: add random access
    // std::vector<size_t> m_interval_size;

    static constexpr bool intervals_intersect(interval_type const& a, interval_type const& b)
    { return a.lowest_inclusive()  <= b.highest_exclusive() 
          && a.highest_exclusive() >= b.lowest_inclusive(); }

    static constexpr bool begins_with_is_less_than(
        interval_type const& left, interval_type const& right )
    { return interval_begin(left) < interval_begin(right); }

    static constexpr auto pre_increment( T& value )
    { return pre_incrementer(value); }

    static constexpr bool is_less_than( T const& a, T const& b )
    { return comparer(a, b); }

    static constexpr compare_type comparer = {};
    static constexpr pre_increment_type pre_incrementer = {};

};




#endif