#include "chunks/chunk.hpp"
#include "chunks/chunk_collection.hpp"
#include <SDL.h>
#include <graphics/engine.hpp>
#include <iostream>

int main(int argc, char **argv)
{
  try {
    auto chunks = siliconia::chunks::ChunkCollection{
        "C:/Users/matth/Documents/lidar/TL45nw/"};
    auto r = chunks.rect;
    std::cout << "(" << r.x << ", " << r.y << ") " << r.width << "x" << r.height
              << " " << chunks.range.min << "-" << chunks.range.max
              << std::endl;

    auto engine = siliconia::graphics::Engine{1200, 1200};
    engine.init();
    engine.run(chunks);
  } catch (const siliconia::chunks::asc_parse_exception &e) {
    std::cout << e.what() << std::endl;
    return 1;
  }
  return 0;
}