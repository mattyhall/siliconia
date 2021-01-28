#ifndef SILICONIA_INIT_HPP
#define SILICONIA_INIT_HPP

#include <vulkan/vulkan.h>

namespace siliconia::graphics::vk {

VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(
    VkShaderStageFlagBits stage, VkShaderModule shader_module);

VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();

VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info(
    VkPrimitiveTopology topology);

VkPipelineRasterizationStateCreateInfo rasterisation_state_create_info(
    VkPolygonMode mode);

VkPipelineMultisampleStateCreateInfo multisample_state_create_info();

VkPipelineColorBlendAttachmentState colour_blend_attachment_state();

VkPipelineLayoutCreateInfo pipeline_layout_create_info();

VkImageCreateInfo image_create_info(
    VkFormat format, VkImageUsageFlags flags, VkExtent3D extend);

VkImageViewCreateInfo image_view_create_info(
    VkFormat format, VkImage image, VkImageAspectFlags flags);

VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(
    bool depth_test, bool depth_write, VkCompareOp op);
} // namespace siliconia::graphics::vk

#endif // SILICONIA_INIT_HPP
