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
                                                      m_scene.layoutInstanceData(),
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
                                                          m_scene.layoutInstanceData(),
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

    VULKAN_CHECK_CRITICAL(
        m_rendererSkybox.initSwapChainResources(swapchainExtent, m_renderPassForward.renderPass(), VK_SAMPLE_COUNT_1_BIT));
    VULKAN_CHECK_CRITICAL(m_rendererPBR.initSwapChainResources(
        swapchainExtent, m_renderPassForward.renderPass(), m_swapchain.imageCount(), VK_SAMPLE_COUNT_1_BIT));
    VULKAN_CHECK_CRITICAL(m_rendererLambert.initSwapChainResources(
        swapchainExtent, m_renderPassForward.renderPass(), m_swapchain.imageCount(), VK_SAMPLE_COUNT_1_BIT));
    VULKAN_CHECK_CRITICAL(m_rendererPost.initSwapChainResources(swapchainExtent,
                                                                m_renderPassPost.renderPass(),
                                                                m_swapchain.imageCount(),
                                                                VK_SAMPLE_COUNT_1_BIT,
                                                                m_attachmentColorForwardOutput,
                                                                m_attachmentHighlightForwardOutput));
    VULKAN_CHECK_CRITICAL(m_renderer3DUI.initSwapChainResources(
        swapchainExtent, m_renderPassUI.renderPass(), m_swapchain.imageCount(), VK_SAMPLE_COUNT_1_BIT));

    return VK_SUCCESS;
}

VkResult VulkanRenderer::releaseSwapChainResources()
{
    m_attachmentColorForwardOutput.destroy(m_vkctx.device());
    m_attachmentHighlightForwardOutput.destroy(m_vkctx.device());

    m_frameBufferForward.destroy(m_vkctx.device());
    m_framebufferPost.destroy(m_vkctx.device());
    m_framebufferUI.destroy(m_vkctx.device());

    m_rendererSkybox.releaseSwapChainResources();
    m_rendererPBR.releaseSwapChainResources();
    m_rendererLambert.releaseSwapChainResources();
    m_rendererPost.releaseSwapChainResources();
    m_renderer3DUI.releaseSwapChainResources();

    m_renderPassForward.destroy(m_vkctx.device());
    m_renderPassPost.destroy(m_vkctx.device());
    m_renderPassUI.destroy(m_vkctx.device());

    m_imageTempColorSelection.destroy(m_vkctx.device());

    return VK_SUCCESS;
}

VkResult VulkanRenderer::releaseResources()
{
    m_random.releaseResources();

    for (uint32_t f = 0; f < MAX_FRAMES_IN_FLIGHT; f++) {
        vkDestroySemaphore(m_vkctx.device(), m_semaphoreImageAvailable[f], nullptr);
        vkDestroySemaphore(m_vkctx.device(), m_semaphoreForwardFinished[f], nullptr);
        vkDestroySemaphore(m_vkctx.device(), m_semaphorePostFinished[f], nullptr);
        vkDestroySemaphore(m_vkctx.device(), m_semaphoreUIFinished[f], nullptr);
        vkDestroySemaphore(m_vkctx.device(), m_semaphoreOutputFinished[f], nullptr);
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

    /* Render frame */
    VULKAN_CHECK(buildFrame(sceneGraphArray, imageIndex));

    /* Present to swapchain */
    VkSemaphore waitSemaphores[] = {m_semaphoreOutputFinished[m_currentFrame]};
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = waitSemaphores;
    VkSwapchainKHR swapChains[] = {m_swapchain.swapchain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    VULKAN_CHECK(vkQueuePresentKHR(m_vkctx.presentQueue(), &presentInfo));

    return VK_SUCCESS;
}

VkResult VulkanRenderer::buildFrame(SceneGraph &sceneGraphArray, uint32_t imageIndex)
{
    /* Update scene and frame buffers */
    m_scene.updateFrame({m_vkctx.physicalDevice(), m_vkctx.device(), m_vkctx.renderCommandPool(), m_vkctx.renderQueue()}, imageIndex);
    m_materials.updateBuffers(static_cast<uint32_t>(imageIndex));
    m_textures.updateTextures();

    /* Forward pass */
    {
        /* Get forward command buffer */
        VkCommandBuffer &commandBufferForward = m_commandBufferForward[m_currentFrame];
        VULKAN_CHECK(vkResetCommandBuffer(commandBufferForward, 0));
        VkCommandBufferBeginInfo beginInfo = vkinit::commandBufferBeginInfo();
        VULKAN_CHECK_CRITICAL(vkBeginCommandBuffer(commandBufferForward, &beginInfo));

        glm::vec3 clearColor = m_scene.backgroundColor();
        std::array<VkClearValue, 3> clearValues{};
        VkClearColorValue cl = {{clearColor.r, clearColor.g, clearColor.b, 1.0F}};
        clearValues[0].color = cl;
        clearValues[1].color = {0, 0, 0, 0};
        clearValues[2].depthStencil = {1.0f, 0};
        VkRenderPassBeginInfo rpBeginInfo = vkinit::renderPassBeginInfo(m_renderPassForward.renderPass(),
                                                                        m_frameBufferForward.framebuffer(imageIndex),
                                                                        static_cast<uint32_t>(clearValues.size()),
                                                                        clearValues.data());
        rpBeginInfo.renderArea.extent = m_swapchain.extent();
        vkCmdBeginRenderPass(commandBufferForward, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        /* Get skybox material and check if its parameters have changed */
        auto skybox = dynamic_cast<VulkanMaterialSkybox *>(m_scene.skyboxMaterial());
        assert(skybox != nullptr);
        /* If material parameters have changed, update descriptor */
        if (skybox->needsUpdate(imageIndex)) {
            skybox->updateDescriptorSet(m_vkctx.device(), imageIndex);
        }

        /* Draw opaque */
        {
            m_rendererLambert.renderObjectsForwardOpaque(commandBufferForward,
                                                         m_scene.m_instances,
                                                         m_scene.descriptorSetSceneData(imageIndex),
                                                         m_scene.descriptorSetInstanceData(imageIndex),
                                                         m_scene.descriptorSetLight(imageIndex),
                                                         skybox->getDescriptor(imageIndex),
                                                         m_materials.descriptorSet(imageIndex),
                                                         m_textures.descriptorSet(),
                                                         m_scene.descriptorSetTLAS(imageIndex),
                                                         m_scene.instancesManager().lights());
            m_rendererPBR.renderObjectsForwardOpaque(commandBufferForward,
                                                     m_scene.m_instances,
                                                     m_scene.descriptorSetSceneData(imageIndex),
                                                     m_scene.descriptorSetInstanceData(imageIndex),
                                                     m_scene.descriptorSetLight(imageIndex),
                                                     skybox->getDescriptor(imageIndex),
                                                     m_materials.descriptorSet(imageIndex),
                                                     m_textures.descriptorSet(),
                                                     m_scene.descriptorSetTLAS(imageIndex),
                                                     m_scene.instancesManager().lights());
        }

        /* Draw skybox if needed */
        if (m_scene.environmentType() == EnvironmentType::HDRI) {
            m_rendererSkybox.renderSkybox(commandBufferForward, m_scene.descriptorSetSceneData(imageIndex), imageIndex, skybox);
        }

        /* Draw transparent */
        {
            /* Sort with distance to camera */
            glm::vec3 cameraPos = m_scene.camera()->transform().position();
            m_scene.m_instances.sortTransparent(cameraPos);

            std::unordered_map<MaterialType, VulkanRendererBase *> renderers = {{MaterialType::MATERIAL_LAMBERT, &m_rendererLambert},
                                                                                {MaterialType::MATERIAL_PBR_STANDARD, &m_rendererPBR}};
            for (auto itr : m_scene.m_instances.transparentMeshes()) {
                auto renderer = renderers[itr->get<ComponentMaterial>().material->type()];

                renderer->renderObjectsForwardTransparent(commandBufferForward,
                                                          m_scene.m_instances,
                                                          m_scene.descriptorSetSceneData(imageIndex),
                                                          m_scene.descriptorSetInstanceData(imageIndex),
                                                          m_scene.descriptorSetLight(imageIndex),
                                                          skybox->getDescriptor(imageIndex),
                                                          m_materials.descriptorSet(imageIndex),
                                                          m_textures.descriptorSet(),
                                                          m_scene.descriptorSetTLAS(imageIndex),
                                                          itr,
                                                          m_scene.instancesManager().lights());
            }
        }

        vkCmdEndRenderPass(commandBufferForward);
        vkEndCommandBuffer(commandBufferForward);

        /* Submit forward command buffer */
        VkSemaphore waitSemaphores[] = {m_semaphoreImageAvailable[m_currentFrame]};
        VkSemaphore signalSemaphores[] = {m_semaphoreForwardFinished[m_currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo submitInfo = vkinit::submitInfo(1, &commandBufferForward);
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        VULKAN_CHECK(vkQueueSubmit(m_vkctx.renderQueue(), 1, &submitInfo, VK_NULL_HANDLE));
    }

    /* Post process pass, draw highlight */
    {
        /* Get post command buffer */
        VkCommandBuffer &commandBufferPost = m_commandBufferPost[m_currentFrame];
        VULKAN_CHECK(vkResetCommandBuffer(commandBufferPost, 0));
        VkCommandBufferBeginInfo beginInfo = vkinit::commandBufferBeginInfo();
        VULKAN_CHECK_CRITICAL(vkBeginCommandBuffer(commandBufferPost, &beginInfo));

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0, 0, 0, 0};
        clearValues[1].color = {0, 0, 0, 0};
        VkRenderPassBeginInfo rpBeginInfo = vkinit::renderPassBeginInfo(m_renderPassPost.renderPass(),
                                                                        m_framebufferPost.framebuffer(imageIndex),
                                                                        static_cast<uint32_t>(clearValues.size()),
                                                                        clearValues.data());
        rpBeginInfo.renderArea.extent = m_swapchain.extent();

        vkCmdBeginRenderPass(commandBufferPost, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        m_rendererPost.render(commandBufferPost, imageIndex);
        vkCmdEndRenderPass(commandBufferPost);
        vkEndCommandBuffer(commandBufferPost);

        /* Submit post command buffer */
        VkSemaphore waitSemaphores[] = {m_semaphoreForwardFinished[m_currentFrame]};
        VkSemaphore signalSemaphores[] = {m_semaphorePostFinished[m_currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT};
        VkSubmitInfo submitInfo = vkinit::submitInfo(1, &commandBufferPost);
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        VULKAN_CHECK(vkQueueSubmit(m_vkctx.renderQueue(), 1, &submitInfo, VK_NULL_HANDLE));
    }

    bool has_selected = m_selectedObject != nullptr;
    /* UI pass */
    {
        VkCommandBuffer &commandBufferUI = m_commandBufferUI[m_currentFrame];
        /* Get command buffer */
        VULKAN_CHECK(vkResetCommandBuffer(commandBufferUI, 0));
        /* Begin command buffer */
        VkCommandBufferBeginInfo beginInfo = vkinit::commandBufferBeginInfo();
        VULKAN_CHECK_CRITICAL(vkBeginCommandBuffer(commandBufferUI, &beginInfo));

        /* Render a transform if an object is selected */
        VkRenderPassBeginInfo rpBeginInfo =
            vkinit::renderPassBeginInfo(m_renderPassUI.renderPass(), m_framebufferUI.framebuffer(imageIndex), 0, nullptr);
        rpBeginInfo.renderArea.extent = m_swapchain.extent();
        vkCmdBeginRenderPass(commandBufferUI, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        if (has_selected) {
            if (m_showSelectedAABB) {
                m_renderer3DUI.renderAABB3(commandBufferUI,
                                           m_scene.descriptorSetSceneData(imageIndex),
                                           imageIndex,
                                           m_selectedObject->AABB(),
                                           m_scene.camera());
            }

            glm::vec3 transformPosition = m_selectedObject->worldPosition();
            m_renderer3DUI.renderTransform(commandBufferUI,
                                           m_scene.descriptorSetSceneData(imageIndex),
                                           imageIndex,
                                           m_selectedObject->modelMatrix(),
                                           m_scene.camera());
        }

        vkCmdEndRenderPass(commandBufferUI);
        vkEndCommandBuffer(commandBufferUI);

        /* Submit post command buffer */
        VkSemaphore waitSemaphores[] = {m_semaphorePostFinished[m_currentFrame]};
        VkSemaphore signalSemaphores[] = {m_semaphoreUIFinished[m_currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo submitInfo = vkinit::submitInfo(1, &commandBufferUI);
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        VULKAN_CHECK(vkQueueSubmit(m_vkctx.renderQueue(), 1, &submitInfo, VK_NULL_HANDLE));
    }

    /* Copy highlight output to temp color selection iamge */
    {
        VkCommandBuffer &commandBufferOutput = m_commandBufferOutput[m_currentFrame];
        /* Get command buffer */
        VULKAN_CHECK(vkResetCommandBuffer(commandBufferOutput, 0));
        /* Begin command buffer */
        VkCommandBufferBeginInfo beginInfo = vkinit::commandBufferBeginInfo();
        VULKAN_CHECK_CRITICAL(vkBeginCommandBuffer(commandBufferOutput, &beginInfo));

        VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        /* Transition highlight render output to transfer source optimal */
        transitionImageLayout(commandBufferOutput,
                              m_attachmentHighlightForwardOutput.image(imageIndex).image(),
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
        vkCmdCopyImage(commandBufferOutput,
                       m_attachmentHighlightForwardOutput.image(imageIndex).image(),
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       m_imageTempColorSelection.image(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1,
                       &copyRegion);

        VULKAN_CHECK_CRITICAL(vkEndCommandBuffer(commandBufferOutput));

        /* Submit post command buffer */
        VkSemaphore waitSemaphores[] = {m_semaphoreUIFinished[m_currentFrame]};
        VkSemaphore signalSemaphores[] = {m_semaphoreOutputFinished[m_currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_2_TRANSFER_BIT};
        VkSubmitInfo submitInfo = vkinit::submitInfo(1, &commandBufferOutput);
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        VULKAN_CHECK(vkQueueSubmit(m_vkctx.renderQueue(), 1, &submitInfo, m_fenceInFlight[m_currentFrame]));
    }

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
    vkGetImageSubresourceLayout(m_vkctx.device(), m_imageTempColorSelection.image(), &subResource, &subResourceLayout);

    /* Byte index of x,y texel */
    uint32_t index = (row * subResourceLayout.rowPitch + column * 4 * sizeof(float));

    /* Store highlight render result for that texel to the disk */
    float *highlight;
    VkResult res = vkMapMemory(m_vkctx.device(),
                               m_imageTempColorSelection.memory(),
                               subResourceLayout.offset + index,
                               3 * sizeof(float),
                               0,
                               reinterpret_cast<void **>(&highlight));
    glm::vec3 highlightTexelColor;
    memcpy(&highlightTexelColor[0], highlight, 3 * sizeof(float));
    vkUnmapMemory(m_vkctx.device(), m_imageTempColorSelection.memory());

    return highlightTexelColor;
}

VkResult VulkanRenderer::createRenderPasses()
{
    VULKAN_CHECK_CRITICAL(
        m_renderPassForward.init(m_vkctx, m_internalRenderFormat, m_internalRenderFormat, m_swapchain.depthFormat()));
    VULKAN_CHECK_CRITICAL(m_renderPassPost.init(m_vkctx, m_swapchain.format()));
    VULKAN_CHECK_CRITICAL(m_renderPassUI.init(m_vkctx, m_swapchain.format(), m_internalRenderFormat, m_swapchain.depthFormat()));

    return VK_SUCCESS;
}

VkResult VulkanRenderer::createFrameBuffers()
{
    uint32_t swapchainImages = m_swapchain.imageCount();
    VkExtent2D swapchainExtent = m_swapchain.extent();

    /* Create a frame buffer attachment out of the swapchain image */
    VulkanFrameBufferAttachment swapchainAttachment;
    {
        std::vector<VkImageView> swapchainViews;
        for (size_t i = 0; i < swapchainImages; i++) {
            swapchainViews.push_back(m_swapchain.swapchainImageView(i));
        }
        VulkanFrameBufferAttachmentInfo swapchainInfo(
            "swapchain", swapchainExtent.width, swapchainExtent.height, m_swapchain.format(), VK_SAMPLE_COUNT_1_BIT);
        VULKAN_CHECK_CRITICAL(swapchainAttachment.init(swapchainInfo, swapchainViews));
    }

    /* Create a frame buffer attachment out of the depth image */
    VulkanFrameBufferAttachment depthAttachment;
    {
        std::vector<VkImageView> depthViews;
        for (size_t i = 0; i < swapchainImages; i++) {
            depthViews.push_back(m_swapchain.depthStencilImageView(i));
        }
        VulkanFrameBufferAttachmentInfo depthInfo(
            "depth", swapchainExtent.width, swapchainExtent.height, m_swapchain.depthFormat(), VK_SAMPLE_COUNT_1_BIT);
        VULKAN_CHECK_CRITICAL(depthAttachment.init(depthInfo, depthViews));
    }

    /* Create frame buffer attachments for color and highlight */
    VULKAN_CHECK_CRITICAL(m_attachmentColorForwardOutput.init(
        m_vkctx,
        VulkanFrameBufferAttachmentInfo(
            "Forward color", swapchainExtent.width, swapchainExtent.height, m_internalRenderFormat, VK_SAMPLE_COUNT_1_BIT),
        swapchainImages));

    VULKAN_CHECK_CRITICAL(m_attachmentHighlightForwardOutput.init(m_vkctx,
                                                                  VulkanFrameBufferAttachmentInfo("Forward highlight",
                                                                                                  swapchainExtent.width,
                                                                                                  swapchainExtent.height,
                                                                                                  m_internalRenderFormat,
                                                                                                  VK_SAMPLE_COUNT_1_BIT,
                                                                                                  VK_IMAGE_USAGE_TRANSFER_SRC_BIT),
                                                                  swapchainImages));

    /* Forward frame buffer */
    {
        std::vector<VulkanFrameBufferAttachment> forwardAttachments = {
            m_attachmentColorForwardOutput, m_attachmentHighlightForwardOutput, depthAttachment};

        VULKAN_CHECK_CRITICAL(m_frameBufferForward.create(m_vkctx,
                                                          swapchainImages,
                                                          m_renderPassForward.renderPass(),
                                                          swapchainExtent.width,
                                                          swapchainExtent.height,
                                                          forwardAttachments));
    }

    /* Post frame buffer */
    {
        std::vector<VulkanFrameBufferAttachment> postAttachments = {swapchainAttachment};
        VULKAN_CHECK_CRITICAL(m_framebufferPost.create(
            m_vkctx, swapchainImages, m_renderPassPost.renderPass(), swapchainExtent.width, swapchainExtent.height, postAttachments));
    }

    /* ui framebuffer */
    {
        std::vector<VulkanFrameBufferAttachment> uiAttachments = {
            swapchainAttachment, m_attachmentHighlightForwardOutput, depthAttachment};

        VULKAN_CHECK_CRITICAL(m_framebufferUI.create(
            m_vkctx, swapchainImages, m_renderPassUI.renderPass(), swapchainExtent.width, swapchainExtent.height, uiAttachments));
    }

    return VK_SUCCESS;
}

VkResult VulkanRenderer::createCommandBuffers()
{
    m_commandBufferForward.resize(MAX_FRAMES_IN_FLIGHT);
    m_commandBufferPost.resize(MAX_FRAMES_IN_FLIGHT);
    m_commandBufferUI.resize(MAX_FRAMES_IN_FLIGHT);
    m_commandBufferOutput.resize(MAX_FRAMES_IN_FLIGHT);

    {
        VkCommandBufferAllocateInfo allocInfo = vkinit::commandBufferAllocateInfo(
            VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_vkctx.renderCommandPool(), static_cast<uint32_t>(m_commandBufferForward.size()));
        VULKAN_CHECK_CRITICAL(vkAllocateCommandBuffers(m_vkctx.device(), &allocInfo, m_commandBufferForward.data()));
    }
    {
        VkCommandBufferAllocateInfo allocInfo = vkinit::commandBufferAllocateInfo(
            VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_vkctx.renderCommandPool(), static_cast<uint32_t>(m_commandBufferPost.size()));
        VULKAN_CHECK_CRITICAL(vkAllocateCommandBuffers(m_vkctx.device(), &allocInfo, m_commandBufferPost.data()));
    }
    {
        VkCommandBufferAllocateInfo allocInfo = vkinit::commandBufferAllocateInfo(
            VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_vkctx.renderCommandPool(), static_cast<uint32_t>(m_commandBufferUI.size()));
        VULKAN_CHECK_CRITICAL(vkAllocateCommandBuffers(m_vkctx.device(), &allocInfo, m_commandBufferUI.data()));
    }
    {
        VkCommandBufferAllocateInfo allocInfo = vkinit::commandBufferAllocateInfo(
            VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_vkctx.renderCommandPool(), static_cast<uint32_t>(m_commandBufferOutput.size()));
        VULKAN_CHECK_CRITICAL(vkAllocateCommandBuffers(m_vkctx.device(), &allocInfo, m_commandBufferOutput.data()));
    }

    return VK_SUCCESS;
}

VkResult VulkanRenderer::createSyncObjects()
{
    m_fenceInFlight.resize(MAX_FRAMES_IN_FLIGHT);
    m_semaphoreImageAvailable.resize(MAX_FRAMES_IN_FLIGHT);
    m_semaphoreForwardFinished.resize(MAX_FRAMES_IN_FLIGHT);
    m_semaphorePostFinished.resize(MAX_FRAMES_IN_FLIGHT);
    m_semaphoreUIFinished.resize(MAX_FRAMES_IN_FLIGHT);
    m_semaphoreOutputFinished.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    for (uint32_t f = 0; f < MAX_FRAMES_IN_FLIGHT; f++) {
        VULKAN_CHECK_CRITICAL(vkCreateSemaphore(m_vkctx.device(), &semaphoreInfo, nullptr, &m_semaphoreImageAvailable[f]));
        VULKAN_CHECK_CRITICAL(vkCreateSemaphore(m_vkctx.device(), &semaphoreInfo, nullptr, &m_semaphoreForwardFinished[f]));
        VULKAN_CHECK_CRITICAL(vkCreateSemaphore(m_vkctx.device(), &semaphoreInfo, nullptr, &m_semaphorePostFinished[f]));
        VULKAN_CHECK_CRITICAL(vkCreateSemaphore(m_vkctx.device(), &semaphoreInfo, nullptr, &m_semaphoreUIFinished[f]));
        VULKAN_CHECK_CRITICAL(vkCreateSemaphore(m_vkctx.device(), &semaphoreInfo, nullptr, &m_semaphoreOutputFinished[f]));
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
                                      m_imageTempColorSelection.image(),
                                      m_imageTempColorSelection.memory()));

    /* Transition image to the approriate layout ready for render */
    VkCommandBuffer cmdBuf;
    VULKAN_CHECK_CRITICAL(beginSingleTimeCommands(m_vkctx.device(), m_vkctx.graphicsCommandPool(), cmdBuf));

    transitionImageLayout(cmdBuf,
                          m_imageTempColorSelection.image(),
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    VULKAN_CHECK_CRITICAL(endSingleTimeCommands(m_vkctx.device(), m_vkctx.graphicsCommandPool(), m_vkctx.graphicsQueue(), cmdBuf));

    return VK_SUCCESS;
}

}  // namespace vengine