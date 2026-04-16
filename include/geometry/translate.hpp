#ifndef __GEOMETRY_TRANSLATE_HPP__
#define __GEOMETRY_TRANSLATE_HPP__

#include "geometry/space.hpp"

namespace geometry {

template< typename ObjT, typename V >
Projection< Projection< ObjT, 
    extend_space< space_of< ObjT >, long double >, space_of< ObjT >> 
translate( ObjT obj, V by )
{
    
}


} // namespace geometry 

#endif