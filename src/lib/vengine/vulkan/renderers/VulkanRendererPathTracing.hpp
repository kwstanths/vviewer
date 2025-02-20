#ifndef __VulkanRendererPathTracing_hpp__
#define __VulkanRendererPathTracing_hpp__

#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "core/Renderer.hpp"
#include "core/io/FileTypes.hpp"
#include "core/Scene.hpp"
#include "core/SceneObject.hpp"
#include "math/Transform.hpp"
#include "vulkan/VulkanContext.hpp"
#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanScene.hpp"
#include "vulkan/common/VulkanStructs.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/resources/VulkanMaterials.hpp"
#include "vulkan/resources/VulkanBuffer.hpp"
#include "vulkan/resources/VulkanTextures.hpp"
#include "vulkan/resources/VulkanRandom.hpp"
#include "vulkan/resources/VulkanAccelerationStructure.hpp"
#include "vulkan/resources/VulkanImage.hpp"

namespace vengine
{

class VulkanRendererPathTracing : public RendererPathTracing
{
    friend class VulkanRenderer;

public:
    VulkanRendererPathTracing(VulkanContext &vkctx,
                              VulkanScene &scene,
                              VulkanMaterials &materials,
                              VulkanTextures &textures,
                              VulkanRandom &random);

    VkResult initResources(VkFormat colorFormat, VkDescriptorSetLayout skyboxDescriptorLayout);

    VkResult releaseResources();

    bool isRayTracingEnabled() const override;
    void render() override;
    float renderProgress() override;

    static VkBufferUsageFlags getBufferUsageFlags()
    {
        return VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }

    static uint32_t SBTMaterialOffset(MaterialType type);

private:
    struct PathTracingData {
        glm::uvec4 samplesBatchesDepthIndex =
            glm::vec4(256, 16, 5, 0); /* R = total samples, G = Total number of batches, B = max depth per ray, A = batch index */
        glm::uvec4 lights;            /* R = total number of lights */
    };

    VulkanContext &m_vkctx;
    VulkanScene &m_scene;
    VulkanMaterials &m_materials;
    VulkanTextures &m_textures;
    VulkanRandom &m_random;

    VkDevice m_device{};
    VkCommandPool m_commandPool;
    VkQueue m_queue;

    /* Device data */
    bool m_isInitialized = false;
    VkPhysicalDeviceProperties m_physicalDeviceProperties{};
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rayTracingPipelineProperties{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR m_accelerationStructureFeatures{};

    /* Image result data */
    VkFormat m_format;
    VulkanImage m_renderResultRadiance, m_renderResultAlbedo, m_renderResultNormal, m_tempImage;
    PathTracingData m_pathTracingData;

    bool m_renderInProgress = false;
    float m_renderProgress = 0.0F;

    /* Descriptor sets */
    VulkanBuffer m_uniformBufferScene; /* Holds scene data and path tracing data */
    VkDescriptorPool m_descriptorPool;
    VkDescriptorSet m_descriptorSet;

    /* Ray tracing pipeline */
    VkDescriptorSetLayout m_descriptorSetLayoutMain;
    VkDescriptorSetLayout m_descriptorSetLayoutSkybox;
    VkPipelineLayout m_pipelineLayout;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_shaderGroups;
    VkPipeline m_pipeline;
    struct ShaderBindingTable {
        VulkanBuffer raygenBuffer;
        VulkanBuffer raychitBuffer;
        VulkanBuffer raymissBuffer;
        VkStridedDeviceAddressRegionKHR raygenSbtEntry;
        VkStridedDeviceAddressRegionKHR raymissSbtEntry;
        VkStridedDeviceAddressRegionKHR raychitSbtEntry;
        VkStridedDeviceAddressRegionKHR callableSbtEntry;
    } m_sbt;

    static std::vector<const char *> getRequiredExtensions();
    static bool checkRayTracingSupport(VkPhysicalDevice device);

    static VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice, uint32_t &queueFamilyIndex);

    void setResolution();
    /* Create render target image */
    VkResult createStorageImage(uint32_t width, uint32_t height);

    /* Buffers */
    VkResult createBuffers();
    VkResult updateBuffers(const SceneData &sceneData, const PathTracingData &ptData);
    VkResult updateBuffersPathTracingData(const PathTracingData &ptData);

    /* Descriptor sets */
    VkResult createDescriptorSets();
    void updateDescriptorSets();

    VkResult createRayTracingPipeline();
    VkResult createShaderBindingTable();

    VkResult render(VkDescriptorSet skyboxDescriptor);

    VkResult getRenderTargetData(const VulkanImage &target, std::vector<float> &data);

    VkResult storeToDisk(std::vector<float> &radiance, std::vector<float> &albedo, std::vector<float> &normal) const;
};

}  // namespace vengine

#endif