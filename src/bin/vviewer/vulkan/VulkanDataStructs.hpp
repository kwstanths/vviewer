#ifndef __VulkanDataStructs_hpp__
#define __VulkanDataStructs_hpp__

#include <glm/glm.hpp>

#include "IncludeVulkan.hpp"

struct ModelData {
    glm::mat4 m_modelMatrix;
};

struct MaterialData {
    glm::vec4 albedo; /* RGB: albedo, A: alpha */
    glm::vec4 metallicRoughnessAOEmissive;  /* R = metallic, G = roughness, B = AO, A = emissive */
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
    VkDeviceMemory memory;
    VkImage image;
    VkImageView view;
    VkFormat format;

    void destroy(VkDevice device);
};

#endif