#include "VulkanRenderer.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#include <utils/Console.hpp>

#include "core/Image.hpp"
#include "core/EnvironmentMap.hpp"
#include "models/AssimpLoadModel.hpp"
#include "Shader.hpp"
#include "VulkanUtils.hpp"

VulkanRenderer::VulkanRenderer(QVulkanWindow * window, VulkanScene* scene) : m_window(window) 
{
    m_scene = scene;
}

void VulkanRenderer::preInitResources()
{
    bool res = pickPhysicalDevice();
    if (!res) {
        utils::ConsoleFatal("Failed to find suitable Vulkan device");
        return;
    }

    VkFormat preferredFormat = VK_FORMAT_B8G8R8A8_SRGB;
    m_window->setPreferredColorFormats({ preferredFormat });
    
    /* Set device extensions */
    //QByteArrayList requiuredExtensions;
    //m_window->setDeviceExtensions(requiuredExtensions);
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
    
    /* Initialize dynamic uniform buffers for model matrix data, max 100 items */
    m_scene->m_modelDataDynamicUBO.init(m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment, 100);
    /* Initialize dynamic uniform buffers for material data, max 100 items */
    m_materialsUBO.init(m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment, 100);

    /* Create descriptor set layouts for scene and model data */
    createDescriptorSetsLayouts();

    /* Initialize some data */
    Texture* whiteTexture = createTexture("white", new Image<stbi_uc>(Image<stbi_uc>::Color::WHITE), VK_FORMAT_R8G8B8A8_UNORM);
    Texture* whiteColor = createTexture("whiteColor", new Image<stbi_uc>(Image<stbi_uc>::Color::WHITE), VK_FORMAT_R8G8B8A8_SRGB);
    createTexture("normalmapdefault", new Image<stbi_uc>(Image<stbi_uc>::Color::NORMAL_MAP), VK_FORMAT_R8G8B8A8_UNORM);

    /* Initialize renderers */
    m_rendererSkybox.initResources(m_physicalDevice, 
        m_device, 
        m_window->graphicsQueue(), 
        m_window->graphicsCommandPool(), 
        m_descriptorSetLayoutScene);
    m_rendererPBR.initResources(m_physicalDevice, 
        m_device, 
        m_window->graphicsQueue(), 
        m_window->graphicsCommandPool(), 
        m_physicalDeviceProperties, 
        m_descriptorSetLayoutScene, 
        m_descriptorSetLayoutModel, 
        m_rendererSkybox.getDescriptorSetLayout());
    m_rendererLambert.initResources(m_physicalDevice,
        m_device,
        m_window->graphicsQueue(),
        m_window->graphicsCommandPool(),
        m_physicalDeviceProperties,
        m_descriptorSetLayoutScene,
        m_descriptorSetLayoutModel,
        m_rendererSkybox.getDescriptorSetLayout());
    m_rendererRayTracing.initResources(m_physicalDevice, VK_FORMAT_R32G32B32A32_SFLOAT, 1024u, 1024u);

    /* Create a default material */
    MaterialPBRStandard * defaultMaterial = static_cast<MaterialPBRStandard *>(createMaterial("defaultMaterial", MaterialType::MATERIAL_PBR_STANDARD, false));
    defaultMaterial->albedo() = glm::vec4(0.5, 0.5, 0.5, 1);
    defaultMaterial->metallic() = 0.5;
    defaultMaterial->roughness() = 0.5;
    defaultMaterial->ao() = 1.0f;
    defaultMaterial->emissive() = 0.0f;

    /* Import some models */
    createVulkanMeshModel("assets/models/uvsphere.obj");
    createVulkanMeshModel("assets/models/plane.obj");
    createVulkanMeshModel("assets/models/rtscene.obj");
    createVulkanMeshModel("assets/models/teapot.obj");
    createVulkanMeshModel("assets/models/cube.obj");

    /* Create a skybox material */
    {
        EnvironmentMap* envMap = createEnvironmentMap("assets/HDR/harbor.hdr");
        VulkanMaterialSkybox* skybox = new VulkanMaterialSkybox("skybox", envMap, m_device, m_rendererSkybox.getDescriptorSetLayout());
        AssetManager<std::string, MaterialSkybox*>& instance = AssetManager<std::string, MaterialSkybox*>::getInstance();
        instance.Add(skybox->m_name, skybox);
        m_scene->setSkybox(skybox);
    }
}

void VulkanRenderer::initSwapChainResources()
{
    QSize size = m_window->swapChainImageSize();
    m_swapchainExtent.width = static_cast<uint32_t>(size.width());
    m_swapchainExtent.height = static_cast<uint32_t>(size.height());

    m_swapchainFormat = m_window->colorFormat();

    createRenderPass();
    createFrameBuffers();
    createUniformBuffers();
    createDescriptorPool(100);
    createDescriptorSets();
    m_rendererSkybox.initSwapChainResources(m_swapchainExtent, m_renderPassForward);
    m_rendererPBR.initSwapChainResources(m_swapchainExtent, m_renderPassForward, m_window->swapChainImageCount());
    m_rendererLambert.initSwapChainResources(m_swapchainExtent, m_renderPassForward, m_window->swapChainImageCount());

    /* Update descriptor sets */
    {
        AssetManager<std::string, Material *>& instance = AssetManager<std::string, Material *>::getInstance();
        for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
            VulkanMaterialDescriptor* material = dynamic_cast<VulkanMaterialDescriptor*>(itr->second);
            material->createDescriptors(m_device, m_descriptorPool, m_window->swapChainImageCount());
            material->updateDescriptorSets(m_device, m_window->swapChainImageCount());
        }
    }
    {
        AssetManager<std::string, MaterialSkybox*>& instance = AssetManager<std::string, MaterialSkybox*>::getInstance();
        for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
            VulkanMaterialSkybox* material = static_cast<VulkanMaterialSkybox*>(itr->second);
            material->createDescriptors(m_device, m_descriptorPool, m_window->swapChainImageCount());
            material->updateDescriptorSets(m_device, m_window->swapChainImageCount());
        }
    }
}

void VulkanRenderer::releaseSwapChainResources()
{
    /* Destroy uniform buffers */
    for (int i = 0; i < m_window->swapChainImageCount(); i++) {
        m_devFunctions->vkDestroyBuffer(m_device, m_scene->m_uniformBuffersScene[i], nullptr);
        m_devFunctions->vkFreeMemory(m_device, m_scene->m_uniformBuffersSceneMemory[i], nullptr);

        m_devFunctions->vkDestroyBuffer(m_device, m_scene->m_modelDataDynamicUBO.getBuffer(i), nullptr);
        m_devFunctions->vkFreeMemory(m_device, m_scene->m_modelDataDynamicUBO.getBufferMemory(i), nullptr);

        m_devFunctions->vkDestroyBuffer(m_device, m_materialsUBO.getBuffer(i), nullptr);
        m_devFunctions->vkFreeMemory(m_device, m_materialsUBO.getBufferMemory(i), nullptr);
    }

    m_devFunctions->vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

    for (auto framebuffer : m_swapChainFramebuffers) {
        m_devFunctions->vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }

    m_rendererSkybox.releaseSwapChainResources();
    m_rendererPBR.releaseSwapChainResources();
    m_rendererLambert.releaseSwapChainResources();
    m_rendererRayTracing.releaseSwapChainResources();

    m_devFunctions->vkDestroyRenderPass(m_device, m_renderPassForward, nullptr);

}

void VulkanRenderer::releaseResources()
{
    m_scene->m_modelDataDynamicUBO.destroyCPUMemory();
    m_materialsUBO.destroyCPUMemory();

    /* Destroy texture data */
    {
        AssetManager<std::string, Texture *>& instance = AssetManager<std::string, Texture *>::getInstance();
        for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
            VulkanTexture * vkTexture = static_cast<VulkanTexture *>(itr->second);

            m_devFunctions->vkDestroyImageView(m_device, vkTexture->getImageView(), nullptr);
            m_devFunctions->vkDestroyImage(m_device, vkTexture->getImage(), nullptr);
            m_devFunctions->vkFreeMemory(m_device, vkTexture->getImageMemory(), nullptr);
            m_devFunctions->vkDestroySampler(m_device, vkTexture->getSampler(), nullptr);
        }
        instance.Reset();
    }

    /* Destroy material data */
    {
        AssetManager<std::string, Material*>& instance = AssetManager<std::string, Material*>::getInstance();
        instance.Reset();
    }

    /* Destroy cubemaps */
    {
        AssetManager<std::string, Cubemap *>& instance = AssetManager<std::string, Cubemap *>::getInstance();
        for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
            VulkanCubemap * vkCubemap = static_cast<VulkanCubemap *>(itr->second);

            m_devFunctions->vkDestroyImageView(m_device, vkCubemap->getImageView(), nullptr);
            m_devFunctions->vkDestroyImage(m_device, vkCubemap->getImage(), nullptr);
            m_devFunctions->vkFreeMemory(m_device, vkCubemap->getDeviceMemory(), nullptr);
            m_devFunctions->vkDestroySampler(m_device, vkCubemap->getSampler(), nullptr);
        }
    }

    /* Destroy descriptor sets layouts */
    m_devFunctions->vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayoutScene, nullptr);
    m_devFunctions->vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayoutModel, nullptr);
    m_rendererSkybox.releaseResources();
    m_rendererPBR.releaseResources();
    m_rendererLambert.releaseResources();
    m_rendererRayTracing.releaseResources();

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
    /* TODO(optimization) check if anything has changed to not traverse the entire tree */
    m_scene->updateSceneGraph();
    /* TODO(optimization) check if anything has changed to not traverse the entire tree */
    std::vector<std::shared_ptr<SceneObject>> sceneObjects = m_scene->getSceneObjects();

    int imageIndex = m_window->currentSwapChainImageIndex();

    /* Update GPU buffers */
    updateUniformBuffers(imageIndex);

    std::array<VkClearValue, 2> clearValues{};
    VkClearColorValue clearColor = { { m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a } };
    clearValues[0].color = clearColor;
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo rpBeginInfo;
    memset(&rpBeginInfo, 0, sizeof(rpBeginInfo));
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = m_renderPassForward;
    rpBeginInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
    rpBeginInfo.renderArea.offset = { 0, 0 };
    rpBeginInfo.renderArea.extent.width = m_swapchainExtent.width;
    rpBeginInfo.renderArea.extent.height = m_swapchainExtent.height;
    rpBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;
    rpBeginInfo.pClearValues = clearValues.data();
    VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();
    m_devFunctions->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    /* Draw skybox */
    VulkanMaterialSkybox* skybox = dynamic_cast<VulkanMaterialSkybox*>(m_scene->getSkybox());
    assert(skybox != nullptr);
    m_rendererSkybox.renderSkybox(cmdBuf, m_descriptorSetsScene[imageIndex], imageIndex, skybox);

    /* Batch objects into materials */
    std::vector<std::shared_ptr<SceneObject>> pbrStandardObjects;
    std::vector<std::shared_ptr<SceneObject>> lambertObjects;
    for (auto& itr : sceneObjects)
    {
        if (itr->getMaterial() == nullptr) continue;

        switch (itr->getMaterial()->getType())
        {
        case MaterialType::MATERIAL_PBR_STANDARD:
            pbrStandardObjects.push_back(itr);
            break;
        case MaterialType::MATERIAL_LAMBERT:
            lambertObjects.push_back(itr);
            break;
        default:
            break;
        }
    }

    /* Draw PBR material objects */
    m_rendererPBR.renderObjects(cmdBuf, 
        m_descriptorSetsScene[imageIndex],
        m_descriptorSetsModel[imageIndex],
        skybox,
        imageIndex, 
        m_scene->m_modelDataDynamicUBO,
        pbrStandardObjects);

    /* Draw lambert material objects */
    m_rendererLambert.renderObjects(cmdBuf,
        m_descriptorSetsScene[imageIndex],
        m_descriptorSetsModel[imageIndex],
        skybox,
        imageIndex,
        m_scene->m_modelDataDynamicUBO,
        lambertObjects);

    m_devFunctions->vkCmdEndRenderPass(cmdBuf);
    
    m_window->frameReady();
    m_window->requestUpdate(); // render continuously, throttled by the presentation rate
}

bool VulkanRenderer::createVulkanMeshModel(std::string filename)
{
    AssetManager<std::string, MeshModel *>& instance = AssetManager<std::string, MeshModel *>::getInstance();

    if (instance.isPresent(filename)) return false;
    
    try {
        std::vector<Mesh> meshes = assimpLoadModel(filename);
        VulkanMeshModel * vkmesh = new VulkanMeshModel(m_physicalDevice, m_device, m_window->graphicsQueue(), m_window->graphicsCommandPool(), meshes);
        vkmesh->setName(filename);
        instance.Add(filename, vkmesh);
    } catch (std::runtime_error& e) {
        utils::Console(utils::DebugLevel::WARNING, "Failed to create a Vulkan Mesh Model: " + std::string(e.what()));
        return false;
    }

    return true;
}

VulkanScene* VulkanRenderer::getActiveScene() const
{
    return m_scene;
}

Material * VulkanRenderer::createMaterial(std::string name, MaterialType type, bool createDescriptors)
{
    AssetManager<std::string, Material *>& instance = AssetManager<std::string, Material *>::getInstance();
    if (instance.isPresent(name)) {
        utils::ConsoleWarning("Material name already present");
        return nullptr;
    }
    
    Material* temp = nullptr;
    switch (type)
    {
    case MaterialType::MATERIAL_PBR_STANDARD:
    {
        temp = m_rendererPBR.createMaterial(name, glm::vec4(1, 1, 1, 1), 1, 1, 1, 0, m_materialsUBO, m_materialsIndexUBO++);
        break;
    }
    case MaterialType::MATERIAL_LAMBERT:
    {
        temp = m_rendererLambert.createMaterial(name, glm::vec4(1, 1, 1, 1), 1, 0, m_materialsUBO, m_materialsIndexUBO++);
        break;
    }
    default:
    {
        throw std::runtime_error("Material type not present");
        break;
    }
    }

    if (createDescriptors) {
        VulkanMaterialDescriptor* matdesc = dynamic_cast<VulkanMaterialDescriptor*>(temp);
        matdesc->createDescriptors(m_device, m_descriptorPool, m_window->swapChainImageCount());
        matdesc->updateDescriptorSets(m_device, m_window->swapChainImageCount());
    }

    instance.Add(temp->m_name, temp);
    return temp;
}

void VulkanRenderer::renderRT()
{
    m_scene->updateSceneGraph();
    m_rendererRayTracing.renderScene(m_scene);
}

Texture * VulkanRenderer::createTexture(std::string imagePath, VkFormat format, bool keepImage)
{
    try {
        AssetManager<std::string, Image<stbi_uc> *>& instance = AssetManager<std::string, Image<stbi_uc> *>::getInstance();
        
        Image<stbi_uc> * image = nullptr;
        if (instance.isPresent(imagePath)) {
            image = instance.Get(imagePath);
        }
        else {
            image = new Image<stbi_uc>(imagePath);
            instance.Add(imagePath, image);
        }

        Texture * temp = createTexture(imagePath, image, format);

        if (!keepImage) {
            instance.Remove(imagePath);
            delete image;
        }

        return temp;
    } catch (std::runtime_error& e) {
        utils::ConsoleCritical("Failed to create a vulkan texture: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
}

Texture * VulkanRenderer::createTexture(std::string id, Image<stbi_uc> * image, VkFormat format)
{
    try {
        AssetManager<std::string, Texture *>& instance = AssetManager<std::string, Texture *>::getInstance();
        
        if (instance.isPresent(id)) {
            return instance.Get(id);
        }

        TextureType type = TextureType::NO_TYPE;
        if (format == VK_FORMAT_R8G8B8A8_SRGB) type = TextureType::COLOR;
        else if (format == VK_FORMAT_R8G8B8A8_UNORM) type = TextureType::LINEAR;

        VulkanTexture * temp = new VulkanTexture(id, 
            image, 
            type, 
            m_physicalDevice, 
            m_device, 
            m_window->graphicsQueue(), 
            m_window->graphicsCommandPool(), 
            format, 
            true);

        instance.Add(id, temp);
        return temp;
    }
    catch (std::runtime_error& e) {
        utils::ConsoleCritical("Failed to create a vulkan texture: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
}

Texture * VulkanRenderer::createTextureHDR(std::string imagePath, bool keepImage)
{
    try {
        AssetManager<std::string, Texture *>& instance = AssetManager<std::string, Texture *>::getInstance();
        if (instance.isPresent(imagePath)) {
            return instance.Get(imagePath);
        }
        
        Image<float> * image = nullptr;
        AssetManager<std::string, Image<float> *>& images = AssetManager<std::string, Image<float> *>::getInstance();
        if (images.isPresent(imagePath)) {
            image = images.Get(imagePath);
        }
        else {
            image = new Image<float>(imagePath);
            images.Add(imagePath, image);
        }

        VulkanTexture * temp = new VulkanTexture(imagePath, 
            image, 
            TextureType::HDR, 
            m_physicalDevice, 
            m_device, 
            m_window->graphicsQueue(), 
            m_window->graphicsCommandPool(),
            false);
        instance.Add(imagePath, temp);

        if (!keepImage) {
            images.Remove(imagePath);
            delete image;
        }

        return temp;
    }
    catch (std::runtime_error& e) {
        utils::ConsoleCritical("Failed to create a vulkan HDR texture: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;

}

Cubemap * VulkanRenderer::createCubemap(std::string directory)
{
    AssetManager<std::string, Cubemap *>& instance = AssetManager<std::string, Cubemap *>::getInstance();
    if (instance.isPresent(directory)) {
        return instance.Get(directory);
    }

    VulkanCubemap * cubemap = new VulkanCubemap(directory, m_physicalDevice, m_device, m_window->graphicsQueue(), m_window->graphicsCommandPool());
    instance.Add(directory, cubemap);
    return cubemap;
}

EnvironmentMap* VulkanRenderer::createEnvironmentMap(std::string imagePath, bool keepTexture)
{
    try {
        /* Check if an environment map for that imagePath already exists */
        AssetManager<std::string, EnvironmentMap*>& envMaps = AssetManager<std::string, EnvironmentMap*>::getInstance();
        if (envMaps.isPresent(imagePath)) {
            return envMaps.Get(imagePath);
        }

        /* Read HDR image */
        VulkanTexture* hdrImage = static_cast<VulkanTexture*>(createTextureHDR(imagePath));

        /* Transform input texture into a cubemap */
        VulkanCubemap* cubemap = m_rendererSkybox.createCubemap(hdrImage);
        /* Compute irradiance map */
        VulkanCubemap* irradiance = m_rendererSkybox.createIrradianceMap(cubemap);
        /* Compute prefiltered map */
        VulkanCubemap* prefiltered = m_rendererSkybox.createPrefilteredCubemap(cubemap);

        AssetManager<std::string, Cubemap*>& instance = AssetManager<std::string, Cubemap*>::getInstance();
        instance.Add(cubemap->m_name, cubemap);
        instance.Add(irradiance->m_name, irradiance);
        instance.Add(prefiltered->m_name, prefiltered);

        EnvironmentMap* envMap = new EnvironmentMap(imagePath, cubemap, irradiance, prefiltered);
        envMaps.Add(imagePath, envMap);

        if (!keepTexture) {
            AssetManager<std::string, Texture*>& textures = AssetManager<std::string, Texture*>::getInstance();
            textures.Remove(imagePath);
            hdrImage->Destroy(m_device);
        }

        return envMap;
    }
    catch (std::runtime_error& e) {
        utils::ConsoleCritical("Failed to create a vulkan environment map: " + std::string(e.what()));
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
    /* TODO check if physical device has the available extensions */
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

    if (m_devFunctions->vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPassForward) != VK_SUCCESS) {
        utils::ConsoleFatal("Failed to create a render pass");
        return false;
    }

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
        framebufferInfo.renderPass = m_renderPassForward;
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
    /* Create binding for scene data */
    {
        VkDescriptorSetLayoutBinding sceneDataLayoutBinding{};
        sceneDataLayoutBinding.binding = 0;
        sceneDataLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        sceneDataLayoutBinding.descriptorCount = 1;    /* If we have an array of uniform buffers */
        sceneDataLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        sceneDataLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &sceneDataLayoutBinding;

        if (m_devFunctions->vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayoutScene) != VK_SUCCESS) {
            utils::ConsoleCritical("Failed to create a descriptor set layout for the scene data");
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

    return true;
}

bool VulkanRenderer::createUniformBuffers()
{
    {
        VkDeviceSize bufferSize = sizeof(SceneData);

        m_scene->m_uniformBuffersScene.resize(m_window->swapChainImageCount());
        m_scene->m_uniformBuffersSceneMemory.resize(m_window->swapChainImageCount());

        for (int i = 0; i < m_window->swapChainImageCount(); i++) {
            createBuffer(m_physicalDevice, m_device, bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                m_scene->m_uniformBuffersScene[i],
                m_scene->m_uniformBuffersSceneMemory[i]);
        }
    }

    m_scene->m_modelDataDynamicUBO.createBuffers(m_physicalDevice, m_device, m_window->swapChainImageCount());
    m_materialsUBO.createBuffers(m_physicalDevice, m_device, m_window->swapChainImageCount());

    return true;
}

bool VulkanRenderer::updateUniformBuffers(size_t index)
{
    /* Flush scene data changes to GPU */
    m_scene->updateBuffers(m_device, index);
    /* Flush material data changes to GPU */
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

    VkDescriptorPoolSize sceneDataPoolSize{};
    sceneDataPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sceneDataPoolSize.descriptorCount = nImages;

    VkDescriptorPoolSize modelDataPoolSize{};
    modelDataPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    modelDataPoolSize.descriptorCount = nImages;

    VkDescriptorPoolSize materialDataPoolSize{};
    materialDataPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    materialDataPoolSize.descriptorCount = static_cast<uint32_t>(nMaterials * nImages);

    VkDescriptorPoolSize materialTexturesPoolSize{};
    materialTexturesPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    materialTexturesPoolSize.descriptorCount = static_cast<uint32_t>(nMaterials * 6 * nImages);

    std::array<VkDescriptorPoolSize, 4> poolSizes = { sceneDataPoolSize, modelDataPoolSize, materialDataPoolSize, materialTexturesPoolSize };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(2 * nImages + nMaterials * nImages);

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
        m_descriptorSetsScene.resize(nImages);

        std::vector<VkDescriptorSetLayout> layouts(nImages, m_descriptorSetLayoutScene);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = nImages;
        allocInfo.pSetLayouts = layouts.data();
        if (m_devFunctions->vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSetsScene.data()) != VK_SUCCESS) {
            utils::ConsoleCritical("Failed to allocate descriptor sets for the scene data");
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

        VkDescriptorBufferInfo bufferInfoScene{};
        bufferInfoScene.buffer = m_scene->m_uniformBuffersScene[i];
        bufferInfoScene.offset = 0;
        bufferInfoScene.range = sizeof(SceneData);
        VkWriteDescriptorSet descriptorWriteScene{};
        descriptorWriteScene.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteScene.dstSet = m_descriptorSetsScene[i];
        descriptorWriteScene.dstBinding = 0;
        descriptorWriteScene.dstArrayElement = 0;
        descriptorWriteScene.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWriteScene.descriptorCount = 1;
        descriptorWriteScene.pBufferInfo = &bufferInfoScene;
        descriptorWriteScene.pImageInfo = nullptr; // Optional
        descriptorWriteScene.pTexelBufferView = nullptr; // Optional
        
        VkDescriptorBufferInfo bufferInfoModel{};
        bufferInfoModel.buffer = m_scene->m_modelDataDynamicUBO.getBuffer(i);
        bufferInfoModel.offset = 0;
        bufferInfoModel.range = m_scene->m_modelDataDynamicUBO.getBlockSizeAligned();
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

        std::array<VkWriteDescriptorSet, 2> writeSets = { descriptorWriteScene, descriptorWriteModel };
        m_devFunctions->vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
    }

    return true;
}


