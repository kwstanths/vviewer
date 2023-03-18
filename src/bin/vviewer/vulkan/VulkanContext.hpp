#ifndef __VulkanCore_hpp__
#define __VulkanCore_hpp__

#include <vector>

#include <qvulkaninstance.h>
#include <qvulkanfunctions.h>

#include "IncludeVulkan.hpp"
#include "VulkanStructs.hpp"

static const std::vector<const char*> VULKAN_CORE_VALIDATION_LAYERS = 
{
    "VK_LAYER_KHRONOS_validation"
};

static const std::vector<const char*> VULKAN_CORE_INSTANCE_EXTENSIONS = 
{
    "VK_EXT_debug_utils",
    "VK_KHR_get_physical_device_properties2"
};

static const std::vector<std::pair<const char*, bool>> VULKAN_CORE_DEVICE_EXTENSIONS = 
{
    {"VK_KHR_swapchain", true},                 /* Presentation extension, is mandatory */
    {"VK_KHR_acceleration_structure", false},   /* Ray tracing extensions, not mandatory */
    {"VK_KHR_ray_tracing_pipeline", false},
    {"VK_KHR_buffer_device_address",  false},
    {"VK_KHR_deferred_host_operations", false},
    {"VK_EXT_descriptor_indexing", false},
    {"VK_KHR_spirv_1_4", false },
    {"VK_KHR_shader_float_controls", false},
    {"VK_KHR_maintenance3", false},
}; 

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) 
    {
        //std::cerr << "Debug callback: " << pCallbackData->pMessage << std::endl;
    }
    return VK_FALSE;
}


/* Encapsulates the vulkan instance, physical device and logical device creation */
class VulkanContext {
public:
    VulkanContext();
    ~VulkanContext();

    bool init(VkSurfaceKHR surface);
    void destroy();

    QVulkanInstance* instance() { return m_vulkanInstance; }

    VkPhysicalDevice& physicalDevice() { return m_physicalDevice; }
    VkPhysicalDeviceProperties& physicalDeviceProperties() { return m_physicalDeviceProperties; }
    VkDevice& device() { return m_device; }

    VkSurfaceKHR& surface() { return m_surface; }
    QueueFamilyIndices& queueFamilyIndices() { return m_queueFamilyIndices; }

    QVulkanFunctions * functions() { return m_vkFunctions; }
    QVulkanDeviceFunctions * deviceFunctions() { return m_vkDeviceFunctions; }

    VkQueue& graphicsQueue() { return m_graphicsQueue; }
    VkQueue& presentQueue() { return m_presentQueue; }
    VkQueue& renderQueue() { return m_renderQueue; }

    VkCommandPool& graphicsCommandPool() { return m_graphicsCommandPool; }
    VkCommandPool& renderCommandPool() { return m_renderCommandPool; }

    VkSampleCountFlagBits& msaaSamples() { return m_msaaSamples; }

private:
    bool m_initialized = false;

    QVulkanInstance * m_vulkanInstance = nullptr;
    QVulkanFunctions * m_vkFunctions = nullptr;
    QVulkanDeviceFunctions * m_vkDeviceFunctions = nullptr;
    
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

    /* Device data */
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties m_physicalDeviceProperties;
    VkDevice m_device;
    VkSampleCountFlagBits m_msaaSamples;
    VkDebugUtilsMessengerEXT m_debugCallback;

    /* Queues */
    QueueFamilyIndices m_queueFamilyIndices;
    VkQueue m_graphicsQueue, m_presentQueue, m_renderQueue;

    /* Command pools */
    VkCommandPool m_graphicsCommandPool;
    VkCommandPool m_renderCommandPool;

    void createVulkanInstance();
    void createDebugCallback();
    void destroyDebugCallback();

    bool pickPhysicalDevice();
    bool isPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties physicalDeviceProperties);
    VkSampleCountFlagBits getMaxUsableSampleCount(const VkPhysicalDeviceProperties& deviceProperties);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    bool createLogicalDevice();
    void createCommandPool();
};

#endif
