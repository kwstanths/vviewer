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

#include "IncludeVulkan.hpp"
#include "VulkanDataStructs.hpp"
#include "VulkanScene.hpp"
#include "VulkanMesh.hpp"
#include "VulkanUtils.hpp"
#include "VulkanSceneObject.hpp"
#include "VulkanMaterials.hpp"
#include "VulkanTexture.hpp"
#include "VulkanRendererPBR.hpp"
#include "VulkanRendererSkybox.hpp"
#include "VulkanRayTracingRenderer.hpp"

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

    Texture* createTexture(std::string imagePath, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
    Texture* createTexture(std::string id, Image<stbi_uc>* image, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
    Texture* createTextureHDR(std::string imagePath);
    Cubemap* createCubemap(std::string directory);
    EnvironmentMap* createEnvironmentMap(std::string imagePath, bool keepHDRTex = false);
    Material* createMaterial(std::string name,
        glm::vec4 albedo, float metallic, float roughness, float ao, float emissive,
        bool createDescriptors = true
    );

    void renderRT();

private:

    bool createDebugCallback();
    void destroyDebugCallback();

    bool pickPhysicalDevice();
    bool isPhysicalDeviceSuitable(VkPhysicalDeviceProperties device);

    /* Graphics pipeline */
    bool createRenderPass();
    bool createFrameBuffers();

    /* Descriptor resources */
    bool createDescriptorSetsLayouts();
    bool createUniformBuffers();
    bool updateUniformBuffers(size_t index);
    bool createDescriptorPool(size_t nMaterials);
    bool createDescriptorSets();
    
    /* */
    void destroyVulkanMeshModel(MeshModel model);

private:
    /* Qt vulkan data */
    QVulkanWindow * m_window;
    QVulkanFunctions * m_functions;
    QVulkanDeviceFunctions * m_devFunctions;
    
    /* Swpachain data */
    std::vector<VkFramebuffer> m_swapChainFramebuffers;
    VkExtent2D m_swapchainExtent;
    VkFormat m_swapchainFormat;

    /* Device data */
    VkDebugUtilsMessengerEXT m_debugCallback;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkPhysicalDeviceProperties m_physicalDeviceProperties;

    /* Renderers */
    VkRenderPass m_renderPass;
    VulkanRendererPBR m_rendererPBR;
    VulkanRendererSkybox m_rendererSkybox;
    VulkanRayTracingRenderer m_rendererRayTracing;

    /* Active scene */
    VulkanScene * m_scene = nullptr;

    /* Descriptor data */
    VkDescriptorSetLayout m_descriptorSetLayoutScene;
    VkDescriptorSetLayout m_descriptorSetLayoutModel;
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSetsScene;
    std::vector<VkDescriptorSet> m_descriptorSetsModel;

    glm::vec4 m_clearColor = glm::vec4(0, 0.5, 0.5, 1);
};

#endif