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
    void initSwapChainResources(VkExtent2D swapchainExtent, VkRenderPass renderPass, uint32_t swapchainImages, VkSampleCountFlagBits msaaSamples);

    void releaseSwapChainResources();
    void releaseResources();

    VulkanMaterialPBRStandard* createMaterial(std::string name,
        glm::vec4 albedo, float metallic, float roughness, float ao, float emissive,
        VulkanDynamicUBO<MaterialData>& materialsUBO);

    /**
     * @brief Render objects with the base pass, IBL + directional light + selection info
     * 
     * @param cmdBuf 
     * @param descriptorScene 
     * @param descriptorModel 
     * @param skybox 
     * @param imageIndex 
     * @param dynamicUBOModels 
     * @param objects 
     */
    void renderObjectsBasePass(VkCommandBuffer& cmdBuf, 
        VkDescriptorSet& descriptorScene,
        VkDescriptorSet& descriptorModel,
        VulkanMaterialSkybox * skybox,
        uint32_t imageIndex, 
        const VulkanDynamicUBO<ModelData>& dynamicUBOModels,
        std::vector<std::shared_ptr<SceneObject>>& objects) const;

    /**
     * @brief Render objects with an additive pass
     * 
     * @param cmdBuf 
     * @param descriptorScene 
     * @param descriptorModel 
     * @param skybox 
     * @param imageIndex 
     * @param dynamicUBOModels 
     * @param objects 
     */
    void renderObjectsAddPass(VkCommandBuffer& cmdBuf, 
        VkDescriptorSet& descriptorScene,
        VkDescriptorSet& descriptorModel,
        uint32_t imageIndex, 
        const VulkanDynamicUBO<ModelData>& dynamicUBOModels,
        std::shared_ptr<SceneObject> object,
        const PushBlockForwardAddPass& lightInfo) const;

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

    VkPipelineLayout m_pipelineLayoutBasePass, m_pipelineLayoutAddPass;
    VkPipeline m_graphicsPipelineBasePass, m_graphicsPipelineAddPass;
    VkRenderPass m_renderPass;
    VkSampleCountFlagBits m_msaaSamples;

    VulkanTexture* createBRDFLUT(uint32_t resolution = 512) const;

    bool createDescriptorSetsLayout();
    bool createGraphicsPipelineBasePass();
    bool createGraphicsPipelineAddPass();
};

#endif