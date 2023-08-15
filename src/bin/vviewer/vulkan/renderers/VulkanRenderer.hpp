#ifndef __VulkanRenderer_hpp__
#define __VulkanRenderer_hpp__

#include <optional>
#include <vector>
#include <chrono>
#include <thread>

#include <qfuturewatcher.h>

#include <utils/Console.hpp>

#include "core/MeshModel.hpp"
#include "core/Camera.hpp"
#include "core/AssetManager.hpp"
#include "core/Materials.hpp"
#include "core/EnvironmentMap.hpp"
#include "core/Lights.hpp"
#include "core/Renderer.hpp"

#include "vulkan/IncludeVulkan.hpp"
#include "vulkan/VulkanContext.hpp"
#include "vulkan/VulkanSwapchain.hpp"
#include "vulkan/VulkanStructs.hpp"
#include "vulkan/VulkanScene.hpp"
#include "vulkan/VulkanUtils.hpp"
#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanMaterials.hpp"
#include "vulkan/VulkanFramebuffer.hpp"
#include "VulkanRendererPBRStandard.hpp"
#include "VulkanRendererLambert.hpp"
#include "VulkanRendererSkybox.hpp"
#include "VulkanRendererPost.hpp"
#include "VulkanRenderer3DUI.hpp"
#include "VulkanRendererRayTracing.hpp"
#include "vulkan/resources/VulkanMesh.hpp"
#include "vulkan/resources/VulkanTexture.hpp"
#include "vulkan/resources/VulkanMaterials.hpp"
#include "vulkan/resources/VulkanTextures.hpp"

/* Main renderer */
class VulkanRenderer : public Renderer {
public:
    VulkanRenderer(VulkanContext& context, 
        VulkanSwapchain& swapchain,
        VulkanTextures& textures, 
        VulkanMaterials& materials, 
        VulkanScene* scene);

    void initResources();
    void initSwapChainResources();

    void releaseSwapChainResources();
    void releaseResources();

    bool isRTEnabled() const;
    void renderRT();

    glm::vec3 selectObject(float x, float y);

    VulkanRendererRayTracing& getRayTracingRenderer() { return m_rendererRayTracing; }

    std::shared_ptr<MeshModel> createVulkanMeshModel(std::string filename);
    std::shared_ptr<Cubemap> createCubemap(std::string directory);
    std::shared_ptr<EnvironmentMap> createEnvironmentMap(std::string imagePath, bool keepTexture = false);

    VkResult renderFrame(SceneGraph& sceneGraphArray);

    /* Blocks and waits for the renderer to idle, stop the renderer before waiting here */
    void waitIdle();

private:
    /* Graphics pipeline */
    bool createRenderPasses();
    bool createFrameBuffers();
    bool createCommandBuffers();
    bool createSyncObjects();

    /* Descriptor resources */
    bool createColorSelectionTempImage();
    
    /* Render */
    void buildFrame(SceneGraph& sceneGraphArray, uint32_t imageIndex, VkCommandBuffer commandBuffer);

private:
    VulkanContext& m_vkctx;
    VulkanSwapchain& m_swapchain;
    VulkanMaterials& m_materials;
    VulkanTextures& m_textures;
    VulkanScene * m_scene = nullptr;

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

#endif