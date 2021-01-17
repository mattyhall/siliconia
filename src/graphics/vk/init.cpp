#include "init.hpp"

namespace siliconia::graphics::vk {

VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(
    VkShaderStageFlagBits stage, VkShaderModule shader_module)
{
  auto info = VkPipelineShaderStageCreateInfo{};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  info.stage = stage;
  info.module = shader_module;
  info.pName = "main";
  return info;
}

VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info()
{
  auto info = VkPipelineVertexInputStateCreateInfo{};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  info.vertexAttributeDescriptionCount = 0;
  info.vertexAttributeDescriptionCount = 0;
  return info;
}

VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info(
    VkPrimitiveTopology topology)
{
  auto info = VkPipelineInputAssemblyStateCreateInfo{};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  info.topology = topology;
  info.primitiveRestartEnable = VK_FALSE;
  return info;
}

VkPipelineRasterizationStateCreateInfo rasterisation_state_create_info(
    VkPolygonMode mode)
{
  auto info = VkPipelineRasterizationStateCreateInfo{};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  info.depthClampEnable = VK_FALSE;
  info.rasterizerDiscardEnable = VK_FALSE;
  info.polygonMode = mode;
  info.lineWidth = 1.0f;
  info.cullMode = VK_CULL_MODE_NONE;
  info.frontFace = VK_FRONT_FACE_CLOCKWISE;
  info.depthBiasEnable = VK_FALSE;
  info.depthBiasConstantFactor = 0.0f;
  info.depthBiasClamp = 0.0f;
  info.depthBiasSlopeFactor = 0.0f;
  return info;
}

VkPipelineMultisampleStateCreateInfo multisample_state_create_info()
{
  auto info = VkPipelineMultisampleStateCreateInfo{};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  info.sampleShadingEnable = VK_FALSE;
  info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  info.minSampleShading = 1.0f;
  info.pSampleMask = nullptr;
  info.alphaToCoverageEnable = VK_FALSE;
  info.alphaToOneEnable = VK_FALSE;
  return info;
}

VkPipelineColorBlendAttachmentState colour_blend_attachment_state()
{
  auto colour_blend_attachment = VkPipelineColorBlendAttachmentState{};
  colour_blend_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colour_blend_attachment.blendEnable = VK_FALSE;
  return colour_blend_attachment;
}

VkPipelineLayoutCreateInfo pipeline_layout_create_info()
{
  auto info = VkPipelineLayoutCreateInfo{};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  return info;
}

} // namespace siliconia::graphics::init
