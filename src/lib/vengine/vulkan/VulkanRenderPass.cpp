#include "VulkanRenderPass.hpp"
#include "vulkan/common/VulkanInitializers.hpp"

#include <array>

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

VkResult VulkanRenderPass::destroy(VkDevice device)
{
    vkDestroyRenderPass(device, m_renderPass, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRenderPassForward::init(VulkanContext &context, VkFormat colorFormat, VkFormat highlightFormat, VkFormat depthFormat)
{
    m_attachments.clear();
    m_subpasses.clear();
    m_subpassDependencies.clear();

    /* Attachments */
    /* Color attachment */
    VkAttachmentDescription colorAttachment = vkinit::attachmentDescriptionInfo(colorFormat,
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
    VkAttachmentDescription highlightAttachment = vkinit::attachmentDescriptionInfo(highlightFormat,
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
    VkAttachmentDescription depthAttachment = vkinit::attachmentDescriptionInfo(depthFormat,
                                                                                VK_SAMPLE_COUNT_1_BIT,
                                                                                VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                                                VK_ATTACHMENT_STORE_OP_STORE,
                                                                                VK_IMAGE_LAYOUT_UNDEFINED,
                                                                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 2;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    m_attachments.push_back(depthAttachment);

    /* Subpasses */
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

VkResult VulkanRenderPassPost::init(VulkanContext &context, VkFormat colorFormat)
{
    m_attachments.clear();
    m_subpasses.clear();
    m_subpassDependencies.clear();

    /* Attachments */
    /* Color attachment */
    VkAttachmentDescription postColorAttachment = vkinit::attachmentDescriptionInfo(colorFormat,
                                                                                    VK_SAMPLE_COUNT_1_BIT,
                                                                                    VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                                                    VK_ATTACHMENT_STORE_OP_STORE,
                                                                                    VK_IMAGE_LAYOUT_UNDEFINED,
                                                                                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    VkAttachmentReference postColorAttachmentRef{};
    postColorAttachmentRef.attachment = 0;
    postColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    m_attachments.push_back(postColorAttachment);

    /* Subpasses */
    VkSubpassDescription renderPassPostSubpass{};
    renderPassPostSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    renderPassPostSubpass.colorAttachmentCount = 1;
    renderPassPostSubpass.pColorAttachments = &postColorAttachmentRef; /* Same as shader locations */
    m_subpasses.push_back(renderPassPostSubpass);

    /* Subpass dependencies */
    VkSubpassDependency dependency{};
    /* Dependency 1: Wait for the previous external pass to finish reading the color, before you start writing the new color */
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstSubpass = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    m_subpassDependencies.push_back(dependency);

    return VulkanRenderPass::createPass(context);
}

VkResult VulkanRenderPassUI::init(VulkanContext &context, VkFormat colorFormat, VkFormat highlightFormat, VkFormat depthFormat)
{
    m_attachments.clear();
    m_subpasses.clear();
    m_subpassDependencies.clear();

    /* Attachments */
    /* Color attachment */
    VkAttachmentDescription swapchainColorAttachment = vkinit::attachmentDescriptionInfo(colorFormat,
                                                                                         VK_SAMPLE_COUNT_1_BIT,
                                                                                         VK_ATTACHMENT_LOAD_OP_LOAD,
                                                                                         VK_ATTACHMENT_STORE_OP_STORE,
                                                                                         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                                                         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    VkAttachmentReference swapchainColorAttachmentRef{};
    swapchainColorAttachmentRef.attachment = 0;
    swapchainColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    m_attachments.push_back(swapchainColorAttachment);
    /* Highlight attachment */
    VkAttachmentDescription highlightAttachment = vkinit::attachmentDescriptionInfo(highlightFormat,
                                                                                    VK_SAMPLE_COUNT_1_BIT,
                                                                                    VK_ATTACHMENT_LOAD_OP_LOAD,
                                                                                    VK_ATTACHMENT_STORE_OP_STORE,
                                                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkAttachmentReference highlightAttachmentRef{};
    highlightAttachmentRef.attachment = 1;
    highlightAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    m_attachments.push_back(highlightAttachment);
    /* Depth attachment */
    VkAttachmentDescription depthAttachment = vkinit::attachmentDescriptionInfo(depthFormat,
                                                                                VK_SAMPLE_COUNT_1_BIT,
                                                                                VK_ATTACHMENT_LOAD_OP_LOAD,
                                                                                VK_ATTACHMENT_STORE_OP_STORE,
                                                                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 2;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    m_attachments.push_back(depthAttachment);

    /* Subpasses */
    VkSubpassDescription renderPassUISubpass{};
    std::array<VkAttachmentReference, 2> colorAttachments{swapchainColorAttachmentRef, highlightAttachmentRef};
    renderPassUISubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    renderPassUISubpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
    renderPassUISubpass.pColorAttachments = colorAttachments.data(); /* Same as shader locations */
    renderPassUISubpass.pDepthStencilAttachment = &depthAttachmentRef;
    m_subpasses.push_back(renderPassUISubpass);

    /* Subpass dependencies */
    {
        VkSubpassDependency dependency{};
        /* Dependency 1: Wait for the previous external pass to finish reading the color, before you start writing the new color */
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstSubpass = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        m_subpassDependencies.push_back(dependency);
    }
    {
        VkSubpassDependency dependency{};
        /* Dependency 1: Make sure that our pass finishes writing before the external pass starts reading */
        dependency.srcSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        m_subpassDependencies.push_back(dependency);
    }

    return VulkanRenderPass::createPass(context);
}

}  // namespace vengine