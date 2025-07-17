project(kv62stl)

add_executable(kv62stl kv62stl.c)
set_target_properties(kv62stl PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
target_link_libraries(kv62stl PRIVATE $<$<BOOL:${VOXLAP_USE_ASAN}>:ASAN::ASAN>)
if(libmath)
	target_link_libraries(kv62stl PRIVATE ${libmath})
endif()
