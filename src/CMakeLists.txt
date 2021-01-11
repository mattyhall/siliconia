add_executable(siliconia main.cpp)

target_compile_features(siliconia PUBLIC cxx_std_20)

target_include_directories(siliconia PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(siliconia sdl2)
