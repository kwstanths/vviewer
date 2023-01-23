#include "VulkanRenderer.hpp"

#include <chrono>

#include "QtConcurrent/qtconcurrentrun.h"
#include "Timer.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stb_image_write.h>

#include <utils/Console.hpp>

#include "core/Image.hpp"
#include "core/EnvironmentMap.hpp"
#include "models/AssimpLoadModel.hpp"
#include "vulkan/Shader.hpp"
#include "vulkan/VulkanUtils.hpp"

VulkanRenderer::VulkanRenderer(QVulkanWindow * window, VulkanScene* scene) : 
    m_window(window), m_materialsUBO(100)
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

    m_window->setSampleCount(m_msaaSamples);
    utils::ConsoleInfo("MSAA samples: " + std::to_string(m_msaaSamples));

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
    
    /* Initialize dynamic uniform buffers for model matrix data */
    m_scene->m_modelDataDynamicUBO.init(m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
    /* Initialize dynamic uniform buffers for material data */
    m_materialsUBO.init(m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);

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
    m_rendererPost.initResources(m_physicalDevice, m_device, m_window->graphicsQueue(), m_window->graphicsCommandPool());

    try {
        m_renderer3DUI.initResources(m_physicalDevice, m_device, m_window->graphicsQueue(), m_window->graphicsCommandPool(), m_descriptorSetLayoutScene);
    }
    catch (std::exception& e) {
        utils::ConsoleCritical("Failed to initialize UI renderer: " + std::string(e.what()));
        return;
    }
    
    try {
        m_rendererRayTracing.initResources(m_physicalDevice, VK_FORMAT_R32G32B32A32_SFLOAT);
    }
    catch (std::exception& e) {
        utils::ConsoleWarning("Failed to initialize GPU ray tracing renderer: " + std::string(e.what()));
    }

    /* Create a default material */
    MaterialPBRStandard * defaultMaterial = static_cast<MaterialPBRStandard *>(createMaterial("defaultMaterial", MaterialType::MATERIAL_PBR_STANDARD, false));
    defaultMaterial->albedo() = glm::vec4(1, 1, 1, 1);
    defaultMaterial->metallic() = 0.5;
    defaultMaterial->roughness() = 0.5;
    defaultMaterial->ao() = 1.0f;
    defaultMaterial->emissive() = 0.0f;

    /* Import some models */
    auto uvsphereMeshModel = createVulkanMeshModel("assets/models/uvsphere.obj");
    auto planeMeshModel = createVulkanMeshModel("assets/models/plane.obj");
    auto cubeMeshModel = createVulkanMeshModel("assets/models/cube.obj");

    /* Create a skybox material */
    {
        EnvironmentMap* envMap = createEnvironmentMap("assets/HDR/harbor.hdr");
        VulkanMaterialSkybox* skybox = new VulkanMaterialSkybox("skybox", envMap, m_device, m_rendererSkybox.getDescriptorSetLayout());
        AssetManager<std::string, MaterialSkybox*>& instance = AssetManager<std::string, MaterialSkybox*>::getInstance();
        instance.add(skybox->m_name, skybox);
        m_scene->setSkybox(skybox);
    }

    // auto cube = m_scene->addSceneObject("cube", Transform());
    // cube->assign(cubeMeshModel->getMeshes()[0]);
    // cube->assign(defaultMaterial);

    // auto sphere = m_scene->addSceneObject("sphere", Transform());
    // sphere->assign(uvsphereMeshModel->getMeshes()[0]);
    // sphere->assign(defaultMaterial);

    // auto plane = m_scene->addSceneObject("plane", Transform({0, -1.0, 0}, {5, 5, 5}));
    // plane->assign(planeMeshModel->getMeshes()[0]);
    // plane->assign(defaultMaterial);
}

void VulkanRenderer::initSwapChainResources()
{
    QSize size = m_window->swapChainImageSize();
    m_swapchainExtent.width = static_cast<uint32_t>(size.width());
    m_swapchainExtent.height = static_cast<uint32_t>(size.height());

    m_swapchainFormat = m_window->colorFormat();

    createRenderPasses();
    createFrameBuffers();
    createColorSelectionTempImage();
    createUniformBuffers();
    createDescriptorPool(100);
    createDescriptorSets();
    m_rendererSkybox.initSwapChainResources(m_swapchainExtent, 
        m_renderPassForward, 
        m_msaaSamples
    );
    m_rendererPBR.initSwapChainResources(m_swapchainExtent, 
        m_renderPassForward, 
        m_window->swapChainImageCount(), 
        m_msaaSamples
    );
    m_rendererLambert.initSwapChainResources(m_swapchainExtent, 
        m_renderPassForward, 
        m_window->swapChainImageCount(), 
        m_msaaSamples
    );
    m_rendererPost.initSwapChainResources(m_swapchainExtent, 
        m_renderPassPost, 
        m_window->swapChainImageCount(), 
        m_msaaSamples, 
        m_attachmentColorForwardOutput, 
        m_attachmentHighlightForwardOutput
    );
    m_renderer3DUI.initSwapChainResources(m_swapchainExtent, 
        m_renderPassUI, 
        m_window->swapChainImageCount(), 
        m_msaaSamples
    );
    if (isRTEnabled()) m_rendererRayTracing.initSwapChainResources(m_swapchainExtent);

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
    /* Wait for all commands submitted to the queue to have finished running */
    m_devFunctions->vkQueueWaitIdle(m_window->graphicsQueue());

    /* Destroy uniform buffers */
    for (int i = 0; i < m_window->swapChainImageCount(); i++) {
        m_devFunctions->vkDestroyBuffer(m_device, m_scene->m_uniformBuffersScene[i], nullptr);
        m_devFunctions->vkFreeMemory(m_device, m_scene->m_uniformBuffersSceneMemory[i], nullptr);

        m_devFunctions->vkDestroyBuffer(m_device, m_scene->m_modelDataDynamicUBO.getBuffer(i), nullptr);
        m_devFunctions->vkFreeMemory(m_device, m_scene->m_modelDataDynamicUBO.getBufferMemory(i), nullptr);

        m_devFunctions->vkDestroyBuffer(m_device, m_materialsUBO.getBuffer(i), nullptr);
        m_devFunctions->vkFreeMemory(m_device, m_materialsUBO.getBufferMemory(i), nullptr);

        m_attachmentColorForwardOutput[i].destroy(m_device);
        m_attachmentHighlightForwardOutput[i].destroy(m_device);
    }

    m_devFunctions->vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

    for (auto framebuffer : m_framebuffersForward) {
        m_devFunctions->vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }
    for (auto framebuffer : m_framebuffersPost) {
        m_devFunctions->vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }
    for (auto framebuffer : m_framebuffersUI) {
        m_devFunctions->vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }

    m_rendererSkybox.releaseSwapChainResources();
    m_rendererPBR.releaseSwapChainResources();
    m_rendererLambert.releaseSwapChainResources();
    m_rendererPost.releaseSwapChainResources();
    m_renderer3DUI.releaseSwapChainResources();
    if (isRTEnabled()) m_rendererRayTracing.releaseSwapChainResources();

    m_devFunctions->vkDestroyRenderPass(m_device, m_renderPassForward, nullptr);
    m_devFunctions->vkDestroyRenderPass(m_device, m_renderPassPost, nullptr);
    m_devFunctions->vkDestroyRenderPass(m_device, m_renderPassUI, nullptr);

    m_devFunctions->vkDestroyImage(m_device, m_imageTempColorSelection.image, nullptr);
    m_devFunctions->vkFreeMemory(m_device, m_imageTempColorSelection.memory, nullptr);
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
            vkTexture->destroy(m_device);
        }
        instance.reset();
    }

    /* Destroy material data */
    {
        AssetManager<std::string, Material*>& instance = AssetManager<std::string, Material*>::getInstance();
        instance.reset();
    }

    /* Destroy cubemaps */
    {
        AssetManager<std::string, Cubemap *>& instance = AssetManager<std::string, Cubemap *>::getInstance();
        for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
            VulkanCubemap * vkCubemap = static_cast<VulkanCubemap *>(itr->second);
            vkCubemap->destroy(m_device);
        }
    }

    /* Destroy descriptor sets layouts */
    m_devFunctions->vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayoutScene, nullptr);
    m_devFunctions->vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayoutModel, nullptr);
    m_rendererSkybox.releaseResources();
    m_rendererPBR.releaseResources();
    m_rendererLambert.releaseResources();
    m_rendererPost.releaseResources();
    m_renderer3DUI.releaseResources();
    if (isRTEnabled()) m_rendererRayTracing.releaseResources();

    /* Destroy imported models */
    {
        AssetManager<std::string, MeshModel *>& instance = AssetManager<std::string, MeshModel *>::getInstance();
        for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
            VulkanMeshModel* vkmeshmodel = static_cast<VulkanMeshModel*>(itr->second);
            vkmeshmodel->destroy(m_device);
        }
        instance.reset();
    }

    destroyDebugCallback();
}

void VulkanRenderer::startNextFrame()
{
    buildFrame();

    m_window->frameReady();
    m_window->requestUpdate();
}

void VulkanRenderer::buildFrame()
{   
    /* calculate delat time */
    auto currentTime = std::chrono::steady_clock::now();
    m_deltaTime = (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_frameTimePrev).count()) / 1000.0f;
    m_frameTimePrev = currentTime;

    /* TODO(optimization) check if anything has changed to not traverse the entire tree */
    m_scene->updateSceneGraph();
    /* TODO(optimization) check if anything has changed to not traverse the entire tree */
    std::vector<std::shared_ptr<SceneObject>> sceneObjects = m_scene->getSceneObjects();

    int imageIndex = m_window->currentSwapChainImageIndex();

    /* Update GPU buffers */
    updateUniformBuffers(imageIndex);

    VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();

    /* Forward pass */
    {
        glm::vec3 clearColor = m_scene->getBackgroundColor();
        std::array<VkClearValue, 5> clearValues{};
        VkClearColorValue cl = { { clearColor.r, clearColor.g, clearColor.b, 1.0F } };
        clearValues[0].color = cl;
        clearValues[1].color = cl;
        clearValues[2].color = { 0, 0, 0, 0 };
        clearValues[3].color = { 0, 0, 0, 0 };
        clearValues[4].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo rpBeginInfo;
        memset(&rpBeginInfo, 0, sizeof(rpBeginInfo));
        rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBeginInfo.renderPass = m_renderPassForward;
        rpBeginInfo.framebuffer = m_framebuffersForward[imageIndex];
        rpBeginInfo.renderArea.offset = { 0, 0 };
        rpBeginInfo.renderArea.extent.width = m_swapchainExtent.width;
        rpBeginInfo.renderArea.extent.height = m_swapchainExtent.height;
        rpBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;
        rpBeginInfo.pClearValues = clearValues.data();
        m_devFunctions->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        /* Get skybox material and check if its paremters have changed */
        VulkanMaterialSkybox* skybox = dynamic_cast<VulkanMaterialSkybox*>(m_scene->getSkybox());
        assert(skybox != nullptr);
        /* If material parameters have changed, update descriptor */
        if (skybox->needsUpdate(imageIndex))
        {
            skybox->updateDescriptorSet(m_device, imageIndex);
        }

        /* Draw skybox if needed */
        if (m_scene->getEnvironmentType() == EnvironmentType::HDRI) {
            m_rendererSkybox.renderSkybox(cmdBuf, m_descriptorSetsScene[imageIndex], imageIndex, skybox);
        }

        /* Batch objects into materials */
        std::vector<std::shared_ptr<SceneObject>> pbrStandardObjects;
        std::vector<std::shared_ptr<SceneObject>> lambertObjects;
        std::vector<std::shared_ptr<SceneObject>> pointLights;
        for (auto& itr : sceneObjects)
        {
            if (itr->has(ComponentType::MATERIAL)) 
            {
                switch (itr->get<Material*>(ComponentType::MATERIAL)->getType())
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

            if (itr->has(ComponentType::POINT_LIGHT)){
                pointLights.push_back(itr);
            }

        }

        /* Draw PBR material objects, base pass */
        m_rendererPBR.renderObjectsBasePass(cmdBuf,
            m_descriptorSetsScene[imageIndex],
            m_descriptorSetsModel[imageIndex],
            skybox,
            imageIndex,
            m_scene->m_modelDataDynamicUBO,
            pbrStandardObjects);
        /* Draw lambert material objects, base pass */
        m_rendererLambert.renderObjectsBasePass(cmdBuf,
            m_descriptorSetsScene[imageIndex],
            m_descriptorSetsModel[imageIndex],
            skybox,
            imageIndex,
            m_scene->m_modelDataDynamicUBO,
            lambertObjects);
        /* Draw additive pass for all point lights in the scene */
        for (auto& pl : pointLights)
        {
            PointLight* light = pl->get<PointLight*>(ComponentType::POINT_LIGHT);
            PushBlockForwardAddPass t;
            t.lightColor = light->intensity * glm::vec4(light->color, 0);
            t.lightPosition = glm::vec4(pl->getWorldPosition(), 0);
            for (auto& obj : pbrStandardObjects)
            {
                /* Check if light contributes enough */
                m_rendererPBR.renderObjectsAddPass(cmdBuf,
                    m_descriptorSetsScene[imageIndex],
                    m_descriptorSetsModel[imageIndex],
                    imageIndex,
                    m_scene->m_modelDataDynamicUBO,
                    obj,
                    t
                );
            }
            for (auto& obj : lambertObjects)
            {
                m_rendererLambert.renderObjectsAddPass(cmdBuf,
                    m_descriptorSetsScene[imageIndex],
                    m_descriptorSetsModel[imageIndex],
                    imageIndex,
                    m_scene->m_modelDataDynamicUBO,
                    obj,
                    t
                );
            }
        }

        m_devFunctions->vkCmdEndRenderPass(cmdBuf);
    }

    /* Post process pass, highlight */
    {
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { 0, 0, 0, 0 };
        clearValues[1].color = { 0, 0, 0, 0 };

        VkRenderPassBeginInfo rpBeginInfo;
        memset(&rpBeginInfo, 0, sizeof(rpBeginInfo));
        rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBeginInfo.renderPass = m_renderPassPost;
        rpBeginInfo.framebuffer = m_framebuffersPost[imageIndex];
        rpBeginInfo.renderArea.offset = { 0, 0 };
        rpBeginInfo.renderArea.extent.width = m_swapchainExtent.width;
        rpBeginInfo.renderArea.extent.height = m_swapchainExtent.height;
        rpBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;
        rpBeginInfo.pClearValues = clearValues.data();
        
        m_devFunctions->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        m_rendererPost.render(cmdBuf, imageIndex);
        m_devFunctions->vkCmdEndRenderPass(cmdBuf);
    }

    /* UI pass */
    {
        /* Render a transform if an object is selected */
        if (m_selectedObject != nullptr) {
            VkRenderPassBeginInfo rpBeginInfo;
            memset(&rpBeginInfo, 0, sizeof(rpBeginInfo));
            rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rpBeginInfo.renderPass = m_renderPassUI;
            rpBeginInfo.framebuffer = m_framebuffersUI[imageIndex];
            rpBeginInfo.renderArea.offset = { 0, 0 };
            rpBeginInfo.renderArea.extent.width = m_swapchainExtent.width;
            rpBeginInfo.renderArea.extent.height = m_swapchainExtent.height;
            rpBeginInfo.clearValueCount = 0;
            rpBeginInfo.pClearValues = nullptr;
            m_devFunctions->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        
            glm::vec3 transformPosition = m_selectedObject->getWorldPosition();
            m_renderer3DUI.renderTransform(cmdBuf,
                m_descriptorSetsScene[imageIndex],
                imageIndex,
                m_selectedObject->m_modelMatrix,
                m_scene->getCamera());
                
            m_devFunctions->vkCmdEndRenderPass(cmdBuf);
        }
    }

    /* Copy highlight output to temp color selection iamge */
    {
        uint32_t width = m_swapchainExtent.width, height = m_swapchainExtent.height;
        VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        /* Transition highlight render output to transfer source optimal */
        transitionImageLayout(cmdBuf, 
            m_attachmentHighlightForwardOutput[imageIndex].getImageResolve(), 
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
            subresourceRange);

        /* Copy highlight image to temp image */
        VkImageCopy copyRegion{};
        copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copyRegion.srcOffset = { 0, 0, 0 };
        copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copyRegion.dstOffset = { 0, 0, 0 };
        copyRegion.extent = { width, height, 1 };
        m_devFunctions->vkCmdCopyImage(cmdBuf, 
            m_attachmentHighlightForwardOutput[imageIndex].getImageResolve(), 
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
            m_imageTempColorSelection.image, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            1, 
            &copyRegion);
    }
}


MeshModel * VulkanRenderer::createVulkanMeshModel(std::string filename)
{
    AssetManager<std::string, MeshModel *>& instance = AssetManager<std::string, MeshModel *>::getInstance();

    if (instance.isPresent(filename)) return instance.get(filename);
    
    try {
        std::vector<Mesh> meshes = assimpLoadModel(filename);
        VulkanMeshModel * vkmesh = new VulkanMeshModel(m_physicalDevice, m_device, m_window->graphicsQueue(), m_window->graphicsCommandPool(), meshes);
        vkmesh->setName(filename);
        instance.add(filename, vkmesh);
        return vkmesh;
    } catch (std::runtime_error& e) {
        utils::Console(utils::DebugLevel::WARNING, "Failed to create a Vulkan Mesh Model: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
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
        temp = m_rendererPBR.createMaterial(name, glm::vec4(1, 1, 1, 1), 0, 1, 1, 0, m_materialsUBO);
        break;
    }
    case MaterialType::MATERIAL_LAMBERT:
    {
        temp = m_rendererLambert.createMaterial(name, glm::vec4(1, 1, 1, 1), 1, 0, m_materialsUBO);
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

    instance.add(temp->m_name, temp);
    return temp;
}

void VulkanRenderer::setSelectedObject(std::shared_ptr<SceneObject> sceneObject)
{
    m_selectedObject = sceneObject;
}

std::shared_ptr<SceneObject> VulkanRenderer::getSelectedObject() const
{
    return m_selectedObject;
}

bool VulkanRenderer::isRTEnabled() const
{
    return m_rendererRayTracing.isInitialized();
}

int64_t VulkanRenderer::renderRT()
{
    if (m_rendererRayTracing.isInitialized()) {
        utils::Timer timer;
        timer.Start();
        
        m_scene->updateSceneGraph();
        m_rendererRayTracing.renderScene(m_scene);

        timer.Stop();
        return timer.ToInt();
    }
    else {
        return 0;
    }
}

glm::vec3 VulkanRenderer::selectObject(float x, float y)
{
    /* Get the texel color at this row and column of the highlight render target */
    uint32_t row = y * m_swapchainExtent.height;
    uint32_t column = x * m_swapchainExtent.width;

    /* Since the image is stored in linear tiling, get the subresource layout to calcualte the padding */
    VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
    VkSubresourceLayout subResourceLayout;
    m_devFunctions->vkGetImageSubresourceLayout(m_device, m_imageTempColorSelection.image, &subResource, &subResourceLayout);

    /* Byte index of x,y texel */
    uint32_t index = (row * subResourceLayout.rowPitch + column * 4 * sizeof(float));

    /* Store highlight render result for that texel to the disk */
    float* highlight;
    VkResult res = m_devFunctions->vkMapMemory(
        m_device,
        m_imageTempColorSelection.memory,
        subResourceLayout.offset + index,
        3 * sizeof(float),
        0,
        reinterpret_cast<void**>(&highlight)
    );
    glm::vec3 highlightTexelColor;
    memcpy(&highlightTexelColor[0], highlight, 3 * sizeof(float));
    m_devFunctions->vkUnmapMemory(m_device, m_imageTempColorSelection.memory);

    return highlightTexelColor;
}

float VulkanRenderer::deltaTime() const
{
    return m_deltaTime;
}

Texture * VulkanRenderer::createTexture(std::string imagePath, VkFormat format, bool keepImage)
{
    try {
        AssetManager<std::string, Image<stbi_uc> *>& instance = AssetManager<std::string, Image<stbi_uc> *>::getInstance();
        
        Image<stbi_uc> * image = nullptr;
        if (instance.isPresent(imagePath)) {
            image = instance.get(imagePath);
        }
        else {
            image = new Image<stbi_uc>(imagePath);
            instance.add(imagePath, image);
        }

        Texture * temp = createTexture(imagePath, image, format);

        if (!keepImage) {
            instance.remove(imagePath);
            delete image;
        }

        return temp;
    } catch (std::runtime_error& e) {
        utils::ConsoleWarning("Can't create a vulkan texture: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
}

Texture * VulkanRenderer::createTexture(std::string id, Image<stbi_uc> * image, VkFormat format)
{
    try {
        AssetManager<std::string, Texture *>& instance = AssetManager<std::string, Texture *>::getInstance();
        
        if (instance.isPresent(id)) {
            return instance.get(id);
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

        instance.add(id, temp);
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
        /* Check if an hdr texture has already been created for that image */
        AssetManager<std::string, Texture *>& instance = AssetManager<std::string, Texture *>::getInstance();
        if (instance.isPresent(imagePath)) {
            return instance.get(imagePath);
        }
        
        /* Check if the image has already been imported, if not read it from disk  */
        Image<float> * image = nullptr;
        AssetManager<std::string, Image<float> *>& images = AssetManager<std::string, Image<float> *>::getInstance();
        if (images.isPresent(imagePath)) {
            image = images.get(imagePath);
        }
        else {
            image = new Image<float>(imagePath);
            images.add(imagePath, image);
        }

        /* Create texture for that image */
        VulkanTexture * temp = new VulkanTexture(imagePath, 
            image, 
            TextureType::HDR, 
            m_physicalDevice, 
            m_device, 
            m_window->graphicsQueue(), 
            m_window->graphicsCommandPool(),
            false);
        instance.add(imagePath, temp);

        /* If you are not to keep the image in memory, remove from the assets, and delete it */
        if (!keepImage) {
            images.remove(imagePath);
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
        return instance.get(directory);
    }

    VulkanCubemap * cubemap = new VulkanCubemap(directory, m_physicalDevice, m_device, m_window->graphicsQueue(), m_window->graphicsCommandPool());
    instance.add(directory, cubemap);
    return cubemap;
}

EnvironmentMap* VulkanRenderer::createEnvironmentMap(std::string imagePath, bool keepTexture)
{
    try {
        /* Check if an environment map for that imagePath already exists */
        AssetManager<std::string, EnvironmentMap*>& envMaps = AssetManager<std::string, EnvironmentMap*>::getInstance();
        if (envMaps.isPresent(imagePath)) {
            return envMaps.get(imagePath);
        }

        /* Read HDR image */
        VulkanTexture* hdrImage = static_cast<VulkanTexture*>(createTextureHDR(imagePath));

        /* Transform input texture into a cubemap */
        VulkanCubemap* cubemap = m_rendererSkybox.createCubemap(hdrImage);
        /* Compute irradiance map */
        VulkanCubemap* irradiance = m_rendererSkybox.createIrradianceMap(cubemap);
        /* Compute prefiltered map */
        VulkanCubemap* prefiltered = m_rendererSkybox.createPrefilteredCubemap(cubemap);

        /* Added created cubemaps to the cubemap assets */
        AssetManager<std::string, Cubemap*>& instance = AssetManager<std::string, Cubemap*>::getInstance();
        instance.add(cubemap->m_name, cubemap);
        instance.add(irradiance->m_name, irradiance);
        instance.add(prefiltered->m_name, prefiltered);

        EnvironmentMap* envMap = new EnvironmentMap(imagePath, cubemap, irradiance, prefiltered);
        envMaps.add(imagePath, envMap);

        if (!keepTexture) {
            AssetManager<std::string, Texture*>& textures = AssetManager<std::string, Texture*>::getInstance();
            textures.remove(imagePath);
            hdrImage->destroy(m_device);
        }

        return envMap;
    }
    catch (std::runtime_error& e) {
        utils::ConsoleCritical("Failed to create a vulkan environment map: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
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
        utils::ConsoleWarning("Failed to setup the debug callback");
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
            m_msaaSamples = getMaxUsableSampleCount(deviceProperties[i]);
            utils::ConsoleInfo("Using device: " + std::string(deviceProperties[i].deviceName));
            return true;
        }
    }

    utils::ConsoleFatal("Failed to find a proper physical device");
    return false;
}

bool VulkanRenderer::isPhysicalDeviceSuitable(const VkPhysicalDeviceProperties& deviceProperties)
{
    bool suitable = true;
    suitable = suitable && getMaxUsableSampleCount(deviceProperties) != VK_SAMPLE_COUNT_1_BIT;
    return suitable;
}

VkSampleCountFlagBits VulkanRenderer::getMaxUsableSampleCount(const VkPhysicalDeviceProperties& deviceProperties)
{
    VkSampleCountFlags counts = deviceProperties.limits.framebufferColorSampleCounts & deviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

bool VulkanRenderer::createRenderPasses()
{

    /* Render pass 1: Forward pass */
    {
        /* ------------------------------ SUBPASS 1 ---------------------------------- */ 
        VkSubpassDescription renderPassForwardSubpass{};
        /* 2 color attachments, one for color output, one for highlight information, plus depth */
        
        /* Color */
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_internalRenderFormat;
        colorAttachment.samples = m_msaaSamples;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  /* Before the subpass */
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  /* After the subpass */
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;   /* During the subpass */
        /* Resolve attacment for color */
        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = m_internalRenderFormat;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  /* Before the subpass */
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  /* After the subpass */
        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 1;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;   /* During the subpass */

        /* Highlight */
        VkAttachmentDescription highlightAttachment{};
        highlightAttachment.format = m_internalRenderFormat;
        highlightAttachment.samples = m_msaaSamples;
        highlightAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        highlightAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        highlightAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        highlightAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        highlightAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  /* Before the subpass */
        highlightAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  /* After the subpass */
        VkAttachmentReference highlightAttachmentRef{};
        highlightAttachmentRef.attachment = 2;
        highlightAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;   /* During the subpass */
        VkAttachmentDescription highlightAttachmentResolve{};
        highlightAttachmentResolve.format = m_internalRenderFormat;
        highlightAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        highlightAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        highlightAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        highlightAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        highlightAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        highlightAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  /* Before the subpass */
        highlightAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  /* After the subpass */
        VkAttachmentReference highlightAttachmentResolveRef{};
        highlightAttachmentResolveRef.attachment = 3;
        highlightAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;   /* During the subpass */

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = m_window->depthStencilFormat();
        depthAttachment.samples = m_msaaSamples;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 4;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        std::array<VkAttachmentReference, 2> colorAttachments{colorAttachmentRef, highlightAttachmentRef };
        std::array<VkAttachmentReference, 2> resolveAttachments{colorAttachmentResolveRef, highlightAttachmentResolveRef };
        renderPassForwardSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        renderPassForwardSubpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
        renderPassForwardSubpass.pColorAttachments = colorAttachments.data();    /* Same as shader locations */
        renderPassForwardSubpass.pResolveAttachments = resolveAttachments.data(); 
        renderPassForwardSubpass.pDepthStencilAttachment = &depthAttachmentRef;

        /* ---------------- SUBPASS DEPENDENCIES ----------------------- */
        std::array<VkSubpassDependency, 2> forwardDependencies{};
        /* Dependency 1: Wait for the previous external pass to finish reading the color, before you start writing the new color */
        forwardDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        forwardDependencies[0].dstSubpass = 0;
        forwardDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        forwardDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        forwardDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        forwardDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        /* Dependency 2: Wait to finish subpass 0, before the external pass starts reading */
        forwardDependencies[1].srcSubpass = 0;
        forwardDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        forwardDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        forwardDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        forwardDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        forwardDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

        std::array<VkAttachmentDescription, 5> forwardAttachments = { 
            colorAttachment, 
            colorAttachmentResolve, 
            highlightAttachment, 
            highlightAttachmentResolve, 
            depthAttachment };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(forwardAttachments.size());
        renderPassInfo.pAttachments = forwardAttachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &renderPassForwardSubpass;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(forwardDependencies.size());
        renderPassInfo.pDependencies = forwardDependencies.data();

        if (m_devFunctions->vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPassForward) != VK_SUCCESS) {
            utils::ConsoleFatal("Failed to create a render pass");
            return false;
        }
    }

    {
        /* Render pass 2: post process pass */
        /* ---------------------- SUBPASS 1  ------------------------------ */
        VkSubpassDescription renderPassPostSubpass{};
        /* One color attachment which is the swapchain output */
        VkAttachmentDescription postColorAttachment{};
        postColorAttachment.format = m_swapchainFormat;
        postColorAttachment.samples = m_msaaSamples;
        postColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        postColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        postColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        postColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        postColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  /* Before the subpass */
        postColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;  /* After the subpass */
        VkAttachmentReference postColorAttachmentRef{};
        postColorAttachmentRef.attachment = 0;
        postColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;   /* During the subpass */
        VkAttachmentDescription postColorAttachmentResolve{};
        postColorAttachmentResolve.format = m_swapchainFormat;
        postColorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        postColorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        postColorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        postColorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        postColorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        postColorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  /* Before the subpass */
        postColorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  /* After the subpass */
        VkAttachmentReference postColorAttachmentResolveRef{};
        postColorAttachmentResolveRef.attachment = 1;
        postColorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;   /* During the subpass */

        /* setup subpass 1 */
        renderPassPostSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        renderPassPostSubpass.colorAttachmentCount = 1;
        renderPassPostSubpass.pColorAttachments = &postColorAttachmentRef;    /* Same as shader locations */
        renderPassPostSubpass.pResolveAttachments = &postColorAttachmentResolveRef;

        /* ---------------- SUBPASS DEPENDENCIES ----------------------- */
        std::array<VkSubpassDependency, 2> postDependencies{};
        /* Dependency 1: Wait for the previous external pass to finish reading the color, before you start writing the new color */
        postDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        postDependencies[0].dstSubpass = 0;
        postDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        postDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        postDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        postDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        /* Dependency 2: Wait to finish subpass 0, before the external pass starts reading */
        postDependencies[1].srcSubpass = 0;
        postDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        postDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        postDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        postDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        postDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

        std::array<VkAttachmentDescription, 2> postAttachments = { postColorAttachment, postColorAttachmentResolve };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(postAttachments.size());
        renderPassInfo.pAttachments = postAttachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &renderPassPostSubpass;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(postDependencies.size());
        renderPassInfo.pDependencies = postDependencies.data();

        if (m_devFunctions->vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPassPost) != VK_SUCCESS) {
            utils::ConsoleFatal("Failed to create a render pass");
            return false;
        }
    }


    {
        /* Render pass 3: UI pass */
        /* ---------------------- SUBPASS 1  ------------------------------ */
        VkSubpassDescription renderPassUISubpass{};
        /* One color attachment which is the swapchain output, and one with the highlight information */
        VkAttachmentDescription swapchainColorAttachment{};
        swapchainColorAttachment.format = m_swapchainFormat;
        swapchainColorAttachment.samples = m_msaaSamples;
        swapchainColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        swapchainColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        swapchainColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        swapchainColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        swapchainColorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;  /* Before the subpass */
        swapchainColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;  /* After the subpass */
        VkAttachmentReference swapchainColorAttachmentRef{};
        swapchainColorAttachmentRef.attachment = 0;
        swapchainColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;   /* During the subpass */
        VkAttachmentDescription swapchainColorAttachmentResolve{};
        swapchainColorAttachmentResolve.format = m_swapchainFormat;
        swapchainColorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        swapchainColorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        swapchainColorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        swapchainColorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        swapchainColorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        swapchainColorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  /* Before the subpass */
        swapchainColorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  /* After the subpass */
        VkAttachmentReference swapchainColorAttachmentResolveRef{};
        swapchainColorAttachmentResolveRef.attachment = 1;
        swapchainColorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;   /* During the subpass */

        VkAttachmentDescription highlightAttachment{};
        highlightAttachment.format = m_internalRenderFormat;
        highlightAttachment.samples = m_msaaSamples;
        highlightAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        highlightAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        highlightAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        highlightAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        highlightAttachment.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  /* Before the subpass */
        highlightAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  /* After the subpass */
        VkAttachmentReference highlightAttachmentRef{};
        highlightAttachmentRef.attachment = 2;
        highlightAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;   /* During the subpass */
        VkAttachmentDescription highlightAttachmentResolve{};
        highlightAttachmentResolve.format = m_internalRenderFormat;
        highlightAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        highlightAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        highlightAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        highlightAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        highlightAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        highlightAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  /* Before the subpass */
        highlightAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  /* After the subpass */
        VkAttachmentReference highlightAttachmentResolveRef{};
        highlightAttachmentResolveRef.attachment = 3;
        highlightAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;   /* During the subpass */

        /* setup subpass 1 */
        std::array<VkAttachmentReference, 2> colorAttachments{ swapchainColorAttachmentRef, highlightAttachmentRef };
        std::array<VkAttachmentReference, 2> resolveAttachments{ swapchainColorAttachmentResolveRef, highlightAttachmentResolveRef };
        renderPassUISubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        renderPassUISubpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
        renderPassUISubpass.pColorAttachments = colorAttachments.data();    /* Same as shader locations */
        renderPassUISubpass.pResolveAttachments = resolveAttachments.data();

        /* ---------------- SUBPASS DEPENDENCIES ----------------------- */
        std::array<VkSubpassDependency, 2> uiDependencies{};
        /* Dependency 1: Wait for the previous external pass to finish reading the color, before you start writing the new color */
        uiDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        uiDependencies[0].dstSubpass = 0;
        uiDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        uiDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        uiDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        uiDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        /* Dependency 2: Wait to finish subpass 0, before the external pass starts reading */
        uiDependencies[1].srcSubpass = 0;
        uiDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        uiDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        uiDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        uiDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        uiDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

        std::array<VkAttachmentDescription, 4> uiAttachments = { swapchainColorAttachment, swapchainColorAttachmentResolve, highlightAttachment, highlightAttachmentResolve };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(uiAttachments.size());
        renderPassInfo.pAttachments = uiAttachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &renderPassUISubpass;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(uiDependencies.size());
        renderPassInfo.pDependencies = uiDependencies.data();

        if (m_devFunctions->vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPassUI) != VK_SUCCESS) {
            utils::ConsoleFatal("Failed to create a render pass");
            return false;
        }
    }

    return true;
}

bool VulkanRenderer::createFrameBuffers()
{
    uint32_t swapchainImages = m_window->swapChainImageCount();
    /* Create internal render targets for color and highlight */
    m_attachmentColorForwardOutput.resize(swapchainImages);
    m_attachmentHighlightForwardOutput.resize(swapchainImages);

    for (size_t i = 0; i < swapchainImages; i++)
    {
        m_attachmentColorForwardOutput[i].init(m_physicalDevice, 
            m_device, 
            m_swapchainExtent.width, 
            m_swapchainExtent.height, 
            m_internalRenderFormat, 
            m_msaaSamples, 
            true
        );
        m_attachmentHighlightForwardOutput[i].init(m_physicalDevice, 
            m_device, 
            m_swapchainExtent.width, 
            m_swapchainExtent.height, 
            m_internalRenderFormat, 
            m_msaaSamples, 
            true, 
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT
        );
    }

    /* Create framebuffers for forward and post process pass */
    m_framebuffersForward.resize(swapchainImages);
    m_framebuffersPost.resize(swapchainImages);
    m_framebuffersUI.resize(swapchainImages);

    for (size_t i = 0; i < swapchainImages; i++) {
        {
            /* Create forward pass framebuffers */
            std::array<VkImageView, 5> attachments = {
                m_attachmentColorForwardOutput[i].getView(),
                m_attachmentColorForwardOutput[i].getViewResolve(),
                m_attachmentHighlightForwardOutput[i].getView(),
                m_attachmentHighlightForwardOutput[i].getViewResolve(),
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

            if (m_devFunctions->vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_framebuffersForward[i]) != VK_SUCCESS) {
                utils::ConsoleFatal("Failed to create a framebuffer");
                return false;
            }
        }

        {
            /* Create post process pass framebuffers */
            std::array<VkImageView, 2> attachments = {
                m_window->msaaColorImageView(static_cast<int>(i)),
                m_window->swapChainImageView(static_cast<int>(i))
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPassPost;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = m_swapchainExtent.width;
            framebufferInfo.height = m_swapchainExtent.height;
            framebufferInfo.layers = 1;

            if (m_devFunctions->vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_framebuffersPost[i]) != VK_SUCCESS) {
                utils::ConsoleFatal("Failed to create a framebuffer");
                return false;
            }
        }

        {
            /* Create UI pass framebuffers */
            std::array<VkImageView, 4> attachments = {
                m_window->msaaColorImageView(static_cast<int>(i)),
                m_window->swapChainImageView(static_cast<int>(i)),
                m_attachmentHighlightForwardOutput[i].getView(),
                m_attachmentHighlightForwardOutput[i].getViewResolve()
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPassUI;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = m_swapchainExtent.width;
            framebufferInfo.height = m_swapchainExtent.height;
            framebufferInfo.layers = 1;

            if (m_devFunctions->vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_framebuffersUI[i]) != VK_SUCCESS) {
                utils::ConsoleFatal("Failed to create a framebuffer");
                return false;
            }
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
    m_scene->updateBuffers(m_device, static_cast<uint32_t>(index));
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

bool VulkanRenderer::createColorSelectionTempImage()
{
    /* Create temp image used to copy render result from gpu memory to cpu memory */
    bool ret = createImage(m_physicalDevice,
        m_device,
        m_swapchainExtent.width,
        m_swapchainExtent.height,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        m_internalRenderFormat,
        VK_IMAGE_TILING_LINEAR,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_imageTempColorSelection.image,
        m_imageTempColorSelection.memory);

    /* Transition image to the approriate layout ready for render */
    VkCommandBuffer cmdBuf = beginSingleTimeCommands(m_device, m_window->graphicsCommandPool());

    transitionImageLayout(cmdBuf,
        m_imageTempColorSelection.image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

    endSingleTimeCommands(m_device, m_window->graphicsCommandPool(), m_window->graphicsQueue(), cmdBuf);

    return true;
}


