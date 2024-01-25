#ifndef __VulkanLimits_hpp__
#define __VulkanLimits_hpp__

#include <cstdint>

namespace vengine
{

static constexpr uint32_t VULKAN_LIMITS_MAX_OBJECTS = 1024;
static constexpr uint32_t VULKAN_LIMITS_MAX_MATERIALS = 200;
static constexpr uint32_t VULKAN_LIMITS_MAX_TEXTURES = 500;
static constexpr uint32_t VULKAN_LIMITS_MAX_UNIQUE_LIGHTS = 200;
static constexpr uint32_t VULKAN_LIMITS_MAX_LIGHTS = 200; /* Affects PT only */

}  // namespace vengine

#endif
