#include <string>
#include <vector>
#include "chunk.hpp"

namespace siliconia::chunks {
  
class ChunkCollection {
public:
  ChunkCollection(const std::string &path);

  rect rect;
private:
  std::vector<Chunk> chunks_;
};

}