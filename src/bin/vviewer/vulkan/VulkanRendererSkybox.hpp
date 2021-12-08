#ifndef __VulkanRendererSkybox_hpp__
#define __VulkanRendererSkybox_hpp__

#include <vector>

#include <core/Image.hpp>

#include "IncludeVulkan.hpp"
#include "VulkanCubemap.hpp"
#include "VulkanMesh.hpp"
#include "VulkanMaterials.hpp"
#include "VulkanTexture.hpp"

class VulkanRendererSkybox {
public:
    VulkanRendererSkybox();
    
    void initResources(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool, VkDescriptorSetLayout cameraDescriptorLayout);
    void initSwapChainResources(VkExtent2D swapchainExtent, VkRenderPass renderPass);

    void releaseSwapChainResources();
    void releaseResources();
    
    VkPipeline getPipeline() const;
    VkPipelineLayout getPipelineLayout() const;
    VkDescriptorSetLayout getDescriptorSetLayout() const;

    /**
        Render a skybox 
    */
    void renderSkybox(VkCommandBuffer cmdBuf, VkDescriptorSet cameraDescriptorSet, int imageIndex, VulkanMaterialSkybox * skybox) const;
    
    /**
        Create a cubemap from an image, inputImage should be an HDR equirectangular projection
    */
    VulkanCubemap* createCubemap(VulkanTexture* inputImage) const;

    /**
        Create an irradiance map for a cubemap
    */
    VulkanCubemap* createIrradianceMap(VulkanCubemap* inputMap, uint32_t resolution = 32) const;

private:
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_queue;
    VkCommandPool m_commandPool;
    VkExtent2D m_swapchainExtent;

    /* Skybox rendering pipeline */
    VkDescriptorSetLayout m_descriptorSetLayoutCamera;
    VkDescriptorSetLayout m_descriptorSetLayoutSkybox;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    VkRenderPass m_renderPass;
    bool createGraphicsPipeline();
    bool createDescriptorSetsLayouts();

    VulkanCube * m_cube;
};

#endif