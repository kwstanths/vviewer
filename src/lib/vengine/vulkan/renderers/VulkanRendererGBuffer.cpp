#include "VulkanRendererGBuffer.hpp"

#include "utils/Algorithms.hpp"

#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanShader.hpp"
#include "vulkan/resources/VulkanMesh.hpp"
#include "vulkan/common/VulkanStructs.hpp"

namespace vengine
{

VulkanRendererGBuffer::VulkanRendererGBuffer(VulkanContext &context)
    : m_ctx(context)
{
}

VkResult VulkanRendererGBuffer::initResources(VkDescriptorSetLayout sceneDataDescriptorLayout,
                                              VkDescriptorSetLayout instanceDataDescriptorLayout,
                                              VkDescriptorSetLayout materialDescriptorLayout,
                                              VkDescriptorSetLayout texturesDescriptorLayout)
{
    m_descriptorSetLayoutSceneData = sceneDataDescriptorLayout;
    m_descriptorSetLayoutInstanceData = instanceDataDescriptorLayout;
    m_descriptorSetLayoutMaterial = materialDescriptorLayout;
    m_descriptorSetLayoutTextures = texturesDescriptorLayout;
    return VK_SUCCESS;
}

VkResult VulkanRendererGBuffer::initSwapChainResources(VkExtent2D swapchainExtent, const VulkanRenderPassDeferred &renderPass)
{
    m_swapchainExtent = swapchainExtent;

    VULKAN_CHECK_CRITICAL(createPipeline(renderPass));

    return VK_SUCCESS;
}

VkResult VulkanRendererGBuffer::releaseResources()
{
    return VK_SUCCESS;
}

VkResult VulkanRendererGBuffer::releaseSwapChainResources()
{
    vkDestroyPipeline(m_ctx.device(), m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_ctx.device(), m_pipelineLayout, nullptr);
    return VK_SUCCESS;
}

VkResult VulkanRendererGBuffer::renderOpaqueInstances(VkCommandBuffer &cmdBuf,
                                                      const VulkanInstancesManager &instances,
                                                      VkDescriptorSet &descriptorSceneData,
                                                      VkDescriptorSet &descriptorInstanceData,
                                                      VkDescriptorSet &descriptorMaterials,
                                                      VkDescriptorSet &descriptorTextures) const
{
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    std::array<VkDescriptorSet, 4> descriptorSets = {
        descriptorSceneData,
        descriptorInstanceData,
        descriptorMaterials,
        descriptorTextures,
    };
    vkCmdBindDescriptorSets(cmdBuf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayout,
                            0,
                            static_cast<uint32_t>(descriptorSets.size()),
                            &descriptorSets[0],
                            0,
                            nullptr);

    for (auto &meshGroup : instances.opaqueMeshes()) {
        const VulkanMesh *vkmesh = static_cast<const VulkanMesh *>(meshGroup.first);
        assert(vkmesh != nullptr);
        renderMeshGroup(cmdBuf, vkmesh, meshGroup.second);
    }

    return VK_SUCCESS;
}

VkResult VulkanRendererGBuffer::renderMeshGroup(VkCommandBuffer &commandBuffer,
                                                const VulkanMesh *mesh,
                                                const InstancesManager::MeshGroup &meshGroup) const
{
    VkBuffer vertexBuffers[] = {mesh->vertexBuffer().buffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer().buffer(), 0, mesh->indexType());

    vkCmdDrawIndexed(
        commandBuffer, static_cast<uint32_t>(mesh->indices().size()), meshGroup.sceneObjects.size(), 0, 0, meshGroup.startIndex);

    return VK_SUCCESS;
}

VkResult VulkanRendererGBuffer::createPipeline(const VulkanRenderPassDeferred &renderPass)
{
    VkShaderModule vertexShader = VulkanShader::load(m_ctx.device(), "shaders/SPIRV/standard.vert.spv");
    VkShaderModule fragmentShader = VulkanShader::load(m_ctx.device(), "shaders/SPIRV/gbuffer.frag.spv");

    VkPipelineShaderStageCreateInfo vertShaderStageInfo =
        vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, "main");
    VkPipelineShaderStageCreateInfo fragShaderStageInfo =
        vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader, "main");
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    auto bindingDescription = VulkanVertex::getBindingDescription();
    auto attributeDescriptions = VulkanVertex::getAttributeDescriptionsFull();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputStateCreateInfo(
        1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::pipelineInputAssemblyCreateInfo();

    VkViewport viewport = vkinit::viewport(m_swapchainExtent.width, m_swapchainExtent.height, 0.0F, 1.0F);
    VkRect2D scissor = vkinit::rect2D(m_swapchainExtent.width, m_swapchainExtent.height, 0.0, 0.0);
    VkPipelineViewportStateCreateInfo viewportState = vkinit::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);

    VkPipelineRasterizationStateCreateInfo rasterizer =
        vkinit::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
    VkPipelineDepthStencilStateCreateInfo depthStencil =
        vkinit::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);

    VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinit::pipelineColorBlendAttachmentState(
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments{
        colorBlendAttachment, colorBlendAttachment, colorBlendAttachment};
    VkPipelineColorBlendStateCreateInfo colorBlending =
        vkinit::pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data());

    std::vector<VkDescriptorSetLayout> descriptorSetsLayouts{m_descriptorSetLayoutSceneData,
                                                             m_descriptorSetLayoutInstanceData,
                                                             m_descriptorSetLayoutMaterial,
                                                             m_descriptorSetLayoutTextures};
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(
        static_cast<uint32_t>(descriptorSetsLayouts.size()), descriptorSetsLayouts.data(), 0, nullptr);
    VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_ctx.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo =
        vkinit::graphicsPipelineCreateInfo(m_pipelineLayout, renderPass.renderPass(), renderPass.subpassIndexGBuffer());
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

    vkDestroyShaderModule(m_ctx.device(), vertexShader, nullptr);
    vkDestroyShaderModule(m_ctx.device(), fragmentShader, nullptr);

    return VK_SUCCESS;
}

}  // namespace vengine