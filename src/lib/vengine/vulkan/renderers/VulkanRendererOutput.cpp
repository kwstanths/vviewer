#include "VulkanRendererOutput.hpp"

#include <Console.hpp>

#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanUtils.hpp"
#include "vulkan/common/VulkanShader.hpp"
#include "vulkan/resources/VulkanMesh.hpp"

namespace vengine
{

VulkanRendererOutput::VulkanRendererOutput(VulkanContext &context)
    : m_ctx(context)
{
}

VkResult VulkanRendererOutput::initResources(VkDescriptorSetLayout sceneDataDescriptorLayout)
{
    m_descriptorSetLayoutSceneData = sceneDataDescriptorLayout;

    VULKAN_CHECK_CRITICAL(createDescriptorSetsLayout());
    VULKAN_CHECK_CRITICAL(createSampler(m_inputSampler));

    return VK_SUCCESS;
}

VkResult VulkanRendererOutput::initSwapChainResources(VkExtent2D swapchainExtent,
                                                      uint32_t swapchainImages,
                                                      VulkanRenderPassOutput renderPass)
{
    m_swapchainExtent = swapchainExtent;
    m_swapchainImages = swapchainImages;
    m_renderPass = renderPass;

    VULKAN_CHECK_CRITICAL(createDescriptorPool(swapchainImages));
    VULKAN_CHECK_CRITICAL(createDescriptors(swapchainImages));
    VULKAN_CHECK_CRITICAL(createGraphicsPipeline());

    return VK_SUCCESS;
}

VkResult VulkanRendererOutput::releaseSwapChainResources()
{
    vkDestroyDescriptorPool(m_ctx.device(), m_descriptorPool, nullptr);
    vkDestroyPipeline(m_ctx.device(), m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_ctx.device(), m_pipelineLayout, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRendererOutput::releaseResources()
{
    vkDestroySampler(m_ctx.device(), m_inputSampler, nullptr);
    vkDestroyDescriptorSetLayout(m_ctx.device(), m_descriptorSetLayout, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRendererOutput::setInputAttachment(const VulkanFrameBufferAttachment &inputAttachment)
{
    VULKAN_CHECK_CRITICAL(updateDescriptors(inputAttachment));

    return VK_SUCCESS;
}

VkResult VulkanRendererOutput::render(VkCommandBuffer cmdBuf, VkDescriptorSet &descriptorSceneData, uint32_t imageIndex)
{
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    PushBlockOutput pushConstants;
    vkCmdPushConstants(cmdBuf, m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlockOutput), &pushConstants);

    std::array<VkDescriptorSet, 2> descriptorSets = {m_descriptorSets[imageIndex], descriptorSceneData};
    vkCmdBindDescriptorSets(cmdBuf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayout,
                            0,
                            static_cast<uint32_t>(descriptorSets.size()),
                            descriptorSets.data(),
                            0,
                            nullptr);

    vkCmdDraw(cmdBuf, 3, 1, 0, 0);

    return VK_SUCCESS;
}

VkResult VulkanRendererOutput::createGraphicsPipeline()
{
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_VERTEX_BIT, VulkanShader::load(m_ctx.device(), "shaders/SPIRV/quad.vert.spv"), "main");
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_FRAGMENT_BIT, VulkanShader::load(m_ctx.device(), "shaders/SPIRV/output.frag.spv"), "main");
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputStateCreateInfo(0, nullptr, 0, nullptr);
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::pipelineInputAssemblyCreateInfo();

    VkViewport viewport = vkinit::viewport(static_cast<float>(m_swapchainExtent.width), static_cast<float>(m_swapchainExtent.height), 0.0F, 1.0F);
    VkRect2D scissor = vkinit::rect2D(     static_cast<int32_t>(m_swapchainExtent.width), static_cast<int32_t>(m_swapchainExtent.height), 0, 0);
    VkPipelineViewportStateCreateInfo viewportState = vkinit::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);

    VkPipelineRasterizationStateCreateInfo rasterizer =
        vkinit::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
    VkPipelineDepthStencilStateCreateInfo depthStencil =
        vkinit::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS);

    VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinit::pipelineColorBlendAttachmentState(
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
    VkPipelineColorBlendStateCreateInfo colorBlending = vkinit::pipelineColorBlendStateCreateInfo(1, &colorBlendAttachment);

    std::array<VkDescriptorSetLayout, 2> descriptorSetsLayouts{m_descriptorSetLayout, m_descriptorSetLayoutSceneData};
    VkPushConstantRange pushConstantRange = vkinit::pushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushBlockOutput), 0);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(
        static_cast<uint32_t>(descriptorSetsLayouts.size()), descriptorSetsLayouts.data(), 1, &pushConstantRange);
    VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_ctx.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo = vkinit::graphicsPipelineCreateInfo(m_pipelineLayout, m_renderPass.renderPass(), 0);
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    VULKAN_CHECK_CRITICAL(vkCreateGraphicsPipelines(m_ctx.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline));

    vkDestroyShaderModule(m_ctx.device(), fragShaderStageInfo.module, nullptr);
    vkDestroyShaderModule(m_ctx.device(), vertShaderStageInfo.module, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRendererOutput::createDescriptorSetsLayout()
{
    VkDescriptorSetLayoutBinding colorInputLayoutBinding =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1);
    std::vector<VkDescriptorSetLayoutBinding> inputBindings = {colorInputLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo =
        vkinit::descriptorSetLayoutCreateInfo(static_cast<uint32_t>(inputBindings.size()), inputBindings.data());
    VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_ctx.device(), &layoutInfo, nullptr, &m_descriptorSetLayout));

    return VK_SUCCESS;
}

VkResult VulkanRendererOutput::createDescriptorPool(uint32_t imageCount)
{
    VkDescriptorPoolSize colorInputPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount);
    std::vector<VkDescriptorPoolSize> inputPoolSizes = {colorInputPoolSize};

    VkDescriptorPoolCreateInfo poolInfo =
        vkinit::descriptorPoolCreateInfo(static_cast<uint32_t>(inputPoolSizes.size()), inputPoolSizes.data(), imageCount);
    VULKAN_CHECK_CRITICAL(vkCreateDescriptorPool(m_ctx.device(), &poolInfo, nullptr, &m_descriptorPool));

    return VK_SUCCESS;
}

VkResult VulkanRendererOutput::createDescriptors(uint32_t imageCount)
{
    m_descriptorSets.resize(imageCount);

    std::vector<VkDescriptorSetLayout> setLayouts(imageCount, m_descriptorSetLayout);

    VkDescriptorSetAllocateInfo setAllocInfo = vkinit::descriptorSetAllocateInfo(m_descriptorPool, imageCount, setLayouts.data());
    VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(m_ctx.device(), &setAllocInfo, m_descriptorSets.data()));

    return VK_SUCCESS;
}

VkResult VulkanRendererOutput::updateDescriptors(const VulkanFrameBufferAttachment &inputAttachment)
{
    for (uint32_t i = 0; i < m_descriptorSets.size(); i++) {
        VkDescriptorImageInfo colorAttachmentDescriptor =
            vkinit::descriptorImageInfo(m_inputSampler, inputAttachment.image(i).view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkWriteDescriptorSet colorWrite = vkinit::writeDescriptorSet(
            m_descriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, &colorAttachmentDescriptor);

        std::vector<VkWriteDescriptorSet> setWrites{colorWrite};
        vkUpdateDescriptorSets(m_ctx.device(), static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
    }

    return VK_SUCCESS;
}

VkResult VulkanRendererOutput::createSampler(VkSampler &sampler)
{
    VkSamplerCreateInfo samplerInfo = vkinit::samplerCreateInfo(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST);
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    VULKAN_CHECK_CRITICAL(vkCreateSampler(m_ctx.device(), &samplerInfo, nullptr, &sampler));

    return VK_SUCCESS;
}

}  // namespace vengine