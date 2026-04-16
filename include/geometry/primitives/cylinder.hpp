#ifndef __GEOMETRY_PRIMITIVES_CYLINDER_HPP__
#define __GEOMETRY_PRIMITIVES_CYLINDER_HPP__

#include "geometry/primitives/box.hpp"
#include "geometry/revolve.hpp"

namespace geometry {

template< typename U, typename V >
using Disc = Parameterize< RotatePlane< 0, 1, Project< Segment< U >, V >>>;


} // namespace geometry 

#endif