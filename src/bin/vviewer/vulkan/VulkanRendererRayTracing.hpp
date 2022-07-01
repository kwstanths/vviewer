#ifndef __VulkanRendererRayTracing_hpp__
#define __VulkanRendererRayTracing_hpp__

#include "IncludeVulkan.hpp"
#include "VulkanTexture.hpp"
#include "VulkanSceneObject.hpp"
#include "VulkanMaterials.hpp"
#include "VulkanScene.hpp"

class VulkanRendererRayTracing {
    friend class VulkanRenderer;
public:
    VulkanRendererRayTracing();

    void initResources(VkPhysicalDevice physicalDevice, VkFormat colorFormat, uint32_t width, uint32_t height);
    void initSwapChainResources(VkExtent2D swapchainExtent, VkRenderPass renderPass, uint32_t swapchainImages);

    void releaseSwapChainResources();
    void releaseResources();

    VkPipeline getPipeline() const;
    VkPipelineLayout getPipelineLayout() const;
    VkDescriptorSetLayout getDescriptorSetLayout() const;

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
    struct StorageImage
    {
        VkDeviceMemory memory;
        VkImage image;
        VkImageView view;
        VkFormat format;
    };
    StorageImage m_renderResult, m_tempImage;

    /* Objects description data for meshes in the scene */
    struct ObjectDescription
    {
        uint64_t vertexAddress;
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

    /**
        Create a scratch buffer to hold temporary data for a ray tracing acceleration structure
    */
    RayTracingScratchBuffer createScratchBuffer(VkDeviceSize size);
    void deleteScratchBuffer(RayTracingScratchBuffer& scratchBuffer);

};

#endif