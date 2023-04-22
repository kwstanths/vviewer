#include "VulkanRendererLambert.hpp"

#include <utils/Console.hpp>
#include <set>

#include "core/AssetManager.hpp"

#include "vulkan/VulkanUtils.hpp"
#include "vulkan/Shader.hpp"
#include "vulkan/resources/VulkanMesh.hpp"
#include "vulkan/VulkanStructs.hpp"
#include "vulkan/VulkanSceneObject.hpp"

VulkanRendererLambert::VulkanRendererLambert()
{
}

void VulkanRendererLambert::initResources(VkPhysicalDevice physicalDevice, 
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
}

void VulkanRendererLambert::initSwapChainResources(VkExtent2D swapchainExtent, VkRenderPass renderPass, uint32_t swapchainImages, VkSampleCountFlagBits msaaSamples)
{
    m_swapchainExtent = swapchainExtent;
    m_renderPass = renderPass;
    m_msaaSamples = msaaSamples;

    createGraphicsPipelineBasePass();
    createGraphicsPipelineAddPass();
}

void VulkanRendererLambert::releaseSwapChainResources()
{
    vkDestroyPipeline(m_device, m_graphicsPipelineBasePass, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayoutBasePass, nullptr);
    vkDestroyPipeline(m_device, m_graphicsPipelineAddPass, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayoutAddPass, nullptr);
}

void VulkanRendererLambert::releaseResources()
{

}

void VulkanRendererLambert::renderObjectsBasePass(VkCommandBuffer& cmdBuf, 
    VkDescriptorSet& descriptorScene, 
    VkDescriptorSet& descriptorModel, 
    VkDescriptorSet descriptorSkybox,
    VkDescriptorSet& descriptorMaterial,
    VkDescriptorSet& descriptorTextures,
    uint32_t imageIndex, 
    const VulkanUBO<ModelData>& dynamicUBOModels, 
    std::vector<std::shared_ptr<SceneObject>>& objects) const
{
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineBasePass);
    for (size_t i = 0; i < objects.size(); i++)
    {
        VulkanSceneObject* object = static_cast<VulkanSceneObject*>(objects[i].get());

        const VulkanMesh* vkmesh = static_cast<const VulkanMesh*>(object->get<ComponentMesh>().mesh.get());
        if (vkmesh == nullptr) continue;

        VulkanMaterialLambert* material = static_cast<VulkanMaterialLambert*>(object->get<ComponentMaterial>().material.get());

        VkBuffer vertexBuffers[] = { vkmesh->getVertexBuffer().vkbuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(cmdBuf, vkmesh->getIndexBuffer().vkbuffer(), 0, vkmesh->indexType());

        /* Calculate model data offsets */
        std::array<uint32_t, 1> dynamicOffsets = {
            dynamicUBOModels.getBlockSizeAligned() * object->getTransformUBOBlock()
        };
        std::array<VkDescriptorSet, 5> descriptorSets = {
            descriptorScene,
            descriptorModel,
            descriptorMaterial,
            descriptorTextures,
            descriptorSkybox
        };

        PushBlockForwardBasePass pushConstants;
        pushConstants.selected = glm::vec4(object->getIDRGB(), object->m_isSelected);
        pushConstants.material.r = material->getMaterialIndex();
        vkCmdPushConstants(cmdBuf,
            m_pipelineLayoutBasePass,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(PushBlockForwardBasePass),
            &pushConstants);

        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayoutBasePass,
            0, 
            static_cast<uint32_t>(descriptorSets.size()), &descriptorSets[0], 
            static_cast<uint32_t>(dynamicOffsets.size()), &dynamicOffsets[0]
        );

        vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(vkmesh->getIndices().size()), 1, 0, 0, 0);
    }
}

void VulkanRendererLambert::renderObjectsAddPass(VkCommandBuffer& cmdBuf, 
    VkDescriptorSet& descriptorScene,
    VkDescriptorSet& descriptorModel,
    VkDescriptorSet& descriptorMaterial,
    VkDescriptorSet& descriptorTextures,
    uint32_t imageIndex, 
    const VulkanUBO<ModelData>& dynamicUBOModels,
    std::shared_ptr<SceneObject> object,
    PushBlockForwardAddPass& lightInfo) const
{
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineAddPass);

    VulkanSceneObject* vobject = static_cast<VulkanSceneObject*>(object.get());
    const VulkanMesh* vkmesh = static_cast<const VulkanMesh*>(vobject->get<ComponentMesh>().mesh.get());
    if (vkmesh == nullptr) return;

    VulkanMaterialLambert* material = static_cast<VulkanMaterialLambert*>(vobject->get<ComponentMaterial>().material.get());

    VkBuffer vertexBuffers[] = { vkmesh->getVertexBuffer().vkbuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuf, vkmesh->getIndexBuffer().vkbuffer(), 0, vkmesh->indexType());
    
    /* Calculate model data offsets */
    std::array<uint32_t, 1> dynamicOffsets = {
        dynamicUBOModels.getBlockSizeAligned() * vobject->getTransformUBOBlock(),
    };
    /* Create descriptor sets array */
    std::array<VkDescriptorSet, 4> descriptorSets = {
        descriptorScene,
        descriptorModel,
        descriptorMaterial,
        descriptorTextures,
    };

    lightInfo.material.r = material->getMaterialIndex();

    vkCmdPushConstants(cmdBuf,
        m_pipelineLayoutAddPass,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PushBlockForwardAddPass),
        &lightInfo
    );
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayoutAddPass,
        0, 
        static_cast<uint32_t>(descriptorSets.size()), &descriptorSets[0], 
        static_cast<uint32_t>(dynamicOffsets.size()), &dynamicOffsets[0]
    );
    vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(vkmesh->getIndices().size()), 1, 0, 0, 0);
}

bool VulkanRendererLambert::createGraphicsPipelineBasePass()
{
    /* ----------------- SHADERS STAGE ------------------- */
    /* Load shaders */
    auto vertexShaderCode = readSPIRV("shaders/SPIRV/standard.vert.spv");
    auto fragmentShaderCode = readSPIRV("shaders/SPIRV/lambertBase.frag.spv");
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
    auto attributeDescriptions = VulkanVertex::getAttributeDescriptionsFull();
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
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    /* ------------------- MULTISAMPLING ------------------- */
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = m_msaaSamples;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    /* ---------------- DEPTH STENCIL ---------------------- */
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional

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
    std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments{ colorBlendAttachment , colorBlendAttachment };
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
    colorBlending.pAttachments = colorBlendAttachments.data();
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    /* ------------------- PIPELINE LAYOUT ----------------- */
    std::array<VkDescriptorSetLayout, 5> descriptorSetsLayouts{ m_descriptorSetLayoutCamera, m_descriptorSetLayoutModel, m_descriptorSetLayoutMaterial, m_descriptorSetLayoutTextures, m_descriptorSetLayoutSkybox };
    
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushBlockForwardBasePass);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetsLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetsLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayoutBasePass) != VK_SUCCESS) {
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
    pipelineInfo.layout = m_pipelineLayoutBasePass;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipelineBasePass) != VK_SUCCESS) {
        utils::ConsoleFatal("Failed to create a graphics pipeline");
        return false;
    }

    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);

    return true;
}

bool VulkanRendererLambert::createGraphicsPipelineAddPass()
{
    /* ----------------- SHADERS STAGE ------------------- */
    /* Load shaders */
    auto vertexShaderCode = readSPIRV("shaders/SPIRV/standard.vert.spv");
    auto fragmentShaderCode = readSPIRV("shaders/SPIRV/lambertAdd.frag.spv");
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
    auto attributeDescriptions = VulkanVertex::getAttributeDescriptionsFull();
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
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    /* ------------------- MULTISAMPLING ------------------- */
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = m_msaaSamples;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    /* ---------------- DEPTH STENCIL ---------------------- */
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional

    /* ------------------ COLOR BLENDING ------------------- */
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments{ colorBlendAttachment , colorBlendAttachment };
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
    colorBlending.pAttachments = colorBlendAttachments.data();
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    /* ------------------- PIPELINE LAYOUT ----------------- */
    std::array<VkDescriptorSetLayout, 4> descriptorSetsLayouts{ m_descriptorSetLayoutCamera, m_descriptorSetLayoutModel, m_descriptorSetLayoutMaterial, m_descriptorSetLayoutTextures };
    
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushBlockForwardAddPass);
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetsLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetsLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayoutAddPass) != VK_SUCCESS) {
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
    pipelineInfo.layout = m_pipelineLayoutAddPass;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipelineAddPass) != VK_SUCCESS) {
        utils::ConsoleFatal("Failed to create a graphics pipeline");
        return false;
    }

    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);

    return true;
}
