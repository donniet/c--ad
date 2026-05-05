#ifndef __VISITATION_HPP__
#define __VISITATION_HPP__

/// RAII visitor approach
/// 
/// for( visit< each< boolean_expression >> post_card : 
///     itinerary< InOrder >( expr ))
/// /* ... */

#include <ranges>
#include <generator>

namespace visitation {

struct Visitation { };

template< typename T >
struct Itinerary; 

template< typename RangeT >
requires( std::ranges::range< RangeT > )
struct Itinerary< RangeT >
{
    auto operator ()( RangeT& rng )
    { 
        size_t stop_index = 0;
        return plan( rng, stop_index );
    }

    std::generator< Visitation > plan( RangeT& rng, size_t& stop_index )
    {
        size_t range_index = 0;
        for( auto& stop: rng )
        {
            auto place_of_interest = consider( stop, stop_index++, range_index++ );
            if( place_of_interest.will_visit() )
                co_yield place_of_interest;

            if( place_of_interest.will_recurse() )
                for( auto place: plan( place_of_iterest, stop_index )
                    co_yield place;
        }
    }
};

} // namespace visitation

#endif // __VISITATION_HPP__
