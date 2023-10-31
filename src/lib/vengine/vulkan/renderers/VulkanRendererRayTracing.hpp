#ifndef __VulkanRendererRayTracing_hpp__
#define __VulkanRendererRayTracing_hpp__

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

namespace vengine
{

class VulkanRendererRayTracing : public RendererRayTracing
{
    friend class VulkanRenderer;

public:
    VulkanRendererRayTracing(VulkanContext &vkctx, VulkanMaterials &materials, VulkanTextures &textures, VulkanRandom &random);

    VkResult initResources(VkFormat colorFormat, VkDescriptorSetLayout skyboxDescriptorLayout);

    VkResult releaseRenderResources();
    VkResult releaseResources();

    bool isRTEnabled() const override;
    void render(const Scene &scene) override;
    float renderProgress() override;

    static VkBufferUsageFlags getBufferUsageFlags()
    {
        return VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }

private:
    struct DeviceFunctionsRayTracing {
        PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
        PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
        PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
        PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
        PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
        PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
        PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
        PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
        PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
        PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
    } m_devF;

    struct RayTracingData {
        glm::uvec4 samplesBatchesDepthIndex =
            glm::vec4(256, 16, 5, 0); /* R = total samples, G = Total number of batches, B = max depth per ray, A = batch index */
        glm::uvec4 lights;            /* R = total number of lights */
    };

    struct AccelerationStructure {
        VkAccelerationStructureKHR handle;
        uint64_t deviceAddress = 0;
        VkDeviceMemory memory;
        VkBuffer buffer;
    };

    struct BLAS {
        AccelerationStructure as;
        glm::mat4 transform;
        uint32_t sbtOffset; /* Index in the sbt table for the hit shader to be used for this blas */
        BLAS(const AccelerationStructure &_as, const glm::mat4 &_transform, uint32_t _sbtOffset)
            : as(_as)
            , transform(_transform)
            , sbtOffset(_sbtOffset){};
    };

    struct RayTracingScratchBuffer {
        uint64_t deviceAddress = 0;
        VkBuffer handle = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
    };

    VulkanContext &m_vkctx;
    VulkanMaterials &m_materials;
    VulkanTextures &m_textures;
    VulkanRandom &m_random;

    VkDevice m_device{};
    VkCommandPool m_commandPool;
    VkQueue m_queue;

    /* Device data */
    bool m_isInitialized = false;
    VkPhysicalDeviceProperties m_physicalDeviceProperties;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rayTracingPipelineProperties{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR m_accelerationStructureFeatures{};

    /* Buffers used during rendering */
    std::vector<VulkanBuffer> m_renderBuffers;

    std::vector<BLAS> m_blas;
    AccelerationStructure m_tlas;

    /* Image result data */
    VkFormat m_format;
    StorageImage m_renderResult, m_tempImage;
    RayTracingData m_rayTracingData;

    bool m_renderInProgress = false;
    float m_renderProgress = 0.0F;

    std::vector<ObjectDescriptionRT> m_sceneObjectsDescription;

    /* Descriptor sets */
    VulkanBuffer m_uniformBufferScene;             /* Holds scene data, ray tracing data and light data */
    VulkanBuffer m_storageBufferObjectDescription; /* Holds references to scene objects */
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

    uint64_t getBufferDeviceAddress(VkDevice device, VkBuffer buffer);

    static VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice, uint32_t &queueFamilyIndex);

    /**
     * @brief Create a Bottom Level Acceleration Structure object for a mesh
     *
     * @param mesh Mesh object
     * @param transformationMatrix Transform matrix
     * @param materialIndex The material index of the mesh object
     * @return AccelerationStructure
     */
    AccelerationStructure createBottomLevelAccelerationStructure(const VulkanMesh &mesh,
                                                                 const glm::mat4 &transformationMatrix,
                                                                 const uint32_t materialIndex);
    AccelerationStructure createTopLevelAccelerationStructure();
    void destroyAccellerationStructures();

    void setResolution();
    /* Create render target image */
    VkResult createStorageImage(uint32_t width, uint32_t height);

    /* Buffers */
    VkResult createBuffers();
    VkResult updateBuffers(const SceneData &sceneData, const RayTracingData &rtData, const std::vector<LightRT> &lights);
    VkResult updateBuffersRayTracingData(const RayTracingData &rtData);

    /* Descriptor sets */
    VkResult createDescriptorSets();
    void updateDescriptorSets();

    VkResult createRayTracingPipeline();
    VkResult createShaderBindingTable();

    VkResult render(VkDescriptorSet skyboxDescriptor);

    VkResult storeToDisk(std::string filename, FileType type) const;

    /**
        Create a scratch buffer to hold temporary data for a ray tracing acceleration structure
    */
    VkResult createScratchBuffer(VkDeviceSize size, RayTracingScratchBuffer &rayTracingScratchBuffer);
    void deleteScratchBuffer(RayTracingScratchBuffer &scratchBuffer);

    /* Scene lights functions */
    bool isMeshLight(const SceneObject *so);
    void prepareSceneLights(const Scene &scene, std::vector<LightRT> &sceneLights);
    void prepareSceneObjectLight(const SceneObject *so,
                                 uint32_t objectDescriptionIndex,
                                 const glm::mat4 &worldTransform,
                                 std::vector<LightRT> &sceneLights);
};

}  // namespace vengine

#endif