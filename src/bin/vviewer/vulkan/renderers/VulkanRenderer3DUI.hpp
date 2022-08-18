#ifndef __VulkanRenderer3DUI_hpp__
#define __VulkanRenderer3DUI_hpp__

#include "vulkan/IncludeVulkan.hpp"
#include "vulkan/VulkanTexture.hpp"
#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanMaterials.hpp"
#include "vulkan/VulkanFramebuffer.hpp"

class VulkanRenderer3DUI {
    friend class VulkanRenderer;
public:
    VulkanRenderer3DUI();

    void initResources(VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkQueue queue,
        VkCommandPool commandPool,
        VkDescriptorSetLayout cameraDescriptorLayout);
    void initSwapChainResources(VkExtent2D swapchainExtent, VkRenderPass renderPass, uint32_t swapchainImages);

    void releaseSwapChainResources();
    void releaseResources();

    VkPipeline getPipeline() const;
    VkPipelineLayout getPipelineLayout() const;

    /* Draw a transform widget at a certain position */
    void renderTransform(VkCommandBuffer& cmdBuf,
        VkDescriptorSet& descriptorScene,
        uint32_t imageIndex,
        const glm::mat4& modelMatrix,
        float cameraDistance) const;

private:
    VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    VkCommandPool m_commandPool;
    VkQueue m_queue;
    VkExtent2D m_swapchainExtent;

    VkDescriptorSetLayout m_descriptorSetLayoutCamera;
    VkDescriptorSetLayout m_descriptorSetLayoutModel;

    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    VkRenderPass m_renderPass;

    VulkanMeshModel* m_arrow = nullptr;
    glm::vec3 m_rightID, m_upID, m_forwardID;

    bool createGraphicsPipeline();
};

#endif