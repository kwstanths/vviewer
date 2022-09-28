#include "VulkanRendererPost.hpp"

#include <Console.hpp>

#include "vulkan/VulkanUtils.hpp"
#include "vulkan/Shader.hpp"
#include "vulkan/VulkanMesh.hpp"

VulkanRendererPost::VulkanRendererPost()
{
}

void VulkanRendererPost::initResources(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool)
{
	m_physicalDevice = physicalDevice;
	m_device = device;
	m_commandPool = commandPool;
	m_queue = queue;

    createDescriptorSetsLayout();
    createSampler();
}

void VulkanRendererPost::initSwapChainResources(VkExtent2D swapchainExtent, 
    VkRenderPass renderPass, 
    uint32_t swapchainImages, 
    VkSampleCountFlagBits msaaSamples,
    const std::vector<VulkanFrameBufferAttachment>& colorAttachments, 
    const std::vector<VulkanFrameBufferAttachment>& highlightAttachments)
{
	m_swapchainExtent = swapchainExtent;
	m_renderPass = renderPass;
    m_msaaSamples = msaaSamples;

    createDescriptorPool(swapchainImages);
    createDescriptors(swapchainImages, colorAttachments, highlightAttachments);

	createGraphicsPipeline();
}

void VulkanRendererPost::releaseSwapChainResources()
{
    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
	vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
}

void VulkanRendererPost::releaseResources()
{
    vkDestroySampler(m_device, m_inputSampler, nullptr);
    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
}

VkPipeline VulkanRendererPost::getPipeline() const
{
	return m_graphicsPipeline;
}

VkPipelineLayout VulkanRendererPost::getPipelineLayout() const
{
	return m_pipelineLayout;
}

VkDescriptorSetLayout VulkanRendererPost::getDescriptorSetLayout() const
{
	return VkDescriptorSetLayout();
}

void VulkanRendererPost::render(VkCommandBuffer cmdBuf, uint32_t imageIndex)
{
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    vkCmdBindDescriptorSets(cmdBuf,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout,
        0,
        1,
        &m_descriptorSets[imageIndex],
        0,
        nullptr);

    vkCmdDraw(cmdBuf, 3, 1, 0, 0);
}

bool VulkanRendererPost::createGraphicsPipeline()
{
    /* ----------------- SHADERS STAGE ------------------- */
       /* Load shaders */
    auto vertexShaderCode = readSPIRV("shaders/SPIRV/quadVert.spv");
    auto fragmentShaderCode = readSPIRV("shaders/SPIRV/highlightFrag.spv");
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
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;
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
    /* ------------------- MULTISAMPLING ------------------- */
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = m_msaaSamples;
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
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    /* ------------------- PIPELINE LAYOUT ----------------- */
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
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

bool VulkanRendererPost::createDescriptorSetsLayout()
{
    VkDescriptorSetLayoutBinding colorInputLayoutBinding{};
    colorInputLayoutBinding.binding = 0;
    colorInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    colorInputLayoutBinding.descriptorCount = 1;
    colorInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding highlightInputLayoutBinding{};
    highlightInputLayoutBinding.binding = 1;
    highlightInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    highlightInputLayoutBinding.descriptorCount = 1;
    highlightInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> inputBindings{ colorInputLayoutBinding , highlightInputLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(inputBindings.size());
    layoutInfo.pBindings = inputBindings.data();

    VkResult res = vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a descriptor set layout");
    }

    return true;
}

bool VulkanRendererPost::createDescriptorPool(uint32_t imageCount)
{
    VkDescriptorPoolSize colorInputPoolSize{};
    colorInputPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    colorInputPoolSize.descriptorCount = imageCount;

    VkDescriptorPoolSize highlightInputPoolSize{};
    highlightInputPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    highlightInputPoolSize.descriptorCount = imageCount;

    std::array<VkDescriptorPoolSize, 2> inputPoolSizes{ colorInputPoolSize , highlightInputPoolSize };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = imageCount;
    poolInfo.poolSizeCount = static_cast<uint32_t>(inputPoolSizes.size());
    poolInfo.pPoolSizes = inputPoolSizes.data();

    VkResult res = vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a descriptor pool");
    }
    return true;
}

bool VulkanRendererPost::createDescriptors(uint32_t imageCount, 
    const std::vector<VulkanFrameBufferAttachment>& colorAttachments, 
    const std::vector<VulkanFrameBufferAttachment>& highlightAttachments)
{
    m_descriptorSets.resize(imageCount);

    std::vector<VkDescriptorSetLayout> setLayouts(imageCount, m_descriptorSetLayout);
    
    VkDescriptorSetAllocateInfo setAllocInfo {};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = m_descriptorPool;
    setAllocInfo.descriptorSetCount = imageCount;
    setAllocInfo.pSetLayouts = setLayouts.data();

    VkResult res = vkAllocateDescriptorSets(m_device, &setAllocInfo, m_descriptorSets.data());
    if (res != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    for (size_t i = 0; i < imageCount; i++)
    {
        VkDescriptorImageInfo colorAttachmentDescriptor{};
        colorAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        colorAttachmentDescriptor.imageView = colorAttachments[i].getViewResolve();
        colorAttachmentDescriptor.sampler = m_inputSampler;
        VkWriteDescriptorSet colorWrite{};
        colorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        colorWrite.dstSet = m_descriptorSets[i];
        colorWrite.dstBinding = 0;
        colorWrite.dstArrayElement = 0;
        colorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        colorWrite.descriptorCount = 1;
        colorWrite.pImageInfo = &colorAttachmentDescriptor;

        VkDescriptorImageInfo highlightAttachmentDescriptor{};
        highlightAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        highlightAttachmentDescriptor.imageView = highlightAttachments[i].getViewResolve();
        highlightAttachmentDescriptor.sampler = m_inputSampler;
        VkWriteDescriptorSet highlightWrite{};
        highlightWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        highlightWrite.dstSet = m_descriptorSets[i];
        highlightWrite.dstBinding = 1;
        highlightWrite.dstArrayElement = 0;
        highlightWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        highlightWrite.descriptorCount = 1;
        highlightWrite.pImageInfo = &highlightAttachmentDescriptor;

        std::vector<VkWriteDescriptorSet> setWrites{ colorWrite, highlightWrite };
        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
    }

    return true;
}

bool VulkanRendererPost::createSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_inputSampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a texture sampler");
    }

    return true;
}
