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

VkResult VulkanRenderPass::releaseResources(VkDevice device)
{
    vkDestroyRenderPass(device, m_renderPass, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRenderPassDeferred::initResources(VkFormat gbuffer1Format,
                                                 VkFormat gbuffer2Format,
                                                 VkFormat depthFormat,
                                                 VkFormat colorFormat,
                                                 uint32_t imageCount)
{
    m_attachments.clear();
    m_subpasses.clear();
    m_subpassDependencies.clear();

    /* Attachments */
    /* Gbuffer attachment 1 */
    VkAttachmentDescription attachment1 = vkinit::attachmentDescriptionInfo(gbuffer1Format,
                                                                            VK_SAMPLE_COUNT_1_BIT,
                                                                            VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                                            VK_ATTACHMENT_STORE_OP_STORE,
                                                                            VK_IMAGE_LAYOUT_UNDEFINED,
                                                                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    m_attachments.push_back(attachment1);
    /* Gbuffer attachment 2 */
    VkAttachmentDescription attachment2 = vkinit::attachmentDescriptionInfo(gbuffer2Format,
                                                                            VK_SAMPLE_COUNT_1_BIT,
                                                                            VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                                            VK_ATTACHMENT_STORE_OP_STORE,
                                                                            VK_IMAGE_LAYOUT_UNDEFINED,
                                                                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    m_attachments.push_back(attachment2);
    /* Depth attachment */
    VkAttachmentDescription depthAttachment = vkinit::attachmentDescriptionInfo(depthFormat,
                                                                                VK_SAMPLE_COUNT_1_BIT,
                                                                                VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                                                VK_ATTACHMENT_STORE_OP_STORE,
                                                                                VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                                                VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                                                                VK_IMAGE_LAYOUT_UNDEFINED,
                                                                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    m_attachments.push_back(depthAttachment);
    VkAttachmentReference depthAttachmentRef = {2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    /* Color attachment */
    VkAttachmentDescription colorAttachment = vkinit::attachmentDescriptionInfo(colorFormat,
                                                                                VK_SAMPLE_COUNT_1_BIT,
                                                                                VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                                                VK_ATTACHMENT_STORE_OP_STORE,
                                                                                VK_IMAGE_LAYOUT_UNDEFINED,
                                                                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    m_attachments.push_back(colorAttachment);

    /* Subpasses */
    /* GBuffer fill */
    std::vector<VkAttachmentReference> subpass0colorAttachmentRefs = {{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
                                                                      {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
                                                                      {3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};
    VkSubpassDescription subpassGBuffer{};
    subpassGBuffer.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassGBuffer.colorAttachmentCount = static_cast<uint32_t>(subpass0colorAttachmentRefs.size());
    subpassGBuffer.pColorAttachments = subpass0colorAttachmentRefs.data(); /* Same as shader locations */
    subpassGBuffer.pDepthStencilAttachment = &depthAttachmentRef;
    m_subpasses.push_back(subpassGBuffer);

    /* Light composition */
    std::vector<VkAttachmentReference> subpass1colorAttachmentRefs = {{3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};
    std::vector<VkAttachmentReference> subpass1inputAttachmentRefs = {{0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                                                                      {1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                                                                      {2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
    VkSubpassDescription subpassDeferredLighting{};
    subpassDeferredLighting.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDeferredLighting.colorAttachmentCount = static_cast<uint32_t>(subpass1colorAttachmentRefs.size());
    subpassDeferredLighting.pColorAttachments = subpass1colorAttachmentRefs.data(); /* Same as shader locations */
    subpassDeferredLighting.pDepthStencilAttachment = nullptr;
    subpassDeferredLighting.inputAttachmentCount = static_cast<uint32_t>(subpass1inputAttachmentRefs.size());
    subpassDeferredLighting.pInputAttachments = subpass1inputAttachmentRefs.data();
    m_subpasses.push_back(subpassDeferredLighting);

    /* Forward lighting */
    std::vector<VkAttachmentReference> subpass2colorAttachmentRefs = {{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
                                                                      {3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};

    VkSubpassDescription subpassForwardLighting{};
    subpassForwardLighting.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassForwardLighting.colorAttachmentCount = static_cast<uint32_t>(subpass2colorAttachmentRefs.size());
    subpassForwardLighting.pColorAttachments = subpass2colorAttachmentRefs.data(); /* Same as shader locations */
    subpassForwardLighting.pDepthStencilAttachment = &depthAttachmentRef;
    m_subpasses.push_back(subpassForwardLighting);

    /* Subpass dependencies */
    {
        /* Make sure that reads/writes to the depth of first subpass happen after the previous external finishes writing at the late
         * fragment tests */
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstSubpass = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        m_subpassDependencies.push_back(dependency);
    }
    {
        /* Make sure writes to the color happen after the external previous external finishes writing */
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstSubpass = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        m_subpassDependencies.push_back(dependency);
    }
    {
        /* Makre sure subpass 0 finished writing color before subpass 1 starts reading it */
        VkSubpassDependency dependency{};
        dependency.srcSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstSubpass = 1;
        dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        m_subpassDependencies.push_back(dependency);
    }
    {
        /* Makre sure subpass 1 finishes writing color before subpass 2 starts writing color */
        VkSubpassDependency dependency{};
        dependency.srcSubpass = 1;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstSubpass = 2;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        m_subpassDependencies.push_back(dependency);
    }
    {
        /* Makre sure that subpass 2 finished writing before the subsequent external pass starts reading or writing */
        VkSubpassDependency dependency{};
        dependency.srcSubpass = 2;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        m_subpassDependencies.push_back(dependency);
    }

    VULKAN_CHECK_CRITICAL(VulkanRenderPass::createPass(m_ctx));

    VULKAN_CHECK_CRITICAL(createDescriptorSetLayout());
    VULKAN_CHECK_CRITICAL(createDescriptorPool(imageCount));
    VULKAN_CHECK_CRITICAL(createDescriptors(imageCount));

    return VK_SUCCESS;
}

VkResult VulkanRenderPassDeferred::releaseResources(VkDevice device)
{
    vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);

    VulkanRenderPass::releaseResources(device);

    return VK_SUCCESS;
}

VkResult VulkanRenderPassDeferred::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding gbufferInput1LayoutBinding =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1);
    VkDescriptorSetLayoutBinding gbufferInput2LayoutBinding =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1);
    VkDescriptorSetLayoutBinding depthInputLayoutBinding =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1);
    std::vector<VkDescriptorSetLayoutBinding> inputBindings = {
        gbufferInput1LayoutBinding, gbufferInput2LayoutBinding, depthInputLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo =
        vkinit::descriptorSetLayoutCreateInfo(static_cast<uint32_t>(inputBindings.size()), inputBindings.data());
    VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_ctx.device(), &layoutInfo, nullptr, &m_descriptorSetLayout));

    return VK_SUCCESS;
}

VkResult VulkanRenderPassDeferred::createDescriptorPool(uint32_t imageCount)
{
    VkDescriptorPoolSize inputPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 3 * imageCount);
    std::vector<VkDescriptorPoolSize> inputPoolSizes = {inputPoolSize};

    VkDescriptorPoolCreateInfo poolInfo =
        vkinit::descriptorPoolCreateInfo(static_cast<uint32_t>(inputPoolSizes.size()), inputPoolSizes.data(), imageCount);
    VULKAN_CHECK_CRITICAL(vkCreateDescriptorPool(m_ctx.device(), &poolInfo, nullptr, &m_descriptorPool));

    return VK_SUCCESS;
}

VkResult VulkanRenderPassDeferred::createDescriptors(uint32_t imageCount)
{
    m_descriptorSets.resize(imageCount);

    std::vector<VkDescriptorSetLayout> setLayouts(imageCount, m_descriptorSetLayout);

    VkDescriptorSetAllocateInfo setAllocInfo = vkinit::descriptorSetAllocateInfo(m_descriptorPool, imageCount, setLayouts.data());
    VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(m_ctx.device(), &setAllocInfo, m_descriptorSets.data()));

    return VK_SUCCESS;
}

VkResult VulkanRenderPassDeferred::updateDescriptors(const VulkanFrameBufferAttachment &gbufferAttachment1,
                                                     const VulkanFrameBufferAttachment &gbufferAttachment2,
                                                     const VulkanFrameBufferAttachment &depthAttachment)
{
    for (uint32_t i = 0; i < m_descriptorSets.size(); i++) {
        VkDescriptorImageInfo gbuffer1AttachmentDescriptor =
            vkinit::descriptorImageInfo(VK_NULL_HANDLE, gbufferAttachment1.image(i).view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkWriteDescriptorSet descWrite1 =
            vkinit::writeDescriptorSet(m_descriptorSets[i], VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0, 1, &gbuffer1AttachmentDescriptor);

        VkDescriptorImageInfo gbuffer2AttachmentDescriptor =
            vkinit::descriptorImageInfo(VK_NULL_HANDLE, gbufferAttachment2.image(i).view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkWriteDescriptorSet descWrite2 =
            vkinit::writeDescriptorSet(m_descriptorSets[i], VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, 1, &gbuffer2AttachmentDescriptor);

        VkDescriptorImageInfo depthAttachmentDescriptor =
            vkinit::descriptorImageInfo(VK_NULL_HANDLE, depthAttachment.image(i).view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkWriteDescriptorSet descWrite3 =
            vkinit::writeDescriptorSet(m_descriptorSets[i], VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2, 1, &depthAttachmentDescriptor);

        std::vector<VkWriteDescriptorSet> setWrites{descWrite1, descWrite2, descWrite3};
        vkUpdateDescriptorSets(m_ctx.device(), static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
    }

    return VK_SUCCESS;
}

VkResult VulkanRenderPassOverlay::init(VulkanContext &context, VkFormat colorFormat, VkFormat idFormat, VkFormat depthFormat)
{
    m_attachments.clear();
    m_subpasses.clear();
    m_subpassDependencies.clear();

    /* Attachments */
    /* Color attachment */
    VkAttachmentDescription colorAttachment = vkinit::attachmentDescriptionInfo(colorFormat,
                                                                                VK_SAMPLE_COUNT_1_BIT,
                                                                                VK_ATTACHMENT_LOAD_OP_LOAD,
                                                                                VK_ATTACHMENT_STORE_OP_STORE,
                                                                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    m_attachments.push_back(colorAttachment);

    /* ID attachment */
    VkAttachmentDescription idAttachment = vkinit::attachmentDescriptionInfo(idFormat,
                                                                             VK_SAMPLE_COUNT_1_BIT,
                                                                             VK_ATTACHMENT_LOAD_OP_LOAD,
                                                                             VK_ATTACHMENT_STORE_OP_STORE,
                                                                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    m_attachments.push_back(idAttachment);

    /* Depth attachment */
    VkAttachmentDescription depthAttachment = vkinit::attachmentDescriptionInfo(depthFormat,
                                                                                VK_SAMPLE_COUNT_1_BIT,
                                                                                VK_ATTACHMENT_LOAD_OP_LOAD,
                                                                                VK_ATTACHMENT_STORE_OP_STORE,
                                                                                VK_ATTACHMENT_LOAD_OP_LOAD,
                                                                                VK_ATTACHMENT_STORE_OP_STORE,
                                                                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    m_attachments.push_back(depthAttachment);

    std::vector<VkAttachmentReference> colorAttachments = {{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
                                                           {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};
    VkAttachmentReference depthAttachmentRef{2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    /* Subpasses */
    VkSubpassDescription renderPassOverlaySubpass{};
    renderPassOverlaySubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    renderPassOverlaySubpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
    renderPassOverlaySubpass.pColorAttachments = colorAttachments.data(); /* Same as shader locations */
    renderPassOverlaySubpass.pDepthStencilAttachment = &depthAttachmentRef;
    m_subpasses.push_back(renderPassOverlaySubpass);

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

VkResult VulkanRenderPassOutput::init(VulkanContext &context, VkFormat colorFormat)
{
    m_attachments.clear();
    m_subpasses.clear();
    m_subpassDependencies.clear();

    /* Attachments */
    /* Color attachment */
    VkAttachmentDescription swapchainColorAttachment = vkinit::attachmentDescriptionInfo(colorFormat,
                                                                                         VK_SAMPLE_COUNT_1_BIT,
                                                                                         VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                                                         VK_ATTACHMENT_STORE_OP_STORE,
                                                                                         VK_IMAGE_LAYOUT_UNDEFINED,
                                                                                         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    m_attachments.push_back(swapchainColorAttachment);

    std::vector<VkAttachmentReference> colorAttachments = {{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};

    /* Subpasses */
    VkSubpassDescription renderSubpass{};
    renderSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    renderSubpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
    renderSubpass.pColorAttachments = colorAttachments.data(); /* Same as shader locations */
    m_subpasses.push_back(renderSubpass);

    /* Subpass dependencies */
    {
        VkSubpassDependency dependency{};
        /* Dependency 1: Wait for the previous external pass to finish reading the color, before you start writing the new
         * color */
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
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
        dependency.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        m_subpassDependencies.push_back(dependency);
    }

    return VulkanRenderPass::createPass(context);
}

}  // namespace vengine