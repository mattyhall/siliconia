#ifndef SILICONIA_HELPERS_HPP
#define SILICONIA_HELPERS_HPP

#include <iostream>

#define VK_CHECK(x)                                                            \
  do {                                                                         \
    VkResult err = x;                                                          \
    if (err) {                                                                 \
      std::cout << "Detected Vulkan error: " << err << std::endl;              \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#endif // SILICONIA_HELPERS_HPP
