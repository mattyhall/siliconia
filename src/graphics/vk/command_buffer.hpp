#ifndef SILICONIA_COMMAND_BUFFER_HPP
#define SILICONIA_COMMAND_BUFFER_HPP

#include <array>
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
  void push_constants(VkPipelineLayout layout, VkShaderStageFlags flags,
      size_t size, const void *ptr);
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

  template <size_t N>
  RenderPassGuard begin_render_pass(VkRenderPass pass, VkExtent2D extent,
      VkFramebuffer framebuffer, std::array<VkClearValue, N> clear_values)
  {
    auto rp_info = VkRenderPassBeginInfo{};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_info.pNext = nullptr;
    rp_info.renderPass = pass;
    rp_info.renderArea.offset = {0, 0};
    rp_info.renderArea.extent = extent;
    rp_info.framebuffer = framebuffer;
    rp_info.clearValueCount = N;
    rp_info.pClearValues = &clear_values[0];
    vkCmdBeginRenderPass(buffer_, &rp_info, VK_SUBPASS_CONTENTS_INLINE);

    return RenderPassGuard{buffer_};
  }

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
