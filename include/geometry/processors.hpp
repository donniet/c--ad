#ifndef __GEOMETRY_PROCESSORS_HPP__
#define __GEOMETRY_PROCESSORS_HPP__

#include "units.hpp"

#include "geometry/space.hpp"
#include "geometry/collection.hpp"
#include "geometry/compound.hpp"
#include "geometry/extrude.hpp"
#include "geometry/orient.hpp"
#include "geometry/project.hpp"

#include "geometry/simplex.hpp"

namespace geometry {
namespace processors {

using namespace units;



/// @brief class to transform specifications given a geometric operation
struct Specifications
{
    static constexpr Length transform( Point const&, Length const& length_spec )
    { return length_spec; }

    template< typename ObjU >
    static constexpr Length transform( 
        Extrusion< ObjU, Length, Length > const& ext, 
        Length const& length_spec )
    { return length_spec; }

    template< typename ComponentsU, typename... Objs >
    static constexpr Length transform( 
        Compound< ComponentsU, Objs... > const& compound, 
        Length const& length_spec )
    { return length_spec; }

    template< typename... Objs >
    static constexpr Length transform( Collection< Objs... > const& col, 
        Length const& length_spec )
    { return length_spec; }

    template< typename ObjU, typename AttU >
    static constexpr Length transform( Attribution< ObjU, AttU > const& att,
        Length const& length_spec )
    { return length_spec; }

    template< typename ObjU, size_t Rows, size_t Cols, typename... Ts >
    static constexpr auto transform( 
        Projection< ObjU, Linear< Tensor< Shape< Rows, Cols >, Ts... >>> proj,
        Length const& length_spec )
    { return length_spec; }

    template< typename ObjU, size_t From, typename... Ts >
    static constexpr auto transform( 
        Projection< ObjU, ProjectAt< From, Ts... >> proj,
        Length const& length_spec )
    { return length_spec; }
};

} // namespace processors
} // namespace geometry

#endif