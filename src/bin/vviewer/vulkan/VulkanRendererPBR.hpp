#ifndef __VulkanRendererPBR_hpp__
#define __VulkanRendererPBR_hpp__

#include "IncludeVulkan.hpp"

class VulkanRendererPBR {
public:
    VulkanRendererPBR();

    void initResources(VkDevice device, VkDescriptorSetLayout cameraDescriptorLayout, VkDescriptorSetLayout modelDescriptorLayout, VkDescriptorSetLayout skyboxDescriptorLayout);
    void initSwapChainResources(VkExtent2D swapchainExtent, VkRenderPass renderPass);

    void releaseSwapChainResources();
    void releaseResources();

    VkPipeline getPipeline() const;
    VkPipelineLayout getPipelineLayout() const;
    VkDescriptorSetLayout getDescriptorSetLayout() const;

private:
    VkDevice m_device;
    VkExtent2D m_swapchainExtent;

    VkDescriptorSetLayout m_descriptorSetLayoutCamera;
    VkDescriptorSetLayout m_descriptorSetLayoutModel;
    VkDescriptorSetLayout m_descriptorSetLayoutSkybox;
    VkDescriptorSetLayout m_descriptorSetLayoutMaterial;

    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    VkRenderPass m_renderPass;

    bool createDescriptorSetsLayouts();
    bool createGraphicsPipeline();
};

#endif