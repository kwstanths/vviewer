#ifndef __VulkanRendererPBR_hpp__
#define __VulkanRendererPBR_hpp__

#include "vulkan/IncludeVulkan.hpp"
#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanMaterials.hpp"
#include "vulkan/resources/VulkanTexture.hpp"
#include "vulkan/resources/VulkanTextures.hpp"

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
        VkDescriptorSetLayout skyboxDescriptorLayout,
        VkDescriptorSetLayout materialDescriptorLayout,
        VulkanTextures& textures);
    void initSwapChainResources(VkExtent2D swapchainExtent, VkRenderPass renderPass, uint32_t swapchainImages, VkSampleCountFlagBits msaaSamples);

    void releaseSwapChainResources();
    void releaseResources();

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
        VkDescriptorSet descriptorSkybox,
        VkDescriptorSet& descriptorMaterials,
        VkDescriptorSet& descriptorTextures,
        uint32_t imageIndex, 
        const VulkanUBO<ModelData>& dynamicUBOModels,
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
        VkDescriptorSet& descriptorMaterials,
        VkDescriptorSet& descriptorTextures,
        uint32_t imageIndex, 
        const VulkanUBO<ModelData>& dynamicUBOModels,
        std::shared_ptr<SceneObject> object,
        PushBlockForwardAddPass& lightInfo) const;

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
    VkDescriptorSetLayout m_descriptorSetLayoutTextures;

    VkPipelineLayout m_pipelineLayoutBasePass, m_pipelineLayoutAddPass;
    VkPipeline m_graphicsPipelineBasePass, m_graphicsPipelineAddPass;
    VkRenderPass m_renderPass;
    VkSampleCountFlagBits m_msaaSamples;

    void createBRDFLUT(VulkanTextures& textures, uint32_t resolution = 128) const;

    bool createGraphicsPipelineBasePass();
    bool createGraphicsPipelineAddPass();
};

#endif