#include "pipeline_builder.hpp"
#include <iostream>

namespace siliconia::graphics::vk {

VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass)
{
  auto viewport_state = VkPipelineViewportStateCreateInfo{};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.pViewports = &viewport;
  viewport_state.scissorCount = 1;
  viewport_state.pScissors = &scissor;

  auto colour_blending = VkPipelineColorBlendStateCreateInfo{};
  colour_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colour_blending.logicOpEnable = VK_FALSE;
  colour_blending.logicOp = VK_LOGIC_OP_COPY;
  colour_blending.attachmentCount = 1;
  colour_blending.pAttachments = &colour_blend_attachment;

  auto pipeline_info = VkGraphicsPipelineCreateInfo{};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount = shader_stages.size();
  pipeline_info.pStages = shader_stages.data();
  pipeline_info.pVertexInputState = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &assembly;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &rasteriser;
  pipeline_info.pMultisampleState = &multisampling;
  pipeline_info.pColorBlendState = &colour_blending;
  pipeline_info.pDepthStencilState = &depth_stencil;
  pipeline_info.layout = layout;
  pipeline_info.renderPass = pass;
  pipeline_info.subpass = 0;
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

  auto pipeline = VkPipeline{};
  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS) {
    std::cout << "failed to create pipeline" << std::endl;
    return VK_NULL_HANDLE;
  }
  return pipeline;
}

}