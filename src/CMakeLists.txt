add_executable(siliconia main.cpp
        chunks/chunk.cpp chunks/chunk_collection.cpp
        graphics/engine.cpp graphics/engine.hpp
        graphics/pipeline_builder.cpp graphics/pipeline_builder.hpp graphics/vk_init.hpp graphics/vk_init.cpp graphics/vk_types.cpp graphics/vk_types.hpp)

target_compile_features(siliconia PUBLIC cxx_std_20)

target_include_directories(siliconia PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(siliconia vkbootstrap)
target_link_libraries(siliconia vma glm)
target_link_libraries(siliconia sdl2 Vulkan::Vulkan)
