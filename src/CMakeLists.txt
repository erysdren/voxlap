project(src)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)

add_library(sdl3main STATIC sdl3main.c)
target_link_libraries(sdl3main PUBLIC SDL3::SDL3)
target_include_directories(sdl3main PUBLIC ${PROJECT_SOURCE_DIR})

add_subdirectory(slab6)
