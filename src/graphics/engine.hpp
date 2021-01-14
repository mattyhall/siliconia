#ifndef SILICONIA_ENGINE_HPP
#define SILICONIA_ENGINE_HPP

#include <SDL.h>
#include <chunks/chunk_collection.hpp>
#include <vulkan/vulkan.h>

namespace siliconia::graphics {

class Engine {
public:
  Engine(uint32_t width, uint32_t height);
  ~Engine();

  void init();
  void draw(const siliconia::chunks::ChunkCollection &chunks, SDL_Renderer *renderer);
  void run(const siliconia::chunks::ChunkCollection &chunks);
  void run();
private:
  VkExtent2D  win_size_;
  SDL_Window *window_;
};

} // namespace siliconia::graphics
#endif // SILICONIA_ENGINE_HPP
