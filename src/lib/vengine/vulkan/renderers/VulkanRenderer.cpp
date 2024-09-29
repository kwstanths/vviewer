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
    , m_renderPassDeferred(context)
    , m_rendererGBuffer(context)
    , m_rendererLightComposition(context)
    , m_rendererPBR(context)
    , m_rendererLambert(context)
    , m_rendererOverlay(context)
    , m_rendererOutput(context)
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
                                                         m_scene.descriptorSetlayoutSceneData()));
    VULKAN_CHECK_CRITICAL(m_rendererGBuffer.initResources(m_scene.descriptorSetlayoutSceneData(),
                                                          m_scene.descriptorSetlayoutInstanceData(),
                                                          m_materials.descriptorSetLayout(),
                                                          m_textures.descriptorSetLayout()));
    VULKAN_CHECK_CRITICAL(m_rendererLightComposition.initResources(m_scene.descriptorSetlayoutSceneData(),
                                                                   m_scene.descriptorSetlayoutInstanceData(),
                                                                   m_scene.descriptorSetlayoutLights(),
                                                                   m_rendererSkybox.descriptorSetLayout(),
                                                                   m_materials.descriptorSetLayout(),
                                                                   m_textures.descriptorSetLayout(),
                                                                   m_scene.descriptorSetlayoutTLAS()));
    VULKAN_CHECK_CRITICAL(m_rendererPBR.initResources(m_scene.descriptorSetlayoutSceneData(),
                                                      m_scene.descriptorSetlayoutInstanceData(),
                                                      m_scene.descriptorSetlayoutLights(),
                                                      m_rendererSkybox.descriptorSetLayout(),
                                                      m_materials.descriptorSetLayout(),
                                                      m_textures,
                                                      m_scene.descriptorSetlayoutTLAS()));
    VULKAN_CHECK_CRITICAL(m_rendererLambert.initResources(m_scene.descriptorSetlayoutSceneData(),
                                                          m_scene.descriptorSetlayoutInstanceData(),
                                                          m_scene.descriptorSetlayoutLights(),
                                                          m_rendererSkybox.descriptorSetLayout(),
                                                          m_materials.descriptorSetLayout(),
                                                          m_textures.descriptorSetLayout(),
                                                          m_scene.descriptorSetlayoutTLAS()));

    try {
        VULKAN_CHECK_CRITICAL(m_rendererOverlay.initResources(m_scene.descriptorSetlayoutSceneData()));
    } catch (std::exception &e) {
        debug_tools::ConsoleCritical("VulkanRenderer::initResources(): Failed to initialize UI renderer: " + std::string(e.what()));
    }

    VULKAN_CHECK_CRITICAL(m_rendererOutput.initResources(m_scene.descriptorSetlayoutSceneData()));

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
    VULKAN_CHECK_CRITICAL(createSelectionImage());

    VULKAN_CHECK_CRITICAL(m_rendererGBuffer.initSwapChainResources(swapchainExtent, m_renderPassDeferred));
    VULKAN_CHECK_CRITICAL(m_rendererLightComposition.initSwapChainResources(swapchainExtent, m_renderPassDeferred));
    VULKAN_CHECK_CRITICAL(m_rendererSkybox.initSwapChainResources(swapchainExtent, m_renderPassDeferred));
    VULKAN_CHECK_CRITICAL(m_rendererPBR.initSwapChainResources(swapchainExtent, m_renderPassDeferred));
    VULKAN_CHECK_CRITICAL(m_rendererLambert.initSwapChainResources(swapchainExtent, m_renderPassDeferred));
    VULKAN_CHECK_CRITICAL(m_rendererOverlay.initSwapChainResources(swapchainExtent, m_renderPassOverlay));
    VULKAN_CHECK_CRITICAL(m_rendererOutput.initSwapChainResources(swapchainExtent, m_swapchain.imageCount(), m_renderPassOutput));

    m_rendererOutput.setInputAttachment(m_attachmentInternalColor);

    return VK_SUCCESS;
}

VkResult VulkanRenderer::releaseSwapChainResources()
{
    m_attachmentInternalColor.destroy(m_vkctx.device());
    m_attachmentGBuffer1.destroy(m_vkctx.device());
    m_attachmentGBuffer2.destroy(m_vkctx.device());

    m_frameBufferDeferred.destroy(m_vkctx.device());
    // m_framebufferPost.destroy(m_vkctx.device());
    m_framebufferOverlay.destroy(m_vkctx.device());
    m_framebufferOutput.destroy(m_vkctx.device());

    m_rendererGBuffer.releaseSwapChainResources();
    m_rendererLightComposition.releaseSwapChainResources();
    m_rendererSkybox.releaseSwapChainResources();
    m_rendererPBR.releaseSwapChainResources();
    m_rendererLambert.releaseSwapChainResources();
    // m_rendererPost.releaseSwapChainResources();
    m_rendererOverlay.releaseSwapChainResources();
    m_rendererOutput.releaseSwapChainResources();

    m_renderPassDeferred.releaseResources(m_vkctx.device());
    // m_renderPassPost.destroy(m_vkctx.device());
    m_renderPassOverlay.releaseResources(m_vkctx.device());
    m_renderPassOutput.releaseResources(m_vkctx.device());

    m_imageSelection.destroy(m_vkctx.device());

    return VK_SUCCESS;
}

VkResult VulkanRenderer::releaseResources()
{
    m_random.releaseResources();

    for (uint32_t f = 0; f < MAX_FRAMES_IN_FLIGHT; f++) {
        vkDestroySemaphore(m_vkctx.device(), m_semaphoreImageAvailable[f], nullptr);
        vkDestroySemaphore(m_vkctx.device(), m_semaphoreDeferredFinished[f], nullptr);
        // vkDestroySemaphore(m_vkctx.device(), m_semaphorePostFinished[f], nullptr);
        vkDestroySemaphore(m_vkctx.device(), m_semaphoreOverlayFinished[f], nullptr);
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

    m_rendererGBuffer.releaseResources();
    m_rendererLightComposition.releaseResources();
    m_rendererSkybox.releaseResources();
    m_rendererPBR.releaseResources();
    m_rendererLambert.releaseResources();
    // m_rendererPost.releaseResources();
    m_rendererOverlay.releaseResources();
    m_rendererOutput.releaseResources();
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

VkResult VulkanRenderer::renderFrame()
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

    if (!(imageIndex < m_swapchain.imageCount()))
        return VK_ERROR_SURFACE_LOST_KHR;

    VULKAN_CHECK(vkResetFences(m_vkctx.device(), 1, &m_fenceInFlight[m_currentFrame]));

    /* Render frame */
    VULKAN_CHECK(buildFrame(imageIndex));

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

VkResult VulkanRenderer::buildFrame(uint32_t imageIndex)
{
    /* Update scene and buffers */
    m_scene.updateFrame({m_vkctx.physicalDevice(), m_vkctx.device(), m_vkctx.renderCommandPool(), m_vkctx.renderQueue()}, imageIndex);
    m_materials.updateBuffers(static_cast<uint32_t>(imageIndex));
    m_textures.updateTextures();

    /* Deferred pass */
    {
        VkCommandBuffer &commandBufferDeferred = m_commandBufferDeferred[m_currentFrame];
        VULKAN_CHECK(vkResetCommandBuffer(commandBufferDeferred, 0));

        VkCommandBufferBeginInfo beginInfo = vkinit::commandBufferBeginInfo();
        VULKAN_CHECK_CRITICAL(vkBeginCommandBuffer(commandBufferDeferred, &beginInfo));

        /* Start deferred pass */
        glm::vec3 clearColor = m_scene.backgroundColor();
        std::array<VkClearValue, 4> clearValues{};
        VkClearColorValue cl = {{clearColor.r, clearColor.g, clearColor.b, 1.0F}};
        clearValues[0].color = {0, 0, 0, 0};
        clearValues[1].color = {0, 0, 0, 0};
        clearValues[2].depthStencil = {1.0f, 0};
        clearValues[3].color = cl;
        VkRenderPassBeginInfo rpBeginInfo = vkinit::renderPassBeginInfo(m_renderPassDeferred.renderPass(),
                                                                        m_frameBufferDeferred.framebuffer(imageIndex),
                                                                        static_cast<uint32_t>(clearValues.size()),
                                                                        clearValues.data());
        rpBeginInfo.renderArea.extent = m_swapchain.extent();
        vkCmdBeginRenderPass(commandBufferDeferred, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        /* Get skybox material and check if its parameters have changed */
        auto skybox = dynamic_cast<VulkanMaterialSkybox *>(m_scene.skyboxMaterial());
        assert(skybox != nullptr);
        /* If material parameters have changed, update descriptor */
        if (skybox->needsUpdate(imageIndex)) {
            skybox->updateDescriptorSet(m_vkctx.device(), imageIndex);
        }

        /* GBuffer subpass */
        {
            m_rendererGBuffer.renderOpaqueInstances(commandBufferDeferred,
                                                    m_scene.m_instances,
                                                    m_scene.descriptorSetSceneData(imageIndex),
                                                    m_scene.descriptorSetInstanceData(imageIndex),
                                                    m_materials.descriptorSet(imageIndex),
                                                    m_textures.descriptorSet());

            if (m_scene.environmentType() == EnvironmentType::HDRI) {
                m_rendererSkybox.renderSkybox(commandBufferDeferred, m_scene.descriptorSetSceneData(imageIndex), imageIndex, skybox);
            }
        }

        /* Light composition subpass */
        vkCmdNextSubpass(commandBufferDeferred, VK_SUBPASS_CONTENTS_INLINE);
        {
            /* Perform IBL if needed */
            if (m_scene.environmentIntensity() > 0.F) {
                m_rendererLightComposition.renderIBL(commandBufferDeferred,
                                                     m_scene.m_instances,
                                                     m_renderPassDeferred.descriptor(imageIndex),
                                                     m_scene.descriptorSetSceneData(imageIndex),
                                                     m_scene.descriptorSetInstanceData(imageIndex),
                                                     m_scene.descriptorSetLight(imageIndex),
                                                     skybox->getDescriptor(imageIndex),
                                                     m_materials.descriptorSet(imageIndex),
                                                     m_textures.descriptorSet(),
                                                     m_scene.descriptorSetTLAS(imageIndex));
            }

            m_rendererLightComposition.renderLights(commandBufferDeferred,
                                                    m_scene.m_instances,
                                                    m_renderPassDeferred.descriptor(imageIndex),
                                                    m_scene.descriptorSetSceneData(imageIndex),
                                                    m_scene.descriptorSetInstanceData(imageIndex),
                                                    m_scene.descriptorSetLight(imageIndex),
                                                    skybox->getDescriptor(imageIndex),
                                                    m_materials.descriptorSet(imageIndex),
                                                    m_textures.descriptorSet(),
                                                    m_scene.descriptorSetTLAS(imageIndex));
        }

        /* Perform forward subpass */
        vkCmdNextSubpass(commandBufferDeferred, VK_SUBPASS_CONTENTS_INLINE);
        {
            /* Sort with distance to camera */
            glm::vec3 cameraPos = m_scene.camera()->transform().position();
            m_scene.m_instances.sortTransparent(cameraPos);

            std::unordered_map<MaterialType, VulkanRendererForward *> renderers = {
                {MaterialType::MATERIAL_LAMBERT, &m_rendererLambert}, {MaterialType::MATERIAL_PBR_STANDARD, &m_rendererPBR}};
            for (auto itr : m_scene.m_instances.transparentMeshes()) {
                auto renderer = renderers[itr->get<ComponentMaterial>().material()->type()];

                renderer->renderObject(commandBufferDeferred,
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

        vkCmdEndRenderPass(commandBufferDeferred);
        vkEndCommandBuffer(commandBufferDeferred);

        /* Submit deferred command buffer */
        VkSemaphore waitSemaphores[] = {m_semaphoreImageAvailable[m_currentFrame]};
        VkSemaphore signalSemaphores[] = {m_semaphoreDeferredFinished[m_currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo submitInfo = vkinit::submitInfo(1, &commandBufferDeferred);
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        VULKAN_CHECK(vkQueueSubmit(m_vkctx.renderQueue(), 1, &submitInfo, VK_NULL_HANDLE));
    }

    /* Post process pass, draw highlight */
    // {
    //     /* Get post command buffer */
    //     VkCommandBuffer &commandBufferPost = m_commandBufferPost[m_currentFrame];
    //     VULKAN_CHECK(vkResetCommandBuffer(commandBufferPost, 0));
    //     VkCommandBufferBeginInfo beginInfo = vkinit::commandBufferBeginInfo();
    //     VULKAN_CHECK_CRITICAL(vkBeginCommandBuffer(commandBufferPost, &beginInfo));

    //     std::array<VkClearValue, 2> clearValues{};
    //     clearValues[0].color = {0, 0, 0, 0};
    //     clearValues[1].color = {0, 0, 0, 0};
    //     VkRenderPassBeginInfo rpBeginInfo = vkinit::renderPassBeginInfo(m_renderPassPost.renderPass(),
    //                                                                     m_framebufferPost.framebuffer(imageIndex),
    //                                                                     static_cast<uint32_t>(clearValues.size()),
    //                                                                     clearValues.data());
    //     rpBeginInfo.renderArea.extent = m_swapchain.extent();

    //     vkCmdBeginRenderPass(commandBufferPost, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    //     m_rendererPost.render(commandBufferPost, imageIndex);
    //     vkCmdEndRenderPass(commandBufferPost);
    //     vkEndCommandBuffer(commandBufferPost);

    //     /* Submit post command buffer */
    //     VkSemaphore waitSemaphores[] = {m_semaphoreForwardFinished[m_currentFrame]};
    //     VkSemaphore signalSemaphores[] = {m_semaphorePostFinished[m_currentFrame]};
    //     VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT};
    //     VkSubmitInfo submitInfo = vkinit::submitInfo(1, &commandBufferPost);
    //     submitInfo.waitSemaphoreCount = 1;
    //     submitInfo.pWaitSemaphores = waitSemaphores;
    //     submitInfo.pWaitDstStageMask = waitStages;
    //     submitInfo.signalSemaphoreCount = 1;
    //     submitInfo.pSignalSemaphores = signalSemaphores;
    //     VULKAN_CHECK(vkQueueSubmit(m_vkctx.renderQueue(), 1, &submitInfo, VK_NULL_HANDLE));
    // }

    /* Overlay pass */
    {
        VkCommandBuffer &commandBufferOverlay = m_commandBufferOverlay[m_currentFrame];
        VULKAN_CHECK(vkResetCommandBuffer(commandBufferOverlay, 0));

        VkCommandBufferBeginInfo beginInfo = vkinit::commandBufferBeginInfo();
        VULKAN_CHECK_CRITICAL(vkBeginCommandBuffer(commandBufferOverlay, &beginInfo));

        /* Render a transform if an object is selected */
        VkRenderPassBeginInfo rpBeginInfo =
            vkinit::renderPassBeginInfo(m_renderPassOverlay.renderPass(), m_framebufferOverlay.framebuffer(imageIndex), 0, nullptr);
        rpBeginInfo.renderArea.extent = m_swapchain.extent();
        vkCmdBeginRenderPass(commandBufferOverlay, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        bool has_selected = m_selectedObject != nullptr;
        if (has_selected) {
            if (m_showSelectedAABB) {
                m_rendererOverlay.renderAABB3(commandBufferOverlay,
                                              m_scene.descriptorSetSceneData(imageIndex),
                                              imageIndex,
                                              m_selectedObject->AABB(),
                                              m_scene.camera());
            }

            glm::vec3 transformPosition = m_selectedObject->worldPosition();
            m_rendererOverlay.render3DTransform(commandBufferOverlay,
                                                m_scene.descriptorSetSceneData(imageIndex),
                                                imageIndex,
                                                m_selectedObject->modelMatrix(),
                                                m_scene.camera());
        }

        vkCmdEndRenderPass(commandBufferOverlay);
        vkEndCommandBuffer(commandBufferOverlay);

        /* Submit post command buffer */
        VkSemaphore waitSemaphores[] = {m_semaphoreDeferredFinished[m_currentFrame]};
        VkSemaphore signalSemaphores[] = {m_semaphoreOverlayFinished[m_currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo submitInfo = vkinit::submitInfo(1, &commandBufferOverlay);
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        VULKAN_CHECK(vkQueueSubmit(m_vkctx.renderQueue(), 1, &submitInfo, VK_NULL_HANDLE));
    }

    /* Output pass */
    {
        VkCommandBuffer &commandBufferOutput = m_commandBufferOutput[m_currentFrame];
        VULKAN_CHECK(vkResetCommandBuffer(commandBufferOutput, 0));

        VkCommandBufferBeginInfo beginInfo = vkinit::commandBufferBeginInfo();
        VULKAN_CHECK_CRITICAL(vkBeginCommandBuffer(commandBufferOutput, &beginInfo));

        /* Copy Gbuffer 1 attachment to selection iamge */
        {
            VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            /* Transition attachment to transfer source optimal */
            transitionImageLayout(commandBufferOutput,
                                  m_attachmentGBuffer1.image(imageIndex).image(),
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                  subresourceRange);

            /* Copy images */
            VkImageCopy copyRegion{};
            copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            copyRegion.srcOffset = {0, 0, 0};
            copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            copyRegion.dstOffset = {0, 0, 0};
            copyRegion.extent = {m_swapchain.extent().width, m_swapchain.extent().height, 1};
            vkCmdCopyImage(commandBufferOutput,
                           m_attachmentGBuffer1.image(imageIndex).image(),
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           m_imageSelection.image(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &copyRegion);
        }

        /* Render internal color target to swapchain image */
        {
            VkClearValue clearValue;
            clearValue.color = {0, 0, 0, 0};
            VkRenderPassBeginInfo rpBeginInfo = vkinit::renderPassBeginInfo(
                m_renderPassOutput.renderPass(), m_framebufferOutput.framebuffer(imageIndex), 1, &clearValue);
            rpBeginInfo.renderArea.extent = m_swapchain.extent();
            vkCmdBeginRenderPass(commandBufferOutput, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            m_rendererOutput.render(commandBufferOutput, m_scene.descriptorSetSceneData(imageIndex), imageIndex);

            vkCmdEndRenderPass(commandBufferOutput);
            vkEndCommandBuffer(commandBufferOutput);
        }

        /* Submit output command buffer */
        VkSemaphore waitSemaphores[] = {m_semaphoreOverlayFinished[m_currentFrame]};
        VkSemaphore signalSemaphores[] = {m_semaphoreOutputFinished[m_currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
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

ID VulkanRenderer::findID(float x, float y)
{
    /* Get the texel color at this row and column of the highlight render target */
    uint32_t row = static_cast<uint32_t>(y * m_swapchain.extent().height);
    uint32_t column = static_cast<uint32_t>(x * m_swapchain.extent().width);

    /* Since the image is stored in linear tiling, get the subresource layout to calculate the padding */
    VkImageSubresource subResource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
    VkSubresourceLayout subResourceLayout;
    vkGetImageSubresourceLayout(m_vkctx.device(), m_imageSelection.image(), &subResource, &subResourceLayout);

    /* Byte index of x,y texel */
    uint32_t index = (row * static_cast<uint32_t>(subResourceLayout.rowPitch) + column * 4 * sizeof(float));

    /* Store highlight render result for that texel to the disk */
    float *gbuffer1Tex;
    VkResult res = vkMapMemory(m_vkctx.device(),
                               m_imageSelection.memory(),
                               subResourceLayout.offset + index,
                               4 * sizeof(float),
                               0,
                               reinterpret_cast<void **>(&gbuffer1Tex));
    float id;
    memcpy(&id, gbuffer1Tex + 3, sizeof(float));
    vkUnmapMemory(m_vkctx.device(), m_imageSelection.memory());

    return static_cast<ID>(id);
}

VkResult VulkanRenderer::createRenderPasses()
{
    VULKAN_CHECK_CRITICAL(m_renderPassDeferred.initResources(
        m_formatGBuffer1, m_formatGBuffer2, m_swapchain.depthFormat(), m_formatInternalColor, m_swapchain.imageCount()));
    VULKAN_CHECK_CRITICAL(m_renderPassOverlay.init(m_vkctx, m_formatInternalColor, m_formatGBuffer1, m_swapchain.depthFormat()));
    VULKAN_CHECK_CRITICAL(m_renderPassOutput.init(m_vkctx, m_swapchain.format()));

    return VK_SUCCESS;
}

VkResult VulkanRenderer::createFrameBuffers()
{
    uint32_t swapchainImages = m_swapchain.imageCount();
    VkExtent2D swapchainExtent = m_swapchain.extent();

    /* Initialize frame buffer attachments */
    VULKAN_CHECK_CRITICAL(m_attachmentInternalColor.init(
        m_vkctx,
        VulkanFrameBufferAttachmentInfo(
            "Internal color", swapchainExtent.width, swapchainExtent.height, m_formatInternalColor, VK_SAMPLE_COUNT_1_BIT),
        swapchainImages));
    VULKAN_CHECK_CRITICAL(m_attachmentGBuffer1.init(
        m_vkctx,
        VulkanFrameBufferAttachmentInfo("GBuffer 1",
                                        swapchainExtent.width,
                                        swapchainExtent.height,
                                        m_formatGBuffer1,
                                        VK_SAMPLE_COUNT_1_BIT,
                                        VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT),
        swapchainImages));
    VULKAN_CHECK_CRITICAL(m_attachmentGBuffer2.init(m_vkctx,
                                                    VulkanFrameBufferAttachmentInfo("GBuffer 2",
                                                                                    swapchainExtent.width,
                                                                                    swapchainExtent.height,
                                                                                    m_formatGBuffer2,
                                                                                    VK_SAMPLE_COUNT_1_BIT,
                                                                                    VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT),
                                                    swapchainImages));

    /* Create frame buffers */
    /* Deferred frame buffer */
    {
        std::vector<VulkanFrameBufferAttachment> deferredAttachments = {
            m_attachmentGBuffer1, m_attachmentGBuffer2, m_swapchain.depthAttachment(), m_attachmentInternalColor};

        VULKAN_CHECK_CRITICAL(m_frameBufferDeferred.create(m_vkctx,
                                                           swapchainImages,
                                                           m_renderPassDeferred.renderPass(),
                                                           swapchainExtent.width,
                                                           swapchainExtent.height,
                                                           deferredAttachments));
    }

    /* Connect frame buffer attachment with the render pass */
    m_renderPassDeferred.updateDescriptors(m_attachmentGBuffer1, m_attachmentGBuffer2, m_swapchain.depthAttachment());

    /* Post frame buffer */
    // {
    //     std::vector<VulkanFrameBufferAttachment> postAttachments = {swapchainAttachment};
    //     VULKAN_CHECK_CRITICAL(m_framebufferPost.create(
    //         m_vkctx, swapchainImages, m_renderPassPost.renderPass(), swapchainExtent.width, swapchainExtent.height,
    //         postAttachments));
    // }

    /* Overlay framebuffer */
    {
        std::vector<VulkanFrameBufferAttachment> overlayAttachments = {
            m_attachmentInternalColor, m_attachmentGBuffer1, m_swapchain.depthAttachment()};

        VULKAN_CHECK_CRITICAL(m_framebufferOverlay.create(m_vkctx,
                                                          swapchainImages,
                                                          m_renderPassOverlay.renderPass(),
                                                          swapchainExtent.width,
                                                          swapchainExtent.height,
                                                          overlayAttachments));
    }

    /* Output framebuffer */
    {
        std::vector<VulkanFrameBufferAttachment> outputAttachments = {m_swapchain.swapchainAttachment()};

        VULKAN_CHECK_CRITICAL(m_framebufferOutput.create(m_vkctx,
                                                         swapchainImages,
                                                         m_renderPassOutput.renderPass(),
                                                         swapchainExtent.width,
                                                         swapchainExtent.height,
                                                         outputAttachments));
    }

    return VK_SUCCESS;
}

VkResult VulkanRenderer::createCommandBuffers()
{
    m_commandBufferDeferred.resize(MAX_FRAMES_IN_FLIGHT);
    // m_commandBufferPost.resize(MAX_FRAMES_IN_FLIGHT);
    m_commandBufferOverlay.resize(MAX_FRAMES_IN_FLIGHT);
    m_commandBufferOutput.resize(MAX_FRAMES_IN_FLIGHT);

    {
        VkCommandBufferAllocateInfo allocInfo = vkinit::commandBufferAllocateInfo(
            VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_vkctx.renderCommandPool(), static_cast<uint32_t>(m_commandBufferDeferred.size()));
        VULKAN_CHECK_CRITICAL(vkAllocateCommandBuffers(m_vkctx.device(), &allocInfo, m_commandBufferDeferred.data()));
    }
    // {
    //     VkCommandBufferAllocateInfo allocInfo = vkinit::commandBufferAllocateInfo(
    //         VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_vkctx.renderCommandPool(), static_cast<uint32_t>(m_commandBufferPost.size()));
    //     VULKAN_CHECK_CRITICAL(vkAllocateCommandBuffers(m_vkctx.device(), &allocInfo, m_commandBufferPost.data()));
    // }
    {
        VkCommandBufferAllocateInfo allocInfo = vkinit::commandBufferAllocateInfo(
            VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_vkctx.renderCommandPool(), static_cast<uint32_t>(m_commandBufferOverlay.size()));
        VULKAN_CHECK_CRITICAL(vkAllocateCommandBuffers(m_vkctx.device(), &allocInfo, m_commandBufferOverlay.data()));
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
    m_semaphoreDeferredFinished.resize(MAX_FRAMES_IN_FLIGHT);
    // m_semaphorePostFinished.resize(MAX_FRAMES_IN_FLIGHT);
    m_semaphoreOverlayFinished.resize(MAX_FRAMES_IN_FLIGHT);
    m_semaphoreOutputFinished.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    for (uint32_t f = 0; f < MAX_FRAMES_IN_FLIGHT; f++) {
        VULKAN_CHECK_CRITICAL(vkCreateSemaphore(m_vkctx.device(), &semaphoreInfo, nullptr, &m_semaphoreImageAvailable[f]));
        VULKAN_CHECK_CRITICAL(vkCreateSemaphore(m_vkctx.device(), &semaphoreInfo, nullptr, &m_semaphoreDeferredFinished[f]));
        // VULKAN_CHECK_CRITICAL(vkCreateSemaphore(m_vkctx.device(), &semaphoreInfo, nullptr, &m_semaphorePostFinished[f]));
        VULKAN_CHECK_CRITICAL(vkCreateSemaphore(m_vkctx.device(), &semaphoreInfo, nullptr, &m_semaphoreOverlayFinished[f]));
        VULKAN_CHECK_CRITICAL(vkCreateSemaphore(m_vkctx.device(), &semaphoreInfo, nullptr, &m_semaphoreOutputFinished[f]));
        VULKAN_CHECK_CRITICAL(vkCreateFence(m_vkctx.device(), &fenceInfo, nullptr, &m_fenceInFlight[f]));
    }

    return VK_SUCCESS;
}

VkResult VulkanRenderer::createSelectionImage()
{
    /* Create image used to copy gbuffer 1 result from gpu memory to cpu memory */
    VkImageCreateInfo imageInfo = vkinit::imageCreateInfo({m_swapchain.extent().width, m_swapchain.extent().height, 1},
                                                          m_formatGBuffer1,
                                                          1,
                                                          VK_SAMPLE_COUNT_1_BIT,
                                                          VK_IMAGE_TILING_LINEAR,
                                                          VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    VULKAN_CHECK_CRITICAL(createImage(m_vkctx.physicalDevice(),
                                      m_vkctx.device(),
                                      imageInfo,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                      m_imageSelection.image(),
                                      m_imageSelection.memory()));

    /* Transition image to the approriate layout ready for render */
    VkCommandBuffer cmdBuf;
    VULKAN_CHECK_CRITICAL(beginSingleTimeCommands(m_vkctx.device(), m_vkctx.graphicsCommandPool(), cmdBuf));

    transitionImageLayout(cmdBuf,
                          m_imageSelection.image(),
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    VULKAN_CHECK_CRITICAL(endSingleTimeCommands(m_vkctx.device(), m_vkctx.graphicsCommandPool(), m_vkctx.graphicsQueue(), cmdBuf));

    return VK_SUCCESS;
}

}  // namespace vengine