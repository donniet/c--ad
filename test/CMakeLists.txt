add_executable(test_iso_surface test_iso_surface.cpp)
target_link_libraries(test_iso_surface PUBLIC iso_surface)

message("source dir ${CMAKE_SOURCE_DIR}")

# target_sources(test_iso_surface PUBLIC
#     BASE_DIR ${CMAKE_SOURCE_DIR}
#     FILE_SET CXX_MODULES FILES ${LIB_MODULE_FILES})
