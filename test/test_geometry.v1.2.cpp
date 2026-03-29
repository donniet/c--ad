#include "units.hpp"
#include "geometry.hpp"
#include "formats/stl.hpp"

#include <iostream>

using namespace units;
using namespace geometry;
using namespace formats;


auto mortise_and_tenon()
{

    // auto tenon_layout = Grid< 3, 3 >{};
    // auto mortise_layout = Grid< 3, 3 >{};

    // auto rail_end = component< Box::Top >( rail );
    // auto stile_inside = component< Rectangle::Bottom >( stile );

    // auto tenon = pad( component< grid_cell< 1, 1 >>( 
    //     tenon_layout( rail_end )));
    // auto mortise = pad( component< grid_cell< 1, 1 >>( 
    //     mortise_layout ( stile_inside )));

    // return box( 1_m, 1_ft, 3_in );

    return extrude( rectangle( 5_m, 4_m ), 2_m, 1_m, 1_in );
}

int main( int ac, char* av[] )
{
    auto obj = mortise_and_tenon();

    STLFile file;
    // output( file, extrude( segment( 5_m ), 3_m, 2_m ) );
    output( file, boundary( obj ));
    // std::cout << file.to_string() << std::endl;

    std::cout << STL{ obj } << std::endl;

    return 0;
}