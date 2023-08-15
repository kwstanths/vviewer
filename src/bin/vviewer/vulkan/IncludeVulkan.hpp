#ifndef __IncludeVulkan_hpp__
#define __IncludeVulkan_hpp__

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "utils/Console.hpp"

#ifndef NDEBUG 
    #define VULKAN_CHECK(VK_RESULT) if (VK_RESULT != VK_SUCCESS) { utils::ConsoleWarning(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " " + std::to_string(VK_RESULT)); }
#else
    #define VULKAN_CHECK(VK_RESULT) 
#endif

#endif