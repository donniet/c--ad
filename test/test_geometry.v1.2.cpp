#include "units.hpp"
#include "expressions.hpp"

#include <print>



template< size_t Dimensions >
struct Volume 
{
    static constexpr size_t dimensions = Dimensions;
    using unit_type = units::unit_power_t< dimensions, units::Length >;
};

using Point = Volume< 0 >;

template< typename ObjT >
struct Extrusion
{
    
};


int main( int ac, char* av[] )
{


    return 0;
}