project(src)

# winmain library
if(WIN32 OR MSVC OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_library(winmain STATIC winmain.cpp)
	target_link_directories(winmain PUBLIC ${PROJECT_SOURCE_DIR}/ext/dx8/lib/)
	target_link_libraries(winmain PUBLIC ddraw dinput dxguid)
	target_include_directories(winmain PUBLIC ${PROJECT_SOURCE_DIR})
	if(MSVC)
		# warning C4644: usage of the macro-based offsetof pattern in constant expressions is non-standard
		# warning C4838: conversion from 'int' to 'char' requires a narrowing conversion
		target_compile_options(winmain PRIVATE /wd4644 /wd4838)
	endif()
endif()

# sdl3 port of winmain library
if(EMSCRIPTEN OR WIN32 OR MSVC OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
	find_package(SDL3 CONFIG COMPONENTS SDL3-static)
else()
	find_package(SDL3 CONFIG COMPONENTS SDL3-shared)
endif()
if(SDL3_FOUND)
	add_library(sdl3main STATIC sdl3main.c)
	target_link_libraries(sdl3main PUBLIC SDL3::SDL3)
	target_include_directories(sdl3main PUBLIC ${PROJECT_SOURCE_DIR})
endif()

# ken's picture library
add_library(kplib STATIC kplib.c)
set_source_files_properties(kplib.c PROPERTIES LANGUAGE CXX)
# warning C4838: conversion from 'unsigned __int64' to 'const __int64' requires a narrowing conversion
target_compile_options(kplib PRIVATE $<$<NOT:$<COMPILE_LANGUAGE:ASM_MASM>>:/wd4838>)

# voxlap library
add_library(voxlap STATIC voxlap5.c v5.asm)
set_source_files_properties(voxlap5.c PROPERTIES LANGUAGE CXX)
# warning C4838: conversion from 'unsigned __int64' to 'const __int64' requires a narrowing conversion
target_compile_options(voxlap PRIVATE $<$<NOT:$<COMPILE_LANGUAGE:ASM_MASM>>:/wd4838>)

# subprojects
add_subdirectory(genland)
add_subdirectory(slab6)
