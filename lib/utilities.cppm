module;

#include <ranges>

export module utilities;

/** 
 * Concepts
 */

// view_of is shorthand for a ranges::view 
template<typename T>
concept view_of = std::ranges::view<T>;

/**
 * array_view
 */
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

/**
 * range_selector 
 */
template<typename Map>
auto select_from( Map const& map )
{ 
    auto selector = [&map]( auto const& key )
    { return map[key]; }

    return std::views::transform( selector );
}


struct greater_than_zero
{
    template<typename NumberType>
    static constexpr bool operator()(NumberType x)
    { return x > 0; }
};
