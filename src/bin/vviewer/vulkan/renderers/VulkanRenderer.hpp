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

#include "vulkan/IncludeVulkan.hpp"
#include "vulkan/VulkanCore.hpp"
#include "vulkan/VulkanSwapchain.hpp"
#include "vulkan/VulkanStructs.hpp"
#include "vulkan/VulkanScene.hpp"
#include "vulkan/VulkanMesh.hpp"
#include "vulkan/VulkanUtils.hpp"
#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanMaterials.hpp"
#include "vulkan/VulkanTexture.hpp"
#include "vulkan/VulkanFramebuffer.hpp"
#include "VulkanRendererPBRStandard.hpp"
#include "VulkanRendererLambert.hpp"
#include "VulkanRendererSkybox.hpp"
#include "VulkanRendererPost.hpp"
#include "VulkanRenderer3DUI.hpp"
#include "VulkanRendererRayTracing.hpp"


/* Main renderer */
class VulkanRenderer {
public:
    enum class STATUS {
        RUNNING,
        IDLE,
        STOPPED,
    };

    VulkanRenderer(VulkanCore& vkcore, VulkanScene* scene);

    void initResources();
    void initSwapChainResources(VulkanSwapchain * swapchain);

    void releaseSwapChainResources();
    void releaseResources();

    void renderFrame();
    
    VulkanScene* getActiveScene() const;

    void setSelectedObject(std::shared_ptr<SceneObject> sceneObject);
    std::shared_ptr<SceneObject> getSelectedObject() const;

    bool isRTEnabled() const;
    int64_t renderRT();

    glm::vec3 selectObject(float x, float y);

    float deltaTime() const;

    VulkanRendererRayTracing * getRayTracingRenderer() { return &m_rendererRayTracing; }

    std::shared_ptr<MeshModel> createVulkanMeshModel(std::string filename);
    std::shared_ptr<Texture> createTexture(std::string imagePath, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, bool keepImage = false);
    std::shared_ptr<Texture> createTexture(std::string id, std::shared_ptr<Image<stbi_uc>> image, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
    std::shared_ptr<Texture> createTextureHDR(std::string imagePath, bool keepImage = false);
    std::shared_ptr<Cubemap> createCubemap(std::string directory);
    std::shared_ptr<EnvironmentMap> createEnvironmentMap(std::string imagePath, bool keepTexture = false);
    std::shared_ptr<Material> createMaterial(std::string name, MaterialType type, bool createDescriptors = true);

    void waitIdle();
    void startRenderLoop();
    void renderLoopActive(bool active);
    void exitRenderLoop();

private:
    /* Graphics pipeline */
    bool createRenderPasses();
    bool createFrameBuffers();
    bool createCommandBuffers();
    bool createSyncObjects();

    /* Descriptor resources */
    bool createDescriptorSetsLayouts();
    bool createUniformBuffers();
    bool updateUniformBuffers(size_t imageIndex);
    bool createDescriptorPool(size_t nMaterials);
    bool createDescriptorSets();
    bool createColorSelectionTempImage();
    
    /* Render */
    void buildFrame(uint32_t imageIndex, VkCommandBuffer commandBuffer);

private:
    VulkanCore& m_vkcore;
    VulkanSwapchain * m_swapchain = VK_NULL_HANDLE;

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

    /* Active scene */
    VulkanScene * m_scene = nullptr;
    std::chrono::steady_clock::time_point m_frameTimePrev;
    float m_deltaTime = 0.016;

    /* Descriptor data */
    VkDescriptorSetLayout m_descriptorSetLayoutScene;
    VkDescriptorSetLayout m_descriptorSetLayoutModel;
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSetsScene;
    std::vector<VkDescriptorSet> m_descriptorSetsModel;

    /* Dynamic uniform buffer object to hold material data */
    std::shared_ptr<VulkanDynamicUBO<MaterialData>> m_materialsUBO;

    std::shared_ptr<SceneObject> m_selectedObject = nullptr;

    /* Render loop data */
    std::thread m_renderThread;
    bool m_renderLoopActive = false;
    bool m_renderLoopExit = false;
    STATUS m_status = STATUS::IDLE;
};

#endif