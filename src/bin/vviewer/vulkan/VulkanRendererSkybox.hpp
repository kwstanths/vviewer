#ifndef __VulkanRendererSkybox_hpp__
#define __VulkanRendererSkybox_hpp__

#include <vector>

#include <core/Image.hpp>

#include "IncludeVulkan.hpp"
#include "VulkanCubemap.hpp"
#include "VulkanMesh.hpp"
#include "VulkanMaterials.hpp"

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

    void renderSkybox(VkCommandBuffer cmdBuf, VkDescriptorSet cameraDescriptorSet, int imageIndex, VulkanMaterialSkybox * skybox) const;

private:
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_queue;
    VkCommandPool m_commandPool;
    VkExtent2D m_swapchainExtent;

    VkDescriptorSetLayout m_descriptorSetLayoutCamera;
    VkDescriptorSetLayout m_descriptorSetLayoutCubemap;

    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    VkRenderPass m_renderPass;

    VulkanCube * m_cube;

    bool createGraphicsPipeline();
    bool createDescriptorSetsLayouts();
};

#endif