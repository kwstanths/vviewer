#ifndef __VulkanRendererPost_hpp__
#define __VulkanRendererPost_hpp__

#include <vector>

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/VulkanFramebuffer.hpp"

namespace vengine
{

class VulkanRendererPost
{
    friend class VulkanRenderer;

public:
    VulkanRendererPost();

    VkResult initResources(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool);
    VkResult initSwapChainResources(VkExtent2D swapchainExtent,
                                    VkRenderPass renderPass,
                                    uint32_t swapchainImages,
                                    VkSampleCountFlagBits msaaSamples,
                                    const std::vector<VulkanFrameBufferAttachment> &colorattachments,
                                    const std::vector<VulkanFrameBufferAttachment> &highlightAttachments);

    VkResult releaseSwapChainResources();
    VkResult releaseResources();

    VkPipeline getPipeline() const;
    VkPipelineLayout getPipelineLayout() const;
    VkDescriptorSetLayout getDescriptorSetLayout() const;

    VkResult render(VkCommandBuffer cmdBuf, uint32_t imageIndex);

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

    VkResult createGraphicsPipeline();
    VkResult createDescriptorSetsLayout();
    VkResult createDescriptorPool(uint32_t imageCount);
    VkResult createSampler(VkSampler &sampler);
    VkResult createDescriptors(uint32_t imageCount,
                               const std::vector<VulkanFrameBufferAttachment> &colorAttachments,
                               const std::vector<VulkanFrameBufferAttachment> &highlightAttachments);
};

}  // namespace vengine

#endif