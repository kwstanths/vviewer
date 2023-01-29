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

/* ------------------ Ray Tracing structs ------------------------- */

/* Objects description data for meshes in the scene */
struct ObjectDescriptionRT
{
    /* A pointer to a buffer holding mesh vertex data */
    uint64_t vertexAddress;
    /* A pointer to a buffer holding mesh index data*/
    uint64_t indexAddress;
    /* An index to point to the materials buffer */
    int materialIndex;
};

struct StorageImage
{
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkFormat format;

    void destroy(VkDevice device);
};

/* Types of lights:
    0: point light
    1: directional light
    2: mesh light
    3: environment map
*/
struct LightRT {
    glm::vec4 position;  /* RGB = world space position, A = light type  */
    glm::vec4 direction; /* RGB = world space direction, A = mesh id */
    glm::vec4 color;     /* RGB = color, A = mesh material id */
};

struct MaterialRT {
    glm::vec4 albedo;
};

#endif