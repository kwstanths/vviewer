#include "VulkanRenderPass.hpp"
#include "vulkan/common/VulkanInitializers.hpp"

namespace vengine
{

VkResult VulkanRenderPass::createPass(VulkanContext &context)
{
    VkRenderPassCreateInfo renderPassInfo = vkinit::renderPassCreateInfo(static_cast<uint32_t>(m_attachments.size()),
                                                                         m_attachments.data(),
                                                                         static_cast<uint32_t>(m_subpasses.size()),
                                                                         m_subpasses.data(),
                                                                         static_cast<uint32_t>(m_subpassDependencies.size()),
                                                                         m_subpassDependencies.data());
    VULKAN_CHECK_CRITICAL(vkCreateRenderPass(context.device(), &renderPassInfo, nullptr, &m_renderPass));

    return VK_SUCCESS;
}

VkResult VulkanRenderPassForward::initSwapChainResources(VulkanContext &context, VulkanSwapchain &swapchain, VkFormat format)
{
    /* Create render pass attachments */

    /* Color attachment */
    VkAttachmentDescription colorAttachment = vkinit::attachmentDescriptionInfo(format,
                                                                                VK_SAMPLE_COUNT_1_BIT,
                                                                                VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                                                VK_ATTACHMENT_STORE_OP_STORE,
                                                                                VK_IMAGE_LAYOUT_UNDEFINED,
                                                                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    m_attachments.push_back(colorAttachment);

    /* Highlight attachment */
    VkAttachmentDescription highlightAttachment = vkinit::attachmentDescriptionInfo(format,
                                                                                    VK_SAMPLE_COUNT_1_BIT,
                                                                                    VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                                                    VK_ATTACHMENT_STORE_OP_STORE,
                                                                                    VK_IMAGE_LAYOUT_UNDEFINED,
                                                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkAttachmentReference highlightAttachmentRef{};
    highlightAttachmentRef.attachment = 1;
    highlightAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    m_attachments.push_back(highlightAttachment);

    /* Depth attachment */
    VkAttachmentDescription depthAttachment = vkinit::attachmentDescriptionInfo(swapchain.depthFormat(),
                                                                                VK_SAMPLE_COUNT_1_BIT,
                                                                                VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                                                VK_ATTACHMENT_STORE_OP_STORE,
                                                                                VK_IMAGE_LAYOUT_UNDEFINED,
                                                                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 2;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    m_attachments.push_back(depthAttachment);

    /* Create subpass */
    std::array<VkAttachmentReference, 2> colorAttachments{colorAttachmentRef, highlightAttachmentRef};
    VkSubpassDescription renderPassForwardSubpass{};
    renderPassForwardSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    renderPassForwardSubpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
    renderPassForwardSubpass.pColorAttachments = colorAttachments.data(); /* Same as shader locations */
    renderPassForwardSubpass.pDepthStencilAttachment = &depthAttachmentRef;
    m_subpasses.push_back(renderPassForwardSubpass);

    /* Subpass dependencies */
    VkSubpassDependency dependency{};
    /* Dependency 1: Wait for the previous external pass to finish reading the color, before you start writing the new color */
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependency.dstSubpass = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    m_subpassDependencies.push_back(dependency);

    return VulkanRenderPass::createPass(context);
}

VkResult VulkanRenderPassForward::releaseSwapChainResources(VulkanContext &context)
{
    vkDestroyRenderPass(context.device(), m_renderPass, nullptr);

    return VK_SUCCESS;
}

}  // namespace vengine