cmake_minimum_required(VERSION 3.21)
project(voxlap LANGUAGES C CXX)

option(VOXLAP_USE_ASAN "Use Address Sanitizer" OFF)

add_library(ASAN::ASAN IMPORTED INTERFACE)
target_compile_options(ASAN::ASAN INTERFACE -fsanitize=address $<$<NOT:$<COMPILE_LANG_AND_ID:C,MSVC>>:-fno-omit-frame-pointer>)
target_link_options(ASAN::ASAN INTERFACE -fsanitize=address)

add_subdirectory(src)
