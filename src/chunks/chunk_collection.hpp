#include "chunk.hpp"
#include <string>
#include <vector>

namespace siliconia::chunks {

class ChunkCollection {
public:
  ChunkCollection(const std::string &path);

  const std::vector<Chunk> &chunks() const;

  rect rect;
  range range;

private:
  std::vector<Chunk> chunks_;
};

} // namespace siliconia::chunks