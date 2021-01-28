#ifndef SILICONIA_TYPES_HPP
#define SILICONIA_TYPES_HPP

#include "command_buffer.hpp"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace siliconia::graphics::vk {

struct AllocatorBuffer {
  VkBuffer buffer;
  VmaAllocation allocation;
};

struct VertexInputDescription {
  std::vector<VkVertexInputBindingDescription> bindings;
  std::vector<VkVertexInputAttributeDescription> attributes;
  VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Vertex {
  glm::vec3 position;
  glm::vec3 colour;

  static VertexInputDescription get_vertex_description();
};

struct Mesh {
  std::vector<Vertex> vertices;
  AllocatorBuffer vertex_buffer;

  std::vector<uint32_t> indices;
  AllocatorBuffer index_buffer;

  glm::mat4 model_matrix;
};

struct MeshPushConstants {
  glm::mat4 model_matrix;
};


struct AllocatedImage {
  VkImage image;
  VmaAllocation allocation;
};

struct UploadContext {
  VkFence upload_fence;
  CommandPool command_pool;
};

}
#endif // SILICONIA_TYPES_HPP
