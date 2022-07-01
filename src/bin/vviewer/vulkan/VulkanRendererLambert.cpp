#include "VulkanRendererLambert.hpp"

#include <utils/Console.hpp>
#include <set>

#include "core/AssetManager.hpp"

#include "VulkanUtils.hpp"
#include "Shader.hpp"
#include "VulkanMesh.hpp"
#include "VulkanDataStructs.hpp"

VulkanRendererLambert::VulkanRendererLambert()
{
}

void VulkanRendererLambert::initResources(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool, VkPhysicalDeviceProperties physicalDeviceProperties, VkDescriptorSetLayout cameraDescriptorLayout, VkDescriptorSetLayout modelDescriptorLayout, VkDescriptorSetLayout skyboxDescriptorLayout)
{
    m_physicalDevice = physicalDevice;
    m_device = device;
    m_commandPool = commandPool;
    m_queue = queue;
    m_descriptorSetLayoutCamera = cameraDescriptorLayout;
    m_descriptorSetLayoutModel = modelDescriptorLayout;
    m_descriptorSetLayoutSkybox = skyboxDescriptorLayout;

    createDescriptorSetsLayout();
}

void VulkanRendererLambert::initSwapChainResources(VkExtent2D swapchainExtent, VkRenderPass renderPass, uint32_t swapchainImages)
{
    m_swapchainExtent = swapchainExtent;
    m_renderPass = renderPass;

    createGraphicsPipeline();
}

void VulkanRendererLambert::releaseSwapChainResources()
{
    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
}

void VulkanRendererLambert::releaseResources()
{
    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayoutMaterial, nullptr);
}

VkPipeline VulkanRendererLambert::getPipeline() const
{
    return m_graphicsPipeline;
}

VkPipelineLayout VulkanRendererLambert::getPipelineLayout() const
{
    return m_pipelineLayout;
}

VkDescriptorSetLayout VulkanRendererLambert::getDescriptorSetLayout() const
{
    return m_descriptorSetLayoutMaterial;
}

VulkanMaterialLambert* VulkanRendererLambert::createMaterial(std::string name, glm::vec4 albedo, float ao, float emissive,
    VulkanDynamicUBO<MaterialData>& materialsUBO,
    int index)
{
    return new VulkanMaterialLambert(name, albedo, ao, emissive, m_device, m_descriptorSetLayoutMaterial, materialsUBO, index);
}

void VulkanRendererLambert::renderObjects(VkCommandBuffer& cmdBuf, VkDescriptorSet& descriptorScene, VkDescriptorSet& descriptorModel, VulkanMaterialSkybox* skybox, uint32_t imageIndex, VulkanDynamicUBO<ModelData>& dynamicUBOModels, std::vector<std::shared_ptr<SceneObject>>& objects) const
{
    assert(skybox != nullptr);

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
    for (size_t i = 0; i < objects.size(); i++)
    {
        VulkanSceneObject* object = static_cast<VulkanSceneObject*>(objects[i].get());

        const VulkanMesh* vkmesh = static_cast<const VulkanMesh*>(object->getMesh());
        if (vkmesh == nullptr) continue;

        VulkanMaterialLambert* material = static_cast<VulkanMaterialLambert*>(object->getMaterial());
        /* Check if material parameters have changed for that imageIndex descriptor set */
        if (material->needsUpdate(imageIndex)) material->updateDescriptorSet(m_device, imageIndex);

        VkBuffer vertexBuffers[] = { vkmesh->m_vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(cmdBuf, vkmesh->m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        /* Calculate model data offsets */
        uint32_t dynamicOffsets[2] = {
            static_cast<uint32_t>(dynamicUBOModels.getBlockSizeAligned()) * object->getTransformUBOBlock(),
            static_cast<uint32_t>(material->getBlockSizeAligned()) * material->getUBOBlockIndex()
        };
        VkDescriptorSet descriptorSets[4] = {
            descriptorScene,
            descriptorModel,
            material->getDescriptor(imageIndex),
            skybox->getDescriptor(imageIndex)
        };

        PushBlockForwardPass pushConstants;
        pushConstants.selected.r = static_cast<float>(object->m_isSelected);
        vkCmdPushConstants(cmdBuf,
            m_pipelineLayout,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(PushBlockForwardPass),
            &pushConstants);

        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
            0, 4, &descriptorSets[0], 2, &dynamicOffsets[0]);

        vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(vkmesh->getIndices().size()), 1, 0, 0, 0);
    }
}

bool VulkanRendererLambert::createDescriptorSetsLayout()
{
    {
        /* Create binding for material data */
        VkDescriptorSetLayoutBinding materiaDatalLayoutBinding{};
        materiaDatalLayoutBinding.binding = 0;
        materiaDatalLayoutBinding.descriptorCount = 1;   /* If we have an array of uniform buffers */
        materiaDatalLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        materiaDatalLayoutBinding.pImmutableSamplers = nullptr;
        materiaDatalLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding materialTexturesLayoutBinding{};
        materialTexturesLayoutBinding.binding = 1;
        materialTexturesLayoutBinding.descriptorCount = 4;  /* An array of 4 textures */
        materialTexturesLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        materialTexturesLayoutBinding.pImmutableSamplers = nullptr;
        materialTexturesLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> setBindings = { materiaDatalLayoutBinding, materialTexturesLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(setBindings.size());
        layoutInfo.pBindings = setBindings.data();

        if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayoutMaterial) != VK_SUCCESS) {
            utils::ConsoleCritical("Failed to create a descriptor set layout for the PBR material");
            return false;
        }
    }

    return true;
}

bool VulkanRendererLambert::createGraphicsPipeline()
{
    /* ----------------- SHADERS STAGE ------------------- */
    /* Load shaders */
    auto vertexShaderCode = readSPIRV("shaders/SPIRV/standardVert.spv");
    auto fragmentShaderCode = readSPIRV("shaders/SPIRV/lambertFrag.spv");
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
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
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
    std::array<VkDescriptorSetLayout, 4> descriptorSetsLayouts{ m_descriptorSetLayoutCamera, m_descriptorSetLayoutModel, m_descriptorSetLayoutMaterial, m_descriptorSetLayoutSkybox };
    
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushBlockForwardPass);

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
