#include "VulkanRenderer3DUI.hpp"

#include "Console.hpp"

#include <models/AssimpLoadModel.hpp>

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include "vulkan/Shader.hpp"

VulkanRenderer3DUI::VulkanRenderer3DUI()
{
}

void VulkanRenderer3DUI::initResources(VkPhysicalDevice physicalDevice, 
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
    
    std::vector<Mesh> meshes = assimpLoadModel("assets/models/arrow.obj");
    m_arrow = new VulkanMeshModel(m_physicalDevice, m_device, queue, commandPool, meshes);

    ID rightID = static_cast<ID>(ReservedObjectID::RIGHT_TRANSFORM_ARROW);
    m_rightID = IDGeneration::toRGB(rightID);
    ID upID = static_cast<ID>(ReservedObjectID::UP_TRANSFORM_ARROW);
    m_upID = IDGeneration::toRGB(upID);
    ID forwardID = static_cast<ID>(ReservedObjectID::FORWARD_TRANSFORM_ARROW);
    m_forwardID = IDGeneration::toRGB(forwardID);
}

void VulkanRenderer3DUI::initSwapChainResources(VkExtent2D swapchainExtent, VkRenderPass renderPass, uint32_t swapchainImages)
{
    m_swapchainExtent = swapchainExtent;
    m_renderPass = renderPass;

    createGraphicsPipeline();
}

void VulkanRenderer3DUI::releaseSwapChainResources()
{
    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
}

void VulkanRenderer3DUI::releaseResources()
{
    m_arrow->destroy(m_device);
}

VkPipeline VulkanRenderer3DUI::getPipeline() const
{
    return m_graphicsPipeline;
}

VkPipelineLayout VulkanRenderer3DUI::getPipelineLayout() const
{
    return m_pipelineLayout;
}

void VulkanRenderer3DUI::renderTransform(VkCommandBuffer& cmdBuf, 
    VkDescriptorSet& descriptorScene, 
    uint32_t imageIndex, 
    const glm::mat4& modelMatrix,
    float cameraDistance) const
{
    /* Keep size the same */
    float scale = cameraDistance * 0.01;

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
    
    const VulkanMesh* vkmesh = static_cast<const VulkanMesh*>(m_arrow->getMeshes()[0]);

    VkBuffer vertexBuffers[] = { vkmesh->m_vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuf, vkmesh->m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    VkDescriptorSet descriptorSets[1] = {
        descriptorScene,
    };
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
        0, 1, &descriptorSets[0], 0, nullptr);

    PushBlockForward3DUI pushConstants;
    
    /* Calculate the unscaled version of the input model matrix */
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
    pushConstants.modelMatrix = glm::scale(modelMatrixUnscaled, { scale, scale, scale });
    pushConstants.color = glm::vec4(0, 0, 1, 1);
    pushConstants.selected = glm::vec4(m_forwardID, 0);
    vkCmdPushConstants(cmdBuf,
        m_pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PushBlockForward3DUI),
        &pushConstants);
    vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(vkmesh->getIndices().size()), 1, 0, 0, 0);
    /* Render X arrow */
    pushConstants.modelMatrix = glm::scale(glm::rotate(modelMatrixUnscaled, glm::radians(90.F), { 0, 1, 0 }), {scale, scale, scale});
    pushConstants.color = glm::vec4(1, 0, 0, 1);
    pushConstants.selected = glm::vec4(m_rightID, 0);
    vkCmdPushConstants(cmdBuf,
        m_pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PushBlockForward3DUI),
        &pushConstants);
    vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(vkmesh->getIndices().size()), 1, 0, 0, 0);
    /* Render Y arrow */
    pushConstants.modelMatrix = glm::scale(glm::rotate(modelMatrixUnscaled, glm::radians(-90.F), { 1, 0, 0 }), { scale, scale, scale });
    pushConstants.color = glm::vec4(0, 1, 0, 1);
    pushConstants.selected = glm::vec4(m_upID, 0);
    vkCmdPushConstants(cmdBuf,
        m_pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PushBlockForward3DUI),
        &pushConstants);
    vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(vkmesh->getIndices().size()), 1, 0, 0, 0);
}

bool VulkanRenderer3DUI::createGraphicsPipeline()
{
    /* ----------------- SHADERS STAGE ------------------- */
       /* Load shaders */
    auto vertexShaderCode = readSPIRV("shaders/SPIRV/3duiVert.spv");
    auto fragmentShaderCode = readSPIRV("shaders/SPIRV/3duiFrag.spv");
    VkShaderModule vertShaderModule = Shader::load(m_device, vertexShaderCode);
    VkShaderModule fragShaderModule = Shader::load(m_device, fragmentShaderCode);
    /* Prepare pipeline stage */
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    /* -------------------- VERTEX INPUT ------------------ */
    auto bindingDescription = VulkanVertex::getBindingDescription();
    auto attributeDescriptions = VulkanVertex::getAttributeDescriptionsPos();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    /* ----------------- INPUT ASSEMBLY ------------------- */
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    /* ----------------- VIEWPORT AND SCISSORS ------------ */
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_swapchainExtent.width;
    viewport.height = (float)m_swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = m_swapchainExtent;
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    /* ------------------- RASTERIZER ---------------------- */
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    /* ------------------- MULTISAMPLING ------------------- */
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    /* ---------------- DEPTH STENCIL ---------------------- */
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    /* ------------------ COLOR BLENDING ------------------- */
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
    std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments{ colorBlendAttachment, colorBlendAttachment };
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
    colorBlending.pAttachments = colorBlendAttachments.data();

    /* ------------------- PIPELINE LAYOUT ----------------- */
    std::array<VkDescriptorSetLayout, 1> descriptorSetsLayouts{ m_descriptorSetLayoutCamera };

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushBlockForward3DUI);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetsLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetsLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        utils::ConsoleFatal("Failed to create a graphics pipeline layout");
        return false;
    }

    /* Create the graphics pipeline */
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
        utils::ConsoleFatal("Failed to create a graphics pipeline");
        return false;
    }

    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);

    return true;
}
