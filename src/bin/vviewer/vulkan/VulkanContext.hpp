#ifndef __VulkanContext_hpp__
#define __VulkanContext_hpp__

#include <vector>

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/common/VulkanStructs.hpp"

namespace vengine
{

static const std::vector<const char *> VULKAN_VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};

static const std::vector<const char *> VULKAN_INSTANCE_EXTENSIONS = {"VK_EXT_debug_report",
                                                                     "VK_EXT_debug_utils",
                                                                     "VK_KHR_get_physical_device_properties2",
                                                                     "VK_KHR_surface",
#ifdef __linux__
                                                                     "VK_KHR_wayland_surface",
                                                                     "VK_KHR_xcb_surface",
                                                                     "VK_KHR_xlib_surface"
#elif _WIN32
                                                                     error TODO
#endif
};

/* true means mandatory */
static const std::vector<std::pair<const char *, bool>> VULKAN_DEVICE_EXTENSIONS = {
    {"VK_KHR_swapchain", true},               /* Presentation extension, is mandatory */
    {"VK_KHR_acceleration_structure", false}, /* Ray tracing extensions, not mandatory */
    {"VK_KHR_ray_tracing_pipeline", false},
    {"VK_KHR_buffer_device_address", false},
    {"VK_KHR_deferred_host_operations", false},
    {"VK_EXT_descriptor_indexing", true},
    {"VK_KHR_spirv_1_4", false},
    {"VK_KHR_shader_float_controls", false},
    {"VK_KHR_maintenance3", false},
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                    void *pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << pCallbackData->pMessage << std::endl;
    }
    return VK_FALSE;
}

/* Encapsulates the vulkan instance, physical device and logical device creation */
class VulkanContext
{
public:
    VulkanContext(const std::string &applicationName);
    ~VulkanContext();

    bool init(VkSurfaceKHR surface);
    void destroy();

    VkInstance &instance() { return m_instance; }

    VkPhysicalDevice &physicalDevice() { return m_physicalDevice; }
    VkPhysicalDeviceProperties &physicalDeviceProperties() { return m_physicalDeviceProperties; }
    VkDevice &device() { return m_device; }

    VkSurfaceKHR &surface() { return m_surface; }
    QueueFamilyIndices &queueFamilyIndices() { return m_queueFamilyIndices; }

    VkQueue &graphicsQueue() { return m_graphicsQueue; }
    VkQueue &presentQueue() { return m_presentQueue; }
    VkQueue &renderQueue() { return m_renderQueue; }

    VkCommandPool &graphicsCommandPool() { return m_graphicsCommandPool; }
    VkCommandPool &renderCommandPool() { return m_renderCommandPool; }

    VkSampleCountFlagBits &msaaSamples() { return m_msaaSamples; }

private:
    bool m_initialized = false;

    VkInstance m_instance = VK_NULL_HANDLE;

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

    VkResult createVulkanInstance(const std::string &applicationName);
    void createDebugCallback();
    void destroyDebugCallback();

    bool pickPhysicalDevice();
    bool isPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties physicalDeviceProperties);
    VkSampleCountFlagBits getMaxUsableSampleCount(const VkPhysicalDeviceProperties &deviceProperties);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    bool createLogicalDevice();
    void createCommandPool();
};

}  // namespace vengine

#endif
