#ifndef __VulkanRenderer_hpp__
#define __VulkanRenderer_hpp__

#include <optional>
#include <vector>

#include <qvulkanwindow.h>
#include <qvulkanfunctions.h>
#include <qfuturewatcher.h>

#include <utils/Console.hpp>

#include "core/MeshModel.hpp"
#include "core/Camera.hpp"
#include "core/AssetManager.hpp"
#include "core/Materials.hpp"
#include "core/EnvironmentMap.hpp"
#include "core/Lights.hpp"

#include "vulkan/IncludeVulkan.hpp"
#include "vulkan/VulkanDataStructs.hpp"
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

class VulkanRenderer : public QVulkanWindowRenderer {
public:
    VulkanRenderer(QVulkanWindow* w, VulkanScene* scene);

    void preInitResources() override;
    void initResources() override;
    void initSwapChainResources() override;

    void releaseSwapChainResources() override;
    void releaseResources() override;

    void startNextFrame() override;
    
    MeshModel * createVulkanMeshModel(std::string filename);

    VulkanScene* getActiveScene() const;

    Texture* createTexture(std::string imagePath, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, bool keepImage = false);
    Texture* createTexture(std::string id, Image<stbi_uc>* image, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
    Texture* createTextureHDR(std::string imagePath, bool keepImage = false);
    Cubemap* createCubemap(std::string directory);
    EnvironmentMap* createEnvironmentMap(std::string imagePath, bool keepTexture = false);
    Material* createMaterial(std::string name, MaterialType type, bool createDescriptors = true);

    void setSelectedObject(std::shared_ptr<SceneObject> sceneObject);
    std::shared_ptr<SceneObject> getSelectedObject() const;

    bool isRTEnabled() const;
    void renderRT();

    glm::vec3 selectObject(float x, float y);

private:

    bool createDebugCallback();
    void destroyDebugCallback();

    bool pickPhysicalDevice();
    bool isPhysicalDeviceSuitable(const VkPhysicalDeviceProperties& deviceProperties);
    VkSampleCountFlagBits getMaxUsableSampleCount(const VkPhysicalDeviceProperties& deviceProperties);

    /* Graphics pipeline */
    bool createRenderPasses();
    bool createFrameBuffers();

    /* Descriptor resources */
    bool createDescriptorSetsLayouts();
    bool createUniformBuffers();
    bool updateUniformBuffers(size_t index);
    bool createDescriptorPool(size_t nMaterials);
    bool createDescriptorSets();
    bool createColorSelectionTempImage();
    
    /* Render */
    void buildFrame();

private:
    /* Qt vulkan data */
    QVulkanWindow * m_window;
    QVulkanFunctions * m_functions;
    QVulkanDeviceFunctions * m_devFunctions;
    /* For concurrent frame render */
    QFutureWatcher<void> m_frameWatcher;
    bool m_framePending = false;
    
    /* Swpachain data */
    VkExtent2D m_swapchainExtent;
    VkFormat m_swapchainFormat;

    /* Device data */
    VkDebugUtilsMessengerEXT m_debugCallback;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkPhysicalDeviceProperties m_physicalDeviceProperties;
    VkSampleCountFlagBits m_msaaSamples;

    /* Render pipeline data  */
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

    /* Active scene */
    VulkanScene * m_scene = nullptr;

    /* Descriptor data */
    VkDescriptorSetLayout m_descriptorSetLayoutScene;
    VkDescriptorSetLayout m_descriptorSetLayoutModel;
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSetsScene;
    std::vector<VkDescriptorSet> m_descriptorSetsModel;

    /* Dynamic uniform buffer object to hold material data */
    VulkanDynamicUBO<MaterialData> m_materialsUBO;

    std::shared_ptr<SceneObject> m_selectedObject = nullptr;
};

#endif