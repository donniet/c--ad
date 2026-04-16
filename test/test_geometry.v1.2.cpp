#include "units.hpp"
#include "geometry.hpp"
#include "formats/stl.hpp"

#include <iostream>

using namespace units;
using namespace geometry;
using namespace formats;

auto segment()
{ return extrude< Length >( Point{} ); }

auto plane()
{ return extrude< Length >( segment() ); }

auto box()
{ return extrude< Length >( plane() ); }

auto mortise_and_tenon()
{
    auto stile = box();
    auto stile_end = component< extrusion_boundary::cap >( stile );
    

}

int main( int ac, char* av[] )
{
    auto obj = mortise_and_tenon();

    std::cout << STL{ obj } << std::endl;

    return 0;
}