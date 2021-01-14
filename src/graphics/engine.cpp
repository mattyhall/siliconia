#include "engine.hpp"
#include <array>
#include <chunks/chunk_collection.hpp>

namespace siliconia::graphics {

struct colour {
  colour(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
  uint8_t r, g, b;
};

struct gradient_point {
  gradient_point(float point, colour colour) : point(point), colour(colour) {}

  float point;
  colour colour;
};

Engine::Engine(uint32_t width, uint32_t height)
  : win_size_({width, height}), window_(nullptr)
{
}

Engine::~Engine()
{
  SDL_DestroyWindow(window_);
}

void Engine::init()
{
  SDL_Init(SDL_INIT_VIDEO);
  auto flags = (SDL_WindowFlags)SDL_WINDOW_VULKAN;
  window_ = SDL_CreateWindow("Siliconia", SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED, win_size_.width, win_size_.height, flags);
}

void Engine::run(const siliconia::chunks::ChunkCollection &chunks)
{
  auto e = SDL_Event{};
  auto renderer = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);

  while (true) {
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT)
        return;
    }

    draw(chunks, renderer);
  }
}

void Engine::draw(
    const siliconia::chunks::ChunkCollection &chunks, SDL_Renderer *renderer)
{
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
  }

  SDL_SetRenderTarget(renderer, nullptr);
  SDL_RenderCopy(renderer, texture, nullptr, nullptr);

  SDL_RenderPresent(renderer);
}

} // namespace siliconia::graphics
