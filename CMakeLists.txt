cmake_minimum_required(VERSION 3.21)
project(voxlap LANGUAGES C CXX ASM_MASM)

option(VOXLAP_USE_ASAN "Use Address Sanitizer" OFF)

add_library(ASAN::ASAN IMPORTED INTERFACE)
target_compile_options(ASAN::ASAN INTERFACE -fsanitize=address $<$<NOT:$<COMPILE_LANG_AND_ID:C,MSVC>>:-fno-omit-frame-pointer>)
target_link_options(ASAN::ASAN INTERFACE -fsanitize=address)

# get libm on unix-like systems
if(UNIX OR NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
	find_library(libmath m)
endif()

add_subdirectory(src)
