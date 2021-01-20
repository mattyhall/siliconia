#ifndef SILICONIA_COMMAND_BUFFER_HPP
#define SILICONIA_COMMAND_BUFFER_HPP

#include <cstdint>
#include <vulkan/vulkan.h>

namespace siliconia::graphics::vk {

class RenderPassGuard {
public:
  RenderPassGuard(VkCommandBuffer buffer);
  ~RenderPassGuard();

  void bind_pipeline(VkPipeline pipeline);
  void bind_vertex_buffers(
      uint32_t first_binding, uint32_t binding_count, const VkBuffer *buffers);
  void bind_index_buffer(VkBuffer buffer);
  void draw(uint32_t vertex_count, uint32_t instance_count,
      uint32_t first_vertex, uint32_t first_instance);
  void draw_indexed(uint32_t index_count, uint32_t instance_count,
      uint32_t first_index, int32_t vertex_offset, uint32_t first_instance);

private:
  VkCommandBuffer buffer_;
};

class CommandBufferGuard {
public:
  CommandBufferGuard(VkCommandBuffer buffer);

  ~CommandBufferGuard();

  RenderPassGuard begin_render_pass(VkRenderPass pass, VkExtent2D extent,
      VkFramebuffer framebuffer, const VkClearValue &clear_value);

private:
  VkCommandBuffer buffer_;
};

class CommandBuffer {
  friend class CommandPool;

public:
  CommandBuffer() = default;
  CommandBuffer(VkCommandBuffer buffer);

  void reset();

  CommandBufferGuard begin();

  VkCommandBuffer buffer() const;

private:
  VkCommandBuffer buffer_;
};

class CommandPool {
public:
  CommandPool() = default;
  CommandPool(VkDevice device, uint32_t family);

  CommandBuffer allocate_buffer();

  VkCommandPool pool() const;

private:
  VkDevice device_;
  uint32_t family_;
  VkCommandPool pool_;
};

} // namespace siliconia::graphics::vk

#endif // SILICONIA_COMMAND_BUFFER_HPP
