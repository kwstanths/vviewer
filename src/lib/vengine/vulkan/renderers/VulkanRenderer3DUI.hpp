#ifndef __VulkanRenderer3DUI_hpp__
#define __VulkanRenderer3DUI_hpp__

#include "core/Camera.hpp"
#include "math/AABB.hpp"

#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanFramebuffer.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/resources/VulkanTexture.hpp"
#include "vulkan/resources/VulkanMaterial.hpp"
#include "vulkan/resources/VulkanModel3D.hpp"

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

    /* Draw a transform widget at a certain position */
    VkResult renderTransform(VkCommandBuffer &cmdBuf,
                             VkDescriptorSet &descriptorScene,
                             uint32_t imageIndex,
                             const glm::mat4 &modelMatrix,
                             const std::shared_ptr<Camera> &camera) const;

    /* Draw an AABB */
    VkResult renderAABB3(VkCommandBuffer &cmdBuf,
                         VkDescriptorSet &descriptorScene,
                         uint32_t imageIndex,
                         const AABB3 &aabb,
                         const std::shared_ptr<Camera> &camera) const;

private:
    VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    VkCommandPool m_commandPool;
    VkQueue m_queue;
    VkExtent2D m_swapchainExtent;

    VkDescriptorSetLayout m_descriptorSetLayoutCamera;

    VkSampleCountFlagBits m_msaaSamples;
    VkRenderPass m_renderPass;

    VkPipelineLayout m_pipelineLayoutTransform, m_pipelineLayoutAABB;
    VkPipeline m_graphicsPipelineTransform, m_graphicsPipelineAABB;

    VulkanModel3D *m_arrow = nullptr;
    glm::vec3 m_rightID, m_upID, m_forwardID;

    VkResult createGraphicsPipelines();
};

}  // namespace vengine

#endif