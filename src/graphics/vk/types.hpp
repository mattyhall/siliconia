#ifndef SILICONIA_TYPES_HPP
#define SILICONIA_TYPES_HPP

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

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
}

#endif // SILICONIA_TYPES_HPP
