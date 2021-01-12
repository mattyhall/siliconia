#include "chunk.hpp"
#include <string>
#include <vector>

namespace siliconia::chunks {

class ChunkCollection {
public:
  ChunkCollection(const std::string &path);

  rect rect;

private:
  std::vector<Chunk> chunks_;
};

} // namespace siliconia::chunks