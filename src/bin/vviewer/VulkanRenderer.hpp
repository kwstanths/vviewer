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

    bool createVulkanMeshModel(std::string filename);

    SceneObject * addSceneObject(std::string meshModel, Transform transform, std::string material);
    Material * createMaterial(std::string name, 
        glm::vec4 albedo, float metallic, float roughness, float ao, float emissive,
        bool createDescriptors = true
    );
    Texture * createTexture(std::string imagePath, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
    Texture * createTexture(std::string id, Image * image, VkFormat format);
    Cubemap * createCubemap(std::string directory);

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

    /* Graphics pipeline */
    VkRenderPass m_renderPass;

    /* Renderers */
    VulkanRendererPBR m_rendererPBR;
    VulkanRendererSkybox m_rendererSkybox;

    VulkanMaterialSkybox * m_skybox = nullptr;

    /* Descriptor data */
    VkDescriptorSetLayout m_descriptorSetLayoutCamera;
    VkDescriptorSetLayout m_descriptorSetLayoutModel;
    std::vector<VkDescriptorSet> m_descriptorSetsCamera;
    std::vector<VkDescriptorSet> m_descriptorSetsModel;
    VkDescriptorPool m_descriptorPool;
    std::vector<VkBuffer> m_uniformBuffersCamera;
    std::vector<VkDeviceMemory> m_uniformBuffersCameraMemory;
    VulkanDynamicUBO<ModelData> m_modelDataDynamicUBO;
    size_t m_transformIndexUBO = 0;
    VulkanDynamicUBO<MaterialPBRData> m_materialsUBO;
    size_t m_materialsIndexUBO = 0;

    std::vector<VulkanSceneObject *> m_objects;

    std::shared_ptr<Camera> m_camera;

    glm::vec4 m_clearColor = glm::vec4(0, 0.5, 0.5, 1);
};

#endif