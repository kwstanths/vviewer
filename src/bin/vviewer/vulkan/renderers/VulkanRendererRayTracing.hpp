#ifndef __VulkanRendererRayTracing_hpp__
#define __VulkanRendererRayTracing_hpp__

#include "vulkan/IncludeVulkan.hpp"
#include "vulkan/VulkanTexture.hpp"
#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanMaterials.hpp"
#include "vulkan/VulkanScene.hpp"
#include "vulkan/VulkanDataStructs.hpp"

class VulkanRendererRayTracing {
    friend class VulkanRenderer;
public:
    enum class OutputFileType {
        PNG = 0,
        HDR = 1,
    };

    VulkanRendererRayTracing();

    void initResources(VkPhysicalDevice physicalDevice, VkFormat colorFormat);
    void initSwapChainResources(VkExtent2D swapchainExtent);

    void releaseSwapChainResources();
    void releaseResources();

    bool isInitialized() const;

    void renderScene(const VulkanScene* scene);

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

    /* Device data */
    bool m_isInitialized = false;
    VkPhysicalDevice m_physicalDevice{};
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR  m_rayTracingPipelineProperties{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR m_accelerationStructureFeatures{};

    /* Logical device data */
    VkDevice m_device{};
    uint32_t m_queueFamilyIndex{};
    VkQueue m_queue{};
    VkCommandPool m_commandPool{};

    /* Accelerations structure data */
    struct AccelerationStructure {
        VkAccelerationStructureKHR handle;
        uint64_t deviceAddress = 0;
        VkDeviceMemory memory;
        VkBuffer buffer;
    };
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

    /* Objects description data for meshes in the scene */
    struct ObjectDescription
    {
        /* A pointer to a buffer holding mesh vertex data */
        uint64_t vertexAddress;
        /* A pointer to a buffer holding mesh index data*/
        uint64_t indexAddress;
    };
    std::vector<ObjectDescription> m_sceneObjects;

    /* Descriptor sets */
    VkBuffer m_uniformBufferScene;  /* Holds scene data */
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
    static bool checkRayTracingSupport(VkPhysicalDevice device, std::vector<VkExtensionProperties>& availableExtensions);

    uint64_t getBufferDeviceAddress(VkDevice device, VkBuffer buffer);
    
    static VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice, uint32_t& queueFamilyIndex);

    AccelerationStructure createBottomLevelAccelerationStructure(const VulkanMesh& mesh, const glm::mat4& transformationMatrix);
    AccelerationStructure createTopLevelAccelerationStructure();
    void destroyAccellerationStructures();

    /* Create render target image */
    void createStorageImage();

    /* Create uniform buffers for RT */
    void createUniformBuffers();
    void updateUniformBuffers(const SceneData& sceneData);

    void createRayTracingPipeline();
    void createShaderBindingTable();
    void createDescriptorSets();
    void updateDescriptorSets();

    void render();

    void storeToDisk(std::string filename, OutputFileType type) const;

    /**
        Create a scratch buffer to hold temporary data for a ray tracing acceleration structure
    */
    RayTracingScratchBuffer createScratchBuffer(VkDeviceSize size);
    void deleteScratchBuffer(RayTracingScratchBuffer& scratchBuffer);

};

#endif