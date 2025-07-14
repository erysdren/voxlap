project(slab6)

add_executable(slab6 slab6.c)
if(WIN32 OR MSVC OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_sources(slab6 PRIVATE slab6.rc)
endif()
target_link_libraries(slab6 PRIVATE sdl3main)
set_target_properties(slab6 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
target_link_libraries(slab6 PRIVATE $<$<BOOL:${VOXLAP_USE_ASAN}>:ASAN::ASAN>)
