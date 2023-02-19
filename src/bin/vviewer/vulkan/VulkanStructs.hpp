#ifndef __VulkanDataStructs_hpp__
#define __VulkanDataStructs_hpp__

#include <optional>
#include <vector>

#include <glm/glm.hpp>

#include "IncludeVulkan.hpp"

/* Stores supported queue family indices */
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete();
};

/* Stores supported swapchain details */
struct SwapChainDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct ModelData {
    glm::mat4 m_modelMatrix;
};

struct MaterialData {
    glm::vec4 albedo; /* RGB: albedo, A: alpha */
    glm::vec4 metallicRoughnessAOEmissive;  /* R = metallic, G = roughness, B = AO, A = emissive */
    glm::vec4 uvTiling; /* R = u tiling, G = v tiling, B = unused, A = unused */
};

struct PushBlockForwardBasePass {
    glm::vec4 selected; /* RGB = ID of object, A = if object is selected */
};

struct PushBlockForwardAddPass {
    glm::vec4 lightPosition; /* RGB = World space light position,  A = unused */
    glm::vec4 lightColor;   /* RGB = light color, A = unused */
};

struct PushBlockForward3DUI {
    glm::mat4 modelMatrix;
    glm::vec4 color;    /* RGB = color of the UI, A = unused */
    glm::vec4 selected; /* RGB = ID of object, A = if object is selected */
};

struct StorageImage
{
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkFormat format;

    void destroy(VkDevice device);
};

#endif