#ifndef __VulkanRenderer3DUI_hpp__
#define __VulkanRenderer3DUI_hpp__

#include "core/Camera.hpp"

#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanFramebuffer.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/resources/VulkanTexture.hpp"
#include "vulkan/resources/VulkanMaterial.hpp"

namespace vengine
{

class VulkanRenderer3DUI
{
    friend class VulkanRenderer;

public:
    VulkanRenderer3DUI();

    VkResult initResources(VkPhysicalDevice physicalDevice,
                           VkDevice device,
                           VkQueue queue,
                           VkCommandPool commandPool,
                           VkDescriptorSetLayout cameraDescriptorLayout);
    VkResult initSwapChainResources(VkExtent2D swapchainExtent,
                                    VkRenderPass renderPass,
                                    uint32_t swapchainImages,
                                    VkSampleCountFlagBits msaaSamples);

    VkResult releaseSwapChainResources();
    VkResult releaseResources();

    VkPipeline getPipeline() const;
    VkPipelineLayout getPipelineLayout() const;

    /* Draw a transform widget at a certain position */
    VkResult renderTransform(VkCommandBuffer &cmdBuf,
                             VkDescriptorSet &descriptorScene,
                             uint32_t imageIndex,
                             const glm::mat4 &modelMatrix,
                             const std::shared_ptr<Camera> &camera) const;

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
    VkSampleCountFlagBits m_msaaSamples;

    VulkanMeshModel *m_arrow = nullptr;
    glm::vec3 m_rightID, m_upID, m_forwardID;

    VkResult createGraphicsPipeline();
};

}  // namespace vengine

#endif