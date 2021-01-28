#include "camera.hpp"
#include <SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace siliconia::graphics {

camera::camera(glm::vec3 pos, glm::vec3 look_at, glm::vec3 up)
  : pos_(pos), dir_(glm::normalize(look_at - pos)), up_(up), key_states_{false}
{
}
glm::mat4 camera::matrix() const
{
  return glm::lookAt(pos_, pos_ + dir_, up_);
}

void camera::handle_input(const SDL_Event &e)
{
#define HANDLE(sdl_key, dir)                                                   \
  if (e.key.keysym.sym == sdl_key) {                                           \
    key_states_[static_cast<size_t>(dir)] = e.type == SDL_KEYDOWN;             \
  }

  if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
    HANDLE(SDLK_w, Keys::forward)
    HANDLE(SDLK_s, Keys::back)
    HANDLE(SDLK_a, Keys::left)
    HANDLE(SDLK_d, Keys::right)
    HANDLE(SDLK_SPACE, Keys::space)
  }
  auto mod_state = SDL_GetModState();
  key_states_[static_cast<size_t>(Keys::shift)] = mod_state & KMOD_LSHIFT;
}

void camera::update()
{
  auto orth_dir = glm::cross(dir_, up_);
  auto move_dir = glm::vec3{0};
  if (key_states_[static_cast<size_t>(Keys::forward)]) {
    move_dir += dir_;
  }
  if (key_states_[static_cast<size_t>(Keys::back)]) {
    move_dir -= dir_;
  }
  if (key_states_[static_cast<size_t>(Keys::left)]) {
    move_dir -= orth_dir;
  }
  if (key_states_[static_cast<size_t>(Keys::right)]) {
    move_dir += orth_dir;
  }
  if (key_states_[static_cast<size_t>(Keys::space)]) {
    move_dir -= up_;
  }
  if (key_states_[static_cast<size_t>(Keys::shift)]) {
    move_dir += up_;
  }
  if (move_dir != glm::vec3{0.f, 0.f, 0.f}) {
    pos_ += glm::normalize(move_dir);
  }
}

} // namespace siliconia::graphics
