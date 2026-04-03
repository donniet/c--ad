#include "geometry_1.3.hpp"

#include <iostream>

using namespace geometry;

int main( int ac, char* av[] )
{
    auto world = Universe{ "world" };

    auto p = new (world) Point;

    return 0;
}