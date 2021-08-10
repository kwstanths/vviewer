#ifndef __VulkanRenderer_hpp__
#define __VulkanRenderer_hpp__

#include <optional>
#include <vector>

#include <qvulkanwindow.h>
#include <qvulkanfunctions.h>

#include <utils/Console.hpp>

#include "IncludeVulkan.hpp"
#include "Utils.hpp"
#include "Camera.hpp"
#include "Mesh.hpp"

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

    /* meshes */
    Mesh * m_mesh1;
    Mesh * m_mesh2;

    /* Mesh data */
    bool prepareMeshData();
    bool createVertexBuffer(const std::vector<Vertex>& vertices, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    bool createIndexBuffer(const std::vector<uint16_t>& indices, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

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

    /* Vertex data */
    VkBuffer m_vertexBuffer1;
    VkDeviceMemory m_vertexBufferMemory1;
    VkBuffer m_indexBuffer1;
    VkDeviceMemory m_indexBufferMemory1;
    VkBuffer m_vertexBuffer2;
    VkDeviceMemory m_vertexBufferMemory2;
    VkBuffer m_indexBuffer2;
    VkDeviceMemory m_indexBufferMemory2;

    /* Descriptor data */
    VkDescriptorSetLayout m_descriptorSetLayout;
    std::vector<VkBuffer> m_uniformBuffersCamera;
    std::vector<VkDeviceMemory> m_uniformBuffersCameraMemory;
    std::vector<VkBuffer> m_uniformBuffersModel;
    std::vector<VkDeviceMemory> m_uniformBuffersModelMemory;
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSets;

    /* Texture data */
    VkImage m_textureImage;
    VkDeviceMemory m_textureImageMemory;
    VkImageView m_textureImageView;
    VkSampler m_textureSampler;

    std::shared_ptr<Camera> m_camera;

    QColor m_clearColor = QColor(1, 0, 0);
};

#endif