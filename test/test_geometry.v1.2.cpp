#include "units.hpp"
#include "geometry.hpp"
#include "formats/stl.hpp"

#include <iostream>

using namespace units;
using namespace geometry;
using namespace formats;

auto segment( Length width )
{ return extrude( Point{}, 0_m, width ); }

auto plane( Length width, Length height )
{ return extrude( segment( width ), 0_m, height); }

auto box( Length width, Length height, Length depth )
{ return extrude( plane( width, height ), 0_m, depth ); }

auto mortise_and_tenon()
{
    // auto stile = box();
    // auto stile_end = component< extrusion::cap >( stile );
    
    // return stile;
    return box( 2_m, 3_m, 5_m );
}

int main( int ac, char* av[] )
{
    auto obj = mortise_and_tenon();

    std::cout << STL{ obj } << std::endl;

    return 0;
}