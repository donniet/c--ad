#ifndef __GEOMETRY_HPP__
#define __GEOMETRY_HPP__

#include "universe.hpp"
#include "units.hpp"

#include <string>
#include <iostream>

namespace geometry {

using namespace units;

struct Point: virtual Object
{ 
    Point() { std::cout << "point created." << std::endl; }

    virtual ~Point() { std::cout << "point destroyed." << std::endl; }
};

template< size_t Steps, typename ObjectT, unit U >
struct Extrusion: virtual Object
{
    
};

} // geometry

#endif