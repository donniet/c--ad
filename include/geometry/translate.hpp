#ifndef __GEOMETRY_TRANSLATE_HPP__
#define __GEOMETRY_TRANSLATE_HPP__

#include "geometry/project.hpp"
#include "geometry/extrude.hpp"
#include "geometry/boundary.hpp"

namespace geometry {

template< typename ObjT, vector V >
requires( is_empty< ObjT >)
constexpr Empty< ObjT::dimensions() > translate( ObjT, V )
{ return {}; }

/// @brief translating an object amounts to extruding it into a higher dimension
/// then projecting the cap of the boundary of the extrusion back into the
/// objects dimensional space using an augmented identity matrix [N,N+1] where
/// the last column contains the translation vector
/// @tparam ObjT object to be translated
/// @tparam V the translation amount
/// @param object to be translated
/// @param by translation vector
/// @return the object in the same dimensional space but the coordinates are
/// each offset by the translation vector
/// TODO: this method is somehow sneaking a tensor into another tensor as an element...
/// It also is causing a matmul requirement failure likely due to the extrusion
/// unit not being compatible with the vector space.  Or I'm not doing the multiplcation
/// right.  Where is the matmul problem? in the translation_matrix or the project?
/// project has the requires clause.
/// it's all the Projection< Point, ... > variations.  I need to decide what a 
/// projection actually is and what the objects and their types know and when
/// It's close, though, all the problems are concentrated here (at least for geometry)
/// so fixing these bugs will decide if we can do all the geometry we want with just these
/// two operations.
/// the problem is that extrusions add a dimension that may not be compatible when 
/// discerning the type of vector in the projected space. 
/// There is circular logic between this and boundary...
template< typename ObjT, typename V >
requires( not is_empty< ObjT >)
auto translate( ObjT object, V by )
{ return linear_transform( project_at( object, 1 ), translation_matrix( by )); }

} // namespace geometry 

#endif