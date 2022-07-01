#ifndef __VulkanRendererPost_hpp__
#define __VulkanRendererPost_hpp__

#include "IncludeVulkan.hpp"
#include <vector>

class VulkanRendererPost {
    friend class VulkanRenderer;
public:
    VulkanRendererPost();

    void initResources(VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkQueue queue,
        VkCommandPool commandPool);
    void initSwapChainResources(VkExtent2D swapchainExtent, VkRenderPass renderPass, uint32_t swapchainImages, const std::vector<VkImageView>& colorImages, const std::vector<VkImageView>& highlightImages);

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

    VkDescriptorPool m_descriptorPool;
    VkDescriptorSetLayout m_descriptorSetLayout;
    std::vector<VkDescriptorSet> m_descriptorSets;
    VkSampler m_inputSampler;

    bool createGraphicsPipeline();
    bool createDescriptorSetsLayout();
    bool createDescriptorPool(uint32_t imageCount);
    bool createSampler();
    bool createDescriptors(uint32_t imageCount, const std::vector<VkImageView>& colorImages, const std::vector<VkImageView>& highlightImages);
};

#endif