#include "chunks/chunk.hpp"
#include "chunks/chunk_collection.hpp"
#include <iostream>
#include <stdio.h>

int main(int argc, char **argv)
{
  // std::getchar();

  try {
    auto chunks = siliconia::chunks::ChunkCollection{
        "C:/Users/matth/Documents/lidar/TL45nw/"};
    auto r = chunks.rect;
    std::cout << "(" << r.x << ", " << r.y << ") " << r.width << "x" << r.height
              << std::endl;
  } catch (const siliconia::chunks::asc_parse_exception &e) {
    std::cout << e.what() << std::endl;
    return 1;
  }
  // std::getchar();
  return 0;
}