#ifndef __VulkanRenderPass_hpp__
#define __VulkanRenderPass_hpp__

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/VulkanContext.hpp"
#include "vulkan/VulkanSwapchain.hpp"

namespace vengine
{

class VulkanRenderPass
{
public:
    VkRenderPass &renderPass() { return m_renderPass; }
    const VkRenderPass &renderPass() const { return m_renderPass; }

    virtual VkResult releaseResources(VkDevice device);

protected:
    std::vector<VkAttachmentDescription> m_attachments;
    std::vector<VkSubpassDescription> m_subpasses;
    std::vector<VkSubpassDependency> m_subpassDependencies;
    VkRenderPass m_renderPass;

    VkResult createPass(VulkanContext &m_ctx);
};

/**
 * @brief Deferred render pass
 * Subpass 0 : GBuffer
 * Subpass 1 : Light composition
 * Subpass 2 : Forward pass
 * Attachments [GBuffer1, GBuffer2, Depth, Color]
 */
class VulkanRenderPassDeferred : public VulkanRenderPass
{
public:
    VulkanRenderPassDeferred(VulkanContext &context)
        : m_ctx(context){};

    /**
     * @brief
     *
     * @param gbuffer1Format Format for gbuffer1 attachment
     * @param gbuffer2Format Format for gbuffer2 attachment
     * @param depthFormat Format for depth
     * @param colorFormat Format for color attachment
     * @param imageCount Number of gbuffer descriptors to create
     * @return VkResult
     */
    VkResult initResources(VkFormat gbuffer1Format,
                           VkFormat gbuffer2Format,
                           VkFormat depthFormat,
                           VkFormat colorFormat,
                           uint32_t imageCount);
    VkResult releaseResources(VkDevice device) override;

    uint32_t subpassIndexGBuffer() const { return 0; }
    uint32_t subpassIndexLightComposition() const { return 1; }
    uint32_t subpassIndexForward() const { return 2; }

    /**
     * @brief Descriptor set layout for the GBuffer framebuffer attachment
     */
    const VkDescriptorSetLayout &descriptorSetLayoutGBuffer() const { return m_descriptorSetLayout; }

    VkDescriptorSet &descriptor(uint32_t index) { return m_descriptorSets[index]; }
    VkResult updateDescriptors(const VulkanFrameBufferAttachment &gbufferAttachment1,
                               const VulkanFrameBufferAttachment &gbufferAttachment2,
                               const VulkanFrameBufferAttachment &depthAttachment);

private:
    VulkanContext &m_ctx;

    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSets;

    /* Descriptors for the GBuffer attachments */
    VkResult createDescriptorSetLayout();
    VkResult createDescriptorPool(uint32_t imageCount);
    VkResult createDescriptors(uint32_t imageCount);
};

/* Render pass for the overlay */
class VulkanRenderPassOverlay : public VulkanRenderPass
{
public:
    /**
     * @param colorFormat Format for the color output
     * @param depthFormat Depth format
     * @param attchmentFormat Format for the ID render target
     * @return VkResult
     */
    VkResult init(VulkanContext &context, VkFormat colorFormat, VkFormat idFormat, VkFormat depthFormat);
};

class VulkanRenderPassOutput : public VulkanRenderPass
{
public:
    VkResult init(VulkanContext &context, VkFormat colorFormat);
};

}  // namespace vengine

#endif