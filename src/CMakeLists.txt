add_executable(siliconia main.cpp
        chunks/chunk.cpp chunks/chunk_collection.cpp
        graphics/engine.cpp graphics/engine.hpp
        graphics/vk/pipeline_builder.cpp graphics/vk/pipeline_builder.hpp graphics/vk/init.hpp graphics/vk/init.cpp graphics/vk/types.cpp graphics/vk/types.hpp graphics/vk/command_buffer.cpp graphics/vk/command_buffer.hpp graphics/vk/helpers.hpp graphics/camera.cpp graphics/camera.hpp)

target_compile_features(siliconia PUBLIC cxx_std_20)

target_include_directories(siliconia PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(siliconia vkbootstrap)
target_link_libraries(siliconia vma glm)
target_link_libraries(siliconia sdl2 Vulkan::Vulkan)
