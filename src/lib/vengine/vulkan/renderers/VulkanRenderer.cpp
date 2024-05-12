#include "VulkanRenderer.hpp"

#include <chrono>
#include <memory>
#include <set>
#include <algorithm>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stb_image_write.h>

#include <debug_tools/Console.hpp>
#include <debug_tools/Timer.hpp>

#include "core/Light.hpp"
#include "core/Scene.hpp"
#include "core/SceneObject.hpp"
#include "utils/ECS.hpp"
#include "utils/IDGeneration.hpp"
#include "core/Image.hpp"
#include "core/EnvironmentMap.hpp"
#include "core/io/AssimpLoadModel.hpp"
#include "utils/Tree.hpp"
#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanUtils.hpp"
#include "vulkan/common/VulkanLimits.hpp"
#include "vulkan/resources/VulkanModel3D.hpp"
#include "vulkan/resources/VulkanLight.hpp"
#include "VulkanRenderUtils.hpp"

namespace vengine
{

VulkanRenderer::VulkanRenderer(VulkanContext &context,
                               VulkanSwapchain &swapchain,
                               VulkanTextures &textures,
                               VulkanMaterials &materials,
                               VulkanScene &scene)
    : Renderer()
    , m_vkctx(context)
    , m_swapchain(swapchain)
    , m_textures(textures)
    , m_materials(materials)
    , m_scene(scene)
    , m_random(context)
    , m_rendererPathTracing(context, scene, materials, textures, m_random)
{
}

VkResult VulkanRenderer::initResources()
{
    VULKAN_CHECK_CRITICAL(createCommandBuffers());
    VULKAN_CHECK_CRITICAL(createSyncObjects());

    VULKAN_CHECK_CRITICAL(m_random.initResources());

    /* Initialize renderers */
    VULKAN_CHECK_CRITICAL(m_rendererSkybox.initResources(m_vkctx.physicalDevice(),
                                                         m_vkctx.device(),
                                                         m_vkctx.graphicsQueue(),
                                                         m_vkctx.graphicsCommandPool(),
                                                         m_scene.layoutSceneData()));
    VULKAN_CHECK_CRITICAL(m_rendererPBR.initResources(m_vkctx.physicalDevice(),
                                                      m_vkctx.device(),
                                                      m_vkctx.graphicsQueue(),
                                                      m_vkctx.graphicsCommandPool(),
                                                      m_vkctx.physicalDeviceProperties(),
                                                      m_scene.layoutSceneData(),
                                                      m_scene.layoutModelData(),
                                                      m_scene.layoutLights(),
                                                      m_rendererSkybox.descriptorSetLayout(),
                                                      m_materials.descriptorSetLayout(),
                                                      m_textures,
                                                      m_scene.layoutTLAS()));
    VULKAN_CHECK_CRITICAL(m_rendererLambert.initResources(m_vkctx.physicalDevice(),
                                                          m_vkctx.device(),
                                                          m_vkctx.graphicsQueue(),
                                                          m_vkctx.graphicsCommandPool(),
                                                          m_vkctx.physicalDeviceProperties(),
                                                          m_scene.layoutSceneData(),
                                                          m_scene.layoutModelData(),
                                                          m_scene.layoutLights(),
                                                          m_rendererSkybox.descriptorSetLayout(),
                                                          m_materials.descriptorSetLayout(),
                                                          m_textures.descriptorSetLayout(),
                                                          m_scene.layoutTLAS()));
    VULKAN_CHECK_CRITICAL(m_rendererPost.initResources(
        m_vkctx.physicalDevice(), m_vkctx.device(), m_vkctx.graphicsQueue(), m_vkctx.graphicsCommandPool()));

    try {
        VULKAN_CHECK_CRITICAL(m_renderer3DUI.initResources(m_vkctx.physicalDevice(),
                                                           m_vkctx.device(),
                                                           m_vkctx.graphicsQueue(),
                                                           m_vkctx.graphicsCommandPool(),
                                                           m_scene.layoutSceneData()));
    } catch (std::exception &e) {
        debug_tools::ConsoleCritical("VulkanRenderer::initResources(): Failed to initialize UI renderer: " + std::string(e.what()));
    }

    try {
        VULKAN_CHECK_CRITICAL(
            m_rendererPathTracing.initResources(VK_FORMAT_R32G32B32A32_SFLOAT, m_rendererSkybox.descriptorSetLayout()));
    } catch (std::exception &e) {
        debug_tools::ConsoleWarning("VulkanRenderer::initResources():Failed to initialize GPU ray tracing renderer: " +
                                    std::string(e.what()));
    }

    return VK_SUCCESS;
}

VkResult VulkanRenderer::initSwapChainResources()
{
    VkExtent2D swapchainExtent = m_swapchain.extent();
    VkFormat swapchainFormat = m_swapchain.format();

    VULKAN_CHECK_CRITICAL(createRenderPasses());
    VULKAN_CHECK_CRITICAL(createFrameBuffers());
    VULKAN_CHECK_CRITICAL(createColorSelectionTempImage());

    VULKAN_CHECK_CRITICAL(m_rendererSkybox.initSwapChainResources(swapchainExtent, m_renderPassForward, m_vkctx.msaaSamples()));
    VULKAN_CHECK_CRITICAL(
        m_rendererPBR.initSwapChainResources(swapchainExtent, m_renderPassForward, m_swapchain.imageCount(), m_vkctx.msaaSamples()));
    VULKAN_CHECK_CRITICAL(m_rendererLambert.initSwapChainResources(
        swapchainExtent, m_renderPassForward, m_swapchain.imageCount(), m_vkctx.msaaSamples()));
    VULKAN_CHECK_CRITICAL(m_rendererPost.initSwapChainResources(swapchainExtent,
                                                                m_renderPassPost,
                                                                m_swapchain.imageCount(),
                                                                m_vkctx.msaaSamples(),
                                                                m_attachmentColorForwardOutput,
                                                                m_attachmentHighlightForwardOutput));
    VULKAN_CHECK_CRITICAL(
        m_renderer3DUI.initSwapChainResources(swapchainExtent, m_renderPassUI, m_swapchain.imageCount(), m_vkctx.msaaSamples()));

    return VK_SUCCESS;
}

VkResult VulkanRenderer::releaseSwapChainResources()
{
    /* Destroy framebuffer attachments */
    for (int i = 0; i < m_swapchain.imageCount(); i++) {
        m_attachmentColorForwardOutput[i].destroy(m_vkctx.device());
        m_attachmentHighlightForwardOutput[i].destroy(m_vkctx.device());
    }

    for (auto framebuffer : m_framebuffersForward) {
        vkDestroyFramebuffer(m_vkctx.device(), framebuffer, nullptr);
    }
    for (auto framebuffer : m_framebuffersPost) {
        vkDestroyFramebuffer(m_vkctx.device(), framebuffer, nullptr);
    }
    for (auto framebuffer : m_framebuffersUI) {
        vkDestroyFramebuffer(m_vkctx.device(), framebuffer, nullptr);
    }

    m_rendererSkybox.releaseSwapChainResources();
    m_rendererPBR.releaseSwapChainResources();
    m_rendererLambert.releaseSwapChainResources();
    m_rendererPost.releaseSwapChainResources();
    m_renderer3DUI.releaseSwapChainResources();

    vkDestroyRenderPass(m_vkctx.device(), m_renderPassForward, nullptr);
    vkDestroyRenderPass(m_vkctx.device(), m_renderPassPost, nullptr);
    vkDestroyRenderPass(m_vkctx.device(), m_renderPassUI, nullptr);

    vkDestroyImage(m_vkctx.device(), m_imageTempColorSelection.image, nullptr);
    vkFreeMemory(m_vkctx.device(), m_imageTempColorSelection.memory, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRenderer::releaseResources()
{
    m_random.releaseResources();

    for (uint32_t f = 0; f < MAX_FRAMES_IN_FLIGHT; f++) {
        vkDestroySemaphore(m_vkctx.device(), m_semaphoreImageAvailable[f], nullptr);
        vkDestroySemaphore(m_vkctx.device(), m_semaphoreRenderFinished[f], nullptr);
        vkDestroyFence(m_vkctx.device(), m_fenceInFlight[f], nullptr);
    }

    /* Destroy cubemaps */
    {
        auto &cubemapsMap = AssetManager::getInstance().cubemapsMap();
        for (auto itr = cubemapsMap.begin(); itr != cubemapsMap.end(); ++itr) {
            auto vkCubemap = static_cast<VulkanCubemap *>(itr->second);
            vkCubemap->destroy(m_vkctx.device());
        }
    }

    m_rendererSkybox.releaseResources();
    m_rendererPBR.releaseResources();
    m_rendererLambert.releaseResources();
    m_rendererPost.releaseResources();
    m_renderer3DUI.releaseResources();
    if (m_rendererPathTracing.isRayTracingEnabled())
        m_rendererPathTracing.releaseResources();

    /* Destroy imported models */
    {
        auto &meshModelsMap = AssetManager::getInstance().modelsMap();
        for (auto itr = meshModelsMap.begin(); itr != meshModelsMap.end(); ++itr) {
            auto vkmodel = static_cast<VulkanModel3D *>(itr->second);
            vkmodel->destroy(m_vkctx.device());
        }
        meshModelsMap.reset();
    }

    return VK_SUCCESS;
}

void VulkanRenderer::waitIdle()
{
    /* Wait GPU to idle */
    vkDeviceWaitIdle(m_vkctx.device());
}

VkResult VulkanRenderer::renderFrame(SceneGraph &sceneGraphArray)
{
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    /* Wait previous render operation to finish */
    VULKAN_CHECK(vkWaitForFences(m_vkctx.device(), 1, &m_fenceInFlight[m_currentFrame], VK_TRUE, UINT64_MAX));

    /* Get next available image */
    uint32_t imageIndex;
    VULKAN_CHECK(vkAcquireNextImageKHR(m_vkctx.device(),
                                       m_swapchain.swapchain(),
                                       UINT64_MAX,
                                       m_semaphoreImageAvailable[m_currentFrame],
                                       VK_NULL_HANDLE,
                                       &imageIndex));

    VULKAN_CHECK(vkResetFences(m_vkctx.device(), 1, &m_fenceInFlight[m_currentFrame]));

    /* Record command buffer */
    VULKAN_CHECK(vkResetCommandBuffer(m_commandBuffer[m_currentFrame], 0));
    VULKAN_CHECK(buildFrame(sceneGraphArray, imageIndex, m_commandBuffer[m_currentFrame]));

    /* Submit command buffer */
    VkSemaphore waitSemaphores[] = {m_semaphoreImageAvailable[m_currentFrame]};
    VkSemaphore signalSemaphores[] = {m_semaphoreRenderFinished[m_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo = vkinit::submitInfo(1, &m_commandBuffer[m_currentFrame]);
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    VULKAN_CHECK(vkQueueSubmit(m_vkctx.renderQueue(), 1, &submitInfo, m_fenceInFlight[m_currentFrame]));

    /* Present to swapchain */
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {m_swapchain.swapchain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    VULKAN_CHECK(vkQueuePresentKHR(m_vkctx.presentQueue(), &presentInfo));

    return VK_SUCCESS;
}

VkResult VulkanRenderer::buildFrame(SceneGraph &sceneGraphArray, uint32_t imageIndex, VkCommandBuffer commandBuffer)
{
    std::vector<SceneObject *> meshes, lights;
    std::vector<std::pair<SceneObject *, uint32_t>> meshLights;
    parseSceneGraph(sceneGraphArray, meshes, lights, meshLights);

    /* split meshes into materials */
    std::vector<SceneObject *> transparentObjects, opaquePBRStandardObjects, opaqueLambertObjects, volumes;
    for (auto &itr : meshes) {
        Material *mat = itr->get<ComponentMaterial>().material;
        if (mat->isTransparent()) {
            transparentObjects.push_back(itr);
        } else {
            switch (mat->type()) {
                case MaterialType::MATERIAL_PBR_STANDARD:
                    opaquePBRStandardObjects.push_back(itr);
                    break;
                case MaterialType::MATERIAL_LAMBERT:
                    opaqueLambertObjects.push_back(itr);
                    break;
                case MaterialType::MATERIAL_VOLUME:
                    volumes.push_back(itr);
                    break;
                default:
                    debug_tools::ConsoleWarning("VulkanRenderer::buildFrame(): Unexpected material");
            }
        }
    }

    /* Update scene data, builds the TLAS */
    m_scene.updateBuffers(meshes, lights, meshLights, imageIndex);
    m_scene.updateTLAS(
        {m_vkctx.physicalDevice(), m_vkctx.device(), m_vkctx.renderCommandPool(), m_vkctx.renderQueue()}, meshes, imageIndex);
    m_materials.updateBuffers(static_cast<uint32_t>(imageIndex));
    m_textures.updateTextures();

    /* Begin command buffer */
    VkCommandBufferBeginInfo beginInfo = vkinit::commandBufferBeginInfo();
    VULKAN_CHECK_CRITICAL(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    /* Forward pass */
    {
        glm::vec3 clearColor = m_scene.backgroundColor();
        std::array<VkClearValue, 5> clearValues{};
        VkClearColorValue cl = {{clearColor.r, clearColor.g, clearColor.b, 1.0F}};
        clearValues[0].color = cl;
        clearValues[1].color = cl;
        clearValues[2].color = {0, 0, 0, 0};
        clearValues[3].color = {0, 0, 0, 0};
        clearValues[4].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo rpBeginInfo = vkinit::renderPassBeginInfo(
            m_renderPassForward, m_framebuffersForward[imageIndex], static_cast<uint32_t>(clearValues.size()), clearValues.data());
        rpBeginInfo.renderArea.extent = m_swapchain.extent();
        vkCmdBeginRenderPass(commandBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        /* Get skybox material and check if its parameters have changed */
        auto skybox = dynamic_cast<VulkanMaterialSkybox *>(m_scene.skyboxMaterial());
        assert(skybox != nullptr);
        /* If material parameters have changed, update descriptor */
        if (skybox->needsUpdate(imageIndex)) {
            skybox->updateDescriptorSet(m_vkctx.device(), imageIndex);
        }

        /* Draw opaque */
        {
            m_rendererLambert.renderObjectsForwardOpaque(commandBuffer,
                                                         m_scene.descriptorSetSceneData(imageIndex),
                                                         m_scene.descriptorSetModelData(imageIndex),
                                                         m_scene.descriptorSetLight(imageIndex),
                                                         skybox->getDescriptor(imageIndex),
                                                         m_materials.descriptorSet(imageIndex),
                                                         m_textures.descriptorSet(),
                                                         m_scene.descriptorSetTLAS(imageIndex),
                                                         opaqueLambertObjects,
                                                         lights);
            m_rendererPBR.renderObjectsForwardOpaque(commandBuffer,
                                                     m_scene.descriptorSetSceneData(imageIndex),
                                                     m_scene.descriptorSetModelData(imageIndex),
                                                     m_scene.descriptorSetLight(imageIndex),
                                                     skybox->getDescriptor(imageIndex),
                                                     m_materials.descriptorSet(imageIndex),
                                                     m_textures.descriptorSet(),
                                                     m_scene.descriptorSetTLAS(imageIndex),
                                                     opaquePBRStandardObjects,
                                                     lights);
        }

        /* Draw skybox if needed */
        if (m_scene.environmentType() == EnvironmentType::HDRI) {
            m_rendererSkybox.renderSkybox(commandBuffer, m_scene.descriptorSetSceneData(imageIndex), imageIndex, skybox);
        }

        /* Draw transparent */
        {
            /* Sort with distance to camera */
            glm::vec3 cameraPos = m_scene.camera()->transform().position();
            std::sort(transparentObjects.begin(), transparentObjects.end(), [&](SceneObject *a, SceneObject *b) {
                float d1 = glm::distance(cameraPos, a->worldPosition());
                float d2 = glm::distance(cameraPos, b->worldPosition());
                return d1 > d2;
            });

            std::unordered_map<MaterialType, VulkanRendererBase *> renderers = {{MaterialType::MATERIAL_LAMBERT, &m_rendererLambert},
                                                                                {MaterialType::MATERIAL_PBR_STANDARD, &m_rendererPBR}};
            for (auto itr : transparentObjects) {
                auto renderer = renderers[itr->get<ComponentMaterial>().material->type()];

                renderer->renderObjectsForwardTransparent(commandBuffer,
                                                          m_scene.descriptorSetSceneData(imageIndex),
                                                          m_scene.descriptorSetModelData(imageIndex),
                                                          m_scene.descriptorSetLight(imageIndex),
                                                          skybox->getDescriptor(imageIndex),
                                                          m_materials.descriptorSet(imageIndex),
                                                          m_textures.descriptorSet(),
                                                          m_scene.descriptorSetTLAS(imageIndex),
                                                          itr,
                                                          lights);
            }
        }

        vkCmdEndRenderPass(commandBuffer);
    }

    /* Post process pass, highlight */
    {
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0, 0, 0, 0};
        clearValues[1].color = {0, 0, 0, 0};

        VkRenderPassBeginInfo rpBeginInfo = vkinit::renderPassBeginInfo(
            m_renderPassPost, m_framebuffersPost[imageIndex], static_cast<uint32_t>(clearValues.size()), clearValues.data());
        rpBeginInfo.renderArea.extent = m_swapchain.extent();

        vkCmdBeginRenderPass(commandBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        m_rendererPost.render(commandBuffer, imageIndex);
        vkCmdEndRenderPass(commandBuffer);
    }

    /* UI pass */
    {
        /* Render a transform if an object is selected */
        if (m_selectedObject != nullptr) {
            VkRenderPassBeginInfo rpBeginInfo = vkinit::renderPassBeginInfo(m_renderPassUI, m_framebuffersUI[imageIndex], 0, nullptr);
            rpBeginInfo.renderArea.extent = m_swapchain.extent();
            vkCmdBeginRenderPass(commandBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            glm::vec3 transformPosition = m_selectedObject->worldPosition();
            m_renderer3DUI.renderTransform(commandBuffer,
                                           m_scene.descriptorSetSceneData(imageIndex),
                                           imageIndex,
                                           m_selectedObject->modelMatrix(),
                                           m_scene.camera());

            if (m_showSelectedAABB) {
                m_renderer3DUI.renderAABB3(
                    commandBuffer, m_scene.descriptorSetSceneData(imageIndex), imageIndex, m_selectedObject->AABB(), m_scene.camera());
            }

            vkCmdEndRenderPass(commandBuffer);
        }
    }

    /* Copy highlight output to temp color selection iamge */
    {
        VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        /* Transition highlight render output to transfer source optimal */
        transitionImageLayout(commandBuffer,
                              m_attachmentHighlightForwardOutput[imageIndex].getImageResolve(),
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              subresourceRange);

        /* Copy highlight image to temp image */
        VkImageCopy copyRegion{};
        copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copyRegion.srcOffset = {0, 0, 0};
        copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copyRegion.dstOffset = {0, 0, 0};
        copyRegion.extent = {m_swapchain.extent().width, m_swapchain.extent().height, 1};
        vkCmdCopyImage(commandBuffer,
                       m_attachmentHighlightForwardOutput[imageIndex].getImageResolve(),
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       m_imageTempColorSelection.image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1,
                       &copyRegion);
    }

    VULKAN_CHECK_CRITICAL(vkEndCommandBuffer(commandBuffer));

    return VK_SUCCESS;
}

RendererPathTracing &VulkanRenderer::rendererPathTracing()
{
    return m_rendererPathTracing;
}

glm::vec3 VulkanRenderer::selectObject(float x, float y)
{
    /* Get the texel color at this row and column of the highlight render target */
    uint32_t row = y * m_swapchain.extent().height;
    uint32_t column = x * m_swapchain.extent().width;

    /* Since the image is stored in linear tiling, get the subresource layout to calculate the padding */
    VkImageSubresource subResource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
    VkSubresourceLayout subResourceLayout;
    vkGetImageSubresourceLayout(m_vkctx.device(), m_imageTempColorSelection.image, &subResource, &subResourceLayout);

    /* Byte index of x,y texel */
    uint32_t index = (row * subResourceLayout.rowPitch + column * 4 * sizeof(float));

    /* Store highlight render result for that texel to the disk */
    float *highlight;
    VkResult res = vkMapMemory(m_vkctx.device(),
                               m_imageTempColorSelection.memory,
                               subResourceLayout.offset + index,
                               3 * sizeof(float),
                               0,
                               reinterpret_cast<void **>(&highlight));
    glm::vec3 highlightTexelColor;
    memcpy(&highlightTexelColor[0], highlight, 3 * sizeof(float));
    vkUnmapMemory(m_vkctx.device(), m_imageTempColorSelection.memory);

    return highlightTexelColor;
}

VkResult VulkanRenderer::createRenderPasses()
{     /* Render pass 1: Forward pass */
    { /* ------------------------------ SUBPASS 1 ---------------------------------- */
        VkSubpassDescription renderPassForwardSubpass{};
        /* 2 color attachments, one for color output, one for highlight information, plus depth */

        /* Color */
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_internalRenderFormat;
        colorAttachment.samples = m_vkctx.msaaSamples();
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;              /* Before the subpass */
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; /* After the subpass */
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; /* During the subpass */
        /* Resolve attacment for color */
        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = m_internalRenderFormat;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;              /* Before the subpass */
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; /* After the subpass */
        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 1;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; /* During the subpass */

        /* Highlight */
        VkAttachmentDescription highlightAttachment{};
        highlightAttachment.format = m_internalRenderFormat;
        highlightAttachment.samples = m_vkctx.msaaSamples();
        highlightAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        highlightAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        highlightAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        highlightAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        highlightAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;              /* Before the subpass */
        highlightAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; /* After the subpass */
        VkAttachmentReference highlightAttachmentRef{};
        highlightAttachmentRef.attachment = 2;
        highlightAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; /* During the subpass */
        /* Resolve attacment for highlight */
        VkAttachmentDescription highlightAttachmentResolve{};
        highlightAttachmentResolve.format = m_internalRenderFormat;
        highlightAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        highlightAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        highlightAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        highlightAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        highlightAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        highlightAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;              /* Before the subpass */
        highlightAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; /* After the subpass */
        VkAttachmentReference highlightAttachmentResolveRef{};
        highlightAttachmentResolveRef.attachment = 3;
        highlightAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; /* During the subpass */

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = m_swapchain.depthFormat();
        depthAttachment.samples = m_vkctx.msaaSamples();
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 4;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        std::array<VkAttachmentReference, 2> colorAttachments{colorAttachmentRef, highlightAttachmentRef};
        std::array<VkAttachmentReference, 2> resolveAttachments{colorAttachmentResolveRef, highlightAttachmentResolveRef};
        renderPassForwardSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        renderPassForwardSubpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
        renderPassForwardSubpass.pColorAttachments = colorAttachments.data(); /* Same as shader locations */
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
            colorAttachment, colorAttachmentResolve, highlightAttachment, highlightAttachmentResolve, depthAttachment};
        VkRenderPassCreateInfo renderPassInfo = vkinit::renderPassCreateInfo(static_cast<uint32_t>(forwardAttachments.size()),
                                                                             forwardAttachments.data(),
                                                                             1,
                                                                             &renderPassForwardSubpass,
                                                                             static_cast<uint32_t>(forwardDependencies.size()),
                                                                             forwardDependencies.data());
        VULKAN_CHECK_CRITICAL(vkCreateRenderPass(m_vkctx.device(), &renderPassInfo, nullptr, &m_renderPassForward));
    }

    {
        /* Render pass 2: post process pass */
        /* ---------------------- SUBPASS 1  ------------------------------ */
        VkSubpassDescription renderPassPostSubpass{};
        /* One color attachment which is the swapchain output */
        VkAttachmentDescription postColorAttachment{};
        postColorAttachment.format = m_swapchain.format();
        postColorAttachment.samples = m_vkctx.msaaSamples();
        postColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        postColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        postColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        postColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        postColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;              /* Before the subpass */
        postColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; /* After the subpass */
        VkAttachmentReference postColorAttachmentRef{};
        postColorAttachmentRef.attachment = 0;
        postColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; /* During the subpass */
        VkAttachmentDescription postColorAttachmentResolve{};
        postColorAttachmentResolve.format = m_swapchain.format();
        postColorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        postColorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        postColorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        postColorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        postColorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        postColorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;     /* Before the subpass */
        postColorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; /* After the subpass */
        VkAttachmentReference postColorAttachmentResolveRef{};
        postColorAttachmentResolveRef.attachment = 1;
        postColorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; /* During the subpass */

        /* setup subpass 1 */
        renderPassPostSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        renderPassPostSubpass.colorAttachmentCount = 1;
        renderPassPostSubpass.pColorAttachments = &postColorAttachmentRef; /* Same as shader locations */
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

        std::array<VkAttachmentDescription, 2> postAttachments = {postColorAttachment, postColorAttachmentResolve};
        VkRenderPassCreateInfo renderPassInfo = vkinit::renderPassCreateInfo(static_cast<uint32_t>(postAttachments.size()),
                                                                             postAttachments.data(),
                                                                             1,
                                                                             &renderPassPostSubpass,
                                                                             static_cast<uint32_t>(postDependencies.size()),
                                                                             postDependencies.data());
        VULKAN_CHECK_CRITICAL(vkCreateRenderPass(m_vkctx.device(), &renderPassInfo, nullptr, &m_renderPassPost));
    }

    {
        /* Render pass 3: UI pass */
        /* ---------------------- SUBPASS 1  ------------------------------ */
        VkSubpassDescription renderPassUISubpass{};
        /* One color attachment which is the swapchain output, and one with the highlight information */
        VkAttachmentDescription swapchainColorAttachment{};
        swapchainColorAttachment.format = m_swapchain.format();
        swapchainColorAttachment.samples = m_vkctx.msaaSamples();
        swapchainColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        swapchainColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        swapchainColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        swapchainColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        swapchainColorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; /* Before the subpass */
        swapchainColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;   /* After the subpass */
        VkAttachmentReference swapchainColorAttachmentRef{};
        swapchainColorAttachmentRef.attachment = 0;
        swapchainColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; /* During the subpass */
        VkAttachmentDescription swapchainColorAttachmentResolve{};
        swapchainColorAttachmentResolve.format = m_swapchain.format();
        swapchainColorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        swapchainColorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        swapchainColorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        swapchainColorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        swapchainColorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        swapchainColorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; /* Before the subpass */
        swapchainColorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;   /* After the subpass */
        VkAttachmentReference swapchainColorAttachmentResolveRef{};
        swapchainColorAttachmentResolveRef.attachment = 1;
        swapchainColorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; /* During the subpass */

        VkAttachmentDescription highlightAttachment{};
        highlightAttachment.format = m_internalRenderFormat;
        highlightAttachment.samples = m_vkctx.msaaSamples();
        highlightAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        highlightAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        highlightAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        highlightAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        highlightAttachment.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; /* Before the subpass */
        highlightAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;   /* After the subpass */
        VkAttachmentReference highlightAttachmentRef{};
        highlightAttachmentRef.attachment = 2;
        highlightAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; /* During the subpass */
        VkAttachmentDescription highlightAttachmentResolve{};
        highlightAttachmentResolve.format = m_internalRenderFormat;
        highlightAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        highlightAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        highlightAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        highlightAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        highlightAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        highlightAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; /* Before the subpass */
        highlightAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;   /* After the subpass */
        VkAttachmentReference highlightAttachmentResolveRef{};
        highlightAttachmentResolveRef.attachment = 3;
        highlightAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; /* During the subpass */

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = m_swapchain.depthFormat();
        depthAttachment.samples = m_vkctx.msaaSamples();
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 4;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        /* setup subpass 1 */
        std::array<VkAttachmentReference, 2> colorAttachments{swapchainColorAttachmentRef, highlightAttachmentRef};
        std::array<VkAttachmentReference, 2> resolveAttachments{swapchainColorAttachmentResolveRef, highlightAttachmentResolveRef};
        renderPassUISubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        renderPassUISubpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
        renderPassUISubpass.pColorAttachments = colorAttachments.data(); /* Same as shader locations */
        renderPassUISubpass.pResolveAttachments = resolveAttachments.data();
        renderPassUISubpass.pDepthStencilAttachment = &depthAttachmentRef;

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

        std::array<VkAttachmentDescription, 5> uiAttachments = {swapchainColorAttachment,
                                                                swapchainColorAttachmentResolve,
                                                                highlightAttachment,
                                                                highlightAttachmentResolve,
                                                                depthAttachment};
        VkRenderPassCreateInfo renderPassInfo = vkinit::renderPassCreateInfo(static_cast<uint32_t>(uiAttachments.size()),
                                                                             uiAttachments.data(),
                                                                             1,
                                                                             &renderPassUISubpass,
                                                                             static_cast<uint32_t>(uiDependencies.size()),
                                                                             uiDependencies.data());
        VULKAN_CHECK_CRITICAL(vkCreateRenderPass(m_vkctx.device(), &renderPassInfo, nullptr, &m_renderPassUI));
    }

    return VK_SUCCESS;
}

VkResult VulkanRenderer::createFrameBuffers()
{
    uint32_t swapchainImages = m_swapchain.imageCount();
    VkExtent2D swapchainExtent = m_swapchain.extent();

    /* Create internal render targets for color and highlight */
    m_attachmentColorForwardOutput.resize(swapchainImages);
    m_attachmentHighlightForwardOutput.resize(swapchainImages);

    for (size_t i = 0; i < swapchainImages; i++) {
        m_attachmentColorForwardOutput[i].init(m_vkctx.physicalDevice(),
                                               m_vkctx.device(),
                                               swapchainExtent.width,
                                               swapchainExtent.height,
                                               m_internalRenderFormat,
                                               m_vkctx.msaaSamples(),
                                               true);
        m_attachmentHighlightForwardOutput[i].init(m_vkctx.physicalDevice(),
                                                   m_vkctx.device(),
                                                   swapchainExtent.width,
                                                   swapchainExtent.height,
                                                   m_internalRenderFormat,
                                                   m_vkctx.msaaSamples(),
                                                   true,
                                                   VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    }

    /* Create framebuffers for forward and post process pass */
    m_framebuffersForward.resize(swapchainImages);
    m_framebuffersPost.resize(swapchainImages);
    m_framebuffersUI.resize(swapchainImages);

    for (size_t i = 0; i < swapchainImages; i++) {
        {
            /* Create forward pass framebuffers */
            std::array<VkImageView, 5> attachments = {m_attachmentColorForwardOutput[i].getView(),
                                                      m_attachmentColorForwardOutput[i].getViewResolve(),
                                                      m_attachmentHighlightForwardOutput[i].getView(),
                                                      m_attachmentHighlightForwardOutput[i].getViewResolve(),
                                                      m_swapchain.depthStencilImageView()};
            VkFramebufferCreateInfo framebufferInfo = vkinit::framebufferCreateInfo(m_renderPassForward,
                                                                                    static_cast<uint32_t>(attachments.size()),
                                                                                    attachments.data(),
                                                                                    swapchainExtent.width,
                                                                                    swapchainExtent.height);
            VULKAN_CHECK_CRITICAL(vkCreateFramebuffer(m_vkctx.device(), &framebufferInfo, nullptr, &m_framebuffersForward[i]));
        }

        {
            /* Create post process pass framebuffers */
            std::array<VkImageView, 2> attachments = {m_swapchain.msaaImageView(), m_swapchain.swapchainImageView(i)};

            VkFramebufferCreateInfo framebufferInfo = vkinit::framebufferCreateInfo(m_renderPassPost,
                                                                                    static_cast<uint32_t>(attachments.size()),
                                                                                    attachments.data(),
                                                                                    swapchainExtent.width,
                                                                                    swapchainExtent.height);
            VULKAN_CHECK_CRITICAL(vkCreateFramebuffer(m_vkctx.device(), &framebufferInfo, nullptr, &m_framebuffersPost[i]));
        }

        {
            /* Create UI pass framebuffers */
            std::array<VkImageView, 5> attachments = {m_swapchain.msaaImageView(),
                                                      m_swapchain.swapchainImageView(i),
                                                      m_attachmentHighlightForwardOutput[i].getView(),
                                                      m_attachmentHighlightForwardOutput[i].getViewResolve(),
                                                      m_swapchain.depthStencilImageView()};

            VkFramebufferCreateInfo framebufferInfo = vkinit::framebufferCreateInfo(m_renderPassUI,
                                                                                    static_cast<uint32_t>(attachments.size()),
                                                                                    attachments.data(),
                                                                                    swapchainExtent.width,
                                                                                    swapchainExtent.height);
            VULKAN_CHECK_CRITICAL(vkCreateFramebuffer(m_vkctx.device(), &framebufferInfo, nullptr, &m_framebuffersUI[i]));
        }
    }

    return VK_SUCCESS;
}

VkResult VulkanRenderer::createCommandBuffers()
{
    m_commandBuffer.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo = vkinit::commandBufferAllocateInfo(
        VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_vkctx.renderCommandPool(), static_cast<uint32_t>(m_commandBuffer.size()));
    VULKAN_CHECK_CRITICAL(vkAllocateCommandBuffers(m_vkctx.device(), &allocInfo, m_commandBuffer.data()));

    return VK_SUCCESS;
}

VkResult VulkanRenderer::createSyncObjects()
{
    m_fenceInFlight.resize(MAX_FRAMES_IN_FLIGHT);
    m_semaphoreImageAvailable.resize(MAX_FRAMES_IN_FLIGHT);
    m_semaphoreRenderFinished.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    for (uint32_t f = 0; f < MAX_FRAMES_IN_FLIGHT; f++) {
        VULKAN_CHECK_CRITICAL(vkCreateSemaphore(m_vkctx.device(), &semaphoreInfo, nullptr, &m_semaphoreImageAvailable[f]));
        VULKAN_CHECK_CRITICAL(vkCreateSemaphore(m_vkctx.device(), &semaphoreInfo, nullptr, &m_semaphoreRenderFinished[f]));
        VULKAN_CHECK_CRITICAL(vkCreateFence(m_vkctx.device(), &fenceInfo, nullptr, &m_fenceInFlight[f]));
    }

    return VK_SUCCESS;
}

VkResult VulkanRenderer::createColorSelectionTempImage()
{
    /* Create temp image used to copy render result from gpu memory to cpu memory */
    VkImageCreateInfo imageInfo = vkinit::imageCreateInfo({m_swapchain.extent().width, m_swapchain.extent().height, 1},
                                                          m_internalRenderFormat,
                                                          1,
                                                          VK_SAMPLE_COUNT_1_BIT,
                                                          VK_IMAGE_TILING_LINEAR,
                                                          VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    VULKAN_CHECK_CRITICAL(createImage(m_vkctx.physicalDevice(),
                                      m_vkctx.device(),
                                      imageInfo,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                      m_imageTempColorSelection.image,
                                      m_imageTempColorSelection.memory));

    /* Transition image to the approriate layout ready for render */
    VkCommandBuffer cmdBuf;
    VULKAN_CHECK_CRITICAL(beginSingleTimeCommands(m_vkctx.device(), m_vkctx.graphicsCommandPool(), cmdBuf));

    transitionImageLayout(cmdBuf,
                          m_imageTempColorSelection.image,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    VULKAN_CHECK_CRITICAL(endSingleTimeCommands(m_vkctx.device(), m_vkctx.graphicsCommandPool(), m_vkctx.graphicsQueue(), cmdBuf));

    return VK_SUCCESS;
}

}  // namespace vengine