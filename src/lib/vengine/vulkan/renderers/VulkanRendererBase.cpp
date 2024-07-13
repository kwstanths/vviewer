#include "VulkanRendererBase.hpp"

#include "utils/Algorithms.hpp"

#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanShader.hpp"
#include "vulkan/resources/VulkanMesh.hpp"
#include "vulkan/common/VulkanStructs.hpp"

namespace vengine
{

VulkanRendererBase::VulkanRendererBase()
{
}

VkResult VulkanRendererBase::initResources(VkPhysicalDevice physicalDevice,
                                           VkDevice device,
                                           VkQueue queue,
                                           VkCommandPool commandPool,
                                           VkPhysicalDeviceProperties physicalDeviceProperties,
                                           VkDescriptorSetLayout cameraDescriptorLayout,
                                           VkDescriptorSetLayout modelDescriptorLayout,
                                           VkDescriptorSetLayout lightDescriptorLayout,
                                           VkDescriptorSetLayout skyboxDescriptorLayout,
                                           VkDescriptorSetLayout materialDescriptorLayout,
                                           VkDescriptorSetLayout texturesDescriptorLayout,
                                           VkDescriptorSetLayout tlasDescriptorLayout)
{
    m_physicalDevice = physicalDevice;
    m_device = device;
    m_commandPool = commandPool;
    m_queue = queue;
    m_descriptorSetLayoutCamera = cameraDescriptorLayout;
    m_descriptorSetLayoutModel = modelDescriptorLayout;
    m_descriptorSetLayoutLight = lightDescriptorLayout;
    m_descriptorSetLayoutSkybox = skyboxDescriptorLayout;
    m_descriptorSetLayoutMaterial = materialDescriptorLayout;
    m_descriptorSetLayoutTextures = texturesDescriptorLayout;
    m_descriptorSetLayoutTLAS = tlasDescriptorLayout;

    return VK_SUCCESS;
}

VkResult VulkanRendererBase::initSwapChainResources(VkExtent2D swapchainExtent,
                                                    VkRenderPass renderPass,
                                                    uint32_t swapchainImages,
                                                    VkSampleCountFlagBits msaaSamples)
{
    m_swapchainExtent = swapchainExtent;
    m_renderPass = renderPass;
    m_msaaSamples = msaaSamples;

    return VK_SUCCESS;
}

VkResult VulkanRendererBase::createPipelineForwardOpaque(VkShaderModule vertexShader,
                                                         VkShaderModule fragmentShader,
                                                         VkPipelineLayout &pipelineLayout,
                                                         VkPipeline &pipeline)
{
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
    VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(m_msaaSamples);
    VkPipelineDepthStencilStateCreateInfo depthStencil =
        vkinit::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);

    VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinit::pipelineColorBlendAttachmentState(
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
    std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments{colorBlendAttachment, colorBlendAttachment};
    VkPipelineColorBlendStateCreateInfo colorBlending =
        vkinit::pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data());

    std::vector<VkDescriptorSetLayout> descriptorSetsLayouts{m_descriptorSetLayoutCamera,
                                                             m_descriptorSetLayoutModel,
                                                             m_descriptorSetLayoutLight,
                                                             m_descriptorSetLayoutMaterial,
                                                             m_descriptorSetLayoutTextures,
                                                             m_descriptorSetLayoutSkybox,
                                                             m_descriptorSetLayoutTLAS};
    VkPushConstantRange pushConstantRange =
        vkinit::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushBlockForward), 0);
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(
        static_cast<uint32_t>(descriptorSetsLayouts.size()), descriptorSetsLayouts.data(), 1, &pushConstantRange);
    VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo = vkinit::graphicsPipelineCreateInfo(pipelineLayout, m_renderPass, 0);
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    VULKAN_CHECK_CRITICAL(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

    return VK_SUCCESS;
}

VkResult VulkanRendererBase::createPipelineForwardTransparent(VkShaderModule vertexShader,
                                                              VkShaderModule fragmentShader,
                                                              VkPipelineLayout &pipelineLayout,
                                                              VkPipeline &pipeline)
{
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
    VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(m_msaaSamples);
    VkPipelineDepthStencilStateCreateInfo depthStencil =
        vkinit::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS);

    VkPipelineColorBlendAttachmentState colorBlendAttachment1 = vkinit::pipelineColorBlendAttachmentState(
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_TRUE);
    colorBlendAttachment1.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment1.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment1.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment1.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment1.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment1.alphaBlendOp = VK_BLEND_OP_ADD;
    VkPipelineColorBlendAttachmentState colorBlendAttachment2 = vkinit::pipelineColorBlendAttachmentState(
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);

    std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments{colorBlendAttachment1, colorBlendAttachment2};
    VkPipelineColorBlendStateCreateInfo colorBlending =
        vkinit::pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data());

    std::array<VkDescriptorSetLayout, 7> descriptorSetsLayouts{m_descriptorSetLayoutCamera,
                                                               m_descriptorSetLayoutModel,
                                                               m_descriptorSetLayoutLight,
                                                               m_descriptorSetLayoutMaterial,
                                                               m_descriptorSetLayoutTextures,
                                                               m_descriptorSetLayoutSkybox,
                                                               m_descriptorSetLayoutTLAS};
    VkPushConstantRange pushConstantRange =
        vkinit::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushBlockForward), 0);
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(
        static_cast<uint32_t>(descriptorSetsLayouts.size()), descriptorSetsLayouts.data(), 1, &pushConstantRange);
    VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo = vkinit::graphicsPipelineCreateInfo(pipelineLayout, m_renderPass, 0);
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    VULKAN_CHECK_CRITICAL(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

    return VK_SUCCESS;
}

VkResult VulkanRendererBase::renderMaterialGroup(VkCommandBuffer &commandBuffer,
                                                 const VulkanInstancesManager::MaterialGroup &materialGroup,
                                                 const VkPipelineLayout &pipelineLayout,
                                                 const SceneGraph &lights) const
{
    for (auto &meshGroup : materialGroup.meshGroups) {
        const VulkanMesh *vkmesh = static_cast<const VulkanMesh *>(meshGroup.first);
        assert(vkmesh != nullptr);

        VkBuffer vertexBuffers[] = {vkmesh->vertexBuffer().buffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, vkmesh->indexBuffer().buffer(), 0, vkmesh->indexType());

        PushBlockForward pushConstants;
        pushConstants.info.b = std::min<unsigned int>(lights.size(), 4U);
        pushConstants.lights = glm::vec4(0, 1, 2, 3);

        vkCmdPushConstants(commandBuffer,
                           pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(PushBlockForward),
                           &pushConstants);

        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(vkmesh->indices().size()),
                         meshGroup.second.sceneObjects.size(),
                         0,
                         0,
                         meshGroup.second.startIndex);
    }

    return VK_SUCCESS;
}

VkResult VulkanRendererBase::renderObjects(VkCommandBuffer &commandBuffer,
                                           VulkanInstancesManager &instances,
                                           const VkPipelineLayout &pipelineLayout,
                                           const SceneGraph &objects,
                                           const SceneGraph &lights) const
{
    for (size_t i = 0; i < objects.size(); i++) {
        VulkanSceneObject *vkobject = static_cast<VulkanSceneObject *>(objects[i]);

        const VulkanMesh *vkmesh = static_cast<const VulkanMesh *>(vkobject->get<ComponentMesh>().mesh);
        if (vkmesh == nullptr) {
            debug_tools::ConsoleWarning("VulkanRendererBase::renderObjects(): SceneObject mesh component has a null pointer");
            continue;
        }

        VkBuffer vertexBuffers[] = {vkmesh->vertexBuffer().buffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, vkmesh->indexBuffer().buffer(), 0, vkmesh->indexType());

        Material *material = vkobject->get<ComponentMaterial>().material;
        PushBlockForward pushConstants;
        pushConstants.selected = glm::vec4(vkobject->getIDRGB(), vkobject->selected());
        pushConstants.info.r = material->materialIndex();
        pushConstants.info.g = instances.getInstanceDataIndex(vkobject);

        /* Find the 4 strongest lights to current object */
        glm::vec3 pos = vkobject->worldPosition();
        auto closestLights = findNSmallest<SceneObject *, 4>(lights, [&](SceneObject *const &m, SceneObject *const &n) -> bool {
            auto lmp = m->get<ComponentLight>().light->power(m->worldPosition(), pos);
            auto lnp = n->get<ComponentLight>().light->power(n->worldPosition(), pos);
            return lmp > lnp;
        });
        pushConstants.info.b = std::min<unsigned int>(lights.size(), 4U);
        pushConstants.lights = glm::vec4(closestLights[0], closestLights[1], closestLights[2], closestLights[3]);

        vkCmdPushConstants(commandBuffer,
                           pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(PushBlockForward),
                           &pushConstants);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(vkmesh->indices().size()), 1, 0, 0, pushConstants.info.g);
    }

    return VK_SUCCESS;
}

}  // namespace vengine