#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "engine.hpp"
#include "VkBootstrap.h"
#include "vk/helpers.hpp"
#include "vk/pipeline_builder.hpp"
#include <SDL_vulkan.h>
#include <array>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_vulkan.h>

namespace siliconia::graphics {

struct colour {
  colour(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}

  glm::vec3 to_glm() { return {r / 255.0, g / 255.0, b / 255.0}; }

  uint8_t r, g, b;
};

struct gradient_point {
  gradient_point(float point, colour colour) : point(point), colour(colour) {}

  float point;
  colour colour;
};

template <size_t N>
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
  : win_size_({width, height})
  , chunks_(chunks)
  , camera_({100.f, -200.f, -100.f}, {5.0 * 250, 0.0, 5 * 250}, {0.f, 1.f, 0.f})
{
}

Engine::~Engine()
{
  vkWaitForFences(device_, 1, &render_fence_, true, 1e9);

  vkDestroyDescriptorPool(device_, imgui_pool_, nullptr);
  ImGui_ImplVulkan_Shutdown();

  for (const auto &mesh : meshes_) {
    vmaDestroyBuffer(
        allocator_, mesh.index_buffer.buffer, mesh.index_buffer.allocation);
    vmaDestroyBuffer(
        allocator_, mesh.vertex_buffer.buffer, mesh.vertex_buffer.allocation);
  }

  vkDestroyPipeline(device_, pipeline_, nullptr);
  vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);

  vkDestroySemaphore(device_, present_semaphore_, nullptr);
  vkDestroySemaphore(device_, render_semaphore_, nullptr);
  vkDestroyFence(device_, render_fence_, nullptr);
  vkDestroyFence(device_, upload_context_.upload_fence, nullptr);

  vkDestroyCommandPool(device_, command_pool_.pool(), nullptr);
  vkDestroyCommandPool(device_, upload_context_.command_pool.pool(), nullptr);

  vkDestroyImageView(device_, depth_image_view_, nullptr);
  vmaDestroyImage(allocator_, depth_image_.image, depth_image_.allocation);

  vmaDestroyAllocator(allocator_);

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
  init_pipelines();
  load_meshes();
  init_imgui();
}

void Engine::run()
{
  auto e = SDL_Event{};

  uint32_t frame_number = 0;

  auto start = std::chrono::system_clock::now();

  std::cout << "Running" << std::endl;

  while (true) {
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        return;
      }
      ImGui_ImplSDL2_ProcessEvent(&e);
      camera_.handle_input(e);
    }

    camera_.update();

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame(window_);
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();


    ImGui::Render();

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

      auto clear_depth_val = VkClearValue{};
      clear_depth_val.depthStencil.depth = 1.0f;

      auto clears = std::array{clear_val, clear_depth_val};
      auto rp = cmd_guard.begin_render_pass(
          renderpass_, win_size_, framebuffers_[swapchain_image_index], clears);
      rp.bind_pipeline(pipeline_);

      auto view = camera_.matrix();
      auto proj =
          glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 10000000000.0f);

      for (const auto &mesh : meshes_) {
        rp.bind_vertex_buffers(0, 1, &mesh.vertex_buffer.buffer);
        rp.bind_index_buffer(mesh.index_buffer.buffer);
        auto constant = vk::MeshPushConstants{proj * view * mesh.model_matrix};
        rp.push_constants(pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT,
            sizeof(vk::MeshPushConstants), &constant);
        rp.draw_indexed(mesh.indices.size(), 1, 0, 0, 0);
      }

      ImGui_ImplVulkan_RenderDrawData(
          ImGui::GetDrawData(), main_command_buffer_.buffer());
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

    if (frame_number % 30 == 0) {
      auto now = std::chrono::system_clock::now();
      auto elapsed = now - start;
      auto mili =
          std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
      std::cout << "fps: " << 30.0 / ((float)mili.count() / 1000) << std::endl;
      start = now;
    }
  }
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

  auto depth_image_extent = VkExtent3D{win_size_.width, win_size_.height, 1};
  depth_format_ = VK_FORMAT_D32_SFLOAT;
  auto dimg_info = vk::image_create_info(depth_format_,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depth_image_extent);

  auto dimg_alloc_info = VmaAllocationCreateInfo{};
  dimg_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  dimg_alloc_info.requiredFlags =
      VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  vmaCreateImage(allocator_, &dimg_info, &dimg_alloc_info, &depth_image_.image,
      &depth_image_.allocation, nullptr);

  auto dview_info = vk::image_view_create_info(
      depth_format_, depth_image_.image, VK_IMAGE_ASPECT_DEPTH_BIT);

  VK_CHECK(
      vkCreateImageView(device_, &dview_info, nullptr, &depth_image_view_));
}

void Engine::init_commands()
{
  command_pool_ = vk::CommandPool{device_, graphics_queue_family_};
  main_command_buffer_ = command_pool_.allocate_buffer();

  upload_context_.command_pool =
      vk::CommandPool{device_, graphics_queue_family_};
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

  auto depth_attachment = VkAttachmentDescription{};
  depth_attachment.format = depth_format_;
  depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth_attachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  auto depth_attachment_ref = VkAttachmentReference{};
  depth_attachment_ref.attachment = 1;
  depth_attachment_ref.layout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  auto subpass = VkSubpassDescription{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colour_attachment_ref;
  subpass.pDepthStencilAttachment = &depth_attachment_ref;

  VkAttachmentDescription attachments[2] = {
      colour_attachment, depth_attachment};
  auto renderpass_info = VkRenderPassCreateInfo{};
  renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderpass_info.attachmentCount = 2;
  renderpass_info.pAttachments = &attachments[0];
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
    VkImageView attachments[2] = {swapchain_image_views_[i], depth_image_view_};
    fb_info.pAttachments = &attachments[0];
    fb_info.attachmentCount = 2;
    VK_CHECK(
        vkCreateFramebuffer(device_, &fb_info, nullptr, &framebuffers_[i]));
  }
}

void Engine::init_sync_structures()
{
  auto fence_create_info = VkFenceCreateInfo{};
  fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  VK_CHECK(vkCreateFence(device_, &fence_create_info, nullptr, &render_fence_));

  auto semaphore_create_info = VkSemaphoreCreateInfo{};
  semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VK_CHECK(vkCreateSemaphore(
      device_, &semaphore_create_info, nullptr, &present_semaphore_));
  VK_CHECK(vkCreateSemaphore(
      device_, &semaphore_create_info, nullptr, &render_semaphore_));

  auto upload_fence_create_info = VkFenceCreateInfo{};
  upload_fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  VK_CHECK(vkCreateFence(device_, &upload_fence_create_info, nullptr,
      &upload_context_.upload_fence));
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

void Engine::init_pipelines()
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

  auto push_constant = VkPushConstantRange{};
  push_constant.size = sizeof(vk::MeshPushConstants);
  push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  layout_info.pPushConstantRanges = &push_constant;
  layout_info.pushConstantRangeCount = 1;

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

  builder.depth_stencil =
      vk::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

  pipeline_ = builder.build_pipeline(device_, renderpass_);

  vkDestroyShaderModule(device_, vertex, nullptr);
  vkDestroyShaderModule(device_, frag, nullptr);
}

void Engine::init_imgui()
{
  VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  auto pool_info = VkDescriptorPoolCreateInfo{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 1000;
  pool_info.poolSizeCount = std::size(pool_sizes);
  pool_info.pPoolSizes = pool_sizes;

  VK_CHECK(vkCreateDescriptorPool(device_, &pool_info, nullptr, &imgui_pool_));

  ImGui::CreateContext();

  // this initializes imgui for SDL
  ImGui_ImplSDL2_InitForVulkan(window_);

  auto init_info = ImGui_ImplVulkan_InitInfo{};
  init_info.Instance = instance_;
  init_info.PhysicalDevice = chosen_gpu_;
  init_info.Device = device_;
  init_info.Queue = graphics_queue_;
  init_info.DescriptorPool = imgui_pool_;
  init_info.MinImageCount = 3;
  init_info.ImageCount = 3;

  ImGui_ImplVulkan_Init(&init_info, renderpass_);

  immediate_submit([&](auto buf) {
    ImGui_ImplVulkan_CreateFontsTexture(buf);
  });

  ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void Engine::load_meshes()
{
  auto gradient =
      std::array<gradient_point, 2>{{{0.0, {0, 0, 0}}, {1.0, {255, 0, 0}}}};

  auto cell_size = chunks_.chunks()[0].cell_size;
  auto range = chunks_.range;
  auto x_sf = 2.0f / ((float)chunks_.rect.width / cell_size);
  auto z_sf = 2.0f / ((float)chunks_.rect.height / cell_size);

  for (const auto &chunk : chunks_.chunks()) {
    auto mesh = vk::Mesh{};
    auto x_offset = (chunk.rect().x - chunks_.rect.x) / cell_size;
    auto z_offset = chunks_.rect.height / cell_size -
                    (chunk.rect().y - chunks_.rect.y) / cell_size -
                    chunk.rect().height / cell_size;

    for (unsigned int j = 0; j < chunk.nrows; j++) {
      for (unsigned int i = 0; i < chunk.ncols; i++) {
        auto v = chunk.data[i + j * chunk.ncols];
        auto c = get_colour(gradient, chunk.nodata_value, range, v);

        auto vert = vk::Vertex(glm::vec3{i, -v, j}, c.to_glm());
        mesh.vertices.push_back(std::move(vert));

        if (i < chunk.ncols - 1 && j < chunk.nrows - 1) {
          mesh.indices.push_back(i + j * chunk.ncols);       // tl
          mesh.indices.push_back((i + 1) + j * chunk.ncols); // tr
          mesh.indices.push_back(i + (j + 1) * chunk.ncols); // bl

          mesh.indices.push_back((i + 1) + j * chunk.ncols);       // tr
          mesh.indices.push_back((i + 1) + (j + 1) * chunk.ncols); // br
          mesh.indices.push_back(i + (j + 1) * chunk.ncols);       // bl
        }
      }
    }
    mesh.model_matrix = glm::translate(glm::mat4(1), {x_offset, 0, z_offset});

    upload_mesh(mesh);
    meshes_.push_back(std::move(mesh));
  }
}

void Engine::upload_mesh(vk::Mesh &mesh)
{
  // Vertices
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

  // Indices
  buffer_info.size = mesh.indices.size() * sizeof(uint32_t);
  buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  VK_CHECK(vmaCreateBuffer(allocator_, &buffer_info, &alloc_info,
      &mesh.index_buffer.buffer, &mesh.index_buffer.allocation, nullptr));

  vmaMapMemory(allocator_, mesh.index_buffer.allocation, &data);
  memcpy(data, mesh.indices.data(), mesh.indices.size() * sizeof(uint32_t));
  vmaUnmapMemory(allocator_, mesh.index_buffer.allocation);
}

void Engine::immediate_submit(std::function<void(VkCommandBuffer)> &&function)
{
  auto buf = upload_context_.command_pool.allocate_buffer();

  {
    auto guard = buf.begin();
    function(buf.buffer());
  }

  auto submit_info = VkSubmitInfo{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  auto b = buf.buffer();
  submit_info.pCommandBuffers = &b;
  VK_CHECK(vkQueueSubmit(
      graphics_queue_, 1, &submit_info, upload_context_.upload_fence));

  vkWaitForFences(device_, 1, &upload_context_.upload_fence, true, 9999999999);
  vkResetFences(device_, 1, &upload_context_.upload_fence);

  upload_context_.command_pool.reset();
}

} // namespace siliconia::graphics
