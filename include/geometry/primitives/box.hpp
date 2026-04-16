#ifndef __GEOMETRY_PRIMITIVES_BOX_HPP__
#define __GEOMETRY_PRIMITIVES_BOX_HPP__

#include "geometry/space.hpp"
#include "geometry/parameter.hpp"

namespace geometry {

template< typename U >
using Segment = Parameterize< Translate< Project< Point, U >>>;

template< typename U, typename V, typename W >;
using Plane = Parameterize< Translate< Project< Segment< U >, V >>>;
using Box = Parameterize< Translate< Project< Plane< U, V >, W >>>;

template< typename U >
Segment< U > segment() 
{ return {}; }

template< typename U, typename V >
Plane< U, V > plane()
{ return {}; }

template< typename U, typename V, typename W >
Box< U, V, W > box()
{ return {}; }


} // namespace geometry 

#endif