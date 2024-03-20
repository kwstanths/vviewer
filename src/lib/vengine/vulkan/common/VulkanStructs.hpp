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

struct VulkanStorageImage {
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkFormat format;

    void destroy(VkDevice device);
};

/* Used to create commands from a pool and submit them to a queue */
struct VulkanCommandInfo {
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkCommandPool commandPool;
    VkQueue queue;
};

/* --------------- GPU + CPU structs --------------- */
struct ModelData {
    glm::mat4 m_modelMatrix;
    bool operator==(const ModelData &o) const { return m_modelMatrix == o.m_modelMatrix; };
};

struct MaterialData {
    glm::vec4 albedo;              /* RGB: albedo, A: alpha */
    glm::vec4 metallicRoughnessAO; /* R: metallic, G: roughness, B: AO, A: unused */
    glm::vec4 emissive;            /* RGB: emissive color, A: emissive intensity */
    glm::uvec4 gTexturesIndices1;  /* R: albedo texture index, G: metallic texture index, B: roughness texture index, A: AO texture
                            index */
    glm::uvec4 gTexturesIndices2; /* R: emissive texture index, G: normal texture index, B: BRDF LUT texture index, A: alpha texture */
    glm::vec4 uvTiling;           /* R: u tiling, G: v tiling, B: unused, A: unused */

    glm::uvec4 padding1;
    glm::uvec4 padding2;
}; /* sizeof(MaterialData) = 128 */

struct LightData {
    glm::vec4 color; /* RGB: color, A: intensity */
    glm::uvec4 type; /* R = type (LightType), GBA: unused */

    glm::uvec4 padding1;
    glm::uvec4 padding2;
}; /* sizeof(LightData) = 64 */

struct LightInstance {
    glm::uvec4 info;    /* R = LightData index, G = ModelData index, B = Object Description index if Mesh type, A = type (LightType) */
    glm::vec4 position; /* RGB = world position/direction, A = casts shadow or RGBA = row 0 of transform matrix if mesh type */
    glm::vec4 position1; /* RGBA = row 1 of transform matrix if mesh type */
    glm::vec4 position2; /* RGBA = row 2 of transform matrix if mesh type */
};                       /* sizeof(LightInstance) = 64 */

struct PushBlockForward {
    glm::vec4 selected; /* RGB = ID of object, A = if object is selected */
    glm::uvec4 info;    /* R = Material index, G = model index, B = total lights, A = unused */
    glm::uvec4 lights;  /* R = light 1, G = light 2, B = light 3, A = light 4*/
};

struct PushBlockForward3DUITransform {
    glm::mat4 modelMatrix;
    glm::vec4 color;    /* RGB = color of the UI, A = unused */
    glm::vec4 selected; /* RGB = ID of object, A = if object is selected */
};

struct PushBlockForward3DUIAABB {
    glm::vec4 min;      /* RGB = min point of AABB, A = unused */
    glm::vec4 max;      /* RGB = max point of AABB, A = unused */
    glm::vec4 color;    /* RGB = color of the UI, A = unused */
    glm::vec4 selected; /* RGB = ID of object, A = if object is selected */
};

/* Objects description data for meshes in the scene */
struct ObjectDescriptionPT {
    /* A pointer to a buffer holding mesh vertex data */
    uint64_t vertexAddress;
    /* A pointer to a buffer holding mesh index data*/
    uint64_t indexAddress;
    /* An index to point to the materials buffer */
    uint32_t materialIndex;
    /* The number of triangles in the buffer */
    uint32_t numTriangles;
};

}  // namespace vengine

/* Hash function for ModeData struct */
template <>
struct std::hash<vengine::ModelData> {
    size_t operator()(const vengine::ModelData &m) const { return Hash(m); }
};

#endif