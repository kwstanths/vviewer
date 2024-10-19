#include "VulkanRendererOverlay.hpp"

#include "Console.hpp"

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include <core/io/AssimpLoadModel.hpp>
#include <vulkan/common/VulkanInitializers.hpp>
#include <vulkan/common/VulkanShader.hpp>

namespace vengine
{

VulkanRendererOverlay::VulkanRendererOverlay(VulkanContext &context)
    : m_ctx(context)
{
}

VkResult VulkanRendererOverlay::initResources(VkDescriptorSetLayout cameraDescriptorLayout)
{
    m_descriptorSetLayoutCamera = cameraDescriptorLayout;

    AssetInfo arrowInfo = AssetInfo("assets/models/arrow.obj", AssetSource::INTERNAL);
    Tree<ImportedModelNode> modelData = assimpLoadModel(arrowInfo);
    m_arrow = new VulkanModel3D(arrowInfo,
                          modelData,
                          {m_ctx.physicalDevice(), m_ctx.device(), m_ctx.graphicsCommandPool(), m_ctx.queueManager().graphicsQueue()},
                          false);

    m_IdX = static_cast<ID>(ReservedObjectID::TRANSFORM_ARROW_X);
    m_IdY = static_cast<ID>(ReservedObjectID::TRANSFORM_ARROW_Y);
    m_IdZ = static_cast<ID>(ReservedObjectID::TRANSFORM_ARROW_Z);

    return VK_SUCCESS;
}

VkResult VulkanRendererOverlay::initSwapChainResources(VkExtent2D swapchainExtent, const VulkanRenderPassOverlay &renderPass)
{
    m_swapchainExtent = swapchainExtent;

    VULKAN_CHECK_CRITICAL(createGraphicsPipeline3DTransform(renderPass));
    VULKAN_CHECK_CRITICAL(createGraphicsPipelineAABB3(renderPass));
    VULKAN_CHECK_CRITICAL(createGraphicsPipelineOutline(renderPass));

    return VK_SUCCESS;
}

VkResult VulkanRendererOverlay::releaseSwapChainResources()
{
    vkDestroyPipeline(m_ctx.device(), m_graphicsPipeline3DTransform, nullptr);
    vkDestroyPipelineLayout(m_ctx.device(), m_pipelineLayout3DTransform, nullptr);

    vkDestroyPipeline(m_ctx.device(), m_graphicsPipelineAABB3, nullptr);
    vkDestroyPipelineLayout(m_ctx.device(), m_pipelineLayoutAABB3, nullptr);

    vkDestroyPipeline(m_ctx.device(), m_graphicsPipelineOutlineStencil, nullptr);
    vkDestroyPipeline(m_ctx.device(), m_graphicsPipelineOutlineWrite, nullptr);
    vkDestroyPipelineLayout(m_ctx.device(), m_pipelineLayoutOutline, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRendererOverlay::releaseResources()
{
    m_arrow->destroy(m_ctx.device());

    return VK_SUCCESS;
}

VkResult VulkanRendererOverlay::render3DTransform(VkCommandBuffer &cmdBuf,
                                                  VkDescriptorSet &descriptorScene,
                                                  const glm::mat4 &modelMatrix,
                                                  const std::shared_ptr<Camera> &camera) const
{
    /* Get global transform position */
    auto worldPos = getTranslation(modelMatrix);
    auto cameraDistance = glm::distance(camera->transform().position(), worldPos);

    /* Keep the same size for the transform on screen at all times */
    float scale = 0.0155f;
    scale *= cameraDistance;
    if (camera->type() == CameraType::PERSPECTIVE) {
        float fov = reinterpret_cast<PerspectiveCamera *>(camera.get())->fov();
        scale *= fov / 60.0F;
    }

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline3DTransform);

    const auto &vkmesh = static_cast<VulkanMesh *>(m_arrow->mesh("Cone"));

    VkBuffer vertexBuffers[] = {vkmesh->vertexBuffer().buffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuf, vkmesh->indexBuffer().buffer(), 0, vkmesh->indexType());

    VkDescriptorSet descriptorSets[1] = {
        descriptorScene,
    };
    vkCmdBindDescriptorSets(
        cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout3DTransform, 0, 1, &descriptorSets[0], 0, nullptr);

    PushBlockOverlayTransform3D pushConstants;

    /* Calculate the unscaled version of the input model matrix, if scale is zero this will fail */
    glm::mat4 modelMatrixUnscaled;
    {
        glm::vec3 scaleM, translation;
        glm::quat rotation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(modelMatrix, scaleM, rotation, translation, skew, perspective);
        modelMatrixUnscaled = glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(rotation);
    }

    /* Render Z arrow */
    pushConstants.modelMatrix = glm::scale(modelMatrixUnscaled, {scale, scale, scale});
    pushConstants.color = glm::vec4(0, 0, 1, m_IdZ);
    vkCmdPushConstants(cmdBuf,
                       m_pipelineLayout3DTransform,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       sizeof(PushBlockOverlayTransform3D),
                       &pushConstants);
    vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(vkmesh->indices().size()), 1, 0, 0, 0);
    /* Render X arrow */
    pushConstants.modelMatrix = glm::scale(glm::rotate(modelMatrixUnscaled, glm::radians(90.F), {0, 1, 0}), {scale, scale, scale});
    pushConstants.color = glm::vec4(1, 0, 0, m_IdX);
    vkCmdPushConstants(cmdBuf,
                       m_pipelineLayout3DTransform,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       sizeof(PushBlockOverlayTransform3D),
                       &pushConstants);
    vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(vkmesh->indices().size()), 1, 0, 0, 0);
    /* Render Y arrow */
    pushConstants.modelMatrix = glm::scale(glm::rotate(modelMatrixUnscaled, glm::radians(-90.F), {1, 0, 0}), {scale, scale, scale});
    pushConstants.color = glm::vec4(0, 1, 0, m_IdY);
    vkCmdPushConstants(cmdBuf,
                       m_pipelineLayout3DTransform,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       sizeof(PushBlockOverlayTransform3D),
                       &pushConstants);
    vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(vkmesh->indices().size()), 1, 0, 0, 0);

    return VK_SUCCESS;
}

VkResult VulkanRendererOverlay::renderAABB3(VkCommandBuffer &cmdBuf,
                                            VkDescriptorSet &descriptorScene,
                                            const AABB3 &aabb,
                                            const std::shared_ptr<Camera> &camera) const
{
    vkCmdSetLineWidth(cmdBuf, 1.0f);
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineAABB3);

    VkDescriptorSet descriptorSets[1] = {
        descriptorScene,
    };
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayoutAABB3, 0, 1, &descriptorSets[0], 0, nullptr);

    PushBlockOverlayAABB3 pushConstants;
    pushConstants.min = glm::vec4(aabb.min(), 1);
    pushConstants.max = glm::vec4(aabb.max(), 1);
    pushConstants.color = glm::vec4(0.8, 0.8, 0, 1);
    pushConstants.selected = glm::vec4(0);
    vkCmdPushConstants(cmdBuf,
                       m_pipelineLayoutAABB3,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       sizeof(PushBlockOverlayAABB3),
                       &pushConstants);

    vkCmdDraw(cmdBuf, 1U, 1U, 0, 0);

    return VK_SUCCESS;
}

VkResult VulkanRendererOverlay::renderOutline(VkCommandBuffer &cmdBuf,
                                              VkDescriptorSet &descriptorScene,
                                              SceneObject *so,
                                              const std::shared_ptr<Camera> &camera) const
{
    if (!so->has<ComponentMesh>())
        return VK_SUCCESS;

    std::array<VkDescriptorSet, 1> descriptorSets = {descriptorScene};

    const VulkanMesh *vkmesh = static_cast<const VulkanMesh *>(so->get<ComponentMesh>().mesh());

    VkBuffer vertexBuffers[] = {vkmesh->vertexBuffer().buffer()};
    VkDeviceSize offsets[] = {0};
    PushBlockOverlayOutline pushConstants;
    pushConstants.modelMatrix = so->modelMatrix();
    
    vkCmdBindDescriptorSets(cmdBuf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayoutOutline,
                            0,
                            static_cast<uint32_t>(descriptorSets.size()),
                            &descriptorSets[0],
                            0,
                            nullptr);
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuf, vkmesh->indexBuffer().buffer(), 0, vkmesh->indexType());

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineOutlineStencil);
    pushConstants.color = glm::vec4(1, 1, 1, 0);
    vkCmdPushConstants(cmdBuf,
                       m_pipelineLayoutOutline,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       sizeof(PushBlockOverlayOutline),
                       &pushConstants);
    vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(vkmesh->indices().size()), 1, 0, 0, 0);

    glm::vec3 worldPos = so->worldPosition();
    float cameraDistance = glm::distance(camera->transform().position(), worldPos);

    /* Calculate the scale of the model matrix */
    glm::vec3 scale;
    {
        glm::vec3 translation;
        glm::quat rotation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(so->modelMatrix(), scale, rotation, translation, skew, perspective);
    }

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineOutlineWrite);
    float scaleFactor = 0.05F / scale.x;
    scaleFactor *= cameraDistance / 20.F;
    pushConstants.color.a = scaleFactor;
    vkCmdPushConstants(cmdBuf,
                       m_pipelineLayoutOutline,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       sizeof(PushBlockOverlayOutline),
                       &pushConstants);
    vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(vkmesh->indices().size()), 1, 0, 0, 0);

    return VK_SUCCESS;
}

VkResult VulkanRendererOverlay::createGraphicsPipeline3DTransform(const VulkanRenderPassOverlay &renderPass)
{
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_VERTEX_BIT, VulkanShader::load(m_ctx.device(), "shaders/SPIRV/overlay/transform3D.vert.spv"), "main");
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_FRAGMENT_BIT, VulkanShader::load(m_ctx.device(), "shaders/SPIRV/overlay/transform3D.frag.spv"), "main");
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    auto bindingDescription = VulkanVertex::getBindingDescription();
    auto attributeDescriptions = VulkanVertex::getAttributeDescriptionsPos();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputStateCreateInfo(
        1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());

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
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
    VkPipelineColorBlendAttachmentState idBlendAttachment =
        vkinit::pipelineColorBlendAttachmentState(VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = {colorBlendAttachment, idBlendAttachment};
    VkPipelineColorBlendStateCreateInfo colorBlending =
        vkinit::pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data());

    std::array<VkDescriptorSetLayout, 1> descriptorSetsLayouts{m_descriptorSetLayoutCamera};
    VkPushConstantRange pushConstantRange =
        vkinit::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushBlockOverlayTransform3D), 0);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(
        static_cast<uint32_t>(descriptorSetsLayouts.size()), descriptorSetsLayouts.data(), 1, &pushConstantRange);
    VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_ctx.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout3DTransform));

    VkGraphicsPipelineCreateInfo pipelineInfo =
        vkinit::graphicsPipelineCreateInfo(m_pipelineLayout3DTransform, renderPass.renderPass(), 0);
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
        vkCreateGraphicsPipelines(m_ctx.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline3DTransform));

    vkDestroyShaderModule(m_ctx.device(), fragShaderStageInfo.module, nullptr);
    vkDestroyShaderModule(m_ctx.device(), vertShaderStageInfo.module, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRendererOverlay::createGraphicsPipelineAABB3(const VulkanRenderPassOverlay &renderPass)
{
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_VERTEX_BIT, VulkanShader::load(m_ctx.device(), "shaders/SPIRV/overlay/AABB3.vert.spv"), "main");
    VkPipelineShaderStageCreateInfo geomShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_GEOMETRY_BIT, VulkanShader::load(m_ctx.device(), "shaders/SPIRV/overlay/AABB3.geom.spv"), "main");
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_FRAGMENT_BIT, VulkanShader::load(m_ctx.device(), "shaders/SPIRV/overlay/AABB3.frag.spv"), "main");
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo, geomShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputStateCreateInfo(0, nullptr, 0, nullptr);
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::pipelineInputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);

    VkViewport viewport =
        vkinit::viewport(static_cast<float>(m_swapchainExtent.width), static_cast<float>(m_swapchainExtent.height), 0.0F, 1.0F);
    VkRect2D scissor =
        vkinit::rect2D(static_cast<int32_t>(m_swapchainExtent.width), static_cast<int32_t>(m_swapchainExtent.height), 0, 0);
    VkPipelineViewportStateCreateInfo viewportState = vkinit::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);

    VkPipelineRasterizationStateCreateInfo rasterizer =
        vkinit::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
    VkPipelineDepthStencilStateCreateInfo depthStencil =
        vkinit::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS);

    VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinit::pipelineColorBlendAttachmentState(
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
    VkPipelineColorBlendAttachmentState idBlendAttachment = vkinit::pipelineColorBlendAttachmentState(0, VK_FALSE);
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = {colorBlendAttachment, idBlendAttachment};
    VkPipelineColorBlendStateCreateInfo colorBlending =
        vkinit::pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data());

    std::array<VkDescriptorSetLayout, 1> descriptorSetsLayouts{m_descriptorSetLayoutCamera};
    VkPushConstantRange pushConstantRange = vkinit::pushConstantRange(
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushBlockOverlayAABB3), 0);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(
        static_cast<uint32_t>(descriptorSetsLayouts.size()), descriptorSetsLayouts.data(), 1, &pushConstantRange);
    VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_ctx.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayoutAABB3));

    VkGraphicsPipelineCreateInfo pipelineInfo = vkinit::graphicsPipelineCreateInfo(m_pipelineLayoutAABB3, renderPass.renderPass(), 0);
    pipelineInfo.stageCount = 3;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    VULKAN_CHECK_CRITICAL(
        vkCreateGraphicsPipelines(m_ctx.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipelineAABB3));

    vkDestroyShaderModule(m_ctx.device(), fragShaderStageInfo.module, nullptr);
    vkDestroyShaderModule(m_ctx.device(), geomShaderStageInfo.module, nullptr);
    vkDestroyShaderModule(m_ctx.device(), vertShaderStageInfo.module, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRendererOverlay::createGraphicsPipelineOutline(const VulkanRenderPassOverlay &renderPass)
{
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_VERTEX_BIT, VulkanShader::load(m_ctx.device(), "shaders/SPIRV/overlay/outline.vert.spv"), "main");
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_FRAGMENT_BIT, VulkanShader::load(m_ctx.device(), "shaders/SPIRV/overlay/outline.frag.spv"), "main");
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
        vkinit::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
    VkPipelineDepthStencilStateCreateInfo depthStencil =
        vkinit::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    /* Perform a depth test, but don't write the depth */
    /* Enable depth stencil test and always pass it. Write the value of 1 to the stencil buffer */
    depthStencil.stencilTestEnable = VK_TRUE;
    depthStencil.back.compareOp = VK_COMPARE_OP_ALWAYS;
    depthStencil.back.failOp = VK_STENCIL_OP_REPLACE;
    depthStencil.back.depthFailOp = VK_STENCIL_OP_REPLACE;
    depthStencil.back.passOp = VK_STENCIL_OP_REPLACE;
    depthStencil.back.compareMask = 0xff;
    depthStencil.back.writeMask = 0xff;
    depthStencil.back.reference = 1;
    depthStencil.front = depthStencil.back;

    /* no color writing */
    VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinit::pipelineColorBlendAttachmentState(0, VK_FALSE);
    VkPipelineColorBlendAttachmentState idBlendAttachment = vkinit::pipelineColorBlendAttachmentState(0, VK_FALSE);
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = {colorBlendAttachment, idBlendAttachment};
    VkPipelineColorBlendStateCreateInfo colorBlending =
        vkinit::pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data());

    std::array<VkDescriptorSetLayout, 1> descriptorSetsLayouts{m_descriptorSetLayoutCamera};
    VkPushConstantRange pushConstantRange =
        vkinit::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushBlockOverlayOutline), 0);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(
        static_cast<uint32_t>(descriptorSetsLayouts.size()), descriptorSetsLayouts.data(), 1, &pushConstantRange);
    VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_ctx.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayoutOutline));

    VkGraphicsPipelineCreateInfo pipelineInfo =
        vkinit::graphicsPipelineCreateInfo(m_pipelineLayoutOutline, renderPass.renderPass(), 0);
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
        vkCreateGraphicsPipelines(m_ctx.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipelineOutlineStencil));

    /* Don't perform a depth test and don't write depth */
    /* Do a stencil test and pass if the stencil is not equal */
    depthStencil.stencilTestEnable = VK_TRUE;
    depthStencil.back.compareOp = VK_COMPARE_OP_NOT_EQUAL;
    depthStencil.back.failOp = VK_STENCIL_OP_KEEP;
    depthStencil.back.depthFailOp = VK_STENCIL_OP_KEEP;
    depthStencil.back.passOp = VK_STENCIL_OP_REPLACE;
    depthStencil.front = depthStencil.back;
    depthStencil.depthTestEnable = VK_FALSE;
    colorBlendAttachment = vkinit::pipelineColorBlendAttachmentState(
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT, VK_FALSE);
    idBlendAttachment = vkinit::pipelineColorBlendAttachmentState(0, VK_FALSE);
    colorBlendAttachments = {colorBlendAttachment, idBlendAttachment};
    colorBlending =
        vkinit::pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data());

    VULKAN_CHECK_CRITICAL(
        vkCreateGraphicsPipelines(m_ctx.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipelineOutlineWrite));

    vkDestroyShaderModule(m_ctx.device(), fragShaderStageInfo.module, nullptr);
    vkDestroyShaderModule(m_ctx.device(), vertShaderStageInfo.module, nullptr);

    return VK_SUCCESS;
}

}  // namespace vengine