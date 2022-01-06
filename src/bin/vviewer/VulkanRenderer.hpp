#ifndef __VulkanRenderer_hpp__
#define __VulkanRenderer_hpp__

#include <optional>
#include <vector>

#include <qvulkanwindow.h>
#include <qvulkanfunctions.h>

#include <utils/Console.hpp>

#include "vulkan/IncludeVulkan.hpp"
#include "vulkan/VulkanDataStructs.hpp"
#include "vulkan/VulkanDynamicUBO.hpp"
#include "vulkan/VulkanMesh.hpp"
#include "vulkan/VulkanUtils.hpp"
#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanMaterials.hpp"
#include "vulkan/VulkanTexture.hpp"
#include "vulkan/VulkanRendererPBR.hpp"
#include "vulkan/VulkanRendererSkybox.hpp"

#include "core/MeshModel.hpp"
#include "core/Camera.hpp"
#include "core/AssetManager.hpp"
#include "core/Materials.hpp"
#include "core/EnvironmentMap.hpp"
#include "core/Lights.hpp"

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) 
{
    //std::cerr << "Debug callback: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

class VulkanRenderer : public QVulkanWindowRenderer {
public:
    VulkanRenderer(QVulkanWindow *w) : m_window(w) { }

    void preInitResources() override;
    void initResources() override;
    void initSwapChainResources() override;

    void releaseSwapChainResources() override;
    void releaseResources() override;

    void startNextFrame() override;

    void setCamera(std::shared_ptr<Camera> camera);
    void setDirectionalLight(std::shared_ptr<DirectionalLight> light);

    bool createVulkanMeshModel(std::string filename);


    SceneObject * addSceneObject(std::string meshModel, Transform transform, std::string material);

    Texture* createTexture(std::string imagePath, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
    Texture* createTexture(std::string id, Image<stbi_uc>* image, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
    Texture* createTextureHDR(std::string imagePath);
    Cubemap* createCubemap(std::string directory);
    EnvironmentMap* createEnvironmentMap(std::string imagePath);
    Material* createMaterial(std::string name,
        glm::vec4 albedo, float metallic, float roughness, float ao, float emissive,
        bool createDescriptors = true
    );

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

    VulkanMaterialSkybox * m_skybox = nullptr;

    /* Descriptor data */
    VkDescriptorSetLayout m_descriptorSetLayoutScene;
    VkDescriptorSetLayout m_descriptorSetLayoutModel;
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSetsScene;
    std::vector<VkBuffer> m_uniformBuffersScene;
    std::vector<VkDeviceMemory> m_uniformBuffersSceneMemory;
    std::vector<VkDescriptorSet> m_descriptorSetsModel;
    VulkanDynamicUBO<ModelData> m_modelDataDynamicUBO;
    size_t m_transformIndexUBO = 0;
    /* This probably needs to be moved to the VulkanRendererPBR */
    VulkanDynamicUBO<MaterialPBRData> m_materialsUBO;
    size_t m_materialsIndexUBO = 0;

    std::vector<VulkanSceneObject *> m_objects;
    std::shared_ptr<Camera> m_camera;
    std::shared_ptr<DirectionalLight> m_directionalLight;
    glm::vec4 m_clearColor = glm::vec4(0, 0.5, 0.5, 1);
};

#endif