#ifndef __VulkanRendererPost_hpp__
#define __VulkanRendererPost_hpp__

#include <vector>

#include "vulkan/IncludeVulkan.hpp"
#include "vulkan/VulkanFramebuffer.hpp"

class VulkanRendererPost {
    friend class VulkanRenderer;
public:
    VulkanRendererPost();

    void initResources(VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkQueue queue,
        VkCommandPool commandPool);
    void initSwapChainResources(VkExtent2D swapchainExtent, 
        VkRenderPass renderPass, 
        uint32_t swapchainImages, 
        VkSampleCountFlagBits msaaSamples,
        const std::vector<VulkanFrameBufferAttachment>& colorattachments, 
        const std::vector<VulkanFrameBufferAttachment>& highlightAttachments);

    void releaseSwapChainResources();
    void releaseResources();

    VkPipeline getPipeline() const;
    VkPipelineLayout getPipelineLayout() const;
    VkDescriptorSetLayout getDescriptorSetLayout() const;

    void render(VkCommandBuffer cmdBuf, uint32_t imageIndex);

private:
    VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    VkCommandPool m_commandPool;
    VkQueue m_queue;
    VkExtent2D m_swapchainExtent;

    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    VkRenderPass m_renderPass;
    VkSampleCountFlagBits m_msaaSamples;

    VkDescriptorPool m_descriptorPool;
    VkDescriptorSetLayout m_descriptorSetLayout;
    std::vector<VkDescriptorSet> m_descriptorSets;
    VkSampler m_inputSampler;

    bool createGraphicsPipeline();
    bool createDescriptorSetsLayout();
    bool createDescriptorPool(uint32_t imageCount);
    bool createSampler();
    bool createDescriptors(uint32_t imageCount, const std::vector<VulkanFrameBufferAttachment>& colorAttachments, const std::vector<VulkanFrameBufferAttachment>& highlightAttachments);
};

#endif