#include "units.hpp"
#include "geometry.hpp"
#include "formats/stl.hpp"

#include <iostream>

using namespace units;
using namespace geometry;
using namespace formats;

auto segment()
{ return extrude( Point{}, 0_m, 1_m ); }

auto plane()
{ return extrude( segment(), 0_m, 1_m ); }

auto box()
{ return extrude( plane(), 0_m, 1_m ); }

auto mortise_and_tenon()
{
    // auto stile = box();
    // auto stile_end = component< extrusion::cap >( stile );
    
    // return stile;
    return box();
}

int main( int ac, char* av[] )
{
    auto obj = mortise_and_tenon();

    std::cout << STL{ obj } << std::endl;

    return 0;
}