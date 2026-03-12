add_executable(test_iso_surface test_iso_surface.cpp)
target_link_libraries(test_iso_surface PUBLIC iso_surface)

add_executable(test_vcg test_vcg.cpp)
target_link_libraries(test_vcg PRIVATE Eigen3::Eigen)

add_executable(test_disjoint_intervals test_disjoint_intervals.cpp)

add_executable(test_geometry test_geometry.cpp)
target_link_libraries(test_geometry geometry)

add_executable(test_geometry2 test_geometry2.cpp)
# target_link_libraries(test_geometry geometry)

add_executable(test_units test_units.cpp)
add_executable(test_memory test_memory.cpp)
add_executable(test_constraint_playground test_constraint_playground.cpp)
add_executable(test_constraint_playground2 test_constraint_playground2.cpp)
add_executable(test_tensor test_tensor.cpp)
add_executable(test_expressions test_expressions.cpp)

add_test(NAME test_units COMMAND test_units)

# message("source dir ${CMAKE_SOURCE_DIR}")

# target_sources(test_iso_surface PUBLIC
#     BASE_DIR ${CMAKE_SOURCE_DIR}
#     FILE_SET CXX_MODULES FILES ${LIB_MODULE_FILES})
