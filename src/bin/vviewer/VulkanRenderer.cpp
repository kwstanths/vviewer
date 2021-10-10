#include "VulkanRenderer.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#include <utils/Console.hpp>

#include "core/Image.hpp"
#include "models/AssimpLoadModel.hpp"
#include "vulkan/IncludeVulkan.hpp"
#include "vulkan/Shader.hpp"
#include "vulkan/Utils.hpp"

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

    m_functions->vkGetPhysicalDeviceProperties(m_physicalDevice, &m_physicalDeviceProperties);
    
    m_modelDataDynamicUBO.init(m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment, 10);

    createTextureImage();
    createTextureImageView();
    createTextureSampler();
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
        m_devFunctions->vkFreeMemory(m_device, m_uniformBuffersCameraMemory[i], nullptr);

        m_devFunctions->vkDestroyBuffer(m_device, m_modelDataDynamicUBO.getBuffer(i), nullptr);
        m_devFunctions->vkFreeMemory(m_device, m_modelDataDynamicUBO.getBufferMemory(i), nullptr);
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
    m_modelDataDynamicUBO.destroy();

    m_devFunctions->vkDestroySampler(m_device, m_textureSampler, nullptr);
    m_devFunctions->vkDestroyImageView(m_device, m_textureImageView, nullptr);
    m_devFunctions->vkDestroyImage(m_device, m_textureImage, nullptr);
    m_devFunctions->vkFreeMemory(m_device, m_textureImageMemory, nullptr);

    AssetManager<std::string, MeshModel *>& instance = AssetManager<std::string, MeshModel *>::getInstance();
    for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
        destroyVulkanMeshModel(*itr->second);
    }

    destroyDebugCallback();
}

void VulkanRenderer::startNextFrame()
{
    int imageIndex = m_window->currentSwapChainImageIndex();

    updateUniformBuffers(imageIndex);

    std::array<VkClearValue, 2> clearValues{};
    VkClearColorValue clearColor = { { m_clearColor.redF(), m_clearColor.greenF(), m_clearColor.blueF(), 1.0f } };
    clearValues[0].color = clearColor;
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo rpBeginInfo;
    memset(&rpBeginInfo, 0, sizeof(rpBeginInfo));
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = m_renderPass;
    rpBeginInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
    rpBeginInfo.renderArea.offset = { 0, 0 };
    rpBeginInfo.renderArea.extent.width = m_swapchainExtent.width;
    rpBeginInfo.renderArea.extent.height = m_swapchainExtent.height;
    rpBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;
    rpBeginInfo.pClearValues = clearValues.data();
    VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();
    
    m_devFunctions->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    m_devFunctions->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
    
    for (size_t i = 0; i < m_objects.size(); i++)
    {
        VulkanSceneObject * object = m_objects[i];

        std::vector<Mesh *> meshes = object->getMeshModel()->getMeshes();
        for (size_t i = 0; i < meshes.size(); i++) {
            VulkanMesh * vkmesh = static_cast<VulkanMesh *>(meshes[i]);

            VkBuffer vertexBuffers[] = { vkmesh->m_vertexBuffer };
            VkDeviceSize offsets[] = { 0 };
            m_devFunctions->vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
            m_devFunctions->vkCmdBindIndexBuffer(cmdBuf, vkmesh->m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

            /* Calculate model data offsets */
            uint32_t dynamicOffset = static_cast<uint32_t>(m_modelDataDynamicUBO.getBlockSizeAligned()) * object->getTransformUBOBlock();
            m_devFunctions->vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
                0, 1, &m_descriptorSets[imageIndex], 1, &dynamicOffset);

            m_devFunctions->vkCmdDrawIndexed(cmdBuf, vkmesh->getIndices().size(), 1, 0, 0, 0);
        }
    }

    m_devFunctions->vkCmdEndRenderPass(cmdBuf);
    
    m_window->frameReady();
    m_window->requestUpdate(); // render continuously, throttled by the presentation rate
}

void VulkanRenderer::setCamera(std::shared_ptr<Camera> camera)
{
    m_camera = camera;
}

bool VulkanRenderer::createVulkanMeshModel(std::string filename)
{
    AssetManager<std::string, MeshModel *>& instance = AssetManager<std::string, MeshModel *>::getInstance();

    if (instance.isPresent(filename)) return false;
    
    try {
        VulkanMeshModel * vkmesh = new VulkanMeshModel(m_physicalDevice, m_device, m_window->graphicsQueue(), m_window->graphicsCommandPool(), assimpLoadModel(filename), true);
        instance.Add(filename, vkmesh);
    }
    catch (std::runtime_error& e) {
        return false;
    }

    return true;
}

VulkanSceneObject * VulkanRenderer::addSceneObject(std::string meshModel, Transform transform)
{
    AssetManager<std::string, MeshModel *>& instance = AssetManager<std::string, MeshModel *>::getInstance();

    if (!instance.isPresent(meshModel)) return nullptr;

    MeshModel * vkmesh = instance.Get(meshModel);

    VulkanSceneObject * object = new VulkanSceneObject(vkmesh, transform, m_modelDataDynamicUBO, m_transformIndexUBO++);
    m_objects.push_back(object);

    return object;
}

void VulkanRenderer::destroyVulkanMeshModel(MeshModel model)
{
    std::vector<Mesh *> meshes = model.getMeshes();
    for (size_t i = 0; i < meshes.size(); i++) {
        VulkanMesh * vkmesh = static_cast<VulkanMesh *>(meshes[i]);
        m_devFunctions->vkDestroyBuffer(m_device, vkmesh->m_vertexBuffer, nullptr);
        m_devFunctions->vkFreeMemory(m_device, vkmesh->m_vertexBufferMemory, nullptr);
        m_devFunctions->vkDestroyBuffer(m_device, vkmesh->m_indexBuffer, nullptr);
        m_devFunctions->vkFreeMemory(m_device, vkmesh->m_indexBufferMemory, nullptr);
    }
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
    auto deviceProperties = m_window->availablePhysicalDevices();
    for (int i = 0; i < deviceProperties.size(); i++) {
        VkPhysicalDeviceProperties properties{};
        if (isPhysicalDeviceSuitable(deviceProperties[i])) {
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

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = m_window->depthStencilFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;    /* Same as shader locations */
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    /* Wait for the previous external pass to finish reading the color, before you start writing the new color */
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstSubpass = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
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
    auto bindingDescription = VulkanVertex::getBindingDescription();
    auto attributeDescriptions = VulkanVertex::getAttributeDescriptions();
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
    pipelineInfo.pDepthStencilState = &depthStencil;
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
        std::array<VkImageView, 2> attachments = {
            m_window->swapChainImageView(i),
            m_window->depthStencilImageView()
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
        framebufferInfo.pAttachments = attachments.data();
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
    modelDataLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    modelDataLayoutBinding.descriptorCount = 1;    /* If we have an array of uniform buffers */
    modelDataLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    modelDataLayoutBinding.pImmutableSamplers = nullptr;

    /* Create binding for texture data */
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 2;
    samplerLayoutBinding.descriptorCount = 1;   /* If we have an array of uniform buffers */
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 3> setBindings = { cameraDataLayoutBinding, modelDataLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(setBindings.size());
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
            createBuffer(m_physicalDevice, m_device, bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                m_uniformBuffersCamera[i], 
                m_uniformBuffersCameraMemory[i]);
        }
    }

    m_modelDataDynamicUBO.createBuffers(m_physicalDevice, m_device, m_window->swapChainImageCount());

    return true;
}

bool VulkanRenderer::updateUniformBuffers(size_t index)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    /* Copy camera data */
    {
        CameraData cameraData;
        cameraData.m_view = m_camera->getViewMatrix();
        cameraData.m_projection = m_camera->getProjectionMatrix();
        cameraData.m_projection[1][1] *= -1;

        void* data;
        m_devFunctions->vkMapMemory(m_device, m_uniformBuffersCameraMemory[index], 0, sizeof(CameraData), 0, &data);
        memcpy(data, &cameraData, sizeof(cameraData));
        m_devFunctions->vkUnmapMemory(m_device, m_uniformBuffersCameraMemory[index]);
    }

    void* data;
    m_devFunctions->vkMapMemory(m_device, m_modelDataDynamicUBO.getBufferMemory(index), 0, m_modelDataDynamicUBO.getBlockSizeAligned() * m_modelDataDynamicUBO.getNBlocks(), 0, &data);
    memcpy(data, m_modelDataDynamicUBO.getBlock(0), m_modelDataDynamicUBO.getBlockSizeAligned() * m_modelDataDynamicUBO.getNBlocks());
    m_devFunctions->vkUnmapMemory(m_device, m_modelDataDynamicUBO.getBufferMemory(index));

    return true;
}

bool VulkanRenderer::createDescriptorPool()
{
    uint32_t nImages = static_cast<uint32_t>(m_window->swapChainImageCount());

    VkDescriptorPoolSize cameraDataPoolSize{};
    cameraDataPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraDataPoolSize.descriptorCount = nImages;

    VkDescriptorPoolSize modelDataPoolSize{};
    modelDataPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    modelDataPoolSize.descriptorCount = nImages;

    VkDescriptorPoolSize texturesPoolSize{};
    texturesPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texturesPoolSize.descriptorCount = nImages;

    std::array<VkDescriptorPoolSize, 3> poolSizes = { cameraDataPoolSize, modelDataPoolSize, texturesPoolSize };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = nImages;

    if (m_devFunctions->vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to create a descriptor pool");
        return false;
    }

    return true;
}

bool VulkanRenderer::createDescriptorSets()
{
    uint32_t nImages = static_cast<uint32_t>(m_window->swapChainImageCount());
    m_descriptorSets.resize(nImages);

    std::vector<VkDescriptorSetLayout> layouts(nImages, m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = nImages;
    allocInfo.pSetLayouts = layouts.data();
    if (m_devFunctions->vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to allocate descriptor sets");
        return false;
    }

    for (size_t i = 0; i < nImages; i++) {

        VkDescriptorBufferInfo bufferInfoCamera{};
        bufferInfoCamera.buffer = m_uniformBuffersCamera[i];
        bufferInfoCamera.offset = 0;
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
        bufferInfoModel.buffer = m_modelDataDynamicUBO.getBuffer(i);
        bufferInfoModel.offset = 0;
        bufferInfoModel.range = m_modelDataDynamicUBO.getBlockSizeAligned();
        VkWriteDescriptorSet descriptorWriteModel{};
        descriptorWriteModel.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteModel.dstSet = m_descriptorSets[i];
        descriptorWriteModel.dstBinding = 1;
        descriptorWriteModel.dstArrayElement = 0;
        descriptorWriteModel.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptorWriteModel.descriptorCount = 1;
        descriptorWriteModel.pBufferInfo = &bufferInfoModel;
        descriptorWriteModel.pImageInfo = nullptr; // Optional
        descriptorWriteModel.pTexelBufferView = nullptr; // Optional

        VkDescriptorImageInfo  imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_textureImageView;
        imageInfo.sampler = m_textureSampler;
        VkWriteDescriptorSet descriptorWriteImage{};
        descriptorWriteImage.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteImage.dstSet = m_descriptorSets[i];
        descriptorWriteImage.dstBinding = 2;
        descriptorWriteImage.dstArrayElement = 0;
        descriptorWriteImage.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWriteImage.descriptorCount = 1;
        descriptorWriteImage.pImageInfo = &imageInfo;

        std::array<VkWriteDescriptorSet, 3> writeSets = { descriptorWriteCamera, descriptorWriteModel, descriptorWriteImage };
        m_devFunctions->vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
        
    }

    return true;
}

bool VulkanRenderer::createTextureImage()
{
    try {
        /* Read image from disk */
        Image image("assets/texture.jpg");
        VkDeviceSize imageSize = image.getWidth() * image.getHeight() * image.getChannels();

        /* Create staging buffer */
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(m_physicalDevice, m_device, imageSize,
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
        createImage(m_physicalDevice, m_device, image.getWidth(), 
            image.getHeight(), 
            VK_FORMAT_R8G8B8A8_SRGB, 
            VK_IMAGE_TILING_OPTIMAL, 
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            m_textureImage, 
            m_textureImageMemory);

        /* Transition image to DST_OPTIMAL in order to transfer the data */
        transitionImageLayout(m_device, m_window->graphicsQueue(), m_window->graphicsCommandPool(), 
            m_textureImage, 
            VK_FORMAT_R8G8B8A8_SRGB, 
            VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        /* copy data fronm staging buffer to image */
        copyBufferToImage(m_device, m_window->graphicsQueue(), m_window->graphicsCommandPool(), stagingBuffer, m_textureImage, static_cast<uint32_t>(image.getWidth()), static_cast<uint32_t>(image.getHeight()));

        /* Transition to SHADER_READ_ONLY layout */
        transitionImageLayout(m_device, m_window->graphicsQueue(), m_window->graphicsCommandPool(), 
            m_textureImage,
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

bool VulkanRenderer::createTextureImageView()
{
    m_textureImageView = createImageView(m_device, m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    return true;
}

bool VulkanRenderer::createTextureSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = m_physicalDeviceProperties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (m_devFunctions->vkCreateSampler(m_device, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to create a texture sampler");
        return false;
    }

    return true;
}

