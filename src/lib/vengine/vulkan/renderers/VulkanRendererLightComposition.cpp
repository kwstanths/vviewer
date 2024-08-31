#include "VulkanRendererLightComposition.hpp"

#include "utils/Algorithms.hpp"

#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanShader.hpp"
#include "vulkan/resources/VulkanMesh.hpp"
#include "vulkan/common/VulkanStructs.hpp"

namespace vengine
{

VulkanRendererLightComposition::VulkanRendererLightComposition(VulkanContext &context)
    : m_ctx(context)
{
}

VkResult VulkanRendererLightComposition::initResources(VkDescriptorSetLayout cameraDescriptorLayout,
                                                       VkDescriptorSetLayout instanceDataDescriptorLayout,
                                                       VkDescriptorSetLayout lightDataDescriptorLayout,
                                                       VkDescriptorSetLayout skyboxDescriptorLayout,
                                                       VkDescriptorSetLayout materialDescriptorLayout,
                                                       VkDescriptorSetLayout texturesDescriptorLayout,
                                                       VkDescriptorSetLayout tlasDescriptorLayout)
{
    m_descriptorSetLayoutCamera = cameraDescriptorLayout;
    m_descriptorSetLayoutInstanceData = instanceDataDescriptorLayout;
    m_descriptorSetLayoutLightData = lightDataDescriptorLayout;
    m_descriptorSetLayoutSkybox = skyboxDescriptorLayout;
    m_descriptorSetLayoutMaterial = materialDescriptorLayout;
    m_descriptorSetLayoutTextures = texturesDescriptorLayout;
    m_descriptorSetLayoutTLAS = tlasDescriptorLayout;
    return VK_SUCCESS;
}

VkResult VulkanRendererLightComposition::initSwapChainResources(VkExtent2D swapchainExtent, const VulkanRenderPassDeferred &renderPass)
{
    m_swapchainExtent = swapchainExtent;

    createPipelineIBL(renderPass);
    createPipelineLightInstance(
        renderPass, "shaders/SPIRV/lightCompositionPoint.frag.spv", m_pipelineLayoutPoint, m_graphicsPipelinePoint);
    createPipelineLightInstance(
        renderPass, "shaders/SPIRV/lightCompositionDirectional.frag.spv", m_pipelineLayoutDirectional, m_graphicsPipelineDirectional);

    return VK_SUCCESS;
}

VkResult VulkanRendererLightComposition::releaseResources()
{
    return VK_SUCCESS;
}

VkResult VulkanRendererLightComposition::releaseSwapChainResources()
{
    vkDestroyPipeline(m_ctx.device(), m_graphicsPipelineIBL, nullptr);
    vkDestroyPipelineLayout(m_ctx.device(), m_pipelineLayoutIBL, nullptr);
    vkDestroyPipeline(m_ctx.device(), m_graphicsPipelinePoint, nullptr);
    vkDestroyPipelineLayout(m_ctx.device(), m_pipelineLayoutPoint, nullptr);
    vkDestroyPipeline(m_ctx.device(), m_graphicsPipelineDirectional, nullptr);
    vkDestroyPipelineLayout(m_ctx.device(), m_pipelineLayoutDirectional, nullptr);
    return VK_SUCCESS;
}

VkResult VulkanRendererLightComposition::renderIBL(VkCommandBuffer &commandBuffer,
                                                   const InstancesManager &instances,
                                                   VkDescriptorSet &descriptorGBuffer,
                                                   VkDescriptorSet &descriptorScene,
                                                   VkDescriptorSet &descriptorModel,
                                                   VkDescriptorSet &descriptorLight,
                                                   VkDescriptorSet descriptorSkybox,
                                                   VkDescriptorSet &descriptorMaterials,
                                                   VkDescriptorSet &descriptorTextures,
                                                   VkDescriptorSet &descriptorTLAS)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineIBL);

    std::vector<VkDescriptorSet> descriptorSets = {descriptorGBuffer,
                                                   descriptorScene,
                                                   descriptorModel,
                                                   descriptorLight,
                                                   descriptorMaterials,
                                                   descriptorTextures,
                                                   descriptorSkybox,
                                                   descriptorTLAS};
    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayoutIBL,
                            0,
                            static_cast<uint32_t>(descriptorSets.size()),
                            &descriptorSets[0],
                            0,
                            nullptr);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    return VK_SUCCESS;
}

VkResult VulkanRendererLightComposition::renderLights(VkCommandBuffer &commandBuffer,
                                                      const InstancesManager &instances,
                                                      VkDescriptorSet &descriptorGBuffer,
                                                      VkDescriptorSet &descriptorScene,
                                                      VkDescriptorSet &descriptorModel,
                                                      VkDescriptorSet &descriptorLight,
                                                      VkDescriptorSet descriptorSkybox,
                                                      VkDescriptorSet &descriptorMaterials,
                                                      VkDescriptorSet &descriptorTextures,
                                                      VkDescriptorSet &descriptorTLAS) const
{
    const SceneObjectVector &lights = instances.lights();

    std::vector<VkDescriptorSet> descriptorSets = {descriptorGBuffer,
                                                   descriptorScene,
                                                   descriptorModel,
                                                   descriptorLight,
                                                   descriptorMaterials,
                                                   descriptorTextures,
                                                   descriptorSkybox,
                                                   descriptorTLAS};

    /* Point lights */
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelinePoint);
    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayoutPoint,
                            0,
                            static_cast<uint32_t>(descriptorSets.size()),
                            &descriptorSets[0],
                            0,
                            nullptr);
    for (uint32_t l = 0; l < lights.size(); l++) {
        if (lights[l]->get<ComponentLight>().light()->type() == LightType::POINT_LIGHT) {
            PushBlockLightComposition pushConstants;
            pushConstants.lights.r = l;

            vkCmdPushConstants(commandBuffer,
                               m_pipelineLayoutPoint,
                               VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               sizeof(PushBlockLightComposition),
                               &pushConstants);

            vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        }
    }

    /* Directional lights */
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineDirectional);
    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayoutDirectional,
                            0,
                            static_cast<uint32_t>(descriptorSets.size()),
                            &descriptorSets[0],
                            0,
                            nullptr);
    for (uint32_t l = 0; l < lights.size(); l++) {
        if (lights[l]->get<ComponentLight>().light()->type() == LightType::DIRECTIONAL_LIGHT) {
            PushBlockLightComposition pushConstants;
            pushConstants.lights.r = l;

            vkCmdPushConstants(commandBuffer,
                               m_pipelineLayoutDirectional,
                               VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               sizeof(PushBlockLightComposition),
                               &pushConstants);

            vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        }
    }

    return VK_SUCCESS;
}

VkResult VulkanRendererLightComposition::createPipelineIBL(const VulkanRenderPassDeferred &renderPass)
{
    VkShaderModule vertexShader = VulkanShader::load(m_ctx.device(), "shaders/SPIRV/quad.vert.spv");
    VkShaderModule fragmentShader = VulkanShader::load(m_ctx.device(), "shaders/SPIRV/lightCompositionIBL.frag.spv");

    VkPipelineShaderStageCreateInfo vertShaderStageInfo =
        vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, "main");
    VkPipelineShaderStageCreateInfo fragShaderStageInfo =
        vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader, "main");
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputStateCreateInfo(0, nullptr, 0, nullptr);
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::pipelineInputAssemblyCreateInfo();

    VkViewport viewport =
        vkinit::viewport(static_cast<float>(m_swapchainExtent.width), static_cast<float>(m_swapchainExtent.height), 0.0F, 1.0F);
    VkRect2D scissor =
        vkinit::rect2D(static_cast<int32_t>(m_swapchainExtent.width), static_cast<int32_t>(m_swapchainExtent.height), 0, 0);
    VkPipelineViewportStateCreateInfo viewportState = vkinit::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);

    VkPipelineRasterizationStateCreateInfo rasterizer =
        vkinit::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
    VkPipelineDepthStencilStateCreateInfo depthStencil =
        vkinit::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS);

    VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinit::pipelineColorBlendAttachmentState(
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_TRUE);
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    std::array<VkPipelineColorBlendAttachmentState, 1> colorBlendAttachments{colorBlendAttachment};
    VkPipelineColorBlendStateCreateInfo colorBlending =
        vkinit::pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data());

    std::vector<VkDescriptorSetLayout> descriptorSetsLayouts{renderPass.descriptorSetLayoutGBuffer(),
                                                             m_descriptorSetLayoutCamera,
                                                             m_descriptorSetLayoutInstanceData,
                                                             m_descriptorSetLayoutLightData,
                                                             m_descriptorSetLayoutMaterial,
                                                             m_descriptorSetLayoutTextures,
                                                             m_descriptorSetLayoutSkybox,
                                                             m_descriptorSetLayoutTLAS};
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(
        static_cast<uint32_t>(descriptorSetsLayouts.size()), descriptorSetsLayouts.data(), 0, nullptr);
    VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_ctx.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayoutIBL));

    VkGraphicsPipelineCreateInfo pipelineInfo =
        vkinit::graphicsPipelineCreateInfo(m_pipelineLayoutIBL, renderPass.renderPass(), renderPass.subpassIndexLightComposition());
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    VULKAN_CHECK_CRITICAL(
        vkCreateGraphicsPipelines(m_ctx.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipelineIBL));

    vkDestroyShaderModule(m_ctx.device(), vertexShader, nullptr);
    vkDestroyShaderModule(m_ctx.device(), fragmentShader, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRendererLightComposition::createPipelineLightInstance(const VulkanRenderPassDeferred &renderPass,
                                                                     std::string fragmentShader,
                                                                     VkPipelineLayout &outPipelineLayout,
                                                                     VkPipeline &outGraphicsPipeline)
{
    VkShaderModule vs = VulkanShader::load(m_ctx.device(), "shaders/SPIRV/quad.vert.spv");
    VkShaderModule fs = VulkanShader::load(m_ctx.device(), fragmentShader);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo =
        vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vs, "main");
    VkPipelineShaderStageCreateInfo fragShaderStageInfo =
        vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fs, "main");
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputStateCreateInfo(0, nullptr, 0, nullptr);
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::pipelineInputAssemblyCreateInfo();

    VkViewport viewport =
        vkinit::viewport(static_cast<float>(m_swapchainExtent.width), static_cast<float>(m_swapchainExtent.height), 0.0F, 1.0F);
    VkRect2D scissor =
        vkinit::rect2D(static_cast<int32_t>(m_swapchainExtent.width), static_cast<int32_t>(m_swapchainExtent.height), 0, 0);
    VkPipelineViewportStateCreateInfo viewportState = vkinit::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);

    VkPipelineRasterizationStateCreateInfo rasterizer =
        vkinit::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
    VkPipelineDepthStencilStateCreateInfo depthStencil =
        vkinit::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS);

    VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinit::pipelineColorBlendAttachmentState(
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_TRUE);
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    std::array<VkPipelineColorBlendAttachmentState, 1> colorBlendAttachments{colorBlendAttachment};
    VkPipelineColorBlendStateCreateInfo colorBlending =
        vkinit::pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data());

    std::vector<VkDescriptorSetLayout> descriptorSetsLayouts{renderPass.descriptorSetLayoutGBuffer(),
                                                             m_descriptorSetLayoutCamera,
                                                             m_descriptorSetLayoutInstanceData,
                                                             m_descriptorSetLayoutLightData,
                                                             m_descriptorSetLayoutMaterial,
                                                             m_descriptorSetLayoutTextures,
                                                             m_descriptorSetLayoutSkybox,
                                                             m_descriptorSetLayoutTLAS};
    VkPushConstantRange pushConstantRange =
        vkinit::pushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushBlockLightComposition), 0);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(
        static_cast<uint32_t>(descriptorSetsLayouts.size()), descriptorSetsLayouts.data(), 1, &pushConstantRange);

    VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_ctx.device(), &pipelineLayoutInfo, nullptr, &outPipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo =
        vkinit::graphicsPipelineCreateInfo(outPipelineLayout, renderPass.renderPass(), renderPass.subpassIndexLightComposition());
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    VULKAN_CHECK_CRITICAL(vkCreateGraphicsPipelines(m_ctx.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &outGraphicsPipeline));

    vkDestroyShaderModule(m_ctx.device(), vs, nullptr);
    vkDestroyShaderModule(m_ctx.device(), fs, nullptr);

    return VK_SUCCESS;
}

}  // namespace vengine