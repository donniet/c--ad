add_library(linalg linalg.cppm)
target_sources(linalg PUBLIC
    FILE_SET CXX_MODULES FILES linalg.cppm)

add_library(volume volume.cppm)
target_link_libraries(volume linalg)
target_sources(volume PUBLIC
    FILE_SET CXX_MODULES FILES volume.cppm)

add_library(marching_tetrahedra marching_tetra.cppm)
target_sources(marching_tetrahedra PUBLIC
    FILE_SET CXX_MODULES FILES marching_tetra.cppm)

add_library(iso_surface iso_surface.cppm)
target_sources(iso_surface PUBLIC
    FILE_SET CXX_MODULES FILES iso_surface.cppm)
target_link_libraries(iso_surface PUBLIC linalg)

add_library(utilities utilities.cppm)
target_sources(utilities PUBLIC
    FILE_SET CXX_MODULES FILES utilities.cppm)

add_library(geometry geometry.cppm)
target_sources(geometry PUBLIC
    FILE_SET CXX_MODULES FILES geometry.cppm)