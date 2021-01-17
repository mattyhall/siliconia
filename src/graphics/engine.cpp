#include "engine.hpp"
#include "VkBootstrap.h"
#include "vk/helpers.hpp"
#include "vk/pipeline_builder.hpp"
#include <SDL_vulkan.h>
#include <array>
#include <fstream>
#include <iostream>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace siliconia::graphics {

struct colour {
  colour(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}

  glm::vec3 to_glm() {
    return {r / 255.0, g / 255.0, b / 255.0};
  }

  uint8_t r, g, b;
};

struct gradient_point {
  gradient_point(float point, colour colour) : point(point), colour(colour) {}

  float point;
  colour colour;
};

template<size_t N>
colour get_colour(const std::array<gradient_point, N> &gradient,
    float nodata_value, chunks::range range, float val)
{
  auto c = colour{255, 255, 255};
  if (val != nodata_value) {
    auto scaled = val / range.size();
    for (int g = 0; g < gradient.size(); g++) {
      auto pt = gradient[g];
      if (g == gradient.size() - 1) {
        c = pt.colour;
      } else if (scaled >= pt.point && scaled < gradient[g + 1].point) {
        auto pt_next = gradient[g + 1];
        auto n = (scaled - pt.point) / (pt_next.point - pt.point);
        c = colour{
            (uint8_t)(pt.colour.r + (pt_next.colour.r - pt.colour.r) * n),
            (uint8_t)(pt.colour.g + (pt_next.colour.g - pt.colour.g) * n),
            (uint8_t)(pt.colour.b + (pt_next.colour.b - pt.colour.b) * n)};
        break;
      }
    }
  }
  return c;
}

Engine::Engine(
    uint32_t width, uint32_t height, chunks::ChunkCollection &&chunks)
  : win_size_({width, height}), chunks_(chunks)
{
}

Engine::~Engine()
{
  vkWaitForFences(device_, 1, &render_fence_, true, 1e9);

  vmaDestroyBuffer(
      allocator_, mesh_.vertex_buffer.buffer, mesh_.vertex_buffer.allocation);
  vmaDestroyAllocator(allocator_);

  vkDestroyPipeline(device_, pipeline_, nullptr);
  vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);

  vkDestroySemaphore(device_, present_semaphore_, nullptr);
  vkDestroySemaphore(device_, render_semaphore_, nullptr);
  vkDestroyFence(device_, render_fence_, nullptr);

  vkDestroyCommandPool(device_, command_pool_.pool(), nullptr);

  vkDestroySwapchainKHR(device_, swapchain_, nullptr);
  vkDestroyRenderPass(device_, renderpass_, nullptr);
  for (int i = 0; i < framebuffers_.size(); i++) {
    vkDestroyFramebuffer(device_, framebuffers_[i], nullptr);
    vkDestroyImageView(device_, swapchain_image_views_[i], nullptr);
  }

  vkDestroyDevice(device_, nullptr);
  vkDestroySurfaceKHR(instance_, surface_, nullptr);
  vkb::destroy_debug_utils_messenger(instance_, debug_messenger_, nullptr);
  vkDestroyInstance(instance_, nullptr);

  SDL_DestroyWindow(window_);
}

void Engine::init()
{
  SDL_Init(SDL_INIT_VIDEO);
  auto flags = (SDL_WindowFlags)SDL_WINDOW_VULKAN;
  window_ = SDL_CreateWindow("Siliconia", SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED, win_size_.width, win_size_.height, flags);

  init_vulkan();
  init_swapchain();
  init_commands();
  init_default_renderpass();
  init_framebuffers();
  init_sync_structures();
  init_piplines();
  load_meshes();
}

void Engine::run()
{
  auto e = SDL_Event{};
  auto renderer = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);

  uint32_t frame_number = 0;

  while (true) {
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT)
        return;
    }

    VK_CHECK(vkWaitForFences(device_, 1, &render_fence_, true, 1e9));
    VK_CHECK(vkResetFences(device_, 1, &render_fence_));
    uint32_t swapchain_image_index;
    VK_CHECK(vkAcquireNextImageKHR(device_, swapchain_, 1e9, present_semaphore_,
        nullptr, &swapchain_image_index));

    main_command_buffer_.reset();

    {
      auto cmd_guard = main_command_buffer_.begin();
      auto clear_val = VkClearValue{};
      auto flash = std::abs(std::sin(frame_number / 120.f));
      clear_val.color = {{0.0f, 0.0f, flash, 1.0f}};
      auto rp = cmd_guard.begin_render_pass(renderpass_, win_size_,
          framebuffers_[swapchain_image_index], clear_val);
      rp.bind_pipeline(pipeline_);
      rp.bind_vertex_buffers(0, 1, &mesh_.vertex_buffer.buffer);
      rp.draw(mesh_.vertices.size(), 1, 0, 0);
    }

    auto submit_info = VkSubmitInfo{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkPipelineStageFlags wait_stage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &present_semaphore_;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &render_semaphore_;
    submit_info.commandBufferCount = 1;
    auto buf = main_command_buffer_.buffer();
    submit_info.pCommandBuffers = &buf;
    VK_CHECK(vkQueueSubmit(graphics_queue_, 1, &submit_info, render_fence_));

    auto present_info = VkPresentInfoKHR{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pSwapchains = &swapchain_;
    present_info.swapchainCount = 1;
    present_info.pWaitSemaphores = &render_semaphore_;
    present_info.waitSemaphoreCount = 1;
    present_info.pImageIndices = &swapchain_image_index;
    VK_CHECK(vkQueuePresentKHR(graphics_queue_, &present_info));
    frame_number++;

    //        draw(chunks, renderer);
  }
}

void Engine::draw(SDL_Renderer *renderer)
{
  auto chunks = chunks_;
  auto gradient =
      std::array<gradient_point, 2>{{{0.0, {0, 0, 0}}, {1.0, {255, 0, 0}}}};

  auto cell_size = chunks.chunks()[0].cell_size;
  auto texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
      SDL_TEXTUREACCESS_TARGET, chunks.rect.width / cell_size,
      chunks.rect.height / cell_size);

  SDL_SetRenderTarget(renderer, texture);
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);

  auto range = chunks.range;

  for (const auto &chunk : chunks.chunks()) {
    auto x_offset = (chunk.rect().x - chunks.rect.x) / cell_size;
    auto y_offset = chunks.rect.height / cell_size -
                    (chunk.rect().y - chunks.rect.y) / cell_size -
                    chunk.rect().height / cell_size;

    for (unsigned int j = 0; j < chunk.nrows; j++) {
      for (unsigned int i = 0; i < chunk.ncols; i++) {
        auto val = chunk.data[i + j * chunk.ncols];
        if (val != chunk.nodata_value) {
          auto scaled = val / range.size();
          for (int g = 0; g < gradient.size(); g++) {
            auto pt = gradient[g];
            if (g == gradient.size() - 1) {
              SDL_SetRenderDrawColor(
                  renderer, pt.colour.r, pt.colour.g, pt.colour.b, 255);
            } else if (scaled >= pt.point && scaled < gradient[g + 1].point) {
              auto pt_next = gradient[g + 1];
              auto n = (scaled - pt.point) / (pt_next.point - pt.point);
              SDL_SetRenderDrawColor(renderer,
                  pt.colour.r + (pt_next.colour.r - pt.colour.r) * n,
                  pt.colour.g + (pt_next.colour.g - pt.colour.g) * n,
                  pt.colour.b + (pt_next.colour.b - pt.colour.b) * n, 255);
              break;
            }
          }
        } else {
          SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        }
        SDL_RenderDrawPoint(renderer, x_offset + i, y_offset + j);
      }
    }

    break;
  }

  SDL_SetRenderTarget(renderer, nullptr);
  SDL_RenderCopy(renderer, texture, nullptr, nullptr);

  SDL_RenderPresent(renderer);
}

void Engine::init_vulkan()
{
  auto builder = vkb::InstanceBuilder{};
  auto inst_ret = builder.set_app_name("Siliconia")
                      .request_validation_layers(true)
                      .require_api_version(1, 1, 0)
                      .use_default_debug_messenger()
                      .build();
  auto inst = inst_ret.value();
  instance_ = inst.instance;
  debug_messenger_ = inst.debug_messenger;

  SDL_Vulkan_CreateSurface(window_, instance_, &surface_);

  auto selector = vkb::PhysicalDeviceSelector{inst};
  auto physical_device =
      selector.set_minimum_version(1, 1).set_surface(surface_).select().value();
  auto device_builder = vkb::DeviceBuilder{physical_device};
  auto device = device_builder.build().value();
  device_ = device.device;
  chosen_gpu_ = physical_device.physical_device;

  graphics_queue_ = device.get_queue(vkb::QueueType::graphics).value();
  graphics_queue_family_ =
      device.get_queue_index(vkb::QueueType::graphics).value();

  auto allocator_info = VmaAllocatorCreateInfo{};
  allocator_info.physicalDevice = chosen_gpu_;
  allocator_info.device = device_;
  allocator_info.instance = instance_;
  vmaCreateAllocator(&allocator_info, &allocator_);
}

void Engine::init_swapchain()
{
  auto swapchain_builder =
      vkb::SwapchainBuilder{chosen_gpu_, device_, surface_};
  auto swapchain = swapchain_builder.use_default_format_selection()
                       .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                       .set_desired_extent(win_size_.width, win_size_.height)
                       .build()
                       .value();
  swapchain_ = swapchain.swapchain;
  swapchain_images_ = swapchain.get_images().value();
  swapchain_image_views_ = swapchain.get_image_views().value();
  swapchain_image_format_ = swapchain.image_format;
}

void Engine::init_commands()
{
  command_pool_ = vk::CommandPool{device_, graphics_queue_family_};
  main_command_buffer_ = command_pool_.allocate_buffer();
}

void Engine::init_default_renderpass()
{
  auto colour_attachment = VkAttachmentDescription{};
  colour_attachment.format = swapchain_image_format_;
  colour_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colour_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colour_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colour_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colour_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colour_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colour_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  auto colour_attachment_ref = VkAttachmentReference{};
  colour_attachment_ref.attachment = 0;
  colour_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  auto subpass = VkSubpassDescription{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colour_attachment_ref;

  auto renderpass_info = VkRenderPassCreateInfo{};
  renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderpass_info.attachmentCount = 1;
  renderpass_info.pAttachments = &colour_attachment;
  renderpass_info.subpassCount = 1;
  renderpass_info.pSubpasses = &subpass;

  VK_CHECK(
      vkCreateRenderPass(device_, &renderpass_info, nullptr, &renderpass_));
}

void Engine::init_framebuffers()
{
  auto fb_info = VkFramebufferCreateInfo{};
  fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  fb_info.pNext = nullptr;
  fb_info.renderPass = renderpass_;
  fb_info.attachmentCount = 1;
  fb_info.width = win_size_.width;
  fb_info.height = win_size_.height;
  fb_info.layers = 1;

  const auto swapchain_image_count = swapchain_images_.size();
  framebuffers_ = std::vector<VkFramebuffer>(swapchain_image_count);

  for (int i = 0; i < swapchain_image_count; i++) {
    fb_info.pAttachments = &swapchain_image_views_[i];
    VK_CHECK(
        vkCreateFramebuffer(device_, &fb_info, nullptr, &framebuffers_[i]));
  }
}

void Engine::init_sync_structures()
{
  auto fence_create_info = VkFenceCreateInfo{};
  fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_create_info.pNext = nullptr;
  fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  VK_CHECK(vkCreateFence(device_, &fence_create_info, nullptr, &render_fence_));

  auto semaphore_create_info = VkSemaphoreCreateInfo{};
  semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphore_create_info.pNext = nullptr;
  semaphore_create_info.flags = 0;
  VK_CHECK(vkCreateSemaphore(
      device_, &semaphore_create_info, nullptr, &present_semaphore_));
  VK_CHECK(vkCreateSemaphore(
      device_, &semaphore_create_info, nullptr, &render_semaphore_));
}

bool Engine::load_shader_module(const char *path, VkShaderModule *shader_module)
{
  auto file = std::ifstream{path, std::ios::ate | std::ios::binary};
  if (!file.is_open())
    return false;

  auto file_size = file.tellg();
  auto buffer = std::vector<uint32_t>(file_size / sizeof(uint32_t));
  file.seekg(0);
  file.read((char *)buffer.data(), file_size);
  file.close();

  auto create_info = VkShaderModuleCreateInfo{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = buffer.size() * sizeof(uint32_t);
  create_info.pCode = buffer.data();

  if (vkCreateShaderModule(device_, &create_info, nullptr, shader_module) !=
      VK_SUCCESS)
    return false;

  return true;
}

void Engine::init_piplines()
{
  auto frag = VkShaderModule{};
  if (!load_shader_module("../shaders/triangle.frag.spv", &frag)) {
    std::cout << "Could not load frag shader" << std::endl;
  }
  auto vertex = VkShaderModule{};
  if (!load_shader_module("../shaders/triangle.vert.spv", &vertex)) {
    std::cout << "Could not load vert shader" << std::endl;
  }

  auto layout_info = vk::pipeline_layout_create_info();
  VK_CHECK(vkCreatePipelineLayout(
      device_, &layout_info, nullptr, &pipeline_layout_));

  auto builder = vk::PipelineBuilder{};
  builder.shader_stages.push_back(vk::pipeline_shader_stage_create_info(
      VK_SHADER_STAGE_VERTEX_BIT, vertex));
  builder.shader_stages.push_back(vk::pipeline_shader_stage_create_info(
      VK_SHADER_STAGE_FRAGMENT_BIT, frag));
  builder.assembly =
      vk::input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

  builder.viewport.x = 0.0f;
  builder.viewport.y = 0.0f;
  builder.viewport.width = (float)win_size_.width;
  builder.viewport.height = (float)win_size_.height;
  builder.viewport.minDepth = 0.0f;
  builder.viewport.maxDepth = 1.0f;

  builder.scissor.offset = {0, 0};
  builder.scissor.extent = win_size_;

  builder.rasteriser =
      vk::rasterisation_state_create_info(VK_POLYGON_MODE_FILL);
  builder.multisampling = vk::multisample_state_create_info();
  builder.colour_blend_attachment = vk::colour_blend_attachment_state();
  builder.layout = pipeline_layout_;

  auto desc = vk::Vertex::get_vertex_description();
  builder.vertex_input_info = vk::vertex_input_state_create_info();
  builder.vertex_input_info.pVertexAttributeDescriptions =
      desc.attributes.data();
  builder.vertex_input_info.vertexAttributeDescriptionCount =
      desc.attributes.size();
  builder.vertex_input_info.pVertexBindingDescriptions = desc.bindings.data();
  builder.vertex_input_info.vertexBindingDescriptionCount =
      desc.bindings.size();

  pipeline_ = builder.build_pipeline(device_, renderpass_);

  vkDestroyShaderModule(device_, vertex, nullptr);
  vkDestroyShaderModule(device_, frag, nullptr);
}

void Engine::load_meshes()
{
  auto gradient =
      std::array<gradient_point, 2>{{{0.0, {0, 0, 0}}, {1.0, {255, 0, 0}}}};

  auto chunk = chunks_.chunks()[0];
  auto cell_size = chunk.cell_size;
  auto x_sf = 2.0f / ((float)chunk.rect().width / cell_size);
  auto y_sf = 2.0f / ((float)chunk.rect().height / cell_size);
  auto range = chunk.range;
  for (unsigned int j = 0; j < chunk.nrows - 1; j++) {
    for (unsigned int i = 0; i < chunk.ncols - 1; i++) {
      auto vlt = chunk.data[i + j * chunk.ncols];
      auto vrt = chunk.data[i + 1 + j * chunk.ncols];
      auto vrb = chunk.data[i + 1 + (j + 1) * chunk.ncols];
      auto vlb = chunk.data[i + (j + 1) * chunk.ncols];
      auto clt = get_colour(gradient, chunk.nodata_value, range, vlt);
      auto crt = get_colour(gradient, chunk.nodata_value, range, vrt);
      auto crb = get_colour(gradient, chunk.nodata_value, range, vrb);
      auto clb = get_colour(gradient, chunk.nodata_value, range, vlb);

      mesh_.vertices.push_back(vk::Vertex{{i * x_sf - 1, j * y_sf - 1, 0}, clt.to_glm()});
      mesh_.vertices.push_back(vk::Vertex{{(i + 1) * x_sf - 1, j * y_sf - 1, 0}, crt.to_glm()});
      mesh_.vertices.push_back(vk::Vertex{{i * x_sf - 1, (j + 1) * y_sf - 1, 0}, clb.to_glm()});

      mesh_.vertices.push_back(vk::Vertex{{(i + 1) * x_sf - 1, j * y_sf - 1, 0}, crt.to_glm()});
      mesh_.vertices.push_back(vk::Vertex{{(i + 1) * x_sf - 1, (j + 1) * y_sf - 1, 0}, crb.to_glm()});
      mesh_.vertices.push_back(vk::Vertex{{i * x_sf - 1, (j + 1) * y_sf - 1, 0}, clb.to_glm()});
    }
  }

  upload_mesh(mesh_);
}

void Engine::upload_mesh(vk::Mesh &mesh)
{
  auto buffer_info = VkBufferCreateInfo{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = mesh.vertices.size() * sizeof(vk::Vertex);
  buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

  auto alloc_info = VmaAllocationCreateInfo{};
  alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
  VK_CHECK(vmaCreateBuffer(allocator_, &buffer_info, &alloc_info,
      &mesh.vertex_buffer.buffer, &mesh.vertex_buffer.allocation, nullptr));

  void *data;
  vmaMapMemory(allocator_, mesh.vertex_buffer.allocation, &data);
  memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(vk::Vertex));
  vmaUnmapMemory(allocator_, mesh.vertex_buffer.allocation);
}

} // namespace siliconia::graphics
