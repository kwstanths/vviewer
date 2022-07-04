#ifndef __VulkanRenderer_hpp__
#define __VulkanRenderer_hpp__

#include <optional>
#include <vector>

#include <qvulkanwindow.h>
#include <qvulkanfunctions.h>

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
    
    bool createVulkanMeshModel(std::string filename);

    VulkanScene* getActiveScene() const;

    Texture* createTexture(std::string imagePath, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, bool keepImage = false);
    Texture* createTexture(std::string id, Image<stbi_uc>* image, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
    Texture* createTextureHDR(std::string imagePath, bool keepImage = false);
    Cubemap* createCubemap(std::string directory);
    EnvironmentMap* createEnvironmentMap(std::string imagePath, bool keepTexture = false);
    Material* createMaterial(std::string name, MaterialType type, bool createDescriptors = true);

    void setSelectedNode(std::shared_ptr<SceneNode> sceneNode);

    void renderRT();

private:

    bool createDebugCallback();
    void destroyDebugCallback();

    bool pickPhysicalDevice();
    bool isPhysicalDeviceSuitable(VkPhysicalDeviceProperties device);

    /* Graphics pipeline */
    bool createRenderPasses();
    bool createFrameBuffers();

    /* Descriptor resources */
    bool createDescriptorSetsLayouts();
    bool createUniformBuffers();
    bool updateUniformBuffers(size_t index);
    bool createDescriptorPool(size_t nMaterials);
    bool createDescriptorSets();
    
private:
    /* Qt vulkan data */
    QVulkanWindow * m_window;
    QVulkanFunctions * m_functions;
    QVulkanDeviceFunctions * m_devFunctions;
    
    /* Swpachain data */
    std::vector<VkFramebuffer> m_framebuffersForward;
    std::vector<VkFramebuffer> m_framebuffersPost;
    VkExtent2D m_swapchainExtent;
    VkFormat m_swapchainFormat;
    VkFormat m_internalRenderFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
    std::vector<VkImage> m_highlightImages;
    std::vector<VkImageView> m_highlightImageViews;
    std::vector<VkDeviceMemory> m_highlightDeviceMemory;
    std::vector<VkImage> m_colorImages;
    std::vector<VkImageView> m_colorImageViews;
    std::vector<VkDeviceMemory> m_colorDeviceMemory;

    /* Device data */
    VkDebugUtilsMessengerEXT m_debugCallback;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkPhysicalDeviceProperties m_physicalDeviceProperties;

    /* Renderers */
    VkRenderPass m_renderPassForward;
    VkRenderPass m_renderPassPost;
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

    /* Dynamic uniform buffer object to hold material data, and current index */
    VulkanDynamicUBO<MaterialData> m_materialsUBO;
    size_t m_materialsIndexUBO = 0;

    glm::vec4 m_clearColor = glm::vec4(0, 0.5, 0.5, 1);

    std::shared_ptr<SceneNode> m_selectedNode = nullptr;
};

#endif