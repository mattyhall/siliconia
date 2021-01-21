#include "command_buffer.hpp"
#include "helpers.hpp"

namespace siliconia::graphics::vk {

RenderPassGuard::RenderPassGuard(VkCommandBuffer buffer) : buffer_(buffer)
{
}

RenderPassGuard::~RenderPassGuard()
{
  vkCmdEndRenderPass(buffer_);
}

void RenderPassGuard::bind_pipeline(VkPipeline pipeline)
{
  vkCmdBindPipeline(buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void RenderPassGuard::bind_vertex_buffers(
    uint32_t first_binding, uint32_t binding_count, const VkBuffer *buffers)
{
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(
      buffer_, first_binding, binding_count, buffers, &offset);
}

void RenderPassGuard::bind_index_buffer(VkBuffer buffer)
{
  vkCmdBindIndexBuffer(buffer_, buffer, 0, VK_INDEX_TYPE_UINT32);
}

void RenderPassGuard::draw(uint32_t vertex_count, uint32_t instance_count,
    uint32_t first_vertex, uint32_t first_instance)
{
  vkCmdDraw(
      buffer_, vertex_count, instance_count, first_vertex, first_instance);
}

void RenderPassGuard::draw_indexed(uint32_t index_count,
    uint32_t instance_count, uint32_t first_index, int32_t vertex_offset,
    uint32_t first_instance)
{
  vkCmdDrawIndexed(buffer_, index_count, instance_count, first_index,
      vertex_offset, first_instance);
}

void RenderPassGuard::push_constants(VkPipelineLayout layout, VkShaderStageFlags flags, size_t size, const void *ptr)
{
  vkCmdPushConstants(buffer_, layout, flags, 0, size, ptr);
}

CommandBufferGuard::CommandBufferGuard(VkCommandBuffer buffer) : buffer_(buffer)
{
}

CommandBufferGuard::~CommandBufferGuard()
{
  VK_CHECK(vkEndCommandBuffer(buffer_));
}

RenderPassGuard CommandBufferGuard::begin_render_pass(VkRenderPass pass,
    VkExtent2D extent, VkFramebuffer framebuffer,
    const VkClearValue &clear_value)
{
  auto rp_info = VkRenderPassBeginInfo{};
  rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rp_info.pNext = nullptr;
  rp_info.renderPass = pass;
  rp_info.renderArea.offset = {0, 0};
  rp_info.renderArea.extent = extent;
  rp_info.framebuffer = framebuffer;
  rp_info.clearValueCount = 1;
  rp_info.pClearValues = &clear_value;
  vkCmdBeginRenderPass(buffer_, &rp_info, VK_SUBPASS_CONTENTS_INLINE);

  return RenderPassGuard{buffer_};
}

CommandBuffer::CommandBuffer(VkCommandBuffer buffer) : buffer_(buffer)
{
}

VkCommandBuffer CommandBuffer::buffer() const
{
  return buffer_;
}

void CommandBuffer::reset()
{
  VK_CHECK(vkResetCommandBuffer(buffer_, 0));
}

CommandBufferGuard CommandBuffer::begin()
{
  auto cmd_begin_info = VkCommandBufferBeginInfo{};
  cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_begin_info.pNext = nullptr;
  cmd_begin_info.pInheritanceInfo = nullptr;
  cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  VK_CHECK(vkBeginCommandBuffer(buffer_, &cmd_begin_info));

  return CommandBufferGuard{buffer_};
}

CommandPool::CommandPool(VkDevice device, uint32_t family)
  : device_(device), family_(family)
{
  auto cmd_pool_info = VkCommandPoolCreateInfo{};
  cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmd_pool_info.queueFamilyIndex = family_;
  cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  VK_CHECK(vkCreateCommandPool(device_, &cmd_pool_info, nullptr, &pool_));
}

CommandBuffer CommandPool::allocate_buffer()
{
  auto cmd_alloc_info = VkCommandBufferAllocateInfo{};
  cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmd_alloc_info.pNext = nullptr;
  cmd_alloc_info.commandPool = pool_;
  cmd_alloc_info.commandBufferCount = 1;
  cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  VkCommandBuffer buf;
  VK_CHECK(vkAllocateCommandBuffers(device_, &cmd_alloc_info, &buf));
  return CommandBuffer{buf};
}

VkCommandPool CommandPool::pool() const
{
  return pool_;
}
} // namespace siliconia::graphics::vk
