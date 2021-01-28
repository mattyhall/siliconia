#ifndef SILICONIA_CAMERA_HPP
#define SILICONIA_CAMERA_HPP

#include <SDL.h>
#include <glm/glm.hpp>
#include <array>

namespace siliconia::graphics {
class camera {
public:
  camera(glm::vec3 pos, glm::vec3 look_at, glm::vec3 up);

  void handle_input(const SDL_Event &e);
  void update();
  glm::mat4 matrix() const;

private:
  glm::vec3 pos_;
  glm::vec3 dir_;
  glm::vec3 up_;

  enum class Keys { left = 0, right, forward, back, space, shift, max};
  std::array<bool, static_cast<size_t>(Keys::max)> key_states_;
};

} // namespace siliconia::graphics

#endif // SILICONIA_CAMERA_HPP
