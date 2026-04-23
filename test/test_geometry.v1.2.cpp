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
{ return extrude( segment() ); }

auto box()
{ return extrude( plane() ); }

auto mortise_and_tenon()
{
    // auto stile = box();
    // auto stile_end = component< extrusion::cap >( stile );
    
    // return stile;
    return segment();
}

int main( int ac, char* av[] )
{
    auto obj = mortise_and_tenon();

    std::cout << STL{ obj } << std::endl;

    return 0;
}