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
#include "vulkan/common/VulkanUtils.hpp"
#include "vulkan/common/VulkanStructs.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/renderers/VulkanRendererPBRStandard.hpp"
#include "vulkan/renderers/VulkanRendererLambert.hpp"
#include "vulkan/renderers/VulkanRendererSkybox.hpp"
#include "vulkan/renderers/VulkanRendererPost.hpp"
#include "vulkan/renderers/VulkanRenderer3DUI.hpp"
#include "vulkan/renderers/VulkanRendererRayTracing.hpp"
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

    RendererRayTracing &rendererRayTracing() override;

    glm::vec3 selectObject(float x, float y);

    VulkanRendererSkybox &getRendererSkybox() { return m_rendererSkybox; }

    Cubemap *createCubemap(std::string directory);

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
    VkResult buildFrame(SceneGraph &sceneGraphArray, uint32_t imageIndex, VkCommandBuffer commandBuffer);

private:
    VulkanContext &m_vkctx;
    VulkanSwapchain &m_swapchain;
    VulkanMaterials &m_materials;
    VulkanTextures &m_textures;
    VulkanScene &m_scene;
    VulkanRandom m_random;

    /* Render pass and framebuffers */
    VkRenderPass m_renderPassForward;
    VkRenderPass m_renderPassPost;
    VkRenderPass m_renderPassUI;
    std::vector<VkFramebuffer> m_framebuffersForward;
    std::vector<VkFramebuffer> m_framebuffersPost;
    std::vector<VkFramebuffer> m_framebuffersUI;
    VkFormat m_internalRenderFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
    std::vector<VulkanFrameBufferAttachment> m_attachmentColorForwardOutput;
    std::vector<VulkanFrameBufferAttachment> m_attachmentHighlightForwardOutput;
    StorageImage m_imageTempColorSelection;

    /* Renderers */
    VulkanRendererPBR m_rendererPBR;
    VulkanRendererLambert m_rendererLambert;
    VulkanRendererSkybox m_rendererSkybox;
    VulkanRendererPost m_rendererPost;
    VulkanRenderer3DUI m_renderer3DUI;
    VulkanRendererRayTracing m_rendererRayTracing;

    /* Render commands and synchronization data */
    std::vector<VkCommandBuffer> m_commandBuffer;
    std::vector<VkSemaphore> m_semaphoreImageAvailable;
    std::vector<VkSemaphore> m_semaphoreRenderFinished;
    std::vector<VkFence> m_fenceInFlight;
    const uint32_t MAX_FRAMES_IN_FLIGHT = 3;
    uint32_t m_currentFrame = 0;
};

}  // namespace vengine

#endif