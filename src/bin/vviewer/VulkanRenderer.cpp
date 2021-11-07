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
    
    m_modelDataDynamicUBO.init(m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment, 100);
    m_materialsUBO.init(m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment, 100);

    createDescriptorSetsLayouts();

    Texture * whiteTexture = createTexture("white", new Image(Image::Color::WHITE));

    MaterialPBR * lightMaterial = static_cast<MaterialPBR *>(createMaterial("lightMaterial", glm::vec4(1, 1, 1, 1), 0, 0, 1.0, 1.0f, false));
    MaterialPBR * defaultMaterial = static_cast<MaterialPBR *>(createMaterial("defaultMaterial", glm::vec4(0.5, 0.5, 0.5, 1), 0.5, .5, 1.0, 0.0f, false));
    MaterialPBR * ironMaterial = static_cast<MaterialPBR *>(createMaterial("ironMaterial", glm::vec4(0.5, 0.5, 0.5, 1), 0.5, .5, 1.0, 0.0f, false));
    
    ironMaterial->setAlbedoTexture(createTexture("assets/rustediron/basecolor.png"));
    ironMaterial->setMetallicTexture(createTexture("assets/rustediron/metallic.png"));
    ironMaterial->setRoughnessTexture(createTexture("assets/rustediron/roughness.png"));
    ironMaterial->setAOTexture(whiteTexture);
    ironMaterial->setEmissiveTexture(whiteTexture);
    
    /* Add a sphere in the scene, where the light is */
    {
        createVulkanMeshModel("sphere.obj");
        SceneObject * object = addSceneObject("sphere.obj", Transform({ 3, 3, 3 }, { 0.01, 0.01, 0.01 }), "lightMaterial");
        object->m_name = "hidden";
    }
    {
        createVulkanMeshModel("teapot.obj");
        SceneObject * object = addSceneObject("teapot.obj", Transform({ 0, 0, 0 }), "defaultMaterial");
        object->m_name = "hidden";
    }
}

void VulkanRenderer::initSwapChainResources()
{
    QSize size = m_window->swapChainImageSize();
    m_swapchainExtent.width = static_cast<uint32_t>(size.width());
    m_swapchainExtent.height = static_cast<uint32_t>(size.height());

    m_swapchainFormat = m_window->colorFormat();

    createRenderPass();
    createGraphicsPipeline();
    createFrameBuffers();
    createUniformBuffers();
    createDescriptorPool(100);
    createDescriptorSets();

    AssetManager<std::string, Material *>& instance = AssetManager<std::string, Material *>::getInstance();
    for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
        VulkanMaterialPBR * material = static_cast<VulkanMaterialPBR *>(itr->second);
        material->createDescriptors(m_device, m_descriptorSetLayoutMaterial, m_descriptorPool, m_window->swapChainImageCount());
        material->updateDescriptorSets(m_device, m_window->swapChainImageCount());
    }
}

void VulkanRenderer::releaseSwapChainResources()
{
    for (int i = 0; i < m_window->swapChainImageCount(); i++) {
        m_devFunctions->vkDestroyBuffer(m_device, m_uniformBuffersCamera[i], nullptr);
        m_devFunctions->vkFreeMemory(m_device, m_uniformBuffersCameraMemory[i], nullptr);

        m_devFunctions->vkDestroyBuffer(m_device, m_modelDataDynamicUBO.getBuffer(i), nullptr);
        m_devFunctions->vkFreeMemory(m_device, m_modelDataDynamicUBO.getBufferMemory(i), nullptr);

        m_devFunctions->vkDestroyBuffer(m_device, m_materialsUBO.getBuffer(i), nullptr);
        m_devFunctions->vkFreeMemory(m_device, m_materialsUBO.getBufferMemory(i), nullptr);
    }

    m_devFunctions->vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

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
    m_materialsUBO.destroy();

    /* Destroy texture data */
    {
        AssetManager<std::string, Texture *>& instance = AssetManager<std::string, Texture *>::getInstance();
        for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
            VulkanTexture * vkTexture = static_cast<VulkanTexture *>(itr->second);

            m_devFunctions->vkDestroyImageView(m_device, vkTexture->getImageView(), nullptr);
            m_devFunctions->vkDestroyImage(m_device, vkTexture->getImage(), nullptr);
            m_devFunctions->vkFreeMemory(m_device, vkTexture->getImageMemory(), nullptr);
        }
        instance.Reset();
    }

    /* Destroy material data */
    {
        AssetManager<std::string, Material *>& instance = AssetManager<std::string, Material *>::getInstance();
        for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
            VulkanMaterialPBR * material = static_cast<VulkanMaterialPBR *>(itr->second);
            m_devFunctions->vkDestroySampler(m_device, material->m_sampler, nullptr);
        }
        instance.Reset();
    }

    /* Destroy descriptor sets layouts */
    m_devFunctions->vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayoutCamera, nullptr);
    m_devFunctions->vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayoutModel, nullptr);
    m_devFunctions->vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayoutMaterial, nullptr);

    /* Destroy imported models */
    {
        AssetManager<std::string, MeshModel *>& instance = AssetManager<std::string, MeshModel *>::getInstance();
        for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
            destroyVulkanMeshModel(*itr->second);
        }
        instance.Reset();
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
        VulkanMaterialPBR * material = static_cast<VulkanMaterialPBR *>(object->getMaterial());
        if (material->needsUpdate(imageIndex)) material->updateDescriptorSet(m_device, imageIndex);

        std::vector<Mesh *> meshes = object->getMeshModel()->getMeshes();
        for (size_t i = 0; i < meshes.size(); i++) {
            VulkanMesh * vkmesh = static_cast<VulkanMesh *>(meshes[i]);

            VkBuffer vertexBuffers[] = { vkmesh->m_vertexBuffer };
            VkDeviceSize offsets[] = { 0 };
            m_devFunctions->vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
            m_devFunctions->vkCmdBindIndexBuffer(cmdBuf, vkmesh->m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

            /* Calculate model data offsets */
            uint32_t dynamicOffsets[2] = {
                static_cast<uint32_t>(m_modelDataDynamicUBO.getBlockSizeAligned()) * object->getTransformUBOBlock(),
                static_cast<uint32_t>(m_materialsUBO.getBlockSizeAligned()) * material->getUBOBlockIndex()
            };
            VkDescriptorSet descriptorSets[3] = {
                m_descriptorSetsCamera[imageIndex],
                m_descriptorSetsModel[imageIndex],
                material->getDescriptor(imageIndex),
            };

            m_devFunctions->vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
                0, 3, &descriptorSets[0], 2, &dynamicOffsets[0]);

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
        VulkanMeshModel * vkmesh = new VulkanMeshModel(m_physicalDevice, m_device, m_window->graphicsQueue(), m_window->graphicsCommandPool(), assimpLoadModel(filename));
        instance.Add(filename, vkmesh);
    } catch (std::runtime_error& e) {
        utils::Console(utils::DebugLevel::WARNING, "Failed to create a Vulkan Mesh Model: " + std::string(e.what()));
        return false;
    }

    return true;
}

SceneObject * VulkanRenderer::addSceneObject(std::string meshModel, Transform transform, std::string material)
{
    AssetManager<std::string, MeshModel *>& instanceModels = AssetManager<std::string, MeshModel *>::getInstance();
    if (!instanceModels.isPresent(meshModel)) return nullptr;
    AssetManager<std::string, Material *>& instanceMaterials = AssetManager<std::string, Material *>::getInstance();
    if (!instanceMaterials.isPresent(material)) return nullptr;

    MeshModel * vkmeshModel = instanceModels.Get(meshModel);
    VulkanSceneObject * object = new VulkanSceneObject(vkmeshModel, transform, m_modelDataDynamicUBO, m_transformIndexUBO++);

    object->setMaterial(instanceMaterials.Get(material));

    m_objects.push_back(object);
    return object;
}

Material * VulkanRenderer::createMaterial(std::string name, 
    glm::vec4 albedo, float metallic, float roughness, float ao, float emissive,
    bool createDescriptors)
{
    AssetManager<std::string, Material *>& instance = AssetManager<std::string, Material *>::getInstance();
    if (instance.isPresent(name)) return nullptr;
    
    VulkanMaterialPBR * temp = new VulkanMaterialPBR(name, albedo, metallic, roughness, ao, emissive, m_device, m_materialsUBO, m_materialsIndexUBO++);

    if (createDescriptors) {
        temp->createDescriptors(m_device, m_descriptorSetLayoutMaterial, m_descriptorPool, m_window->swapChainImageCount());
        temp->updateDescriptorSets(m_device, m_window->swapChainImageCount());
    }

    instance.Add(temp->m_name, temp);

    return temp;
}

Texture * VulkanRenderer::createTexture(std::string imagePath)
{
    try {
        AssetManager<std::string, Image *>& instance = AssetManager<std::string, Image *>::getInstance();
        
        Image * image = nullptr;
        if (instance.isPresent(imagePath)) {
            image = instance.Get(imagePath);
        }
        else {
            image = new Image(imagePath);
            instance.Add(imagePath, image);
        }

        return createTexture(imagePath, image);
    } catch (std::runtime_error& e) {
        utils::ConsoleCritical("Failed to create a vulkan texture: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
}

Texture * VulkanRenderer::createTexture(std::string id, Image * image)
{
    try {
        AssetManager<std::string, Texture *>& instance = AssetManager<std::string, Texture *>::getInstance();
        
        if (instance.isPresent(id)) {
            return instance.Get(id);
        }

        VulkanTexture * temp = new VulkanTexture(id, image, m_physicalDevice, m_device, m_window->graphicsQueue(), m_window->graphicsCommandPool());
        instance.Add(id, temp);
        return temp;
    }
    catch (std::runtime_error& e) {
        utils::ConsoleCritical("Failed to create a vulkan texture: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
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
    std::array<VkDescriptorSetLayout, 3> descriptorSetsLayouts{ m_descriptorSetLayoutCamera, m_descriptorSetLayoutModel, m_descriptorSetLayoutMaterial };
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetsLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetsLayouts.data();
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
    {
        VkDescriptorSetLayoutBinding cameraDataLayoutBinding{};
        cameraDataLayoutBinding.binding = 0;
        cameraDataLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        cameraDataLayoutBinding.descriptorCount = 1;    /* If we have an array of uniform buffers */
        cameraDataLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        cameraDataLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &cameraDataLayoutBinding;

        if (m_devFunctions->vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayoutCamera) != VK_SUCCESS) {
            utils::ConsoleCritical("Failed to create a descriptor set layout for the camera data");
            return false;
        }
    }

    {
        /* Create binding for model data */
        VkDescriptorSetLayoutBinding modelDataLayoutBinding{};
        modelDataLayoutBinding.binding = 0;
        modelDataLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        modelDataLayoutBinding.descriptorCount = 1;    /* If we have an array of uniform buffers */
        modelDataLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        modelDataLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &modelDataLayoutBinding;

        if (m_devFunctions->vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayoutModel) != VK_SUCCESS) {
            utils::ConsoleCritical("Failed to create a descriptor set layout for model data");
            return false;
        }
    }

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
        materialTexturesLayoutBinding.descriptorCount = 5;  /* An array of 5 textures */
        materialTexturesLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        materialTexturesLayoutBinding.pImmutableSamplers = nullptr;
        materialTexturesLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> setBindings = { materiaDatalLayoutBinding, materialTexturesLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(setBindings.size());
        layoutInfo.pBindings = setBindings.data();

        if (m_devFunctions->vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayoutMaterial) != VK_SUCCESS) {
            utils::ConsoleCritical("Failed to create a descriptor set layout");
            return false;
        }
    }

    return true;
}

bool VulkanRenderer::createUniformBuffers()
{
    {
        VkDeviceSize bufferSize = sizeof(CameraData);

        m_uniformBuffersCamera.resize(m_window->swapChainImageCount());
        m_uniformBuffersCameraMemory.resize(m_window->swapChainImageCount());

        for (int i = 0; i < m_window->swapChainImageCount(); i++) {
            createBuffer(m_physicalDevice, m_device, bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                m_uniformBuffersCamera[i], 
                m_uniformBuffersCameraMemory[i]);
        }
    }

    m_modelDataDynamicUBO.createBuffers(m_physicalDevice, m_device, m_window->swapChainImageCount());
    m_materialsUBO.createBuffers(m_physicalDevice, m_device, m_window->swapChainImageCount());

    return true;
}

bool VulkanRenderer::updateUniformBuffers(size_t index)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    /* Flush camera data to GPU */
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

    /* Flush Transform changes to GPU */
    {
        void* data;
        m_devFunctions->vkMapMemory(m_device, m_modelDataDynamicUBO.getBufferMemory(index), 0, m_modelDataDynamicUBO.getBlockSizeAligned() * m_modelDataDynamicUBO.getNBlocks(), 0, &data);
        memcpy(data, m_modelDataDynamicUBO.getBlock(0), m_modelDataDynamicUBO.getBlockSizeAligned() * m_modelDataDynamicUBO.getNBlocks());
        m_devFunctions->vkUnmapMemory(m_device, m_modelDataDynamicUBO.getBufferMemory(index));
    }

    /* Flush material changes to GPU */
    {
        void* data;
        m_devFunctions->vkMapMemory(m_device, m_materialsUBO.getBufferMemory(index), 0, m_materialsUBO.getBlockSizeAligned() * m_materialsUBO.getNBlocks(), 0, &data);
        memcpy(data, m_materialsUBO.getBlock(0), m_materialsUBO.getBlockSizeAligned() * m_materialsUBO.getNBlocks());
        m_devFunctions->vkUnmapMemory(m_device, m_materialsUBO.getBufferMemory(index));
    }

    return true;
}

bool VulkanRenderer::createDescriptorPool(size_t nMaterials)
{
    uint32_t nImages = static_cast<uint32_t>(m_window->swapChainImageCount());

    VkDescriptorPoolSize cameraDataPoolSize{};
    cameraDataPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraDataPoolSize.descriptorCount = nImages;

    VkDescriptorPoolSize modelDataPoolSize{};
    modelDataPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    modelDataPoolSize.descriptorCount = nImages;

    VkDescriptorPoolSize materialDataPoolSize{};
    materialDataPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    materialDataPoolSize.descriptorCount = nMaterials * nImages;

    VkDescriptorPoolSize materialTexturesPoolSize{};
    materialTexturesPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    materialTexturesPoolSize.descriptorCount = nMaterials * 5 * nImages;

    std::array<VkDescriptorPoolSize, 4> poolSizes = { cameraDataPoolSize, modelDataPoolSize, materialDataPoolSize, materialTexturesPoolSize };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 2 * nImages + nMaterials * nImages;

    if (m_devFunctions->vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to create a descriptor pool");
        return false;
    }

    return true;
}

bool VulkanRenderer::createDescriptorSets()
{
    uint32_t nImages = static_cast<uint32_t>(m_window->swapChainImageCount());
    
    /* Create descriptor sets */
    {
        m_descriptorSetsCamera.resize(nImages);

        std::vector<VkDescriptorSetLayout> layouts(nImages, m_descriptorSetLayoutCamera);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = nImages;
        allocInfo.pSetLayouts = layouts.data();
        if (m_devFunctions->vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSetsCamera.data()) != VK_SUCCESS) {
            utils::ConsoleCritical("Failed to allocate descriptor sets for camera data");
            return false;
        }
    }

    {
        m_descriptorSetsModel.resize(nImages);

        std::vector<VkDescriptorSetLayout> layouts(nImages, m_descriptorSetLayoutModel);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = nImages;
        allocInfo.pSetLayouts = layouts.data();
        if (m_devFunctions->vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSetsModel.data()) != VK_SUCCESS) {
            utils::ConsoleCritical("Failed to allocate descriptor sets for model data");
            return false;
        }
    }

    /* Write descriptor sets */
    for (size_t i = 0; i < nImages; i++) {

        VkDescriptorBufferInfo bufferInfoCamera{};
        bufferInfoCamera.buffer = m_uniformBuffersCamera[i];
        bufferInfoCamera.offset = 0;
        bufferInfoCamera.range = sizeof(CameraData);
        VkWriteDescriptorSet descriptorWriteCamera{};
        descriptorWriteCamera.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteCamera.dstSet = m_descriptorSetsCamera[i];
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
        descriptorWriteModel.dstSet = m_descriptorSetsModel[i];
        descriptorWriteModel.dstBinding = 0;
        descriptorWriteModel.dstArrayElement = 0;
        descriptorWriteModel.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptorWriteModel.descriptorCount = 1;
        descriptorWriteModel.pBufferInfo = &bufferInfoModel;
        descriptorWriteModel.pImageInfo = nullptr; // Optional
        descriptorWriteModel.pTexelBufferView = nullptr; // Optional

        std::array<VkWriteDescriptorSet, 2> writeSets = { descriptorWriteCamera, descriptorWriteModel };
        m_devFunctions->vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
    }

    return true;
}


