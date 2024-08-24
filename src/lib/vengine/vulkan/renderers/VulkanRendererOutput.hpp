#ifndef __VulkanRendererOutput_hpp__
#define __VulkanRendererOutput_hpp__

#include <vector>

#include "vengine/core/Scene.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/VulkanFramebuffer.hpp"
#include "vulkan/VulkanRenderPass.hpp"

namespace vengine
{

class VulkanRendererOutput
{
    friend class VulkanRenderer;

public:
    VulkanRendererOutput(VulkanContext &context);

    VkResult initResources(VkDescriptorSetLayout sceneDataDescriptorLayout);
    VkResult initSwapChainResources(VkExtent2D swapchainExtent, uint32_t swapchainImages, VulkanRenderPassOutput renderPass);

    VkResult releaseSwapChainResources();
    VkResult releaseResources();

    VkResult setInputAttachment(const VulkanFrameBufferAttachment &inputAttachment);

    const VkPipeline &pipeline() const { return m_graphicsPipeline; }
    const VkPipelineLayout &pipelineLayout() const { return m_pipelineLayout; }
    const VkDescriptorSetLayout &descriptorSetLayout() const { return m_descriptorSetLayout; }

    VkResult render(VkCommandBuffer cmdBuf, VkDescriptorSet &descriptorSceneData, uint32_t imageIndex);

private:
    VulkanContext &m_ctx;

    VkExtent2D m_swapchainExtent;
    uint32_t m_swapchainImages;

    VulkanRenderPassOutput m_renderPass;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;

    VkDescriptorPool m_descriptorPool;
    VkDescriptorSetLayout m_descriptorSetLayout;
    std::vector<VkDescriptorSet> m_descriptorSets;
    VkSampler m_inputSampler;

    VkDescriptorSetLayout m_descriptorSetLayoutSceneData;

    VkResult createGraphicsPipeline();
    VkResult createDescriptorSetsLayout();
    VkResult createDescriptorPool(uint32_t imageCount);
    VkResult createDescriptors(uint32_t imageCount);
    VkResult updateDescriptors(const VulkanFrameBufferAttachment &inputAttachment);
    VkResult createSampler(VkSampler &sampler);
};

}  // namespace vengine

#endif