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
        VkDescriptorSetLayout cameraDescriptorLayout,
        VkDescriptorSetLayout modelDescriptorLayout);
    void initSwapChainResources(VkExtent2D swapchainExtent, VkRenderPass renderPass, uint32_t swapchainImages, const std::vector<VulkanFrameBufferAttachment>& colorattachments);

    void releaseSwapChainResources();
    void releaseResources();

    VkPipeline getPipeline() const;
    VkPipelineLayout getPipelineLayout() const;

    /* Draw a transform widget at a certain position */
    void renderTransform(VkCommandBuffer& cmdBuf,
        VkDescriptorSet& descriptorScene,
        uint32_t imageIndex,
        glm::vec3 position,
        float cameraDistance) const;

    /* Copy pass to the output */
    void renderCopy(VkCommandBuffer& cmdBuf, uint32_t imageIndex) const;

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
    VkPipelineLayout m_pipelineLayoutCopy;
    VkPipeline m_graphicsPipelineCopy;
    VkRenderPass m_renderPass;

    VkDescriptorPool m_descriptorPool;
    VkDescriptorSetLayout m_descriptorSetLayoutInputColor;
    std::vector<VkDescriptorSet> m_descriptorSets;
    VkSampler m_inputSampler;

    VulkanMeshModel* m_arrow = nullptr;

    bool createDescriptorSetsLayout();
    bool createGraphicsPipeline();
    bool createGraphicsPipelineEmptyPass();

    bool createDescriptorPool(uint32_t imageCount);
    bool createSampler();
    bool createDescriptors(uint32_t imageCount, const std::vector<VulkanFrameBufferAttachment>& colorAttachments);
};

#endif