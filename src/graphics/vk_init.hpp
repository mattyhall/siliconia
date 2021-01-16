#ifndef SILICONIA_VK_INIT_HPP
#define SILICONIA_VK_INIT_HPP

#include <vulkan/vulkan.h>

namespace siliconia::graphics::init {

VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(
    VkShaderStageFlagBits stage, VkShaderModule shader_module);

VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();

VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info(
    VkPrimitiveTopology topology);

VkPipelineRasterizationStateCreateInfo rasterisation_state_create_info(VkPolygonMode mode);

VkPipelineMultisampleStateCreateInfo multisample_state_create_info();

VkPipelineColorBlendAttachmentState colour_blend_attachment_state();

VkPipelineLayoutCreateInfo pipeline_layout_create_info();

} // namespace siliconia::graphics::init

#endif // SILICONIA_VK_INIT_HPP
