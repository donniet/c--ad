#include "units.hpp"
#include "dimensions.hpp"
#include "universe.hpp"
#include "compounds.hpp"

#include <iostream>

using std::string;

int main( int ac, char* av[] )
{
    using namespace units::operators;
    using namespace units::literals;
    using namespace units;

    using std::cout, std::cin, std::endl;

    Universe world( "cabinet" );

    // Tensor< Contravariant<>, Covariant< Length, Length >> g;
    // Vector< Length > v;

    // auto y = d( exp( expression_of( v )));

    auto stile = new ( world ) Box{ };
    auto rail = new ( world ) Box{ };

    auto cabinet_width  = constant( 48.00_in );
    auto cabinet_height = constant( 34.50_in );
    auto cabinet_depth  = constant( 24.00_in );
    auto kick_height    = constant(  4.00_in );
    auto door_relief    = constant(  0.25_in );
    auto door_gap       = constant( 0.125_in );
    auto frame_width    = constant(  1.50_in );
    auto door_thickness = constant( 0.625_in );

    auto door_height = ( cabinet_height - kick_height ) - door_relief * 2.0;

    world.constrain( 
        stile.dimensions == { door_height, frame_width, door_thickness });

    // dimensions_of( stile ) = { door_height, frame_width, door_thickness };
    // stile->dimensions = { door_height, frame_width, door_thickness };

    cout << stile->dimensions.to_string() << endl;

    cout << STL{ world } << endl;
    world.save();

    return EXIT_SUCCESS;
}