#include "VulkanRenderer.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#include <utils/Console.hpp>

#include "IncludeVulkan.hpp"
#include "Shader.hpp"
#include "Utils.hpp"
#include "Image.hpp"

void VulkanRenderer::preInitResources()
{
    bool res = pickPhysicalDevice();
    if (!res) {
        utils::ConsoleFatal("Failed to find suitable Vulkan device");
        return;
    }

    VkFormat preferredFormat = VK_FORMAT_B8G8R8A8_SRGB;
    m_window->setPreferredColorFormats({ preferredFormat });
    
    //m_window->setDeviceExtensions(...);
}

void VulkanRenderer::initResources()
{
    utils::ConsoleInfo("Initializing resources...");

    createDebugCallback();

    m_functions = m_window->vulkanInstance()->functions();
    m_devFunctions = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    m_physicalDevice = m_window->physicalDevice();
    m_device = m_window->device();

    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };
    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0
    };
    createVertexBuffer(vertices);
    createIndexBuffer(indices);
    createTextureImages();
}

void VulkanRenderer::initSwapChainResources()
{
    QSize size = m_window->swapChainImageSize();
    m_swapchainExtent.width = static_cast<uint32_t>(size.width());
    m_swapchainExtent.height = static_cast<uint32_t>(size.height());

    m_swapchainFormat = m_window->colorFormat();

    createRenderPass();
    createDescriptorSetsLayouts();
    createGraphicsPipeline();
    createFrameBuffers();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
}

void VulkanRenderer::releaseSwapChainResources()
{
    for (size_t i = 0; i < m_window->swapChainImageCount(); i++) {
        m_devFunctions->vkDestroyBuffer(m_device, m_uniformBuffersCamera[i], nullptr);
        m_devFunctions->vkDestroyBuffer(m_device, m_uniformBuffersModel[i], nullptr);
        m_devFunctions->vkFreeMemory(m_device, m_uniformBuffersCameraMemory[i], nullptr);
        m_devFunctions->vkFreeMemory(m_device, m_uniformBuffersModelMemory[i], nullptr);
    }

    m_devFunctions->vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    m_devFunctions->vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

    for (auto framebuffer : m_swapChainFramebuffers) {
        m_devFunctions->vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }
    m_devFunctions->vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    m_devFunctions->vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    m_devFunctions->vkDestroyRenderPass(m_device, m_renderPass, nullptr);
}

void VulkanRenderer::releaseResources()
{
    m_devFunctions->vkDestroyImage(m_device, m_textureImage, nullptr);
    m_devFunctions->vkFreeMemory(m_device, m_textureImageMemory, nullptr);

    m_devFunctions->vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
    m_devFunctions->vkFreeMemory(m_device, m_indexBufferMemory, nullptr);

    m_devFunctions->vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
    m_devFunctions->vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);

    destroyDebugCallback();
}

void VulkanRenderer::startNextFrame()
{
    int imageIndex = m_window->currentSwapChainImageIndex();

    updateUniformBuffers(imageIndex);

    VkClearColorValue clearColor = { { m_clearColor.red(), m_clearColor.green(), m_clearColor.blue(), 1.0f } };
    VkClearValue clearValues[1];
    memset(clearValues, 0, sizeof(clearValues));
    clearValues[0].color = clearColor;

    VkRenderPassBeginInfo rpBeginInfo;
    memset(&rpBeginInfo, 0, sizeof(rpBeginInfo));
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = m_renderPass;
    rpBeginInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
    rpBeginInfo.renderArea.offset = { 0, 0 };
    rpBeginInfo.renderArea.extent.width = m_swapchainExtent.width;
    rpBeginInfo.renderArea.extent.height = m_swapchainExtent.height;
    rpBeginInfo.clearValueCount = 1;
    rpBeginInfo.pClearValues = clearValues;
    VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();
    
    m_devFunctions->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    m_devFunctions->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
    
    VkBuffer vertexBuffers[] = { m_vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    m_devFunctions->vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
    m_devFunctions->vkCmdBindIndexBuffer(cmdBuf, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    m_devFunctions->vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 
        0, 1, &m_descriptorSets[imageIndex], 0, nullptr);
    m_devFunctions->vkCmdDrawIndexed(cmdBuf, 6, 1, 0, 0, 0);

    m_devFunctions->vkCmdEndRenderPass(cmdBuf);
    
    m_window->frameReady();
    m_window->requestUpdate(); // render continuously, throttled by the presentation rate
}

bool VulkanRenderer::createDebugCallback()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // Optional

    VkResult res;
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) m_window->vulkanInstance()->getInstanceProcAddr("vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        res = func(m_window->vulkanInstance()->vkInstance(), &createInfo, nullptr, &m_debugCallback);
    } else {
        res = VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    if (res != VK_SUCCESS) {
        utils::Console(utils::DebugLevel::CRITICAL, "Failed to setup the debug callback");
        return false;
    }

    return true;
}

void VulkanRenderer::destroyDebugCallback()
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) m_window->vulkanInstance()->getInstanceProcAddr("vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(m_window->vulkanInstance()->vkInstance(), m_debugCallback, nullptr);
    }
}

bool VulkanRenderer::pickPhysicalDevice()
{
    auto devices = m_window->availablePhysicalDevices();
    for (int i = 0; i < devices.size(); i++) {
        if (isPhysicalDeviceSuitable(devices[i])) {
            m_window->setPhysicalDeviceIndex(i);
            return true;
        }
    }

    utils::ConsoleFatal("Failed to find a proper physical device");
    return false;
}

bool VulkanRenderer::isPhysicalDeviceSuitable(VkPhysicalDeviceProperties device)
{
    /* Should probably check features here as well... */
    return true;
}

bool VulkanRenderer::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapchainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  /* Before the subpass */
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  /* After the subpass */

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;   /* During the subpass */

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;    /* Same as shader locations */

    /* Wait for the previous external pass to finish reading the color, before you start writing the new color */
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstSubpass = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (m_devFunctions->vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        utils::ConsoleFatal("Failed to create a render pass");
        return false;
    }

    return true;
}

bool VulkanRenderer::createGraphicsPipeline()
{
    /* ----------------- SHADERS STAGE ------------------- */
    /* Load shaders */
    auto vertexShaderCode = readSPIRV("shaders/vert.spv");
    auto fragmentShaderCode = readSPIRV("shaders/frag.spv");
    VkShaderModule vertShaderModule = Shader::load(m_window->vulkanInstance(), m_device, vertexShaderCode);
    VkShaderModule fragShaderModule = Shader::load(m_window->vulkanInstance(), m_device, fragmentShaderCode);
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
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
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
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    /* ------------------- PIPELINE LAYOUT ----------------- */
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
    if (m_devFunctions->vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
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
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (m_devFunctions->vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
        utils::ConsoleFatal("Failed to create a graphics pipeline");
        return false;
    }

    m_devFunctions->vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    m_devFunctions->vkDestroyShaderModule(m_device, vertShaderModule, nullptr);

    return true;
}

bool VulkanRenderer::createFrameBuffers()
{
    m_swapChainFramebuffers.resize(m_window->swapChainImageCount());

    for (size_t i = 0; i < m_swapChainFramebuffers.size(); i++) {
        VkImageView attachments[] = {
            m_window->swapChainImageView(i)
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapchainExtent.width;
        framebufferInfo.height = m_swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (m_devFunctions->vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS) {
            utils::ConsoleFatal("Failed to create a framebuffer");
            return false;
        }
    }

    return true;
}

bool VulkanRenderer::createVertexBuffer(const std::vector<Vertex>& vertices)
{
    VkDeviceSize size = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(size, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        stagingBuffer, stagingBufferMemory);


    void* data;
    m_devFunctions->vkMapMemory(m_device, stagingBufferMemory, 0, size, 0, &data);
    memcpy(data, vertices.data(), (size_t)size);
    m_devFunctions->vkUnmapMemory(m_device, stagingBufferMemory);

    createBuffer(size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_vertexBuffer,
        m_vertexBufferMemory
    );

    copyBufferToBuffer(stagingBuffer, m_vertexBuffer, size);

    m_devFunctions->vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    m_devFunctions->vkFreeMemory(m_device, stagingBufferMemory, nullptr);

    return true;
}

bool VulkanRenderer::createIndexBuffer(const std::vector<uint16_t>& indices)
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        stagingBuffer, 
        stagingBufferMemory);

    void* data;
    m_devFunctions->vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    m_devFunctions->vkUnmapMemory(m_device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

    copyBufferToBuffer(stagingBuffer, m_indexBuffer, bufferSize);

    m_devFunctions->vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    m_devFunctions->vkFreeMemory(m_device, stagingBufferMemory, nullptr);
    return false;
}

bool VulkanRenderer::createDescriptorSetsLayouts()
{
    /* Create binding for camera data */
    VkDescriptorSetLayoutBinding cameraDataLayoutBinding{};
    cameraDataLayoutBinding.binding = 0;
    cameraDataLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraDataLayoutBinding.descriptorCount = 1;    /* If we have an array of uniform buffers */
    cameraDataLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    cameraDataLayoutBinding.pImmutableSamplers = nullptr;

    /* Create binding for model data */
    VkDescriptorSetLayoutBinding modelDataLayoutBinding{};
    modelDataLayoutBinding.binding = 1;
    modelDataLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    modelDataLayoutBinding.descriptorCount = 1;    /* If we have an array of uniform buffers */
    modelDataLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    modelDataLayoutBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> setBindings = { cameraDataLayoutBinding, modelDataLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = setBindings.data();

    if (m_devFunctions->vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to creaate a descriptor set layout");
        return false;
    }

    return true;
}

bool VulkanRenderer::createUniformBuffers()
{
    {
        VkDeviceSize bufferSize = sizeof(CameraData);

        m_uniformBuffersCamera.resize(m_window->swapChainImageCount());
        m_uniformBuffersCameraMemory.resize(m_window->swapChainImageCount());

        for (size_t i = 0; i < m_window->swapChainImageCount(); i++) {
            createBuffer(bufferSize, 
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                m_uniformBuffersCamera[i], 
                m_uniformBuffersCameraMemory[i]);
        }
    }

    {
        VkDeviceSize bufferSize = sizeof(ModelData);

        m_uniformBuffersModel.resize(m_window->swapChainImageCount());
        m_uniformBuffersModelMemory.resize(m_window->swapChainImageCount());

        for (size_t i = 0; i < m_window->swapChainImageCount(); i++) {
            createBuffer(bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_uniformBuffersModel[i],
                m_uniformBuffersModelMemory[i]);
        }
    }

    return true;
}

bool VulkanRenderer::updateUniformBuffers(size_t index)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    {
        CameraData cameraData;
        cameraData.m_view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        cameraData.m_projection = glm::perspective(glm::radians(45.0f), (float)m_swapchainExtent.width / (float)m_swapchainExtent.height, 0.1f, 10.0f);
        cameraData.m_projection[1][1] *= -1;

        void* data;
        m_devFunctions->vkMapMemory(m_device, m_uniformBuffersCameraMemory[index], 0, sizeof(CameraData), 0, &data);
        memcpy(data, &cameraData, sizeof(cameraData));
        m_devFunctions->vkUnmapMemory(m_device, m_uniformBuffersCameraMemory[index]);
    }

    {
        ModelData modelData;
        modelData.m_modelMatrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        void* data;
        m_devFunctions->vkMapMemory(m_device, m_uniformBuffersModelMemory[index], 0, sizeof(ModelData), 0, &data);
        memcpy(data, &modelData, sizeof(modelData));
        m_devFunctions->vkUnmapMemory(m_device, m_uniformBuffersModelMemory[index]);
    }

    return true;
}

bool VulkanRenderer::createDescriptorPool()
{
    VkDescriptorPoolSize cameraDataPoolSize{};
    cameraDataPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraDataPoolSize.descriptorCount = static_cast<uint32_t>(m_window->swapChainImageCount());

    VkDescriptorPoolSize modelDataPoolSize{};
    modelDataPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    modelDataPoolSize.descriptorCount = static_cast<uint32_t>(m_window->swapChainImageCount());

    std::array<VkDescriptorPoolSize, 2> poolSizes = { cameraDataPoolSize, modelDataPoolSize };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(m_window->swapChainImageCount());

    if (m_devFunctions->vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to create a descriptor pool");
        return false;
    }

    return true;
}

bool VulkanRenderer::createDescriptorSets()
{
    int nImages = m_window->swapChainImageCount();

    std::vector<VkDescriptorSetLayout> layouts(nImages, m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(nImages);
    allocInfo.pSetLayouts = layouts.data();

    m_descriptorSets.resize(nImages);
    if (m_devFunctions->vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to allocate descriptor sets");
        return false;
    }

    for (size_t i = 0; i < nImages; i++) {

        VkDescriptorBufferInfo bufferInfoCamera{};
        bufferInfoCamera.buffer = m_uniformBuffersCamera[i];
        bufferInfoCamera.buffer = m_uniformBuffersCamera[i];
        bufferInfoCamera.offset = 0;
        bufferInfoCamera.offset = 0;
        bufferInfoCamera.range = sizeof(CameraData);
        bufferInfoCamera.range = sizeof(CameraData);
        
        VkWriteDescriptorSet descriptorWriteCamera{};
        descriptorWriteCamera.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteCamera.dstSet = m_descriptorSets[i];
        descriptorWriteCamera.dstBinding = 0;
        descriptorWriteCamera.dstArrayElement = 0;
        descriptorWriteCamera.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWriteCamera.descriptorCount = 1;
        descriptorWriteCamera.pBufferInfo = &bufferInfoCamera;
        descriptorWriteCamera.pImageInfo = nullptr; // Optional
        descriptorWriteCamera.pTexelBufferView = nullptr; // Optional
        
        VkDescriptorBufferInfo bufferInfoModel{};
        bufferInfoModel.buffer = m_uniformBuffersModel[i];
        bufferInfoModel.offset = 0;
        bufferInfoModel.range = sizeof(ModelData);
        
        VkWriteDescriptorSet descriptorWriteModel{};
        descriptorWriteModel.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteModel.dstSet = m_descriptorSets[i];
        descriptorWriteModel.dstBinding = 1;
        descriptorWriteModel.dstArrayElement = 0;
        descriptorWriteModel.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWriteModel.descriptorCount = 1;
        descriptorWriteModel.pBufferInfo = &bufferInfoModel;
        descriptorWriteModel.pImageInfo = nullptr; // Optional
        descriptorWriteModel.pTexelBufferView = nullptr; // Optional

        std::array<VkWriteDescriptorSet, 2> writeSets = { descriptorWriteCamera, descriptorWriteModel };
        m_devFunctions->vkUpdateDescriptorSets(m_device, 2, writeSets.data(), 0, nullptr);
        
    }

    return true;
}

bool VulkanRenderer::createTextureImages()
{
    try {
        /* Read image from disk */
        Image image("assets/texture.jpg");
        VkDeviceSize imageSize = image.getWidth() * image.getHeight() * image.getChannels();

        /* Create staging buffer */
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize, 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            stagingBuffer, 
            stagingBufferMemory);

        /* Copy image data to staging buffer */
        void* data;
        m_devFunctions->vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, image.getData(), static_cast<size_t>(imageSize));
        m_devFunctions->vkUnmapMemory(m_device, stagingBufferMemory);

        /* Create vulkan image and memory */
        createImage(image.getWidth(), 
            image.getHeight(), 
            VK_FORMAT_R8G8B8A8_SRGB, 
            VK_IMAGE_TILING_OPTIMAL, 
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            m_textureImage, 
            m_textureImageMemory);

        /* Transition image to DST_OPTIMAL in orderto transfer the data */
        transitionImageLayout(m_textureImage, 
            VK_FORMAT_R8G8B8A8_SRGB, 
            VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        /* copy data fronm staging buffer to image */
        copyBufferToImage(stagingBuffer, m_textureImage, static_cast<uint32_t>(image.getWidth()), static_cast<uint32_t>(image.getHeight()));

        /* Transition to SHADER_READ_ONLY layout */
        transitionImageLayout(m_textureImage, 
            VK_FORMAT_R8G8B8A8_SRGB, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        /* Cleanup staging buffer */
        m_devFunctions->vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        m_devFunctions->vkFreeMemory(m_device, stagingBufferMemory, nullptr);
    }
    catch (std::runtime_error& e) {
        utils::ConsoleCritical("Failed to load a disk image: " + std::string(e.what()));
        return false;
    }

    return true;
}

bool VulkanRenderer::createBuffer(VkDeviceSize buffer_size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags buffer_properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory)
{
    /* Create vertex buffer */
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = buffer_size;
    buffer_info.usage = buffer_usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult res = m_devFunctions->vkCreateBuffer(m_device, &buffer_info, nullptr, &buffer);
    if (res != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to create a vertex buffer");
        return false;
    }

    /* Get buffer memory requirements */
    VkMemoryRequirements memory_requirements;
    m_devFunctions->vkGetBufferMemoryRequirements(m_device, buffer, &memory_requirements);

    /* Alocate memory for buffer */
    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = findMemoryType(memory_requirements.memoryTypeBits, buffer_properties);

    res = m_devFunctions->vkAllocateMemory(m_device, &allocate_info, nullptr, &buffer_memory);
    if (res != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to allocate device memory");
        return false;
    }
    /* Bind memory to buffer */
    m_devFunctions->vkBindBufferMemory(m_device, buffer, buffer_memory, 0);
    return true;
}

bool VulkanRenderer::copyBufferToBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    /* Create command buffer for transfer operation */
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    /* Record transfer command */
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    m_devFunctions->vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    /* End recording and submit */
    endSingleTimeCommands(commandBuffer);

    return true;
}

bool VulkanRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };

    m_devFunctions-> vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    endSingleTimeCommands(commandBuffer);
    return true;
}

bool VulkanRenderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage & image, VkDeviceMemory & imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (m_devFunctions->vkCreateImage(m_device, &imageInfo, nullptr, &m_textureImage) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to create a vulkan image");
        return false;
    }

    VkMemoryRequirements memRequirements;
    m_devFunctions->vkGetImageMemoryRequirements(m_device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (m_devFunctions->vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to allocate memory for a vulkan image");
        return false;
    }

    m_devFunctions->vkBindImageMemory(m_device, image, imageMemory, 0);
}

uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    m_functions->vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    utils::ConsoleFatal("Failed to find a suitable memory type");

    return 0;
}

void VulkanRenderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;  /* Don't wait on anything */
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;   /* Before it is written */

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;  /* In the transfer stage */
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;   /* Wait on the transfer write to happen */
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;  /* Finish operation before reading from it */

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;   /* Wait the transfer stage to finish */
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;   /* Before the fragment shader */
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }
    
    barrier.srcAccessMask = 0; // TODO
    barrier.dstAccessMask = 0; // TODO
    
    m_devFunctions->vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer VulkanRenderer::beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_window->graphicsCommandPool();
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    m_devFunctions->vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    /* Start recording commands */
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    m_devFunctions->vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VulkanRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    m_devFunctions->vkEndCommandBuffer(commandBuffer);

    /* Submit command buffer */
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    m_devFunctions->vkQueueSubmit(m_window->graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    /* Wait for this single operations to finish */
    m_devFunctions->vkQueueWaitIdle(m_window->graphicsQueue());

    /* Cleanup command buffer */
    m_devFunctions->vkFreeCommandBuffers(m_device, m_window->graphicsCommandPool(), 1, &commandBuffer);
}

