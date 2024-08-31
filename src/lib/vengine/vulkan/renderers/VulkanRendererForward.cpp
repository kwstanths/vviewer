#include "VulkanRendererForward.hpp"

#include "utils/Algorithms.hpp"

#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanShader.hpp"
#include "vulkan/resources/VulkanMesh.hpp"
#include "vulkan/common/VulkanStructs.hpp"

namespace vengine
{

VulkanRendererForward::VulkanRendererForward(VulkanContext &context)
    : m_ctx(context)
{
}

VkResult VulkanRendererForward::initResources(VkDescriptorSetLayout sceneDataDescriptorLayout,
                                              VkDescriptorSetLayout instanceDataDescriptorLayout,
                                              VkDescriptorSetLayout lightDataDescriptorLayout,
                                              VkDescriptorSetLayout skyboxDescriptorLayout,
                                              VkDescriptorSetLayout materialDescriptorLayout,
                                              VkDescriptorSetLayout texturesDescriptorLayout,
                                              VkDescriptorSetLayout tlasDescriptorLayout)
{
    m_descriptorSetLayoutSceneData = sceneDataDescriptorLayout;
    m_descriptorSetLayoutInstanceData = instanceDataDescriptorLayout;
    m_descriptorSetLayoutLightData = lightDataDescriptorLayout;
    m_descriptorSetLayoutSkybox = skyboxDescriptorLayout;
    m_descriptorSetLayoutMaterial = materialDescriptorLayout;
    m_descriptorSetLayoutTextures = texturesDescriptorLayout;
    m_descriptorSetLayoutTLAS = tlasDescriptorLayout;

    return VK_SUCCESS;
}

VkResult VulkanRendererForward::initSwapChainResources(VkExtent2D swapchainExtent,
                                                       const VulkanRenderPassDeferred &renderPass,
                                                       const std::string &fragmentShader)
{
    m_swapchainExtent = swapchainExtent;

    VULKAN_CHECK_CRITICAL(createPipeline(renderPass, fragmentShader));

    return VK_SUCCESS;
}

VkResult VulkanRendererForward::releaseResources()
{
    return VK_SUCCESS;
}

VkResult VulkanRendererForward::releaseSwapChainResources()
{
    vkDestroyPipeline(m_ctx.device(), m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_ctx.device(), m_pipelineLayout, nullptr);
    return VK_SUCCESS;
}

VkResult VulkanRendererForward::renderObject(VkCommandBuffer &cmdBuf,
                                             const VulkanInstancesManager &instances,
                                             VkDescriptorSet &descriptorScene,
                                             VkDescriptorSet &descriptorModel,
                                             VkDescriptorSet &descriptorLight,
                                             VkDescriptorSet descriptorSkybox,
                                             VkDescriptorSet &descriptorMaterials,
                                             VkDescriptorSet &descriptorTextures,
                                             VkDescriptorSet &descriptorTLAS,
                                             SceneObject *object,
                                             const SceneObjectVector &lights) const
{
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    std::array<VkDescriptorSet, 7> descriptorSets = {
        descriptorScene, descriptorModel, descriptorLight, descriptorMaterials, descriptorTextures, descriptorSkybox, descriptorTLAS};

    vkCmdBindDescriptorSets(cmdBuf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayout,
                            0,
                            static_cast<uint32_t>(descriptorSets.size()),
                            &descriptorSets[0],
                            0,
                            nullptr);

    VulkanSceneObject *vkobject = static_cast<VulkanSceneObject *>(object);

    const VulkanMesh *vkmesh = static_cast<const VulkanMesh *>(vkobject->get<ComponentMesh>().mesh());
    if (vkmesh == nullptr) {
        debug_tools::ConsoleWarning("VulkanRendererForward::renderObject(): SceneObject vkmesh is a null pointer");
        return VK_ERROR_UNKNOWN;
    }

    VkBuffer vertexBuffers[] = {vkmesh->vertexBuffer().buffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuf, vkmesh->indexBuffer().buffer(), 0, vkmesh->indexType());

    Material *material = vkobject->get<ComponentMaterial>().material();
    PushBlockForward pushConstants;
    pushConstants.selected = glm::vec4(vkobject->getID(), 0.0, 0.0, 0.0);
    pushConstants.info.r = material->materialIndex();
    pushConstants.info.g = instances.findInstanceDataIndex(vkobject);

    /* Find the 4 strongest lights to current object */
    glm::vec3 pos = vkobject->worldPosition();
    auto closestLights =
        findNSmallest<SceneObject *, 4>(instances.lights(), [&](SceneObject *const &m, SceneObject *const &n) -> bool {
            auto lmp = m->get<ComponentLight>().light()->power(m->worldPosition(), pos);
            auto lnp = n->get<ComponentLight>().light()->power(n->worldPosition(), pos);
            return lmp > lnp;
        });
    pushConstants.info.b = std::min<unsigned int>(static_cast<uint32_t>(instances.lights().size()), 4U);
    pushConstants.lights = glm::vec4(closestLights[0], closestLights[1], closestLights[2], closestLights[3]);

    vkCmdPushConstants(cmdBuf,
                       m_pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       sizeof(PushBlockForward),
                       &pushConstants);

    vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(vkmesh->indices().size()), 1, 0, 0, pushConstants.info.g);

    return VK_SUCCESS;
}

VkResult VulkanRendererForward::createPipeline(const VulkanRenderPassDeferred &renderPass, const std::string &fragmentShader)
{
    VkShaderModule vs = VulkanShader::load(m_ctx.device(), "shaders/SPIRV/standard.vert.spv");
    VkShaderModule fs = VulkanShader::load(m_ctx.device(), fragmentShader);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo =
        vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vs, "main");
    VkPipelineShaderStageCreateInfo fragShaderStageInfo =
        vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fs, "main");
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    auto bindingDescription = VulkanVertex::getBindingDescription();
    auto attributeDescriptions = VulkanVertex::getAttributeDescriptionsFull();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputStateCreateInfo(
        1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::pipelineInputAssemblyCreateInfo();

    VkViewport viewport =
        vkinit::viewport(static_cast<float>(m_swapchainExtent.width), static_cast<float>(m_swapchainExtent.height), 0.0F, 1.0F);
    VkRect2D scissor =
        vkinit::rect2D(static_cast<int32_t>(m_swapchainExtent.width), static_cast<int32_t>(m_swapchainExtent.height), 0, 0);
    VkPipelineViewportStateCreateInfo viewportState = vkinit::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);

    VkPipelineRasterizationStateCreateInfo rasterizer =
        vkinit::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
    VkPipelineDepthStencilStateCreateInfo depthStencil =
        vkinit::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS);

    /* Output 1 is GBuffer0, Output 2 is color */
    VkPipelineColorBlendAttachmentState colorBlendAttachment1 = vkinit::pipelineColorBlendAttachmentState(
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
    /* Blend color */
    VkPipelineColorBlendAttachmentState colorBlendAttachment2 = vkinit::pipelineColorBlendAttachmentState(
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_TRUE);
    colorBlendAttachment2.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment2.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment2.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment2.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment2.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment2.alphaBlendOp = VK_BLEND_OP_ADD;

    std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments{colorBlendAttachment1, colorBlendAttachment2};
    VkPipelineColorBlendStateCreateInfo colorBlending =
        vkinit::pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data());

    std::array<VkDescriptorSetLayout, 7> descriptorSetsLayouts{m_descriptorSetLayoutSceneData,
                                                               m_descriptorSetLayoutInstanceData,
                                                               m_descriptorSetLayoutLightData,
                                                               m_descriptorSetLayoutMaterial,
                                                               m_descriptorSetLayoutTextures,
                                                               m_descriptorSetLayoutSkybox,
                                                               m_descriptorSetLayoutTLAS};
    VkPushConstantRange pushConstantRange =
        vkinit::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushBlockForward), 0);
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(
        static_cast<uint32_t>(descriptorSetsLayouts.size()), descriptorSetsLayouts.data(), 1, &pushConstantRange);
    VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_ctx.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo =
        vkinit::graphicsPipelineCreateInfo(m_pipelineLayout, renderPass.renderPass(), renderPass.subpassIndexForward());
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

    vkDestroyShaderModule(m_ctx.device(), vs, nullptr);
    vkDestroyShaderModule(m_ctx.device(), fs, nullptr);

    return VK_SUCCESS;
}

}  // namespace vengine