#ifndef __VulkanRenderer_hpp__
#define __VulkanRenderer_hpp__

#include <optional>
#include <vector>
#include <chrono>
#include <thread>

#include <debug_tools/Console.hpp>

#include "core/Model3D.hpp"
#include "core/Camera.hpp"
#include "core/AssetManager.hpp"
#include "core/Materials.hpp"
#include "core/EnvironmentMap.hpp"
#include "core/Light.hpp"
#include "core/Renderer.hpp"

#include "vulkan/VulkanContext.hpp"
#include "vulkan/VulkanSwapchain.hpp"
#include "vulkan/VulkanScene.hpp"
#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanFramebuffer.hpp"
#include "vulkan/VulkanRenderPass.hpp"
#include "vulkan/common/VulkanUtils.hpp"
#include "vulkan/common/VulkanStructs.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/renderers/VulkanRendererLightComposition.hpp"
#include "vulkan/renderers/VulkanRendererPBRStandard.hpp"
#include "vulkan/renderers/VulkanRendererLambert.hpp"
#include "vulkan/renderers/VulkanRendererSkybox.hpp"
#include "vulkan/renderers/VulkanRendererOverlay.hpp"
#include "vulkan/renderers/VulkanRendererPost.hpp"
#include "vulkan/renderers/VulkanRendererOutput.hpp"
#include "vulkan/renderers/VulkanRendererPathTracing.hpp"
#include "vulkan/resources/VulkanMesh.hpp"
#include "vulkan/resources/VulkanMaterials.hpp"
#include "vulkan/resources/VulkanTextures.hpp"
#include "vulkan/resources/VulkanRandom.hpp"

namespace vengine
{

/* Main renderer */
class VulkanRenderer : public Renderer
{
public:
    VulkanRenderer(VulkanContext &context,
                   VulkanSwapchain &swapchain,
                   VulkanTextures &textures,
                   VulkanMaterials &materials,
                   VulkanScene &scene);

    VkResult initResources();
    VkResult initSwapChainResources();

    VkResult releaseSwapChainResources();
    VkResult releaseResources();

    RendererPathTracing &rendererPathTracing() override;
    VulkanRendererSkybox &rendererSkybox() { return m_rendererSkybox; }

    /* Find the id of the object that is renderd in x, y pixel coordinates */
    ID findID(float x, float y);

    VkResult renderFrame();

    /* Blocks and waits for the renderer to idle, stop the renderer before waiting here */
    void waitIdle();

private:
    /* Graphics pipeline */
    VkResult createRenderPasses();
    VkResult createFrameBuffers();
    VkResult createCommandBuffers();
    VkResult createSyncObjects();

    VkResult createSelectionImage();

    /* Render */
    VkResult buildFrame(uint32_t imageIndex);

private:
    VulkanContext &m_vkctx;
    VulkanSwapchain &m_swapchain;
    VulkanMaterials &m_materials;
    VulkanTextures &m_textures;
    VulkanScene &m_scene;
    VulkanRandom m_random;

    /* Render passes and framebuffers */
    VkFormat m_formatInternalColor = VK_FORMAT_R32G32B32A32_SFLOAT;
    VkFormat m_formatGBuffer1 = VK_FORMAT_R32G32B32A32_SFLOAT;
    VkFormat m_formatGBuffer2 = VK_FORMAT_R16G16B16A16_UINT;
    VulkanRenderPassDeferred m_renderPassDeferred;
    VulkanRenderPassOverlay m_renderPassOverlay;
    VulkanRenderPassOutput m_renderPassOutput;

    VulkanFrameBufferAttachment m_attachmentInternalColor;
    VulkanFrameBufferAttachment m_attachmentGBuffer1;
    VulkanFrameBufferAttachment m_attachmentGBuffer2;
    VulkanFrameBuffer m_frameBufferDeferred;
    VulkanFrameBuffer m_framebufferOverlay;
    VulkanFrameBuffer m_framebufferOutput;

    /* Object selection image */
    VulkanImage m_imageSelection;

    /* Renderers */
    VulkanRendererLightComposition m_rendererLightComposition;
    VulkanRendererGBuffer m_rendererGBuffer;
    VulkanRendererPBR m_rendererPBR;
    VulkanRendererLambert m_rendererLambert;
    VulkanRendererSkybox m_rendererSkybox;
    VulkanRendererOverlay m_rendererOverlay;
    VulkanRendererOutput m_rendererOutput;
    VulkanRendererPathTracing m_rendererPathTracing;

    /* Command buffers and synchronization data */
    std::vector<VkSemaphore> m_semaphoreImageAvailable;
    std::vector<VkFence> m_fenceInFlight;
    const uint32_t MAX_FRAMES_IN_FLIGHT = 3;
    uint32_t m_currentFrame = 0;

    std::vector<VkCommandBuffer> m_commandBufferDeferred;
    std::vector<VkSemaphore> m_semaphoreDeferredFinished;
    std::vector<VkCommandBuffer> m_commandBufferOverlay;
    std::vector<VkSemaphore> m_semaphoreOverlayFinished;
    std::vector<VkCommandBuffer> m_commandBufferOutput;
    std::vector<VkSemaphore> m_semaphoreOutputFinished;
};

}  // namespace vengine

#endif