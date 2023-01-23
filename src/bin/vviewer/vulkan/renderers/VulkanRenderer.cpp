#include "VulkanRenderer.hpp"
#include "vulkan/VulkanWindow.hpp"

#include <chrono>
#include <set>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stb_image_write.h>

#include <utils/Console.hpp>
#include "utils/Timer.hpp"

#include "core/Image.hpp"
#include "core/EnvironmentMap.hpp"
#include "models/AssimpLoadModel.hpp"
#include "vulkan/Shader.hpp"
#include "vulkan/VulkanUtils.hpp"

VulkanRenderer::VulkanRenderer(VulkanCore& vkcore, VulkanScene* scene) : m_vkcore(vkcore), m_scene(scene), m_rendererRayTracing(m_vkcore)
{
    m_materialsUBO = std::make_shared<VulkanDynamicUBO<MaterialData>>(100);
}

void VulkanRenderer::initResources()
{
    utils::ConsoleInfo("Initializing resources...");
        
    /* Initialize dynamic uniform buffers for model matrix data */
    m_scene->m_modelDataDynamicUBO.init(m_vkcore.physicalDeviceProperties().limits.minUniformBufferOffsetAlignment);
    /* Initialize dynamic uniform buffers for material data */
    m_materialsUBO->init(m_vkcore.physicalDeviceProperties().limits.minUniformBufferOffsetAlignment);

    /* Create descriptor set layouts for scene and model data */
    createDescriptorSetsLayouts();

    createCommandBuffers();
    createSyncObjects();

    /* Initialize some data */
    auto whiteTexture = createTexture("white", std::make_shared<Image<stbi_uc>>(Image<stbi_uc>::Color::WHITE), VK_FORMAT_R8G8B8A8_UNORM);
    auto whiteColor = createTexture("whiteColor", std::make_shared<Image<stbi_uc>>(Image<stbi_uc>::Color::WHITE), VK_FORMAT_R8G8B8A8_SRGB);
    createTexture("normalmapdefault", std::make_shared<Image<stbi_uc>>(Image<stbi_uc>::Color::NORMAL_MAP), VK_FORMAT_R8G8B8A8_UNORM);

    /* Initialize renderers */
    m_rendererSkybox.initResources(m_vkcore.physicalDevice(), 
        m_vkcore.device(), 
        m_vkcore.graphicsQueue(), 
        m_vkcore.graphicsCommandPool(), 
        m_descriptorSetLayoutScene);
    m_rendererPBR.initResources(m_vkcore.physicalDevice(), 
        m_vkcore.device(), 
        m_vkcore.graphicsQueue(), 
        m_vkcore.graphicsCommandPool(), 
        m_vkcore.physicalDeviceProperties(), 
        m_descriptorSetLayoutScene, 
        m_descriptorSetLayoutModel, 
        m_rendererSkybox.getDescriptorSetLayout());
    m_rendererLambert.initResources(m_vkcore.physicalDevice(),
        m_vkcore.device(),
        m_vkcore.graphicsQueue(),
        m_vkcore.graphicsCommandPool(),
        m_vkcore.physicalDeviceProperties(),
        m_descriptorSetLayoutScene,
        m_descriptorSetLayoutModel,
        m_rendererSkybox.getDescriptorSetLayout());
    m_rendererPost.initResources(m_vkcore.physicalDevice(), 
        m_vkcore.device(), 
        m_vkcore.graphicsQueue(), 
        m_vkcore.graphicsCommandPool());

    try {
        m_renderer3DUI.initResources(m_vkcore.physicalDevice(), 
            m_vkcore.device(), 
            m_vkcore.graphicsQueue(), 
            m_vkcore.graphicsCommandPool(), 
            m_descriptorSetLayoutScene);
    }
    catch (std::exception& e) {
        utils::ConsoleCritical("VulkanRenderer::initResources(): Failed to initialize UI renderer: " + std::string(e.what()));
        return;
    }
    
    try {
        m_rendererRayTracing.initResources(VK_FORMAT_R32G32B32A32_SFLOAT);
    }
    catch (std::exception& e) {
        utils::ConsoleWarning("VulkanRenderer::initResources():Failed to initialize GPU ray tracing renderer: " + std::string(e.what()));
    }

    /* Create a default material */
    auto defaultMaterial = std::static_pointer_cast<VulkanMaterialPBRStandard>(createMaterial("defaultMaterial", MaterialType::MATERIAL_PBR_STANDARD, false));
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
        auto envMap = createEnvironmentMap("assets/HDR/harbor.hdr");

        AssetManager<std::string, MaterialSkybox>& instance = AssetManager<std::string, MaterialSkybox>::getInstance();
        auto skybox = instance.add("skybox", std::make_shared<VulkanMaterialSkybox>("skybox", envMap, m_vkcore.device(), m_rendererSkybox.getDescriptorSetLayout()));
        
        m_scene->setSkybox(skybox);
    }

    startRenderLoop();
}

void VulkanRenderer::initSwapChainResources(VulkanSwapchain * swapchain)
{
    m_swapchain = swapchain;

    VkExtent2D swapchainExtent = m_swapchain->extent();
    VkFormat swapchainFormat = m_swapchain->format();

    createRenderPasses();
    createFrameBuffers();
    createColorSelectionTempImage();
    createUniformBuffers();
    createDescriptorPool(100);
    createDescriptorSets();
    m_rendererSkybox.initSwapChainResources(swapchainExtent, 
        m_renderPassForward, 
        m_vkcore.msaaSamples()
    );
    m_rendererPBR.initSwapChainResources(swapchainExtent, 
        m_renderPassForward, 
        m_swapchain->imageCount(), 
        m_vkcore.msaaSamples()
    );
    m_rendererLambert.initSwapChainResources(swapchainExtent, 
        m_renderPassForward, 
        m_swapchain->imageCount(), 
        m_vkcore.msaaSamples()
    );
    m_rendererPost.initSwapChainResources(swapchainExtent, 
        m_renderPassPost, 
        m_swapchain->imageCount(), 
        m_vkcore.msaaSamples(), 
        m_attachmentColorForwardOutput, 
        m_attachmentHighlightForwardOutput
    );
    m_renderer3DUI.initSwapChainResources(swapchainExtent, 
        m_renderPassUI, 
        m_swapchain->imageCount(), 
        m_vkcore.msaaSamples()
    );

    /* Update descriptor sets */
    {
        AssetManager<std::string, Material>& instance = AssetManager<std::string, Material>::getInstance();
        for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
            auto material = std::dynamic_pointer_cast<VulkanMaterialDescriptor>(itr->second);
            material->createDescriptors(m_vkcore.device(), m_descriptorPool, m_swapchain->imageCount());
            material->updateDescriptorSets(m_vkcore.device(), m_swapchain->imageCount());
        }
    }
    {
        AssetManager<std::string, MaterialSkybox>& instance = AssetManager<std::string, MaterialSkybox>::getInstance();
        for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
            auto material = std::static_pointer_cast<VulkanMaterialSkybox>(itr->second);
            material->createDescriptors(m_vkcore.device(), m_descriptorPool, m_swapchain->imageCount());
            material->updateDescriptorSets(m_vkcore.device(), m_swapchain->imageCount());
        }
    }
}

void VulkanRenderer::releaseSwapChainResources()
{
    /* Destroy uniform buffers */
    for (int i = 0; i < m_swapchain->imageCount(); i++) {
        m_vkcore.deviceFunctions()->vkDestroyBuffer(m_vkcore.device(), m_scene->m_uniformBuffersScene[i], nullptr);
        m_vkcore.deviceFunctions()->vkFreeMemory(m_vkcore.device(), m_scene->m_uniformBuffersSceneMemory[i], nullptr);

        m_vkcore.deviceFunctions()->vkDestroyBuffer(m_vkcore.device(), m_scene->m_modelDataDynamicUBO.getBuffer(i), nullptr);
        m_vkcore.deviceFunctions()->vkFreeMemory(m_vkcore.device(), m_scene->m_modelDataDynamicUBO.getBufferMemory(i), nullptr);

        m_vkcore.deviceFunctions()->vkDestroyBuffer(m_vkcore.device(), m_materialsUBO->getBuffer(i), nullptr);
        m_vkcore.deviceFunctions()->vkFreeMemory(m_vkcore.device(), m_materialsUBO->getBufferMemory(i), nullptr);

        m_attachmentColorForwardOutput[i].destroy(m_vkcore.device());
        m_attachmentHighlightForwardOutput[i].destroy(m_vkcore.device());
    }

    m_vkcore.deviceFunctions()->vkDestroyDescriptorPool(m_vkcore.device(), m_descriptorPool, nullptr);

    for (auto framebuffer : m_framebuffersForward) {
        m_vkcore.deviceFunctions()->vkDestroyFramebuffer(m_vkcore.device(), framebuffer, nullptr);
    }
    for (auto framebuffer : m_framebuffersPost) {
        m_vkcore.deviceFunctions()->vkDestroyFramebuffer(m_vkcore.device(), framebuffer, nullptr);
    }
    for (auto framebuffer : m_framebuffersUI) {
        m_vkcore.deviceFunctions()->vkDestroyFramebuffer(m_vkcore.device(), framebuffer, nullptr);
    }

    m_rendererSkybox.releaseSwapChainResources();
    m_rendererPBR.releaseSwapChainResources();
    m_rendererLambert.releaseSwapChainResources();
    m_rendererPost.releaseSwapChainResources();
    m_renderer3DUI.releaseSwapChainResources();

    m_vkcore.deviceFunctions()->vkDestroyRenderPass(m_vkcore.device(), m_renderPassForward, nullptr);
    m_vkcore.deviceFunctions()->vkDestroyRenderPass(m_vkcore.device(), m_renderPassPost, nullptr);
    m_vkcore.deviceFunctions()->vkDestroyRenderPass(m_vkcore.device(), m_renderPassUI, nullptr);

    m_vkcore.deviceFunctions()->vkDestroyImage(m_vkcore.device(), m_imageTempColorSelection.image, nullptr);
    m_vkcore.deviceFunctions()->vkFreeMemory(m_vkcore.device(), m_imageTempColorSelection.memory, nullptr);
}

void VulkanRenderer::releaseResources()
{
    for(uint32_t f = 0; f < MAX_FRAMES_IN_FLIGHT; f++)
    {
        m_vkcore.deviceFunctions()->vkDestroySemaphore(m_vkcore.device(), m_semaphoreImageAvailable[f], nullptr);
        m_vkcore.deviceFunctions()->vkDestroySemaphore(m_vkcore.device(), m_semaphoreRenderFinished[f], nullptr);
        m_vkcore.deviceFunctions()->vkDestroyFence(m_vkcore.device(), m_fenceInFlight[f], nullptr);        
    }

    m_scene->m_modelDataDynamicUBO.destroyCPUMemory();
    m_materialsUBO->destroyCPUMemory();

    /* Destroy texture data */
    {
        AssetManager<std::string, Texture>& instance = AssetManager<std::string, Texture>::getInstance();
        for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
            auto vkTexture = std::static_pointer_cast<VulkanTexture>(itr->second);
            vkTexture->destroy(m_vkcore.device());
        }
        instance.reset();
    }

    /* Destroy material data */
    {
        AssetManager<std::string, Material>& instance = AssetManager<std::string, Material>::getInstance();
        instance.reset();
    }

    /* Destroy cubemaps */
    {
        AssetManager<std::string, Cubemap>& instance = AssetManager<std::string, Cubemap>::getInstance();
        for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
            auto vkCubemap = std::static_pointer_cast<VulkanCubemap>(itr->second);
            vkCubemap->destroy(m_vkcore.device());
        }
    }

    /* Destroy descriptor sets layouts */
    m_vkcore.deviceFunctions()->vkDestroyDescriptorSetLayout(m_vkcore.device(), m_descriptorSetLayoutScene, nullptr);
    m_vkcore.deviceFunctions()->vkDestroyDescriptorSetLayout(m_vkcore.device(), m_descriptorSetLayoutModel, nullptr);
    m_rendererSkybox.releaseResources();
    m_rendererPBR.releaseResources();
    m_rendererLambert.releaseResources();
    m_rendererPost.releaseResources();
    m_renderer3DUI.releaseResources();
    if (isRTEnabled()) m_rendererRayTracing.releaseResources();

    /* Destroy imported models */
    {
        AssetManager<std::string, MeshModel>& instance = AssetManager<std::string, MeshModel>::getInstance();
        for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
            auto vkmeshmodel = std::static_pointer_cast<VulkanMeshModel>(itr->second);
            vkmeshmodel->destroy(m_vkcore.device());
        }
        instance.reset();
    }

    startRenderLoop();
}

void VulkanRenderer::waitIdle()
{
    /* Wait CPU to idle */
    for(;;) {
        if (m_status == STATUS::IDLE || m_status == STATUS::STOPPED) break;
    }

    /* Wait GPU to idle */
    m_vkcore.deviceFunctions()->vkDeviceWaitIdle(m_vkcore.device());
}

void VulkanRenderer::startRenderLoop()
{
    m_renderThread = std::thread(&VulkanRenderer::renderFrame, this);
}

void VulkanRenderer::renderLoopActive(bool active)
{
    if (m_swapchain == nullptr)
    {
        return;
    }

    m_renderLoopActive = active;
}

void VulkanRenderer::exitRenderLoop()
{
    m_renderLoopExit = true;
    m_renderThread.join();
}

void VulkanRenderer::renderFrame()
{
    for(;;)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));

        if (m_renderLoopExit) {
            break;
        }

        if (!m_renderLoopActive)
        {
            m_status = STATUS::IDLE;
            continue;
        }

        m_status = STATUS::RUNNING;
        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        /* Wait previous render operation to finish */
        VkResult result = m_vkcore.deviceFunctions()->vkWaitForFences(m_vkcore.device(), 1, &m_fenceInFlight[m_currentFrame], VK_TRUE, UINT64_MAX);

        /* Get next available image */
        uint32_t imageIndex;
        result = vkAcquireNextImageKHR(m_vkcore.device(), m_swapchain->swapchain(), UINT64_MAX, m_semaphoreImageAvailable[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
        if (result != VK_SUCCESS) {
            continue;
        }

        m_vkcore.deviceFunctions()->vkResetFences(m_vkcore.device(), 1, &m_fenceInFlight[m_currentFrame]);

        /* Record command buffer */
        m_vkcore.deviceFunctions()->vkResetCommandBuffer(m_commandBuffer[m_currentFrame], 0);
        buildFrame(imageIndex, m_commandBuffer[m_currentFrame]);

        /* Submit command buffer */
        VkSemaphore waitSemaphores[] = { m_semaphoreImageAvailable[m_currentFrame] };
        VkSemaphore signalSemaphores[] = { m_semaphoreRenderFinished[m_currentFrame] };
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_commandBuffer[m_currentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        result = m_vkcore.deviceFunctions()->vkQueueSubmit(m_vkcore.renderQueue(), 1, &submitInfo, m_fenceInFlight[m_currentFrame]);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("VulkanRenderer::renderFrame(): Failed to submit the command buffer: " + std::to_string(result));
        }

        /* Present to swapchain */
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapChains[] = { m_swapchain->swapchain() };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        result = vkQueuePresentKHR(m_vkcore.presentQueue(), &presentInfo);
        if (result != VK_SUCCESS) {
            continue;
        }
    }

    m_status = STATUS::STOPPED;
}

void VulkanRenderer::buildFrame(uint32_t imageIndex, VkCommandBuffer commandBuffer)
{   
    /* calculate delta time */
    auto currentTime = std::chrono::steady_clock::now();
    m_deltaTime = (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_frameTimePrev).count()) / 1000.0f;
    m_frameTimePrev = currentTime;

    /* TODO(optimization) check if anything has changed to not traverse the entire tree */
    m_scene->updateSceneGraph();
    /* TODO(optimization) check if anything has changed to not traverse the entire tree */
    std::vector<std::shared_ptr<SceneObject>> sceneObjects = m_scene->getSceneObjects();

    /* Update GPU buffers */
    updateUniformBuffers(imageIndex);

    /* Begin command buffer */
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional
    if (m_vkcore.deviceFunctions()->vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("VulkanRenderer::buildFrame(): Failed to begin recording the command buffer");
    }

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
        rpBeginInfo.renderArea.extent = m_swapchain->extent();
        rpBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;
        rpBeginInfo.pClearValues = clearValues.data();
        m_vkcore.deviceFunctions()->vkCmdBeginRenderPass(commandBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        /* Get skybox material and check if its paremters have changed */
        auto skybox = std::dynamic_pointer_cast<VulkanMaterialSkybox>(m_scene->getSkybox());
        assert(skybox != nullptr);
        /* If material parameters have changed, update descriptor */
        if (skybox->needsUpdate(imageIndex))
        {
            skybox->updateDescriptorSet(m_vkcore.device(), imageIndex);
        }

        /* Draw skybox if needed */
        if (m_scene->getEnvironmentType() == EnvironmentType::HDRI) {
            m_rendererSkybox.renderSkybox(commandBuffer, m_descriptorSetsScene[imageIndex], imageIndex, skybox);
        }

        /* Batch objects into materials */
        std::vector<std::shared_ptr<SceneObject>> pbrStandardObjects;
        std::vector<std::shared_ptr<SceneObject>> lambertObjects;
        std::vector<std::shared_ptr<SceneObject>> pointLights;
        for (auto& itr : sceneObjects)
        {
            if (itr->has<ComponentMesh>() && itr->has<ComponentMaterial>()) 
            {
                switch (itr->get<ComponentMaterial>().material->getType())
                {
                case MaterialType::MATERIAL_PBR_STANDARD:
                    pbrStandardObjects.push_back(itr);
                    break;
                case MaterialType::MATERIAL_LAMBERT:
                    lambertObjects.push_back(itr);
                    break;
                default:
                    throw std::runtime_error("VulkanRenderer::buildFrame(): Unexpected material");
                }
            }

            if (itr->has<ComponentPointLight>()){
                pointLights.push_back(itr);
            }

        }

        /* Draw PBR material objects, base pass */
        m_rendererPBR.renderObjectsBasePass(commandBuffer,
            m_descriptorSetsScene[imageIndex],
            m_descriptorSetsModel[imageIndex],
            skybox,
            imageIndex,
            m_scene->m_modelDataDynamicUBO,
            pbrStandardObjects);
        /* Draw lambert material objects, base pass */
        m_rendererLambert.renderObjectsBasePass(commandBuffer,
            m_descriptorSetsScene[imageIndex],
            m_descriptorSetsModel[imageIndex],
            skybox,
            imageIndex,
            m_scene->m_modelDataDynamicUBO,
            lambertObjects);
        /* Draw additive pass for all point lights in the scene */
        for (auto& pl : pointLights)
        {
            auto light = pl->get<ComponentPointLight>().light;
            PushBlockForwardAddPass t;
            t.lightColor = light->lightMaterial->intensity * glm::vec4(light->lightMaterial->color, 0);
            t.lightPosition = glm::vec4(pl->getWorldPosition(), 0);
            for (auto& obj : pbrStandardObjects)
            {
                m_rendererPBR.renderObjectsAddPass(commandBuffer,
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
                m_rendererLambert.renderObjectsAddPass(commandBuffer,
                    m_descriptorSetsScene[imageIndex],
                    m_descriptorSetsModel[imageIndex],
                    imageIndex,
                    m_scene->m_modelDataDynamicUBO,
                    obj,
                    t
                );
            }
        }

        m_vkcore.deviceFunctions()->vkCmdEndRenderPass(commandBuffer);
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
        rpBeginInfo.renderArea.extent = m_swapchain->extent();
        rpBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;
        rpBeginInfo.pClearValues = clearValues.data();
        
        m_vkcore.deviceFunctions()->vkCmdBeginRenderPass(commandBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        m_rendererPost.render(commandBuffer, imageIndex);
        m_vkcore.deviceFunctions()->vkCmdEndRenderPass(commandBuffer);
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
            rpBeginInfo.renderArea.extent = m_swapchain->extent();
            rpBeginInfo.clearValueCount = 0;
            rpBeginInfo.pClearValues = nullptr;
            m_vkcore.deviceFunctions()->vkCmdBeginRenderPass(commandBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        
            glm::vec3 transformPosition = m_selectedObject->getWorldPosition();
            m_renderer3DUI.renderTransform(commandBuffer,
                m_descriptorSetsScene[imageIndex],
                imageIndex,
                m_selectedObject->m_modelMatrix,
                m_scene->getCamera());
                
            m_vkcore.deviceFunctions()->vkCmdEndRenderPass(commandBuffer);
        }
    }

    /* Copy highlight output to temp color selection iamge */
    {
        VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        /* Transition highlight render output to transfer source optimal */
        transitionImageLayout(commandBuffer, 
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
        copyRegion.extent = { m_swapchain->extent().width, m_swapchain->extent().height, 1};
        m_vkcore.deviceFunctions()->vkCmdCopyImage(commandBuffer, 
            m_attachmentHighlightForwardOutput[imageIndex].getImageResolve(), 
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
            m_imageTempColorSelection.image, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            1, 
            &copyRegion);
    }

    if (m_vkcore.deviceFunctions()->vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
       throw std::runtime_error("VulkanRenderer::buildFrame(): Failed to end the command buffer");
    }
}

VulkanScene* VulkanRenderer::getActiveScene() const
{
    return m_scene;
}

std::shared_ptr<MeshModel> VulkanRenderer::createVulkanMeshModel(std::string filename)
{
    AssetManager<std::string, MeshModel>& instance = AssetManager<std::string, MeshModel>::getInstance();

    if (instance.isPresent(filename)) return instance.get(filename);
    
    try {
        std::vector<Mesh> meshes = assimpLoadModel(filename);
        auto vkmesh = std::make_shared<VulkanMeshModel>(m_vkcore.physicalDevice(), m_vkcore.device(), m_vkcore.graphicsQueue(), m_vkcore.graphicsCommandPool(), meshes);
        vkmesh->setName(filename);
        return instance.add(filename, vkmesh);
    } catch (std::runtime_error& e) {
        utils::ConsoleWarning("Failed to create a Vulkan Mesh Model: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
}

std::shared_ptr<Material> VulkanRenderer::createMaterial(std::string name, MaterialType type, bool createDescriptors)
{    
    std::shared_ptr<Material> temp;
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
        auto matdesc = std::dynamic_pointer_cast<VulkanMaterialDescriptor>(temp);
        matdesc->createDescriptors(m_vkcore.device(), m_descriptorPool, m_swapchain->imageCount());
        matdesc->updateDescriptorSets(m_vkcore.device(), m_swapchain->imageCount());
    }

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

        renderLoopActive(false);
        waitIdle();

        utils::Timer timer;
        timer.Start();
        
        m_scene->updateSceneGraph(); // TODO the render loop already updates the scene graph often enough?
        m_rendererRayTracing.renderScene(m_scene);

        timer.Stop();

        renderLoopActive(true);

        return timer.ToInt();
    }
    else {
        return 0;
    }
}

glm::vec3 VulkanRenderer::selectObject(float x, float y)
{
    /* Get the texel color at this row and column of the highlight render target */
    uint32_t row = y * m_swapchain->extent().height;
    uint32_t column = x * m_swapchain->extent().width;

    /* Since the image is stored in linear tiling, get the subresource layout to calcualte the padding */
    VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
    VkSubresourceLayout subResourceLayout;
    m_vkcore.deviceFunctions()->vkGetImageSubresourceLayout(m_vkcore.device(), m_imageTempColorSelection.image, &subResource, &subResourceLayout);

    /* Byte index of x,y texel */
    uint32_t index = (row * subResourceLayout.rowPitch + column * 4 * sizeof(float));

    /* Store highlight render result for that texel to the disk */
    float* highlight;
    VkResult res = m_vkcore.deviceFunctions()->vkMapMemory(
        m_vkcore.device(),
        m_imageTempColorSelection.memory,
        subResourceLayout.offset + index,
        3 * sizeof(float),
        0,
        reinterpret_cast<void**>(&highlight)
    );
    glm::vec3 highlightTexelColor;
    memcpy(&highlightTexelColor[0], highlight, 3 * sizeof(float));
    m_vkcore.deviceFunctions()->vkUnmapMemory(m_vkcore.device(), m_imageTempColorSelection.memory);

    return highlightTexelColor;
}

float VulkanRenderer::deltaTime() const
{
    return m_deltaTime;
}

std::shared_ptr<Texture> VulkanRenderer::createTexture(std::string imagePath, VkFormat format, bool keepImage)
{
    try {
        AssetManager<std::string, Image<stbi_uc>>& instance = AssetManager<std::string, Image<stbi_uc>>::getInstance();
        
        std::shared_ptr<Image<stbi_uc>> image;
        if (instance.isPresent(imagePath)) {
            image = instance.get(imagePath);
        }
        else {
            image = std::make_shared<Image<stbi_uc>>(imagePath);
            instance.add(imagePath, image);
        }

        auto temp = createTexture(imagePath, image, format);

        if (!keepImage) {
            instance.remove(imagePath);
        }

        return temp;
    } catch (std::runtime_error& e) {
        utils::ConsoleWarning("VulkanRenderer::createTexture(): Can't create a vulkan texture: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
}

std::shared_ptr<Texture> VulkanRenderer::createTexture(std::string id, std::shared_ptr<Image<stbi_uc>> image, VkFormat format)
{
    try {
        AssetManager<std::string, Texture>& instance = AssetManager<std::string, Texture>::getInstance();
        
        if (instance.isPresent(id)) {
            return instance.get(id);
        }

        TextureType type = TextureType::NO_TYPE;
        if (format == VK_FORMAT_R8G8B8A8_SRGB) type = TextureType::COLOR;
        else if (format == VK_FORMAT_R8G8B8A8_UNORM) type = TextureType::LINEAR;

        auto temp = std::make_shared<VulkanTexture>(id, 
            image, 
            type, 
            m_vkcore.physicalDevice(), 
            m_vkcore.device(), 
            m_vkcore.graphicsQueue(), 
            m_vkcore.graphicsCommandPool(), 
            format, 
            true);

        return instance.add(id, temp);
    }
    catch (std::runtime_error& e) {
        utils::ConsoleCritical("VulkanRenderer::createTexture(): Failed to create a vulkan texture: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
}

std::shared_ptr<Texture> VulkanRenderer::createTextureHDR(std::string imagePath, bool keepImage)
{
    try {
        /* Check if an hdr texture has already been created for that image */
        AssetManager<std::string, Texture>& instance = AssetManager<std::string, Texture>::getInstance();
        if (instance.isPresent(imagePath)) {
            return instance.get(imagePath);
        }
        
        /* Check if the image has already been imported, if not read it from disk  */
        std::shared_ptr<Image<float>> image;
        AssetManager<std::string, Image<float>>& images = AssetManager<std::string, Image<float>>::getInstance();
        if (images.isPresent(imagePath)) {
            image = images.get(imagePath);
        }
        else {
            image = std::make_shared<Image<float>>(imagePath);
            images.add(imagePath, image);
        }

        /* Create texture for that image */
        auto temp = std::make_shared<VulkanTexture>(imagePath, 
            image, 
            TextureType::HDR, 
            m_vkcore.physicalDevice(), 
            m_vkcore.device(), 
            m_vkcore.graphicsQueue(), 
            m_vkcore.graphicsCommandPool(),
            false);

        /* If you are not to keep the image in memory, remove from the assets, and delete it */
        if (!keepImage) {
            images.remove(imagePath);
        }
        
        return instance.add(imagePath, temp);
    }
    catch (std::runtime_error& e) {
        utils::ConsoleCritical("VulkanRenderer::createTextureHDR(): Failed to create a vulkan HDR texture: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
}

std::shared_ptr<Cubemap> VulkanRenderer::createCubemap(std::string directory)
{
    AssetManager<std::string, Cubemap>& instance = AssetManager<std::string, Cubemap>::getInstance();
    if (instance.isPresent(directory)) {
        return instance.get(directory);
    }

    auto cubemap = std::make_shared<VulkanCubemap>(directory, m_vkcore.physicalDevice(), m_vkcore.device(), m_vkcore.graphicsQueue(), m_vkcore.graphicsCommandPool());
    return instance.add(directory, cubemap);
}

std::shared_ptr<EnvironmentMap> VulkanRenderer::createEnvironmentMap(std::string imagePath, bool keepTexture)
{
    try {
        /* Check if an environment map for that imagePath already exists */
        AssetManager<std::string, EnvironmentMap>& envMaps = AssetManager<std::string, EnvironmentMap>::getInstance();
        if (envMaps.isPresent(imagePath)) {
            return envMaps.get(imagePath);
        }

        /* Read HDR image */
        std::shared_ptr<VulkanTexture> hdrImage = std::static_pointer_cast<VulkanTexture>(createTextureHDR(imagePath));

        /* Transform input texture into a cubemap */
        auto cubemap = m_rendererSkybox.createCubemap(hdrImage);
        /* Compute irradiance map */
        auto irradiance = m_rendererSkybox.createIrradianceMap(cubemap);
        /* Compute prefiltered map */
        auto prefiltered = m_rendererSkybox.createPrefilteredCubemap(cubemap);

        if (!keepTexture) {
            AssetManager<std::string, Texture>& textures = AssetManager<std::string, Texture>::getInstance();
            textures.remove(imagePath);
            hdrImage->destroy(m_vkcore.device());
        }

        auto envMap = std::make_shared<EnvironmentMap>(imagePath, cubemap, irradiance, prefiltered);
        return envMaps.add(imagePath, envMap);
    }
    catch (std::runtime_error& e) {
        utils::ConsoleCritical("Failed to create a vulkan environment map: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
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
        colorAttachment.samples = m_vkcore.msaaSamples();
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
        highlightAttachment.samples = m_vkcore.msaaSamples();
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
        depthAttachment.format = m_swapchain->depthFormat();
        depthAttachment.samples = m_vkcore.msaaSamples();
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
        std::array<VkSubpassDependency, 1> forwardDependencies{};
        /* Dependency 1: Wait for the previous external pass to finish reading the color, before you start writing the new color */
        forwardDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        forwardDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        forwardDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        forwardDependencies[0].dstSubpass = 0;
        forwardDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        forwardDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

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

        if (m_vkcore.deviceFunctions()->vkCreateRenderPass(m_vkcore.device(), &renderPassInfo, nullptr, &m_renderPassForward) != VK_SUCCESS) {
            utils::ConsoleFatal("VulkanRenderer::createRenderPasses(): Failed to create a render pass");
            return false;
        }
    }

    {
        /* Render pass 2: post process pass */
        /* ---------------------- SUBPASS 1  ------------------------------ */
        VkSubpassDescription renderPassPostSubpass{};
        /* One color attachment which is the swapchain output */
        VkAttachmentDescription postColorAttachment{};
        postColorAttachment.format = m_swapchain->format();
        postColorAttachment.samples = m_vkcore.msaaSamples();
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
        postColorAttachmentResolve.format = m_swapchain->format();
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
        std::array<VkSubpassDependency, 1> postDependencies{};
        /* Dependency 1: Wait for the previous external pass to finish reading the color, before you start writing the new color */
        postDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        postDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        postDependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        postDependencies[0].dstSubpass = 0;
        postDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        postDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

        std::array<VkAttachmentDescription, 2> postAttachments = { postColorAttachment, postColorAttachmentResolve };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(postAttachments.size());
        renderPassInfo.pAttachments = postAttachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &renderPassPostSubpass;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(postDependencies.size());
        renderPassInfo.pDependencies = postDependencies.data();

        if (m_vkcore.deviceFunctions()->vkCreateRenderPass(m_vkcore.device(), &renderPassInfo, nullptr, &m_renderPassPost) != VK_SUCCESS) {
            utils::ConsoleFatal("VulkanRenderer::createRenderPasses(): Failed to create a render pass");
            return false;
        }
    }


    {
        /* Render pass 3: UI pass */
        /* ---------------------- SUBPASS 1  ------------------------------ */
        VkSubpassDescription renderPassUISubpass{};
        /* One color attachment which is the swapchain output, and one with the highlight information */
        VkAttachmentDescription swapchainColorAttachment{};
        swapchainColorAttachment.format = m_swapchain->format();
        swapchainColorAttachment.samples = m_vkcore.msaaSamples();
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
        swapchainColorAttachmentResolve.format = m_swapchain->format();
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
        highlightAttachment.samples = m_vkcore.msaaSamples();
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
        uiDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        uiDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        uiDependencies[0].dstSubpass = 0;
        uiDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        uiDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        /* Dependency 1: Make sure that our pass finishes writing before the external pass starts reading */ 
        uiDependencies[1].srcSubpass = 0;
        uiDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        uiDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        uiDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        uiDependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        uiDependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

        std::array<VkAttachmentDescription, 4> uiAttachments = { swapchainColorAttachment, swapchainColorAttachmentResolve, highlightAttachment, highlightAttachmentResolve };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(uiAttachments.size());
        renderPassInfo.pAttachments = uiAttachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &renderPassUISubpass;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(uiDependencies.size());
        renderPassInfo.pDependencies = uiDependencies.data();

        if (m_vkcore.deviceFunctions()->vkCreateRenderPass(m_vkcore.device(), &renderPassInfo, nullptr, &m_renderPassUI) != VK_SUCCESS) {
            utils::ConsoleFatal("VulkanRenderer::createRenderPasses(): Failed to create a render pass");
            return false;
        }
    }

    return true;
}

bool VulkanRenderer::createFrameBuffers()
{
    uint32_t swapchainImages = m_swapchain->imageCount();
    VkExtent2D swapchainExtent = m_swapchain->extent();

    /* Create internal render targets for color and highlight */
    m_attachmentColorForwardOutput.resize(swapchainImages);
    m_attachmentHighlightForwardOutput.resize(swapchainImages);

    for (size_t i = 0; i < swapchainImages; i++)
    {
        m_attachmentColorForwardOutput[i].init(m_vkcore.physicalDevice(), 
            m_vkcore.device(), 
            swapchainExtent.width, 
            swapchainExtent.height, 
            m_internalRenderFormat, 
            m_vkcore.msaaSamples(), 
            true
        );
        m_attachmentHighlightForwardOutput[i].init(m_vkcore.physicalDevice(), 
            m_vkcore.device(), 
            swapchainExtent.width, 
            swapchainExtent.height, 
            m_internalRenderFormat, 
            m_vkcore.msaaSamples(), 
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
                m_swapchain->depthStencilImageView()
            };
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPassForward;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapchainExtent.width;
            framebufferInfo.height = swapchainExtent.height;
            framebufferInfo.layers = 1;

            if (m_vkcore.deviceFunctions()->vkCreateFramebuffer(m_vkcore.device(), &framebufferInfo, nullptr, &m_framebuffersForward[i]) != VK_SUCCESS) {
                utils::ConsoleFatal("VulkanRenderer::createFrameBuffers(): Failed to create a framebuffer");
                return false;
            }
        }

        {
            /* Create post process pass framebuffers */
            std::array<VkImageView, 2> attachments = {
                m_swapchain->msaaImageView(),
                m_swapchain->swapchainImageView(i)
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPassPost;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapchainExtent.width;
            framebufferInfo.height = swapchainExtent.height;
            framebufferInfo.layers = 1;

            if (m_vkcore.deviceFunctions()->vkCreateFramebuffer(m_vkcore.device(), &framebufferInfo, nullptr, &m_framebuffersPost[i]) != VK_SUCCESS) {
                utils::ConsoleFatal("VulkanRenderer::createFrameBuffers(): Failed to create a framebuffer");
                return false;
            }
        }

        {
            /* Create UI pass framebuffers */
            std::array<VkImageView, 4> attachments = {
                m_swapchain->msaaImageView(),
                m_swapchain->swapchainImageView(i),
                m_attachmentHighlightForwardOutput[i].getView(),
                m_attachmentHighlightForwardOutput[i].getViewResolve()
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPassUI;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapchainExtent.width;
            framebufferInfo.height = swapchainExtent.height;
            framebufferInfo.layers = 1;

            if (m_vkcore.deviceFunctions()->vkCreateFramebuffer(m_vkcore.device(), &framebufferInfo, nullptr, &m_framebuffersUI[i]) != VK_SUCCESS) {
                utils::ConsoleFatal("Failed to create a framebuffer");
                return false;
            }
        }
    }

    return true;
}

bool VulkanRenderer::createCommandBuffers()
{
    m_commandBuffer.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_vkcore.renderCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffer.size());

    if (vkAllocateCommandBuffers(m_vkcore.device(), &allocInfo, m_commandBuffer.data()) != VK_SUCCESS) {
        throw std::runtime_error("VulkanRenderer::createCommandBuffer(): Failed to allocate command buffers");
    }

    return true;
}

bool VulkanRenderer::createSyncObjects()
{
    m_fenceInFlight.resize(MAX_FRAMES_IN_FLIGHT);
    m_semaphoreImageAvailable.resize(MAX_FRAMES_IN_FLIGHT);
    m_semaphoreRenderFinished.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO; 

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(uint32_t f = 0; f < MAX_FRAMES_IN_FLIGHT; f++)
    {
        if (vkCreateSemaphore(m_vkcore.device(), &semaphoreInfo, nullptr, &m_semaphoreImageAvailable[f]) != VK_SUCCESS ||
            vkCreateSemaphore(m_vkcore.device(), &semaphoreInfo, nullptr, &m_semaphoreRenderFinished[f]) != VK_SUCCESS ||
            vkCreateFence(m_vkcore.device(), &fenceInfo, nullptr, &m_fenceInFlight[f]) != VK_SUCCESS) 
        {
            throw std::runtime_error("VulkanRenderer::createSyncObjects(): Failed to create the synchronization objects");
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

        if (m_vkcore.deviceFunctions()->vkCreateDescriptorSetLayout(m_vkcore.device(), &layoutInfo, nullptr, &m_descriptorSetLayoutScene) != VK_SUCCESS) {
            utils::ConsoleCritical("VulkanRenderer::createDescriptorSetsLayouts(): Failed to create a descriptor set layout for the scene data");
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

        if (m_vkcore.deviceFunctions()->vkCreateDescriptorSetLayout(m_vkcore.device(), &layoutInfo, nullptr, &m_descriptorSetLayoutModel) != VK_SUCCESS) {
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

        m_scene->m_uniformBuffersScene.resize(m_swapchain->imageCount());
        m_scene->m_uniformBuffersSceneMemory.resize(m_swapchain->imageCount());

        for (int i = 0; i < m_swapchain->imageCount(); i++) {
            createBuffer(m_vkcore.physicalDevice(), 
                m_vkcore.device(), 
                bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                m_scene->m_uniformBuffersScene[i],
                m_scene->m_uniformBuffersSceneMemory[i]);
        }
    }

    m_scene->m_modelDataDynamicUBO.createBuffers(m_vkcore.physicalDevice(), m_vkcore.device(), m_swapchain->imageCount());
    m_materialsUBO->createBuffers(m_vkcore.physicalDevice(), m_vkcore.device(), m_swapchain->imageCount());

    return true;
}

bool VulkanRenderer::updateUniformBuffers(size_t imageIndex)
{
    /* Flush scene data changes to GPU */
    m_scene->updateBuffers(m_vkcore.device(), static_cast<uint32_t>(imageIndex));
    /* Flush material data changes to GPU */
    {
        void* data;
        m_vkcore.deviceFunctions()->vkMapMemory(m_vkcore.device(), m_materialsUBO->getBufferMemory(imageIndex), 0, m_materialsUBO->getBlockSizeAligned() * m_materialsUBO->getNBlocks(), 0, &data);
        memcpy(data, m_materialsUBO->getBlock(0), m_materialsUBO->getBlockSizeAligned() * m_materialsUBO->getNBlocks());
        m_vkcore.deviceFunctions()->vkUnmapMemory(m_vkcore.device(), m_materialsUBO->getBufferMemory(imageIndex));
    }

    return true;
}

bool VulkanRenderer::createDescriptorPool(size_t nMaterials)
{
    uint32_t nImages = m_swapchain->imageCount();

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

    if (m_vkcore.deviceFunctions()->vkCreateDescriptorPool(m_vkcore.device(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to create a descriptor pool");
        return false;
    }

    return true;
}

bool VulkanRenderer::createDescriptorSets()
{
    uint32_t nImages = m_swapchain->imageCount();
    
    /* Create descriptor sets */
    {
        m_descriptorSetsScene.resize(nImages);

        std::vector<VkDescriptorSetLayout> layouts(nImages, m_descriptorSetLayoutScene);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = nImages;
        allocInfo.pSetLayouts = layouts.data();
        if (m_vkcore.deviceFunctions()->vkAllocateDescriptorSets(m_vkcore.device(), &allocInfo, m_descriptorSetsScene.data()) != VK_SUCCESS) {
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
        if (m_vkcore.deviceFunctions()->vkAllocateDescriptorSets(m_vkcore.device(), &allocInfo, m_descriptorSetsModel.data()) != VK_SUCCESS) {
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
        m_vkcore.deviceFunctions()->vkUpdateDescriptorSets(m_vkcore.device(), static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
    }

    return true;
}

bool VulkanRenderer::createColorSelectionTempImage()
{
    /* Create temp image used to copy render result from gpu memory to cpu memory */
    bool ret = createImage(m_vkcore.physicalDevice(),
        m_vkcore.device(),
        m_swapchain->extent().width,
        m_swapchain->extent().height,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        m_internalRenderFormat,
        VK_IMAGE_TILING_LINEAR,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_imageTempColorSelection.image,
        m_imageTempColorSelection.memory);

    /* Transition image to the approriate layout ready for render */
    VkCommandBuffer cmdBuf = beginSingleTimeCommands(m_vkcore.device(), m_vkcore.graphicsCommandPool());

    transitionImageLayout(cmdBuf,
        m_imageTempColorSelection.image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

    endSingleTimeCommands(m_vkcore.device(), m_vkcore.graphicsCommandPool(), m_vkcore.graphicsQueue(), cmdBuf);

    return true;
}
