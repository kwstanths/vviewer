#ifndef __VulkanRendererSkybox_hpp__
#define __VulkanRendererSkybox_hpp__

#include <vector>

#include <core/Image.hpp>

#include "vulkan/IncludeVulkan.hpp"
#include "vulkan/VulkanMaterials.hpp"
#include "vulkan/resources/VulkanTexture.hpp"
#include "vulkan/resources/VulkanCubemap.hpp"
#include "vulkan/resources/VulkanMesh.hpp"

class VulkanRendererSkybox {
public:
    VulkanRendererSkybox();
    
    void initResources(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool, VkDescriptorSetLayout cameraDescriptorLayout);
    void initSwapChainResources(VkExtent2D swapchainExtent, VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples);

    void releaseSwapChainResources();
    void releaseResources();
    
    VkPipeline getPipeline() const;
    VkPipelineLayout getPipelineLayout() const;
    VkDescriptorSetLayout getDescriptorSetLayout() const;

    /**
        Render a skybox 
    */
    void renderSkybox(VkCommandBuffer cmdBuf, VkDescriptorSet cameraDescriptorSet, int imageIndex, const std::shared_ptr<VulkanMaterialSkybox>& skybox) const;
    
    /**
        Create a cubemap from an image, inputImage should be an HDR equirectangular projection
    */
    std::shared_ptr<VulkanCubemap> createCubemap(std::shared_ptr<VulkanTexture> inputImage) const;

    /**
        Create an irradiance map for a cubemap
    */
    std::shared_ptr<VulkanCubemap> createIrradianceMap(std::shared_ptr<VulkanCubemap> inputMap, uint32_t resolution = 32) const;

    /**
        Create a prefiltered cubemap for different rougness values for the input cubemap     
    */
    std::shared_ptr<VulkanCubemap> createPrefilteredCubemap(std::shared_ptr<VulkanCubemap> inputMap, uint32_t resolution = 512) const;

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
    VkSampleCountFlagBits m_msaaSamples;

    bool createGraphicsPipeline();
    bool createDescriptorSetsLayout();

    VulkanCube * m_cube;
};

#endif