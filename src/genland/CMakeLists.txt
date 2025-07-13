project(genland)

add_executable(genland genland.cpp)
set_target_properties(genland PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

