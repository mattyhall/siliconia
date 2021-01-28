#ifndef SILICONIA_PIPELINE_BUILDER_HPP
#define SILICONIA_PIPELINE_BUILDER_HPP

#include <vulkan/vulkan.h>
#include <vector>

namespace siliconia::graphics::vk {

class PipelineBuilder {
public:
  VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);

  std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
  VkPipelineVertexInputStateCreateInfo vertex_input_info;
  VkPipelineInputAssemblyStateCreateInfo assembly;
  VkViewport viewport;
  VkRect2D scissor;
  VkPipelineRasterizationStateCreateInfo rasteriser;
  VkPipelineColorBlendAttachmentState colour_blend_attachment;
  VkPipelineMultisampleStateCreateInfo multisampling;
  VkPipelineLayout layout;
  VkPipelineDepthStencilStateCreateInfo  depth_stencil;
};

}

#endif // SILICONIA_PIPELINE_BUILDER_HPP
