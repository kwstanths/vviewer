#ifndef __VulkanRenderer_hpp__
#define __VulkanRenderer_hpp__

#include <optional>
#include <vector>

#include <qvulkanwindow.h>
#include <qvulkanfunctions.h>

#include <utils/Console.hpp>

#include "vulkan/IncludeVulkan.hpp"
#include "vulkan/VulkanDynamicUBO.hpp"
#include "vulkan/VulkanMesh.hpp"
#include "vulkan/Utils.hpp"

#include "core/MeshModel.hpp"
#include "Camera.hpp"

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) 
{
    //std::cerr << "Debug callback: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

struct CameraData {
    alignas(16) glm::mat4 m_view;
    alignas(16) glm::mat4 m_projection;
};

struct ModelData {
    alignas(16) glm::mat4 m_modelMatrix;
};

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

private:

    bool createDebugCallback();
    void destroyDebugCallback();

    bool pickPhysicalDevice();
    bool isPhysicalDeviceSuitable(VkPhysicalDeviceProperties device);

    /* Graphics pipeline */
    bool createRenderPass();
    bool createGraphicsPipeline();
    bool createFrameBuffers();

    /* Descriptor resources */
    bool createDescriptorSetsLayouts();
    bool createUniformBuffers();
    bool updateUniformBuffers(size_t index);
    bool createDescriptorPool();
    bool createDescriptorSets();

    /* Textures and images */
    bool createTextureImage();
    bool createTextureImageView();
    bool createTextureSampler();

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
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;

    /* Descriptor data */
    VkDescriptorSetLayout m_descriptorSetLayout;
    std::vector<VkBuffer> m_uniformBuffersCamera;
    std::vector<VkDeviceMemory> m_uniformBuffersCameraMemory;
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSets;
    VulkanDynamicUBO<ModelData> m_modelDataDynamicUBO;

    /* Texture data */
    VkImage m_textureImage;
    VkDeviceMemory m_textureImageMemory;
    VkImageView m_textureImageView;
    VkSampler m_textureSampler;

    /* Asset data */
    MeshModel m_modelDolphin;
    Transform m_modelDolphinTransform;
    int m_modelDolphinTransformUBOBlock = 0;
    MeshModel m_modelTeapot;
    Transform m_modelTeapotTransform;
    int m_modelTeapotTransformUBOBlock = 1;

    std::shared_ptr<Camera> m_camera;

    QColor m_clearColor = QColor(80, 80, 0);
};

#endif