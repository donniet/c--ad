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

    // auto rect = extrude( segment( 5_m ), 2_m, 2_m );

    auto box = extrude( extrude( segment( 5_m ), 1_m ), 
            2_m );

    // return box;

    return rotate_plane< 0, 1 >( box, pi / 4. );
    // return translate( box, { 0.5_m, 0.2_m, 3_m });

    // return select< IndexSelector< 1, 2 >::template selector >( box );

    // constexpr auto tenon_element = box.boundary_element( 1, 0, 0 );
    // auto padded = pad< tenon_element >( box, 3_m );
    // pad( boundary_component< extrusion_shell, 1, 0 >( box ), 3_m );

    // return padded;
}

int main( int ac, char* av[] )
{
    auto obj = mortise_and_tenon();

    // STLFile file;
    // output( file, extrude( segment( 5_m ), 3_m, 2_m ) );
    // output( file, boundary( obj ));
    // std::cout << file.to_string() << std::endl;

    std::cout << STL{ obj } << std::endl;

    return 0;
}