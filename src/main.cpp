#include <stdio.h>
#include <iostream>
#include "chunks/chunk.hpp"
#include "chunks/chunk_collection.hpp"

int main(int argc, char **argv)
{
  auto asc_file = siliconia::chunks::Chunk{"C:/Users/matth/Documents/lidar/TL45nw/tl4055_DSM_2M.asc"};
  std::cout << asc_file.data.size() << std::endl;
	
  try {
    auto chunks = siliconia::chunks::ChunkCollection{"C:/Users/matth/Documents/lidar/TL45nw/"};
    auto r = chunks.rect;
    std::cout << "(" << r.x << ", " << r.y << ") " << r.width << "x" << r.height << std::endl;
  } catch (const siliconia::chunks::asc_parse_exception &e) {
    std::cout << e.what() << std::endl;
    return 1;
  }
  return 0;
}