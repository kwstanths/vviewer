#ifndef __VulkanDataStructs_hpp__
#define __VulkanDataStructs_hpp__

#include <cstdint>
#include <optional>
#include <vector>
#include <functional>

#include <glm/glm.hpp>

#include "IncludeVulkan.hpp"
#include "vengine/utils/Hash.hpp"

namespace vengine
{

/* --------------- CPU only structs --------------- */
struct VulkanQueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool hasGraphics() const;
    bool hasPresent() const;
    bool isComplete() const;
};

struct VulkanSwapChainDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

/* Used to create commands from a pool and submit them to a queue */
struct VulkanCommandInfo {
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkCommandPool commandPool;
    VkQueue queue;
};

/* --------------- GPU + CPU structs --------------- */

/* unused */
struct ModelData {
    glm::mat4 m_modelMatrix;
    bool operator==(const ModelData &o) const { return m_modelMatrix == o.m_modelMatrix; };
};

struct MaterialData {
    glm::vec4 albedo = glm::vec4(0, 0, 0, 1);                  /* RGB: albedo, A: alpha */
    glm::vec4 metallicRoughnessAO = glm::vec4(0.5, 0.5, 1, 0); /* R: metallic, G: roughness, B: AO, A: is transparent */
    glm::vec4 emissive = glm::vec4(1, 1, 1, 0);                /* RGB: emissive color, A: emissive intensity */
    glm::uvec4 gTexturesIndices1 = glm::vec4(0, 0, 0, 0); /* R: albedo texture index, G: metallic texture index, B: roughness texture
                          index, A: AO texture index */
    glm::uvec4 gTexturesIndices2 =
        glm::vec4(0, 0, 0, 0); /* R: emissive texture index, G: normal texture index, B: BRDF LUT texture index, A: alpha texture */
    glm::vec4 uvTiling = glm::vec4(1, 1, 0, 0); /* R: u tiling, G: v tiling, B: material type, A: unused */

    glm::uvec4 padding1;
    glm::uvec4 padding2;
}; /* sizeof(MaterialData) = 128 */

struct LightData {
    glm::vec4 color; /* RGB: color, A: intensity */
    glm::uvec4 type; /* R = type (LightType), GBA: unused */

    glm::uvec4 padding1;
    glm::uvec4 padding2;
}; /* sizeof(LightData) = 64 */

struct PushBlockLightComposition {
    glm::uvec4 lights; /* R = light index, G = unused */
};

struct PushBlockForward {
    glm::vec4 selected; /* R = ID of object */
    glm::uvec4 info;    /* R = Material index, G = InstanceData index, B = Total lights, A = unused */
    glm::uvec4 lights;  /* R = light 1, G = light 2, B = light 3, A = light 4*/
};

struct PushBlockOverlayTransform3D {
    glm::mat4 modelMatrix;
    glm::vec4 color; /* RGB = color, A = selected */
};

struct PushBlockOverlayAABB3 {
    glm::vec4 min;      /* RGB = min point of AABB, A = unused */
    glm::vec4 max;      /* RGB = max point of AABB, A = unused */
    glm::vec4 color;    /* RGB = color of the UI, A = unused */
    glm::vec4 selected; /* RGB = ID of object, A = if object is selected */
};

struct PushBlockOverlayOutline {
    glm::mat4 modelMatrix;  /* model matrix */
    glm::vec4 color;        /* RGB = color, A = scale */
};

struct PushBlockOutput {
    glm::vec4 info;
};

}  // namespace vengine

/* Hash function for ModeData struct */
template <>
struct std::hash<vengine::ModelData> {
    size_t operator()(const vengine::ModelData &m) const { return Hash(m); }
};

#endif