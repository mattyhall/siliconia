find_package(Vulkan REQUIRED)

add_library(vkbootstrap STATIC)
add_library(imgui STATIC)
add_library(glm INTERFACE)
add_library(vma INTERFACE)

target_sources(vkbootstrap PRIVATE 
    vkbootstrap/VkBootstrap.h
    vkbootstrap/VkBootstrap.cpp
    )

target_include_directories(vkbootstrap PUBLIC vkbootstrap)
target_link_libraries(vkbootstrap PUBLIC Vulkan::Vulkan $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>)

target_include_directories(imgui PUBLIC imgui imgui/backends)
target_sources(imgui PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_vulkan.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_sdl.cpp"
        )
target_link_libraries(imgui Vulkan::Vulkan sdl2)

target_include_directories(vma INTERFACE vma)
target_include_directories(glm INTERFACE glm)

