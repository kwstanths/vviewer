#ifndef __IncludeVulkan_hpp__
#define __IncludeVulkan_hpp__

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "debug_tools/Console.hpp"

namespace vengine
{

#ifndef NDEBUG
#define VULKAN_WARNING(VK_COMMAND)                                                                                             \
    if (VkResult VK_RESULT = VK_COMMAND; VK_RESULT != VK_SUCCESS) {                                                            \
        debug_tools::ConsoleWarning(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " " + std::to_string(VK_RESULT)); \
    }
#define VULKAN_CHECK(VK_COMMAND)                                    \
    if (VkResult VK_RESULT = VK_COMMAND; VK_RESULT != VK_SUCCESS) { \
        return VK_RESULT;                                           \
    }
#define VULKAN_CHECK_CRITICAL(VK_COMMAND)                                                                                       \
    if (VkResult VK_RESULT = VK_COMMAND; VK_RESULT != VK_SUCCESS) {                                                             \
        debug_tools::ConsoleCritical(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " " + std::to_string(VK_RESULT)); \
        return VK_RESULT;                                                                                                       \
    }
#define VULKAN_CHECK_FATAL(VK_COMMAND)                                                                                       \
    if (VkResult VK_RESULT = VK_COMMAND; VK_RESULT != VK_SUCCESS) {                                                          \
        debug_tools::ConsoleFatal(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " " + std::to_string(VK_RESULT)); \
        return VK_RESULT;                                                                                                    \
    }

#else
#define VULKAN_CHECK(VK_COMMAND) VK_COMMAND
#define VULKAN_WARNING(VK_COMMAND) VK_COMMAND
#define VULKAN_CHECK_CRITICAL(VK_COMMAND) VK_COMMAND
#define VULKAN_CHECK_FATAL(VK_COMMAND) VK_COMMAND
#endif

}  // namespace vengine

#endif