#ifndef __VulkanRendererRayTracing_hpp__
#define __VulkanRendererRayTracing_hpp__

#include <cstdint>

#include "core/Scene.hpp"
#include "vulkan/IncludeVulkan.hpp"
#include "vulkan/VulkanContext.hpp"
#include "vulkan/VulkanTexture.hpp"
#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanMaterials.hpp"
#include "vulkan/VulkanScene.hpp"
#include "vulkan/VulkanStructs.hpp"

struct RayTracingData {
    glm::uvec4 samplesBatchesDepthIndex = glm::vec4(256, 16, 5, 0);    /* R = total samples, G = Total number of batches, B = max depth per ray, A = batch index */
    glm::uvec4 lights; /* R = total number of lights */
};

enum class OutputFileType {
    PNG = 0,
    HDR = 1,
};

class VulkanRendererRayTracing {
    friend class VulkanRenderer;
public:

    VulkanRendererRayTracing(VulkanContext& vkctx);

    void initResources(VkFormat colorFormat);

    void releaseRenderResources();
    void releaseResources();

    bool isInitialized() const;

    void renderScene(const VulkanScene* scene);

    void setSamples(uint32_t samples) { m_rayTracingData.samplesBatchesDepthIndex.r = samples; }
    uint32_t getSamples() const { return m_rayTracingData.samplesBatchesDepthIndex.r; }

    void setMaxDepth(uint32_t depth) { m_rayTracingData.samplesBatchesDepthIndex.b = depth; }
    uint32_t getMaxDepth() const { return m_rayTracingData.samplesBatchesDepthIndex.b; }

    void setRenderResolution(uint32_t width, uint32_t height);
    void getRenderResolution(uint32_t& width, uint32_t& height) const;

    void setRenderOutputFileName(std::string filename) { m_renderResultOutputFileName = filename; }
    std::string getRenderOutputFileName() const { return m_renderResultOutputFileName; }

    void setRenderOutputFileType(OutputFileType type) { m_renderResultOutputFileType = type; }
    OutputFileType getRenderOutputFileType() const { return m_renderResultOutputFileType; }

    VkBufferUsageFlags getBufferUsageFlags() { return VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; }

    float getRenderProgress() const { return m_renderProgress; }

private:
    const uint32_t MAX_OBJECTS = 200u;
    const uint32_t MAX_LIGHTS = 20u;
    const uint32_t MAX_MATERIALS = 100u;

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

    VulkanContext& m_vkctx;
    bool m_renderInProgress = false;
    float m_renderProgress = 0.0F;

    /* Device data */
    bool m_isInitialized = false;
    VkPhysicalDeviceProperties m_physicalDeviceProperties;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR  m_rayTracingPipelineProperties{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR m_accelerationStructureFeatures{};

    /* Logical device data */
    VkDevice m_device{};
    VkCommandPool m_commandPool;
    VkQueue m_queue;

    /* Accelerations structure data */
    struct AccelerationStructure {
        VkAccelerationStructureKHR handle;
        uint64_t deviceAddress = 0;
        VkDeviceMemory memory;
        VkBuffer buffer;
    };
    std::vector<std::pair<VkBuffer, VkDeviceMemory>> m_meshBuffers;
    std::vector<std::pair<AccelerationStructure, glm::mat4>> m_blas;
    AccelerationStructure m_tlas;

    struct RayTracingScratchBuffer {
        uint64_t deviceAddress = 0;
        VkBuffer handle = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
    };

    /* Image result data */
    VkFormat m_format;
    uint32_t m_width, m_height;
    StorageImage m_renderResult, m_tempImage;
    RayTracingData m_rayTracingData;
    std::string m_renderResultOutputFileName;
    OutputFileType m_renderResultOutputFileType;

    std::vector<ObjectDescriptionRT> m_sceneObjects;

    /* Descriptor sets */
    VkBuffer m_uniformBufferScene;  /* Holds scene data, ray tracing data and light data */
    VkDeviceMemory m_uniformBufferSceneMemory;
    VkBuffer m_uniformBufferObjectDescription;  /* Holds references to scene objects */
    VkDeviceMemory m_uniformBufferObjectDescrptionMemory;
    VkDescriptorPool m_descriptorPool;
    VkDescriptorSet m_descriptorSet;

    /* Ray tracing pipeline */
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkPipelineLayout m_pipelineLayout;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_shaderGroups;
    VkPipeline m_pipeline;
    VkBuffer m_shaderRayGenBuffer;
    VkDeviceMemory m_shaderRayGenBufferMemory;
    VkBuffer m_shaderRayCHitBuffer;
    VkDeviceMemory m_shaderRayCHitBufferMemory;
    VkBuffer m_shaderRayMissBuffer;
    VkDeviceMemory m_shaderRayMissBufferMemory;

    static std::vector<const char *> getRequiredExtensions();
    static bool checkRayTracingSupport(VkPhysicalDevice device);

    uint64_t getBufferDeviceAddress(VkDevice device, VkBuffer buffer);
    
    static VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice, uint32_t& queueFamilyIndex);

    AccelerationStructure createBottomLevelAccelerationStructure(const VulkanMesh& mesh, const glm::mat4& transformationMatrix, const int materialIndex);
    AccelerationStructure createTopLevelAccelerationStructure();
    void destroyAccellerationStructures();

    /* Create render target image */
    void createStorageImage();

    /* Uniform buffers */
    void createUniformBuffers();
    void updateUniformBuffers(const SceneData& sceneData, const RayTracingData& rtData, const std::vector<LightRT>& lights, const std::vector<MaterialRT>& materials);
    void updateUniformBuffersRayTracingData(const RayTracingData& rtData);

    /* Descriptor sets */
    void createDescriptorSets();
    void updateDescriptorSets();

    void createRayTracingPipeline();
    void createShaderBindingTable();

    void render();

    void storeToDisk(std::string filename, OutputFileType type) const;

    /**
        Create a scratch buffer to hold temporary data for a ray tracing acceleration structure
    */
    RayTracingScratchBuffer createScratchBuffer(VkDeviceSize size);
    void deleteScratchBuffer(RayTracingScratchBuffer& scratchBuffer);

    std::vector<LightRT> prepareSceneLights(const VulkanScene * scene, const std::vector<std::shared_ptr<SceneObject>>& sceneObjects);
    std::vector<MaterialRT> prepareSceneMaterials(const std::vector<std::shared_ptr<SceneObject>>& sceneObjects, std::unordered_map<std::string, int>& indices);
};

#endif