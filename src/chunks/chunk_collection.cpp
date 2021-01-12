#include "chunk_collection.hpp"
#include <filesystem>

namespace siliconia::chunks {

ChunkCollection::ChunkCollection(const std::string &path)
  : rect(0, 0, 0, 0), chunks_()
{
  auto first = true;
  for (const auto &p : std::filesystem::directory_iterator{path}) {
    auto chunk = Chunk{p.path().string()};
    if (first) {
      first = false;
      rect = chunk.rect();
    } else {
      rect = rect | chunk.rect();
    }
    chunks_.push_back(std::move(chunk));
  }
}

} // namespace siliconia::chunks