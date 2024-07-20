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

    VkResult destroy(VkDevice device);

protected:
    std::vector<VkAttachmentDescription> m_attachments;
    std::vector<VkSubpassDescription> m_subpasses;
    std::vector<VkSubpassDependency> m_subpassDependencies;
    VkRenderPass m_renderPass;

    VkResult createPass(VulkanContext &m_ctx);
};

class VulkanRenderPassForward : public VulkanRenderPass
{
public:
    VkResult init(VulkanContext &context, VkFormat colorFormat, VkFormat highlightFormat, VkFormat depthFormat);
};

class VulkanRenderPassPost : public VulkanRenderPass
{
public:
    VkResult init(VulkanContext &context, VkFormat colorFormat);
};

class VulkanRenderPassUI : public VulkanRenderPass
{
public:
    VkResult init(VulkanContext &context, VkFormat colorFormat, VkFormat highlightFormat, VkFormat depthFormat);
};

}  // namespace vengine

#endif