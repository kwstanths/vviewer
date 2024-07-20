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
#include "vulkan/renderers/VulkanRendererPBRStandard.hpp"
#include "vulkan/renderers/VulkanRendererLambert.hpp"
#include "vulkan/renderers/VulkanRendererSkybox.hpp"
#include "vulkan/renderers/VulkanRendererPost.hpp"
#include "vulkan/renderers/VulkanRenderer3DUI.hpp"
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

    glm::vec3 selectObject(float x, float y);

    VulkanRendererSkybox &getRendererSkybox() { return m_rendererSkybox; }

    VkResult renderFrame(SceneGraph &sceneGraphArray);

    /* Blocks and waits for the renderer to idle, stop the renderer before waiting here */
    void waitIdle();

private:
    /* Graphics pipeline */
    VkResult createRenderPasses();
    VkResult createFrameBuffers();
    VkResult createCommandBuffers();
    VkResult createSyncObjects();

    /* Descriptor resources */
    VkResult createColorSelectionTempImage();

    /* Render */
    VkResult buildFrame(SceneGraph &sceneGraphArray, uint32_t imageIndex);

private:
    VulkanContext &m_vkctx;
    VulkanSwapchain &m_swapchain;
    VulkanMaterials &m_materials;
    VulkanTextures &m_textures;
    VulkanScene &m_scene;
    VulkanRandom m_random;

    /* Render passes and framebuffers */
    VkFormat m_internalRenderFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
    VulkanRenderPassForward m_renderPassForward;
    VulkanRenderPassPost m_renderPassPost;
    VulkanRenderPassUI m_renderPassUI;

    VulkanFrameBufferAttachment m_attachmentColorForwardOutput;
    VulkanFrameBufferAttachment m_attachmentHighlightForwardOutput;
    VulkanFrameBuffer m_frameBufferForward;
    VulkanFrameBuffer m_framebufferPost;
    VulkanFrameBuffer m_framebufferUI;

    VulkanImage m_imageTempColorSelection;

    /* Renderers */
    VulkanRendererPBR m_rendererPBR;
    VulkanRendererLambert m_rendererLambert;
    VulkanRendererSkybox m_rendererSkybox;
    VulkanRendererPost m_rendererPost;
    VulkanRenderer3DUI m_renderer3DUI;
    VulkanRendererPathTracing m_rendererPathTracing;

    /* Command buffers and synchronization data */
    std::vector<VkSemaphore> m_semaphoreImageAvailable;
    std::vector<VkFence> m_fenceInFlight;
    const uint32_t MAX_FRAMES_IN_FLIGHT = 3;
    uint32_t m_currentFrame = 0;

    std::vector<VkCommandBuffer> m_commandBufferForward;
    std::vector<VkSemaphore> m_semaphoreForwardFinished;
    std::vector<VkCommandBuffer> m_commandBufferPost;
    std::vector<VkSemaphore> m_semaphorePostFinished;
    std::vector<VkCommandBuffer> m_commandBufferUI;
    std::vector<VkSemaphore> m_semaphoreUIFinished;
    std::vector<VkCommandBuffer> m_commandBufferOutput;
    std::vector<VkSemaphore> m_semaphoreOutputFinished;
};

}  // namespace vengine

#endif