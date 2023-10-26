#include "VulkanRendererLambert.hpp"

#include <debug_tools/Console.hpp>
#include <set>

#include "core/AssetManager.hpp"

#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanUtils.hpp"
#include "vulkan/common/VulkanShader.hpp"
#include "vulkan/common/VulkanStructs.hpp"
#include "vulkan/resources/VulkanMesh.hpp"

namespace vengine
{

VulkanRendererLambert::VulkanRendererLambert()
{
}

VkResult VulkanRendererLambert::initResources(VkPhysicalDevice physicalDevice,
                                              VkDevice device,
                                              VkQueue queue,
                                              VkCommandPool commandPool,
                                              VkPhysicalDeviceProperties physicalDeviceProperties,
                                              VkDescriptorSetLayout cameraDescriptorLayout,
                                              VkDescriptorSetLayout modelDescriptorLayout,
                                              VkDescriptorSetLayout skyboxDescriptorLayout,
                                              VkDescriptorSetLayout materialDescriptorLayout,
                                              VkDescriptorSetLayout texturesDescriptorLayout)
{
    m_physicalDevice = physicalDevice;
    m_device = device;
    m_commandPool = commandPool;
    m_queue = queue;

    m_descriptorSetLayoutCamera = cameraDescriptorLayout;
    m_descriptorSetLayoutModel = modelDescriptorLayout;
    m_descriptorSetLayoutSkybox = skyboxDescriptorLayout;
    m_descriptorSetLayoutMaterial = materialDescriptorLayout;
    m_descriptorSetLayoutTextures = texturesDescriptorLayout;

    return VK_SUCCESS;
}

VkResult VulkanRendererLambert::initSwapChainResources(VkExtent2D swapchainExtent,
                                                       VkRenderPass renderPass,
                                                       uint32_t swapchainImages,
                                                       VkSampleCountFlagBits msaaSamples)
{
    m_swapchainExtent = swapchainExtent;
    m_renderPass = renderPass;
    m_msaaSamples = msaaSamples;

    VULKAN_CHECK_CRITICAL(createGraphicsPipelineBasePass());
    VULKAN_CHECK_CRITICAL(createGraphicsPipelineAddPass());

    return VK_SUCCESS;
}

VkResult VulkanRendererLambert::releaseSwapChainResources()
{
    vkDestroyPipeline(m_device, m_graphicsPipelineBasePass, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayoutBasePass, nullptr);
    vkDestroyPipeline(m_device, m_graphicsPipelineAddPass, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayoutAddPass, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRendererLambert::releaseResources()
{
    return VK_SUCCESS;
}

VkResult VulkanRendererLambert::renderObjectsBasePass(VkCommandBuffer &cmdBuf,
                                                      VkDescriptorSet &descriptorScene,
                                                      VkDescriptorSet &descriptorModel,
                                                      VkDescriptorSet descriptorSkybox,
                                                      VkDescriptorSet &descriptorMaterial,
                                                      VkDescriptorSet &descriptorTextures,
                                                      uint32_t imageIndex,
                                                      const VulkanUBO<ModelData> &dynamicUBOModels,
                                                      std::vector<std::shared_ptr<SceneObject>> &objects) const
{
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineBasePass);
    for (size_t i = 0; i < objects.size(); i++) {
        VulkanSceneObject *object = static_cast<VulkanSceneObject *>(objects[i].get());

        const VulkanMesh *vkmesh = static_cast<const VulkanMesh *>(object->get<ComponentMesh>().mesh.get());
        if (vkmesh == nullptr)
            continue;

        VulkanMaterialLambert *material = static_cast<VulkanMaterialLambert *>(object->get<ComponentMaterial>().material.get());

        VkBuffer vertexBuffers[] = {vkmesh->vertexBuffer().buffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(cmdBuf, vkmesh->indexBuffer().buffer(), 0, vkmesh->indexType());

        /* Calculate model data offsets */
        std::array<uint32_t, 1> dynamicOffsets = {dynamicUBOModels.blockSizeAligned() * object->getTransformUBOBlock()};
        std::array<VkDescriptorSet, 5> descriptorSets = {
            descriptorScene, descriptorModel, descriptorMaterial, descriptorTextures, descriptorSkybox};

        PushBlockForwardBasePass pushConstants;
        pushConstants.selected = glm::vec4(object->getIDRGB(), object->m_isSelected);
        pushConstants.material.r = material->getMaterialIndex();
        vkCmdPushConstants(
            cmdBuf, m_pipelineLayoutBasePass, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlockForwardBasePass), &pushConstants);

        vkCmdBindDescriptorSets(cmdBuf,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_pipelineLayoutBasePass,
                                0,
                                static_cast<uint32_t>(descriptorSets.size()),
                                &descriptorSets[0],
                                static_cast<uint32_t>(dynamicOffsets.size()),
                                &dynamicOffsets[0]);

        vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(vkmesh->indices().size()), 1, 0, 0, 0);
    }

    return VK_SUCCESS;
}

VkResult VulkanRendererLambert::renderObjectsAddPass(VkCommandBuffer &cmdBuf,
                                                     VkDescriptorSet &descriptorScene,
                                                     VkDescriptorSet &descriptorModel,
                                                     VkDescriptorSet &descriptorMaterial,
                                                     VkDescriptorSet &descriptorTextures,
                                                     uint32_t imageIndex,
                                                     const VulkanUBO<ModelData> &dynamicUBOModels,
                                                     std::shared_ptr<SceneObject> object,
                                                     PushBlockForwardAddPass &lightInfo) const
{
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineAddPass);

    VulkanSceneObject *vobject = static_cast<VulkanSceneObject *>(object.get());
    const VulkanMesh *vkmesh = static_cast<const VulkanMesh *>(vobject->get<ComponentMesh>().mesh.get());
    assert(vkmesh != nullptr);

    VulkanMaterialLambert *material = static_cast<VulkanMaterialLambert *>(vobject->get<ComponentMaterial>().material.get());

    VkBuffer vertexBuffers[] = {vkmesh->vertexBuffer().buffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuf, vkmesh->indexBuffer().buffer(), 0, vkmesh->indexType());

    /* Calculate model data offsets */
    std::array<uint32_t, 1> dynamicOffsets = {
        dynamicUBOModels.blockSizeAligned() * vobject->getTransformUBOBlock(),
    };
    /* Create descriptor sets array */
    std::array<VkDescriptorSet, 4> descriptorSets = {
        descriptorScene,
        descriptorModel,
        descriptorMaterial,
        descriptorTextures,
    };

    lightInfo.material.r = material->getMaterialIndex();

    vkCmdPushConstants(cmdBuf, m_pipelineLayoutAddPass, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlockForwardAddPass), &lightInfo);
    vkCmdBindDescriptorSets(cmdBuf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayoutAddPass,
                            0,
                            static_cast<uint32_t>(descriptorSets.size()),
                            &descriptorSets[0],
                            static_cast<uint32_t>(dynamicOffsets.size()),
                            &dynamicOffsets[0]);
    vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(vkmesh->indices().size()), 1, 0, 0, 0);

    return VK_SUCCESS;
}

VkResult VulkanRendererLambert::createGraphicsPipelineBasePass()
{
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_VERTEX_BIT, VulkanShader::load(m_device, "shaders/SPIRV/standard.vert.spv"), "main");
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_FRAGMENT_BIT, VulkanShader::load(m_device, "shaders/SPIRV/lambertBase.frag.spv"), "main");
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    auto bindingDescription = VulkanVertex::getBindingDescription();
    auto attributeDescriptions = VulkanVertex::getAttributeDescriptionsFull();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputStateCreateInfo(
        1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::pipelineInputAssemblyCreateInfo();

    VkViewport viewport = vkinit::viewport(m_swapchainExtent.width, m_swapchainExtent.height, 0.0F, 1.0F);
    VkRect2D scissor = vkinit::rect2D(m_swapchainExtent.width, m_swapchainExtent.height, 0, 0);
    VkPipelineViewportStateCreateInfo viewportState = vkinit::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);

    VkPipelineRasterizationStateCreateInfo rasterizer =
        vkinit::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(m_msaaSamples);
    VkPipelineDepthStencilStateCreateInfo depthStencil =
        vkinit::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);

    VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinit::pipelineColorBlendAttachmentState(
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
    std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments{colorBlendAttachment, colorBlendAttachment};
    VkPipelineColorBlendStateCreateInfo colorBlending =
        vkinit::pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data());

    std::array<VkDescriptorSetLayout, 5> descriptorSetsLayouts{m_descriptorSetLayoutCamera,
                                                               m_descriptorSetLayoutModel,
                                                               m_descriptorSetLayoutMaterial,
                                                               m_descriptorSetLayoutTextures,
                                                               m_descriptorSetLayoutSkybox};
    VkPushConstantRange pushConstantRange =
        vkinit::pushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushBlockForwardBasePass), 0);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(
        static_cast<uint32_t>(descriptorSetsLayouts.size()), descriptorSetsLayouts.data(), 1, &pushConstantRange);
    VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayoutBasePass));

    VkGraphicsPipelineCreateInfo pipelineInfo = vkinit::graphicsPipelineCreateInfo(m_pipelineLayoutBasePass, m_renderPass, 0);
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    VULKAN_CHECK_CRITICAL(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipelineBasePass));

    vkDestroyShaderModule(m_device, fragShaderStageInfo.module, nullptr);
    vkDestroyShaderModule(m_device, vertShaderStageInfo.module, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRendererLambert::createGraphicsPipelineAddPass()
{
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_VERTEX_BIT, VulkanShader::load(m_device, "shaders/SPIRV/standard.vert.spv"), "main");
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_FRAGMENT_BIT, VulkanShader::load(m_device, "shaders/SPIRV/lambertAdd.frag.spv"), "main");
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    auto bindingDescription = VulkanVertex::getBindingDescription();
    auto attributeDescriptions = VulkanVertex::getAttributeDescriptionsFull();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputStateCreateInfo(
        1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::pipelineInputAssemblyCreateInfo();

    VkViewport viewport = vkinit::viewport(m_swapchainExtent.width, m_swapchainExtent.height, 0.0F, 1.0F);
    VkRect2D scissor = vkinit::rect2D(m_swapchainExtent.width, m_swapchainExtent.height, 0, 0);
    VkPipelineViewportStateCreateInfo viewportState = vkinit::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);

    VkPipelineRasterizationStateCreateInfo rasterizer =
        vkinit::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(m_msaaSamples);
    VkPipelineDepthStencilStateCreateInfo depthStencil =
        vkinit::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_FALSE, VK_COMPARE_OP_EQUAL);

    VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinit::pipelineColorBlendAttachmentState(
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_TRUE);
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments{colorBlendAttachment, colorBlendAttachment};
    VkPipelineColorBlendStateCreateInfo colorBlending =
        vkinit::pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data());

    std::array<VkDescriptorSetLayout, 4> descriptorSetsLayouts{
        m_descriptorSetLayoutCamera, m_descriptorSetLayoutModel, m_descriptorSetLayoutMaterial, m_descriptorSetLayoutTextures};
    VkPushConstantRange pushConstantRange =
        vkinit::pushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushBlockForwardAddPass), 0);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(
        static_cast<uint32_t>(descriptorSetsLayouts.size()), descriptorSetsLayouts.data(), 1, &pushConstantRange);
    VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayoutAddPass));

    VkGraphicsPipelineCreateInfo pipelineInfo = vkinit::graphicsPipelineCreateInfo(m_pipelineLayoutAddPass, m_renderPass, 0);
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    VULKAN_CHECK_CRITICAL(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipelineAddPass));

    vkDestroyShaderModule(m_device, fragShaderStageInfo.module, nullptr);
    vkDestroyShaderModule(m_device, vertShaderStageInfo.module, nullptr);

    return VK_SUCCESS;
}

}  // namespace vengine