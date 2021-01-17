#include "vk_types.hpp"
#include <vulkan/vulkan.h>

namespace siliconia::graphics::types {

VertexInputDescription Vertex::get_vertex_description()
{
  auto desc = VertexInputDescription{};

  auto main_binding = VkVertexInputBindingDescription{};
  main_binding.binding = 0;
  main_binding.stride = sizeof(Vertex);
  main_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  desc.bindings.push_back(main_binding);

  auto position_attr = VkVertexInputAttributeDescription{};
  position_attr.binding = 0;
  position_attr.location = 0;
  position_attr.format = VK_FORMAT_R32G32B32_SFLOAT;
  position_attr.offset = offsetof(Vertex, position);
  desc.attributes.push_back(position_attr);

  auto colour_attr = VkVertexInputAttributeDescription{};
  colour_attr.binding = 0;
  colour_attr.location = 1;
  colour_attr.format = VK_FORMAT_R32G32B32_SFLOAT;
  colour_attr.offset = offsetof(Vertex, colour);
  desc.attributes.push_back(colour_attr);

  return desc;
}

} // namespace siliconia::graphics::types
