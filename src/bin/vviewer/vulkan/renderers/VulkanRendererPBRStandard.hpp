#ifndef __VulkanRendererPBR_hpp__
#define __VulkanRendererPBR_hpp__

#include "vulkan/IncludeVulkan.hpp"
#include "vulkan/VulkanTexture.hpp"
#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanMaterials.hpp"

class VulkanRendererPBR {
    friend class VulkanRenderer;
public:
    VulkanRendererPBR();

    void initResources(VkPhysicalDevice physicalDevice, 
        VkDevice device, 
        VkQueue queue, 
        VkCommandPool commandPool, 
        VkPhysicalDeviceProperties physicalDeviceProperties,
        VkDescriptorSetLayout cameraDescriptorLayout, 
        VkDescriptorSetLayout modelDescriptorLayout, 
        VkDescriptorSetLayout skyboxDescriptorLayout);
    void initSwapChainResources(VkExtent2D swapchainExtent, VkRenderPass renderPass, uint32_t swapchainImages);

    void releaseSwapChainResources();
    void releaseResources();

    VkPipeline getPipeline() const;
    VkPipelineLayout getPipelineLayout() const;
    VkDescriptorSetLayout getDescriptorSetLayout() const;

    VulkanTexture* createBRDFLUT(uint32_t resolution = 512) const;

    VulkanMaterialPBRStandard* createMaterial(std::string name,
        glm::vec4 albedo, float metallic, float roughness, float ao, float emissive,
        VulkanDynamicUBO<MaterialData>& materialsUBO);

    void renderObjects(VkCommandBuffer& cmdBuf, 
        VkDescriptorSet& descriptorScene,
        VkDescriptorSet& descriptorModel,
        VulkanMaterialSkybox * skybox,
        uint32_t imageIndex, 
        VulkanDynamicUBO<ModelData>& dynamicUBOModels,
        std::vector<std::shared_ptr<SceneObject>>& objects) const;

private:
    VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    VkCommandPool m_commandPool;
    VkQueue m_queue;
    VkExtent2D m_swapchainExtent;

    VkDescriptorSetLayout m_descriptorSetLayoutCamera;
    VkDescriptorSetLayout m_descriptorSetLayoutModel;
    VkDescriptorSetLayout m_descriptorSetLayoutSkybox;
    VkDescriptorSetLayout m_descriptorSetLayoutMaterial;

    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    VkRenderPass m_renderPass;

    bool createDescriptorSetsLayout();
    bool createGraphicsPipeline();
};

#endif