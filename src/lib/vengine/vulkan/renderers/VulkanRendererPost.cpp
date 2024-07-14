#include "VulkanRendererPost.hpp"

#include <Console.hpp>

#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanUtils.hpp"
#include "vulkan/common/VulkanShader.hpp"
#include "vulkan/resources/VulkanMesh.hpp"

namespace vengine
{

VulkanRendererPost::VulkanRendererPost()
{
}

VkResult VulkanRendererPost::initResources(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool)
{
    m_physicalDevice = physicalDevice;
    m_device = device;
    m_commandPool = commandPool;
    m_queue = queue;

    VULKAN_CHECK_CRITICAL(createDescriptorSetsLayout());
    VULKAN_CHECK_CRITICAL(createSampler(m_inputSampler));

    return VK_SUCCESS;
}

VkResult VulkanRendererPost::initSwapChainResources(VkExtent2D swapchainExtent,
                                                    VkRenderPass renderPass,
                                                    uint32_t swapchainImages,
                                                    VkSampleCountFlagBits msaaSamples,
                                                    const VulkanFrameBufferAttachment &colorAttachment,
                                                    const VulkanFrameBufferAttachment &highlightAttachment)
{
    m_swapchainExtent = swapchainExtent;
    m_renderPass = renderPass;
    m_msaaSamples = msaaSamples;

    VULKAN_CHECK_CRITICAL(createDescriptorPool(swapchainImages));
    VULKAN_CHECK_CRITICAL(createDescriptors(swapchainImages, colorAttachment, highlightAttachment));
    VULKAN_CHECK_CRITICAL(createGraphicsPipeline());

    return VK_SUCCESS;
}

VkResult VulkanRendererPost::releaseSwapChainResources()
{
    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRendererPost::releaseResources()
{
    vkDestroySampler(m_device, m_inputSampler, nullptr);
    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

    return VK_SUCCESS;
}

VkPipeline VulkanRendererPost::getPipeline() const
{
    return m_graphicsPipeline;
}

VkPipelineLayout VulkanRendererPost::getPipelineLayout() const
{
    return m_pipelineLayout;
}

VkDescriptorSetLayout VulkanRendererPost::getDescriptorSetLayout() const
{
    return VkDescriptorSetLayout();
}

VkResult VulkanRendererPost::render(VkCommandBuffer cmdBuf, uint32_t imageIndex)
{
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    vkCmdBindDescriptorSets(
        cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[imageIndex], 0, nullptr);

    vkCmdDraw(cmdBuf, 3, 1, 0, 0);

    return VK_SUCCESS;
}

VkResult VulkanRendererPost::createGraphicsPipeline()
{
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_VERTEX_BIT, VulkanShader::load(m_device, "shaders/SPIRV/quad.vert.spv"), "main");
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_FRAGMENT_BIT, VulkanShader::load(m_device, "shaders/SPIRV/highlight.frag.spv"), "main");
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputStateCreateInfo(0, nullptr, 0, nullptr);
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::pipelineInputAssemblyCreateInfo();

    VkViewport viewport = vkinit::viewport(m_swapchainExtent.width, m_swapchainExtent.height, 0.0F, 1.0F);
    VkRect2D scissor = vkinit::rect2D(m_swapchainExtent.width, m_swapchainExtent.height, 0, 0);
    VkPipelineViewportStateCreateInfo viewportState = vkinit::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);

    VkPipelineRasterizationStateCreateInfo rasterizer =
        vkinit::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(m_msaaSamples);
    VkPipelineDepthStencilStateCreateInfo depthStencil =
        vkinit::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS);

    VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinit::pipelineColorBlendAttachmentState(
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
    VkPipelineColorBlendStateCreateInfo colorBlending = vkinit::pipelineColorBlendStateCreateInfo(1, &colorBlendAttachment);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(1, &m_descriptorSetLayout, 0, nullptr);
    VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo = vkinit::graphicsPipelineCreateInfo(m_pipelineLayout, m_renderPass, 0);
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    VULKAN_CHECK_CRITICAL(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline));

    vkDestroyShaderModule(m_device, fragShaderStageInfo.module, nullptr);
    vkDestroyShaderModule(m_device, vertShaderStageInfo.module, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRendererPost::createDescriptorSetsLayout()
{
    VkDescriptorSetLayoutBinding colorInputLayoutBinding =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1);
    VkDescriptorSetLayoutBinding highlightInputLayoutBinding =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1);
    std::array<VkDescriptorSetLayoutBinding, 2> inputBindings{colorInputLayoutBinding, highlightInputLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo =
        vkinit::descriptorSetLayoutCreateInfo(static_cast<uint32_t>(inputBindings.size()), inputBindings.data());
    VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout));

    return VK_SUCCESS;
}

VkResult VulkanRendererPost::createDescriptorPool(uint32_t imageCount)
{
    VkDescriptorPoolSize colorInputPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount);
    VkDescriptorPoolSize highlightInputPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount);
    std::array<VkDescriptorPoolSize, 2> inputPoolSizes{colorInputPoolSize, highlightInputPoolSize};

    VkDescriptorPoolCreateInfo poolInfo =
        vkinit::descriptorPoolCreateInfo(static_cast<uint32_t>(inputPoolSizes.size()), inputPoolSizes.data(), imageCount);
    VULKAN_CHECK_CRITICAL(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool));

    return VK_SUCCESS;
}

VkResult VulkanRendererPost::createDescriptors(uint32_t imageCount,
                                               const VulkanFrameBufferAttachment &colorAttachment,
                                               const VulkanFrameBufferAttachment &highlightAttachment)
{
    m_descriptorSets.resize(imageCount);

    std::vector<VkDescriptorSetLayout> setLayouts(imageCount, m_descriptorSetLayout);

    VkDescriptorSetAllocateInfo setAllocInfo = vkinit::descriptorSetAllocateInfo(m_descriptorPool, imageCount, setLayouts.data());
    VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(m_device, &setAllocInfo, m_descriptorSets.data()));

    for (size_t i = 0; i < imageCount; i++) {
        VkDescriptorImageInfo colorAttachmentDescriptor =
            vkinit::descriptorImageInfo(m_inputSampler, colorAttachment.image(i).view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkWriteDescriptorSet colorWrite = vkinit::writeDescriptorSet(
            m_descriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, &colorAttachmentDescriptor);

        VkDescriptorImageInfo highlightAttachmentDescriptor =
            vkinit::descriptorImageInfo(m_inputSampler, highlightAttachment.image(i).view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkWriteDescriptorSet highlightWrite = vkinit::writeDescriptorSet(
            m_descriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, 1, &highlightAttachmentDescriptor);

        std::vector<VkWriteDescriptorSet> setWrites{colorWrite, highlightWrite};
        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
    }

    return VK_SUCCESS;
}

VkResult VulkanRendererPost::createSampler(VkSampler &sampler)
{
    VkSamplerCreateInfo samplerInfo = vkinit::samplerCreateInfo(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST);
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    VULKAN_CHECK_CRITICAL(vkCreateSampler(m_device, &samplerInfo, nullptr, &sampler));

    return VK_SUCCESS;
}

}  // namespace vengine