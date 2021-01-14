cmake_minimum_required (VERSION 3.8)

project ("siliconia")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

find_package(Vulkan REQUIRED)

add_library(sdl2 INTERFACE)
set(sdl2_DIR "SDL_PATH" CACHE FILEPATH "Path to SDL2")

if (WIN32)
target_include_directories(sdl2 INTERFACE ${sdl2_DIR}/include ${sdl2_image_DIR}/include)
target_link_directories(sdl2 INTERFACE ${sdl2_DIR}/lib/x64 ${sdl2_image_DIR}/lib/x64)
elseif (UNIX)
target_include_directories(sdl2 INTERFACE ${sdl2_DIR}/include/SDL2 ${sdl2_image_DIR}/include/SDL2)
target_link_directories(sdl2 INTERFACE ${sdl2_DIR}/lib ${sdl2_image_DIR}/lib)
endif()

target_link_libraries(sdl2 INTERFACE SDL2 SDL2main)

add_subdirectory(src)
add_subdirectory(third_party)
