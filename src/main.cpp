#include "chunks/chunk.hpp"
#include "chunks/chunk_collection.hpp"
#include <SDL.h>
#include <array>
#include <iostream>
#include <stdio.h>

struct colour {
  colour(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
  uint8_t r, g, b;
};

struct gradient_point {
  gradient_point(double point, colour colour) : point(point), colour(colour) {}

  double point;
  colour colour;
};

void main_loop(
    const siliconia::chunks::ChunkCollection &chunks, SDL_Renderer *renderer)
{
  auto gradient =
      std::array<gradient_point, 2>{{{0.0, {0, 0, 0}}, {1.0, {255, 0, 0}}}};

  auto cell_size = chunks.chunks()[0].cell_size;
  auto texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
      SDL_TEXTUREACCESS_TARGET, chunks.rect.width / cell_size,
      chunks.rect.height / cell_size);

  while (true) {
    auto e = SDL_Event{};
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        return;
      }
    }

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
          if (val) {
            auto scaled = *val / range.size();
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
}

int main(int argc, char **argv)
{
  try {
    auto chunks = siliconia::chunks::ChunkCollection{
        "C:/Users/matth/Documents/lidar/TL45nw/"};
    auto r = chunks.rect;
    std::cout << "(" << r.x << ", " << r.y << ") " << r.width << "x" << r.height
              << " " << chunks.range.min << "-" << chunks.range.max
              << std::endl;

    SDL_Init(SDL_INIT_VIDEO);
    auto win = SDL_CreateWindow("Siliconia", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, 1200, 1200, 0);
    auto surface = SDL_GetWindowSurface(win);
    auto renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    main_loop(chunks, renderer);
  } catch (const siliconia::chunks::asc_parse_exception &e) {
    std::cout << e.what() << std::endl;
    return 1;
  }

  return 0;
}