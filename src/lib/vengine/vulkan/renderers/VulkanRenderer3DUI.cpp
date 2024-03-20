#include "VulkanRenderer3DUI.hpp"

#include "Console.hpp"

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include <core/io/AssimpLoadModel.hpp>
#include <vulkan/common/VulkanInitializers.hpp>
#include <vulkan/common/VulkanShader.hpp>

namespace vengine
{

VulkanRenderer3DUI::VulkanRenderer3DUI()
{
}

VkResult VulkanRenderer3DUI::initResources(VkPhysicalDevice physicalDevice,
                                           VkDevice device,
                                           VkQueue queue,
                                           VkCommandPool commandPool,
                                           VkDescriptorSetLayout cameraDescriptorLayout)
{
    m_physicalDevice = physicalDevice;
    m_device = device;
    m_commandPool = commandPool;
    m_queue = queue;
    m_descriptorSetLayoutCamera = cameraDescriptorLayout;

    AssetInfo arrowInfo = AssetInfo("assets/models/arrow.obj", AssetSource::INTERNAL);
    Tree<ImportedModelNode> modelData = assimpLoadModel(arrowInfo);
    m_arrow = new VulkanModel3D(arrowInfo, modelData, {m_physicalDevice, m_device, m_commandPool, m_queue}, false);

    ID rightID = static_cast<ID>(ReservedObjectID::RIGHT_TRANSFORM_ARROW);
    m_rightID = IDGeneration::toRGB(rightID);
    ID upID = static_cast<ID>(ReservedObjectID::UP_TRANSFORM_ARROW);
    m_upID = IDGeneration::toRGB(upID);
    ID forwardID = static_cast<ID>(ReservedObjectID::FORWARD_TRANSFORM_ARROW);
    m_forwardID = IDGeneration::toRGB(forwardID);

    return VK_SUCCESS;
}

VkResult VulkanRenderer3DUI::initSwapChainResources(VkExtent2D swapchainExtent,
                                                    VkRenderPass renderPass,
                                                    uint32_t swapchainImages,
                                                    VkSampleCountFlagBits msaaSamples)
{
    m_swapchainExtent = swapchainExtent;
    m_renderPass = renderPass;
    m_msaaSamples = msaaSamples;

    VULKAN_CHECK_CRITICAL(createGraphicsPipelines());

    return VK_SUCCESS;
}

VkResult VulkanRenderer3DUI::releaseSwapChainResources()
{
    vkDestroyPipeline(m_device, m_graphicsPipelineTransform, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayoutTransform, nullptr);

    vkDestroyPipeline(m_device, m_graphicsPipelineAABB, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayoutAABB, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRenderer3DUI::releaseResources()
{
    m_arrow->destroy(m_device);

    return VK_SUCCESS;
}

VkResult VulkanRenderer3DUI::renderTransform(VkCommandBuffer &cmdBuf,
                                             VkDescriptorSet &descriptorScene,
                                             uint32_t imageIndex,
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

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineTransform);

    const auto &vkmesh = static_cast<VulkanMesh *>(m_arrow->mesh("Cone"));

    VkBuffer vertexBuffers[] = {vkmesh->vertexBuffer().buffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuf, vkmesh->indexBuffer().buffer(), 0, vkmesh->indexType());

    VkDescriptorSet descriptorSets[1] = {
        descriptorScene,
    };
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayoutTransform, 0, 1, &descriptorSets[0], 0, nullptr);

    PushBlockForward3DUITransform pushConstants;

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
    pushConstants.color = glm::vec4(0, 0, 1, 1);
    pushConstants.selected = glm::vec4(m_forwardID, 0);
    vkCmdPushConstants(cmdBuf,
                       m_pipelineLayoutTransform,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       sizeof(PushBlockForward3DUITransform),
                       &pushConstants);
    vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(vkmesh->indices().size()), 1, 0, 0, 0);
    /* Render X arrow */
    pushConstants.modelMatrix = glm::scale(glm::rotate(modelMatrixUnscaled, glm::radians(90.F), {0, 1, 0}), {scale, scale, scale});
    pushConstants.color = glm::vec4(1, 0, 0, 1);
    pushConstants.selected = glm::vec4(m_rightID, 0);
    vkCmdPushConstants(cmdBuf,
                       m_pipelineLayoutTransform,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       sizeof(PushBlockForward3DUITransform),
                       &pushConstants);
    vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(vkmesh->indices().size()), 1, 0, 0, 0);
    /* Render Y arrow */
    pushConstants.modelMatrix = glm::scale(glm::rotate(modelMatrixUnscaled, glm::radians(-90.F), {1, 0, 0}), {scale, scale, scale});
    pushConstants.color = glm::vec4(0, 1, 0, 1);
    pushConstants.selected = glm::vec4(m_upID, 0);
    vkCmdPushConstants(cmdBuf,
                       m_pipelineLayoutTransform,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       sizeof(PushBlockForward3DUITransform),
                       &pushConstants);
    vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(vkmesh->indices().size()), 1, 0, 0, 0);

    return VK_SUCCESS;
}

VkResult VulkanRenderer3DUI::renderAABB3(VkCommandBuffer &cmdBuf,
                                         VkDescriptorSet &descriptorScene,
                                         uint32_t imageIndex,
                                         const AABB3 &aabb,
                                         const std::shared_ptr<Camera> &camera) const
{
    vkCmdSetLineWidth(cmdBuf, 1.0f);
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineAABB);

    VkDescriptorSet descriptorSets[1] = {
        descriptorScene,
    };
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayoutAABB, 0, 1, &descriptorSets[0], 0, nullptr);

    PushBlockForward3DUIAABB pushConstants;
    pushConstants.min = glm::vec4(aabb.min(), 1);
    pushConstants.max = glm::vec4(aabb.max(), 1);
    pushConstants.color = glm::vec4(0.8, 0.8, 0, 1);
    pushConstants.selected = glm::vec4(0);
    vkCmdPushConstants(cmdBuf,
                       m_pipelineLayoutAABB,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       sizeof(PushBlockForward3DUIAABB),
                       &pushConstants);

    vkCmdDraw(cmdBuf, 1U, 1U, 0, 0);

    return VK_SUCCESS;
}

VkResult VulkanRenderer3DUI::createGraphicsPipelines()
{
    /* Create pipeline for Transform render */
    {
        VkPipelineShaderStageCreateInfo vertShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_VERTEX_BIT, VulkanShader::load(m_device, "shaders/SPIRV/3duiTransform.vert.spv"), "main");
        VkPipelineShaderStageCreateInfo fragShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_FRAGMENT_BIT, VulkanShader::load(m_device, "shaders/SPIRV/3duiTransform.frag.spv"), "main");
        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        auto bindingDescription = VulkanVertex::getBindingDescription();
        auto attributeDescriptions = VulkanVertex::getAttributeDescriptionsPos();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputStateCreateInfo(
            1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());

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
        std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments{colorBlendAttachment, colorBlendAttachment};
        VkPipelineColorBlendStateCreateInfo colorBlending = vkinit::pipelineColorBlendStateCreateInfo(
            static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data());

        std::array<VkDescriptorSetLayout, 1> descriptorSetsLayouts{m_descriptorSetLayoutCamera};
        VkPushConstantRange pushConstantRange = vkinit::pushConstantRange(
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushBlockForward3DUITransform), 0);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(
            static_cast<uint32_t>(descriptorSetsLayouts.size()), descriptorSetsLayouts.data(), 1, &pushConstantRange);
        VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayoutTransform));

        VkGraphicsPipelineCreateInfo pipelineInfo = vkinit::graphicsPipelineCreateInfo(m_pipelineLayoutTransform, m_renderPass, 0);
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
            vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipelineTransform));

        vkDestroyShaderModule(m_device, fragShaderStageInfo.module, nullptr);
        vkDestroyShaderModule(m_device, vertShaderStageInfo.module, nullptr);
    }

    /* Create pipeline for AABB render */
    {
        VkPipelineShaderStageCreateInfo vertShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_VERTEX_BIT, VulkanShader::load(m_device, "shaders/SPIRV/3duiAABB.vert.spv"), "main");
        VkPipelineShaderStageCreateInfo geomShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_GEOMETRY_BIT, VulkanShader::load(m_device, "shaders/SPIRV/3duiAABB.geom.spv"), "main");
        VkPipelineShaderStageCreateInfo fragShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_FRAGMENT_BIT, VulkanShader::load(m_device, "shaders/SPIRV/3duiAABB.frag.spv"), "main");
        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo, geomShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputStateCreateInfo(0, nullptr, 0, nullptr);
        VkPipelineInputAssemblyStateCreateInfo inputAssembly =
            vkinit::pipelineInputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);

        VkViewport viewport = vkinit::viewport(m_swapchainExtent.width, m_swapchainExtent.height, 0.0F, 1.0F);
        VkRect2D scissor = vkinit::rect2D(m_swapchainExtent.width, m_swapchainExtent.height, 0, 0);
        VkPipelineViewportStateCreateInfo viewportState = vkinit::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);

        VkPipelineRasterizationStateCreateInfo rasterizer =
            vkinit::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(m_msaaSamples);
        VkPipelineDepthStencilStateCreateInfo depthStencil =
            vkinit::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS);

        VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinit::pipelineColorBlendAttachmentState(
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
        std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments{colorBlendAttachment, colorBlendAttachment};
        VkPipelineColorBlendStateCreateInfo colorBlending = vkinit::pipelineColorBlendStateCreateInfo(
            static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data());

        std::array<VkDescriptorSetLayout, 1> descriptorSetsLayouts{m_descriptorSetLayoutCamera};
        VkPushConstantRange pushConstantRange =
            vkinit::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                      sizeof(PushBlockForward3DUIAABB),
                                      0);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(
            static_cast<uint32_t>(descriptorSetsLayouts.size()), descriptorSetsLayouts.data(), 1, &pushConstantRange);
        VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayoutAABB));

        VkGraphicsPipelineCreateInfo pipelineInfo = vkinit::graphicsPipelineCreateInfo(m_pipelineLayoutAABB, m_renderPass, 0);
        pipelineInfo.stageCount = 3;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        VULKAN_CHECK_CRITICAL(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipelineAABB));

        vkDestroyShaderModule(m_device, fragShaderStageInfo.module, nullptr);
        vkDestroyShaderModule(m_device, geomShaderStageInfo.module, nullptr);
        vkDestroyShaderModule(m_device, vertShaderStageInfo.module, nullptr);
    }

    return VK_SUCCESS;
}

}  // namespace vengine