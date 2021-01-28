#ifndef SILICONIA_ENGINE_HPP
#define SILICONIA_ENGINE_HPP

#include "camera.hpp"
#include "graphics/vk/init.hpp"
#include <SDL.h>
#include <chunks/chunk_collection.hpp>
#include <graphics/vk/command_buffer.hpp>
#include <graphics/vk/types.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <functional>

namespace siliconia::graphics {

class Engine {
public:
  Engine(uint32_t width, uint32_t height, chunks::ChunkCollection &&chunks);
  ~Engine();

  void init();
  void run();

private:
  void init_vulkan();
  void init_swapchain();
  void init_commands();
  void init_default_renderpass();
  void init_framebuffers();
  void init_sync_structures();
  bool load_shader_module(const char *path, VkShaderModule *shader_module);
  void init_pipelines();
  void init_imgui();
  void load_meshes();
  void upload_mesh(vk::Mesh &mesh);
  void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

  VkExtent2D  win_size_;
  SDL_Window *window_;

  camera camera_;

  chunks::ChunkCollection chunks_;

  VkInstance instance_;
  VkDebugUtilsMessengerEXT  debug_messenger_;
  VkDevice device_;
  VkSurfaceKHR surface_;
  VkPhysicalDevice chosen_gpu_;

  VkSwapchainKHR swapchain_;
  VkFormat swapchain_image_format_;
  std::vector<VkImage> swapchain_images_;
  std::vector<VkImageView> swapchain_image_views_;

  VkImageView depth_image_view_;
  vk::AllocatedImage depth_image_;
  VkFormat depth_format_;

  VkQueue graphics_queue_;
  uint32_t  graphics_queue_family_;

  vk::CommandPool command_pool_;
  vk::CommandBuffer main_command_buffer_;

  VkRenderPass renderpass_;
  std::vector<VkFramebuffer> framebuffers_;

  VkSemaphore present_semaphore_, render_semaphore_;
  VkFence render_fence_;

  VkPipelineLayout  pipeline_layout_;
  VkPipeline pipeline_;

  VmaAllocator allocator_;

  std::vector<vk::Mesh> meshes_;

  vk::UploadContext upload_context_;

  VkDescriptorPool imgui_pool_;
};

} // namespace siliconia::graphics
#endif // SILICONIA_ENGINE_HPP
