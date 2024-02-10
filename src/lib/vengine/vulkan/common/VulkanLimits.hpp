#ifndef __VulkanLimits_hpp__
#define __VulkanLimits_hpp__

#include <cstdint>

namespace vengine
{

static constexpr uint32_t VULKAN_LIMITS_MAX_OBJECTS = 16384;          /* This limit is bound by the Vulkan Storage buffer size */
static constexpr uint32_t VULKAN_LIMITS_MAX_UNIQUE_TRANSFORMS = 1024; /* These limits are bound by the Vulkan Uniform buffer size */
static constexpr uint32_t VULKAN_LIMITS_MAX_MATERIALS = 512;
static constexpr uint32_t VULKAN_LIMITS_MAX_TEXTURES = 1024;
static constexpr uint32_t VULKAN_LIMITS_MAX_UNIQUE_LIGHTS = 1024;
static constexpr uint32_t VULKAN_LIMITS_MAX_LIGHT_INSTANCES = 1024;

}  // namespace vengine

#endif
