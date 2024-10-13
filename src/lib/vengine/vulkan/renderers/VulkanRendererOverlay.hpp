#ifndef __VulkanRendererOverlay_hpp__
#define __VulkanRendererOverlay_hpp__

#include "core/Camera.hpp"
#include "math/AABB.hpp"

#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanFramebuffer.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/resources/VulkanTexture.hpp"
#include "vulkan/resources/VulkanMaterial.hpp"
#include "vulkan/resources/VulkanModel3D.hpp"
#include "vulkan/VulkanRenderPass.hpp"
#include "vulkan/VulkanInstances.hpp"

namespace vengine
{

class VulkanRendererOverlay
{
    friend class VulkanRenderer;

public:
    VulkanRendererOverlay(VulkanContext &context);

    VkResult initResources(VkDescriptorSetLayout cameraDescriptorLayout);
    VkResult initSwapChainResources(VkExtent2D swapchainExtent, const VulkanRenderPassOverlay &renderPass);

    VkResult releaseSwapChainResources();
    VkResult releaseResources();

    /* Draw a transform widget at a certain position */
    VkResult render3DTransform(VkCommandBuffer &cmdBuf,
                               VkDescriptorSet &descriptorScene,
                               const glm::mat4 &modelMatrix,
                               const std::shared_ptr<Camera> &camera) const;

    /* Draw an AABB */
    VkResult renderAABB3(VkCommandBuffer &cmdBuf,
                         VkDescriptorSet &descriptorScene,
                         const AABB3 &aabb,
                         const std::shared_ptr<Camera> &camera) const;

    /* Render an outline */
    VkResult renderOutline(VkCommandBuffer &cmdBuf,
                           VkDescriptorSet &descriptorScene,
                           SceneObject *so,
                           const std::shared_ptr<Camera> &camera) const;

private:
    VulkanContext &m_ctx;
    VkExtent2D m_swapchainExtent;

    VkDescriptorSetLayout m_descriptorSetLayoutCamera;

    VkPipelineLayout m_pipelineLayout3DTransform, m_pipelineLayoutAABB3, m_pipelineLayoutOutline;
    VkPipeline m_graphicsPipeline3DTransform, m_graphicsPipelineAABB3, m_graphicsPipelineOutlineStencil,
        m_graphicsPipelineOutlineWrite;

    VulkanModel3D *m_arrow = nullptr;
    ID m_IdX, m_IdY, m_IdZ;

    VkResult createGraphicsPipeline3DTransform(const VulkanRenderPassOverlay &renderPass);
    VkResult createGraphicsPipelineAABB3(const VulkanRenderPassOverlay &renderPass);
    VkResult createGraphicsPipelineOutline(const VulkanRenderPassOverlay &renderPass);
};

}  // namespace vengine

#endif