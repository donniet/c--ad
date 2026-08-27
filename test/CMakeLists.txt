#add_executable(test_iso_surface test_iso_surface.cpp)
#target_link_libraries(test_iso_surface PUBLIC iso_surface)

#add_executable(test_vcg test_vcg.cpp)
#target_link_libraries(test_vcg PRIVATE Eigen3::Eigen)

add_executable(test_disjoint_intervals test_disjoint_intervals.cpp)

#add_executable(test_geometry test_geometry.cpp)
#target_link_libraries(test_geometry geometry)

#add_executable(test_geometry2 test_geometry2.cpp)
# target_link_libraries(test_geometry geometry)

add_executable(test_units test_units.cpp)
add_executable(test_memory test_memory.cpp)
#add_executable(test_constraint_playground test_constraint_playground.cpp)
#add_executable(test_constraint_playground2 test_constraint_playground2.cpp)
add_executable(test_tensors test_tensors.cpp)
add_executable(test_expression_result test_expression_result.cpp)
add_executable(test_expressions test_expressions.cpp)
add_executable(test_select test_select.cpp)
#add_executable(test_substitution test_substitution.cpp)
add_executable(test_recursion test_recursion.cpp)
#add_executable(test_iteration test_iteration.cpp)
#add_executable(test_geometry_v1.2 test_geometry.v1.2.cpp)
add_executable(test_normalize test_normalize.cpp)
add_executable(test_calculus test_calculus.cpp)
add_executable(test_solvers test_solvers.cpp)

add_test(NAME test_units COMMAND test_units)
add_test(NAME test_expression_result COMMAND test_expression_result)
add_test(NAME test_expressions COMMAND test_expressions)
add_test(NAME test_substitution COMMAND test_substitution)
add_test(NAME test_recursion COMMAND test_recursion)
add_test(NAME test_select COMMAND test_select)
add_test(NAME test_tensors COMMAND test_tensors)
add_test(NAME test_normalize COMMAND test_normalize)
add_test(NAME test_calculus COMMAND test_calculus)
add_test(NAME test_solvers COMMAND test_solvers)
#add_test(NAME test_iteration COMMAND test_iteration)

# message("source dir ${CMAKE_SOURCE_DIR}")

# target_sources(test_iso_surface PUBLIC
#     BASE_DIR ${CMAKE_SOURCE_DIR}
#     FILE_SET CXX_MODULES FILES ${LIB_MODULE_FILES})
